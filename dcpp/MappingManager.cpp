/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdinc.h"
#include "MappingManager.h"

#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "format.h"
#include "LogManager.h"
#include "Mapper_MiniUPnPc.h"
#include "Mapper_NATPMP.h"
#include "ScopedFunctor.h"
#include "SearchManager.h"
#include "Util.h"
#include "version.h"

namespace dcpp {

std::atomic_flag MappingManager::busy = ATOMIC_FLAG_INIT;

MappingManager::MappingManager() :
needsV4PortMap(false),
needsV6PortMap(false),
renewal(0)
{
	// IPv4 mappers.
	addMapper<Mapper_NATPMP>(false);
	addMapper<Mapper_MiniUPnPc>(false);

	// IPv6 mappers.
	addMapper<Mapper_MiniUPnPc>(true);
}

StringList MappingManager::getMappers(bool v6) const {
	StringList ret;
	for(const auto& mapper: (v6 ? mappers6 : mappers4))
		ret.push_back(mapper.first);
	return ret;
}

bool MappingManager::open(bool v4, bool v6) {
	// find out whether port mapping has already worked for an interface, in which case we don't
	// want to redo it.
	if(v4 && getOpened(false)) { v4 = false; }
	if(v6 && getOpened(true)) { v6 = false; }
	if(!v4 && !v6) { return false; }

	if(busy.test_and_set()) {
		log(_("Another port mapping attempt is in progress..."));
		return false;
	}

	// save the requested port mapping flags.
	needsV4PortMap = v4;
	needsV6PortMap = v6;

	// all good! launch the port mapping thread.
	start();
	return true;
}

void MappingManager::close(tribool v6) {
	join();

	needsV4PortMap = false;
	needsV6PortMap = false;

	if(renewal) {
		renewal = 0;
		TimerManager::getInstance()->removeListener(this);
	}

	auto closeWorkingMapper = [this](unique_ptr<Mapper>& pMapper) {
		if(pMapper.get()) {
			close(*pMapper);
			pMapper.reset();
		}
	};
	if(!v6 || indeterminate(v6)) { closeWorkingMapper(working4); }
	if(v6 || indeterminate(v6)) { closeWorkingMapper(working6); }
}

bool MappingManager::getOpened(bool v6) const {
	return (v6 ? working6 : working4).get();
}

string MappingManager::getStatus(bool v6) const {
	auto& pMapper = v6 ? working6 : working4;
	if(pMapper.get()) {
		auto& mapper = *pMapper;
		return str(F_("Successfully created port mappings on the %1% device with the %2% interface") %
			deviceString(mapper) % mapper.getName());
	}
	return _("Failed to create port mappings");
}

int MappingManager::run() {
	ScopedFunctor([this] { busy.clear(); });

	// cache ports
	auto
		conn_port = ConnectionManager::getInstance()->getPort(),
		secure_port = ConnectionManager::getInstance()->getSecurePort(),
		search_port = SearchManager::getInstance()->getPort();

	/** @todo for now renewal is only supported for IPv4 port mappers, which is fine since it is
	 * for NAT-PMP which has not yet been ported to IPv6. */
	if(renewal && working4.get()) {
		Mapper& mapper = *working4;

		ScopedFunctor([&mapper] { mapper.uninit(); });
		if(!mapper.init()) {
			// can't renew; try again later.
			renewLater(mapper);
			return 0;
		}

		auto addRule = [this, &mapper](const string& port, Mapper::Protocol protocol, const string& description) {
			// just launch renewal requests - don't bother with possible failures.
			if(!port.empty()) {
				mapper.open(port, protocol, str(F_("%1% %2% port (%3% %4%)") %
					APPNAME % description % port % Mapper::protocols[protocol]));
			}
		};

		addRule(conn_port, Mapper::PROTOCOL_TCP, _("Transfer"));
		addRule(secure_port, Mapper::PROTOCOL_TCP, _("Encrypted transfer"));
		addRule(search_port, Mapper::PROTOCOL_UDP, _("Search"));

		renewLater(mapper);
		return 0;
	}

	if(needsV4PortMap) { runPortMapping(false, conn_port, secure_port, search_port); }
	if(needsV6PortMap) { runPortMapping(true, conn_port, secure_port, search_port); }

	ConnectivityManager::getInstance()->mappingFinished();

	return 0;
}

void MappingManager::runPortMapping(
	bool v6, const string& conn_port, const string& secure_port, const string& search_port
) {
	auto& mappers = v6 ? mappers6 : mappers4;

	// move the preferred mapper to the top of the stack.
	const auto& prefMapper = v6 ? SETTING(MAPPER6) : SETTING(MAPPER);
	for(auto i = mappers.begin(); i != mappers.end(); ++i) {
		if(i->first == prefMapper) {
			if(i != mappers.begin()) {
				auto mapper = *i;
				mappers.erase(i);
				mappers.insert(mappers.begin(), mapper);
			}
			break;
		}
	}

	// go through available port mappers, until one works.
	for(auto& i: mappers) {
		unique_ptr<Mapper> pMapper(i.second(Util::getLocalIp(v6)));
		Mapper& mapper = *pMapper;

		ScopedFunctor([&mapper] { mapper.uninit(); });
		if(!mapper.init()) {
			log(str(F_("Failed to initialize the %1% interface") % mapper.getName()), v6);
			continue;
		}

		auto addRule = [this, v6, &mapper](const string& port, Mapper::Protocol protocol, const string& description) -> bool {
			if(!port.empty() && !mapper.open(port, protocol, str(F_("%1% %2% port (%3% %4%)") %
				APPNAME % description % port % Mapper::protocols[protocol])))
			{
				this->log(str(F_("Failed to map the %1% port (%2% %3%) with the %4% interface") %
					description % port % Mapper::protocols[protocol] % mapper.getName()), v6);
				mapper.close();
				return false;
			}
			return true;
		};

		if(!(addRule(conn_port, Mapper::PROTOCOL_TCP, _("Transfer")) &&
			addRule(secure_port, Mapper::PROTOCOL_TCP, _("Encrypted transfer")) &&
			addRule(search_port, Mapper::PROTOCOL_UDP, _("Search"))))
			continue;

		log(str(F_("Successfully created port mappings (Transfers: %1%, Encrypted transfers: %2%, Search: %3%) on the %4% device with the %5% interface") %
			conn_port % secure_port % search_port % deviceString(mapper) % mapper.getName()), v6);

		(v6 ? working6 : working4) = move(pMapper);

		if((!v6 && !CONNSETTING(NO_IP_OVERRIDE)) || (v6 && !CONNSETTING(NO_IP_OVERRIDE6))) {
			auto setting = v6 ? SettingsManager::EXTERNAL_IP6 : SettingsManager::EXTERNAL_IP;
			string externalIP = mapper.getExternalIP();
			if(!externalIP.empty()) {
				ConnectivityManager::getInstance()->set(setting, externalIP);
			} else {
				// no cleanup because the mappings work and hubs will likely provide the correct IP.
				log(_("Failed to get external IP"), v6);
			}
		}

		ConnectivityManager::getInstance()->mappingFinished(mapper.getName(), v6);

		renewLater(mapper);
		break;
	}

	if(!getOpened(v6)) { 
		log(_("Failed to create port mappings"), v6); 
		ConnectivityManager::getInstance()->mappingFinished(Util::emptyString, v6);
	}
}

void MappingManager::close(Mapper& mapper) {
	if(mapper.hasRules()) {
		bool ret = mapper.init() && mapper.close();
		mapper.uninit();
		log(ret ?
			str(F_("Successfully removed port mappings from the %1% device with the %2% interface") % deviceString(mapper) % mapper.getName()) :
			str(F_("Failed to remove port mappings from the %1% device with the %2% interface") % deviceString(mapper) % mapper.getName()));
	}
}

void MappingManager::log(const string& message, tribool v6) {
	ConnectivityManager::getInstance()->log(str(F_("Port mapping: %1%") % message), v6);
}

string MappingManager::deviceString(Mapper& mapper) const {
	string name(mapper.getDeviceName());
	if(name.empty())
		name = _("Generic");
	return '"' + name + '"';
}

void MappingManager::renewLater(Mapper& mapper) {
	auto minutes = mapper.renewal();
	if(minutes) {
		bool addTimer = !renewal;
		renewal = GET_TICK() + std::max(minutes, 10u) * 60 * 1000;
		if(addTimer) {
			TimerManager::getInstance()->addListener(this);
		}

	} else if(renewal) {
		renewal = 0;
		TimerManager::getInstance()->removeListener(this);
	}
}

void MappingManager::on(TimerManagerListener::Minute, uint64_t tick) noexcept {
	if(tick >= renewal && !busy.test_and_set()) {
		try { start(); } catch(const ThreadException&) { busy.clear(); }
	}
}

} // namespace dcpp
