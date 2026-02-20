/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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
#include "SearchManager.h"

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/scoped_array.hpp>

#include <openssl/rand.h>

#include "ClientManager.h"
#include "ConnectivityManager.h"
#include "format.h"
#include "LogManager.h"
#include "PluginManager.h"
#include "SearchResult.h"
#include "ShareManager.h"
#include "CryptoManager.h"

namespace dcpp {

const char* SearchManager::types[TYPE_LAST] = {
	N_("Any"),
	N_("Audio"),
	N_("Compressed"),
	N_("Document"),
	N_("Executable"),
	N_("Picture"),
	N_("Video"),
	N_("Directory"),
	N_("TTH")
};
const char* SearchManager::getTypeStr(int type) {
	return _(types[type]);
}

SearchManager::SearchManager() :
	stop(false),
	lastSearch(GET_TICK())
{
	TimerManager::getInstance()->addListener(this);
}

SearchManager::~SearchManager() {
	if(socket.get()) {
		stop = true;
		socket->disconnect();
#ifdef _WIN32
		join();
#endif
	}

	TimerManager::getInstance()->removeListener(this); 
	searchKeys.clear();
}

string SearchManager::normalizeWhitespace(const string& aString){
	string::size_type found = 0;
	string normalized = aString;
	while((found = normalized.find_first_of("\t\n\r", found)) != string::npos) {
		normalized[found] = ' ';
		found++;
	}
	return normalized;
}

void SearchManager::search(const string& aName, int64_t aSize, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */) {
	if(okToSearch()) {
		string aKey;
		genSUDPKey(aKey);
		ClientManager::getInstance()->search(aSizeMode, aSize, aTypeMode, normalizeWhitespace(aName), aToken, aKey);
		lastSearch = GET_TICK();
	}
}

void SearchManager::search(StringList& who, const string& aName, int64_t aSize /* = 0 */, TypeModes aTypeMode /* = TYPE_ANY */, SizeModes aSizeMode /* = SIZE_ATLEAST */, const string& aToken /* = Util::emptyString */, const StringList& aExtList) {
	if(okToSearch()) {
		string aKey;
		genSUDPKey(aKey);
		ClientManager::getInstance()->search(who, aSizeMode, aSize, aTypeMode, normalizeWhitespace(aName), aToken, aExtList, aKey);
		lastSearch = GET_TICK();
	}
}

void SearchManager::listen() {
	disconnect();

	try {
		socket.reset(new Socket(Socket::TYPE_UDP));
		socket->setLocalIp4(CONNSETTING(BIND_ADDRESS));
		socket->setLocalIp6(CONNSETTING(BIND_ADDRESS6));
		port = socket->listen(std::to_string(CONNSETTING(UDP_PORT)));
		start();
	} catch(...) {
		socket.reset();
		throw;
	}
}

void SearchManager::disconnect() noexcept {
	if(socket.get()) {
		stop = true;
		socket->disconnect();
		port.clear();

		join();

		socket.reset();

		stop = false;
	}
}

#define BUFSIZE 8192
int SearchManager::run() {
	boost::scoped_array<uint8_t> buf(new uint8_t[BUFSIZE]);
	int len;
	string remoteAddr;

	while(!stop) {
		try {
			if(!socket->wait(400, true, false).first) {
				continue;
			}

			if((len = socket->read(&buf[0], BUFSIZE, remoteAddr)) > 0) {
				string data(reinterpret_cast<char*>(&buf[0]), len);

				if(SETTING(ENABLE_SUDP) && len >= 32 && ((len & 15) == 0)) {
					decryptPacket(data, len, buf.get());
				}

				if(PluginManager::getInstance()->onUDP(false, remoteAddr, port, data))
					continue;

				onData(data, remoteAddr);
				continue;
			}

		} catch(const SocketException& e) {
			dcdebug("SearchManager::run Error: %s\n", e.getError().c_str());
		}

		bool failed = false;
		while(!stop) {
			try {
				socket->disconnect();
				port = socket->listen(std::to_string(CONNSETTING(UDP_PORT)));
				if(failed) {
					LogManager::getInstance()->message(_("Search enabled again"));
					failed = false;
				}
				break;
			} catch(const SocketException& e) {
				dcdebug("SearchManager::run Stopped listening: %s\n", e.getError().c_str());

				if(!failed) {
					LogManager::getInstance()->message(str(F_("Search disabled: %1%") % e.getError()));
					failed = true;
				}

				// Spin for 60 seconds
				for(auto i = 0; i < 60 && !stop; ++i) {
					Thread::sleep(1000);
				}
			}
		}
	}
	return 0;
}

void SearchManager::onData(const string& x, const string& remoteIp) {
	if(x.empty()) { return; } // shouldn't happen but rather be safe...

	if (x.compare(0, 1, "$") == 0) {
		// NMDC commands
		if (x.compare(1, 3, "SR ") == 0) {
			onSR(x, remoteIp);
		} else {
			dcdebug("Unknown NMDC command received via UDP: %s\n", x.c_str());
		}

		return;
	}

	// ADC commands

	// ADC commands must end with \n
	if (x[x.length() - 1] != 0x0a) {
		dcdebug("Invalid UDP data received: %s (no newline)\n", x.c_str());
		return;
	}

	if (!Text::validateUtf8(x)) {
		dcdebug("UTF-8 validation failed for received UDP data: %s\n", x.c_str());
		return;
	}

	// Dispatch without newline
	dispatch(x.substr(0, x.length() - 1), false, remoteIp);
}

void SearchManager::handle(AdcCommand::RES, AdcCommand& c, const string& remoteIp) noexcept {
	if (c.getParameters().empty())
		return;

	string cid = c.getParam(0);
	if (cid.size() != 39)
		return;

	UserPtr user = ClientManager::getInstance()->findUser(CID(cid));
	if (!user)
		return;

	// This should be handled by AdcCommand really...
	c.getParameters().erase(c.getParameters().begin());

	onRES(c, user, remoteIp);
}

void SearchManager::onSR(const string& x, const string& remoteIp) {
	string::size_type i, j;
	// Directories: $SR <nick><0x20><directory><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
	// Files:		$SR <nick><0x20><filename><0x05><filesize><0x20><free slots>/<total slots><0x05><Hubname><0x20>(<Hubip:port>)
	i = 4;
	if ((j = x.find(' ', i)) == string::npos) {
		return;
	}
	string nick = x.substr(i, j - i);
	i = j + 1;

	// A file has 2 0x05, a directory only one
	size_t cnt = count(x.begin() + j, x.end(), 0x05);

	SearchResult::Types type = SearchResult::TYPE_FILE;
	string file;
	int64_t size = 0;

	if (cnt == 1) {
		// We have a directory...find the first space beyond the first 0x05 from the back
		// (dirs might contain spaces as well...clever protocol, eh?)
		type = SearchResult::TYPE_DIRECTORY;
		// Get past the hubname that might contain spaces
		if ((j = x.rfind(0x05)) == string::npos) {
			return;
		}
		// Find the end of the directory info
		if ((j = x.rfind(' ', j - 1)) == string::npos) {
			return;
		}
		if (j < i + 1) {
			return;
		}
		file = x.substr(i, j - i) + '\\';
	} else if (cnt == 2) {
		if ((j = x.find((char)5, i)) == string::npos) {
			return;
		}
		file = x.substr(i, j - i);
		i = j + 1;
		if ((j = x.find(' ', i)) == string::npos) {
			return;
		}
		size = Util::toInt64(x.substr(i, j - i));
	}
	i = j + 1;

	if ((j = x.find('/', i)) == string::npos) {
		return;
	}
	int freeSlots = Util::toInt(x.substr(i, j - i));
	i = j + 1;
	if ((j = x.find((char)5, i)) == string::npos) {
		return;
	}
	int slots = Util::toInt(x.substr(i, j - i));
	i = j + 1;
	if ((j = x.rfind(" (")) == string::npos) {
		return;
	}
	string hubName = x.substr(i, j - i);
	i = j + 2;
	if ((j = x.rfind(')')) == string::npos) {
		return;
	}

	HintedUser user;

	user.hint = ClientManager::getInstance()->findHub(x.substr(i, j - i));
	if (user.hint.empty()) {
		// Could happen if hub has multiple URLs / IPs
		user = ClientManager::getInstance()->findLegacyUser(nick);
		if (!user)
			return;
	}

	string encoding = ClientManager::getInstance()->findHubEncoding(user.hint);
	nick = Text::toUtf8(nick, encoding);
	file = Text::toUtf8(file, encoding);
	hubName = Text::toUtf8(hubName, encoding);

	if (!user) {
		user.user = ClientManager::getInstance()->findUser(nick, user.hint);
		if (!user)
			return;
	}

	Style style;
	{
		auto lock = ClientManager::getInstance()->lock();
		auto ou = ClientManager::getInstance()->findOnlineUser(user);
		if (ou) {
			style = ou->getIdentity().getStyle();
		}
	}

	string tth;
	if (hubName.compare(0, 4, "TTH:") == 0) {
		tth = hubName.substr(4);
		StringList names = ClientManager::getInstance()->getHubNames(user);
		hubName = names.empty() ? _("Offline") : Util::toString(names);
	}

	if (tth.empty() && type == SearchResult::TYPE_FILE) {
		return;
	}

	fire(SearchManagerListener::SR(), SearchResultPtr(new SearchResult(user, type, slots,
		freeSlots, size, file, hubName, remoteIp, TTHValue(tth), Util::emptyString, style)));
}

void SearchManager::onRES(const AdcCommand& cmd, const UserPtr& from, const string& remoteIp) {
	int freeSlots = -1;
	int64_t size = -1;
	string file;
	string tth;
	string token;

	for(auto& str: cmd.getParameters()) {
		if(str.compare(0, 2, "FN") == 0) {
			file = Util::toNmdcFile(str.substr(2));
		} else if(str.compare(0, 2, "SL") == 0) {
			freeSlots = Util::toInt(str.substr(2));
		} else if(str.compare(0, 2, "SI") == 0) {
			size = Util::toInt64(str.substr(2));
		} else if(str.compare(0, 2, "TR") == 0) {
			tth = str.substr(2);
		} else if(str.compare(0, 2, "TO") == 0) {
			token = str.substr(2);
		}
	}

	if(file.empty() || freeSlots == -1 || size == -1) { return; }

	auto type = (*(file.end() - 1) == '\\' ? SearchResult::TYPE_DIRECTORY : SearchResult::TYPE_FILE);
	if(type == SearchResult::TYPE_FILE && tth.empty()) { return; }

	string hubUrl;
	HintedUser hUser;
	Style style;

	// token format: [per-hub unique id] "/" [per-search actual token] (see AdcHub::search)
	auto slash = token.find('/');
	if(slash == string::npos) { return; }
	{
		auto uniqueId = Util::toUInt32(token.substr(0, slash));
		auto lock = ClientManager::getInstance()->lock();
		auto& clients = ClientManager::getInstance()->getClients();
		auto i = boost::find_if(clients, [uniqueId](const Client* client) { return client->getUniqueId() == uniqueId; });
		if(i == clients.end()) { return; }
		hubUrl = (*i)->getHubUrl();

		hUser = HintedUser(from, hubUrl);
		auto ou = ClientManager::getInstance()->findOnlineUser(hUser);
		if (ou) {
			style = ou->getIdentity().getStyle();
		}
	}
	token.erase(0, slash + 1);

	StringList names = ClientManager::getInstance()->getHubNames(from->getCID());
	string hubName = names.empty() ? _("Offline") : Util::toString(names);

	/// @todo Something about the slots
	fire(SearchManagerListener::SR(), SearchResultPtr(new SearchResult(hUser,
		type, 0, freeSlots, size, file, hubName, remoteIp, TTHValue(tth), token, style)));
}

void SearchManager::genSUDPKey(string& aKey) {
	string keyStr = Util::emptyString;
	if(SETTING(ENABLE_SUDP)) {
		auto key = std::make_unique<uint8_t[]>(16);
		RAND_bytes(key.get(), 16);
		keyStr = Encoder::toBase32(key.get(), 16);
		{
			Lock l(cs);
			searchKeys.emplace_back(move(key), GET_TICK());
		}
	}
	aKey = keyStr;
}

bool SearchManager::decryptPacket(string& x, size_t aLen, const uint8_t* aBuf) {
	Lock l(cs);

	for (const auto& i: searchKeys | boost::adaptors::reversed) {
		if(CryptoManager::getInstance()->decryptSUDP(i.first.get(), aBuf, aLen, x)) {
			return true;
		}
	}

	return false;
}

void SearchManager::respond(const AdcCommand& cmd, const OnlineUser& user) {
	// Filter own searches
	if(user.getUser() == ClientManager::getInstance()->getMe())
		return;

	auto results = ShareManager::getInstance()->search(cmd.getParameters(), user.getIdentity().isUdpActive() ? 10 : 5);
	if(results.empty())
		return;

	string token, key;
	cmd.getParam("TO", 0, token);
	cmd.getParam("KY", 0, key);

	for(auto& i: results) {
		AdcCommand res = i->toRES(AdcCommand::TYPE_UDP);
		if(!token.empty())
			res.addParam("TO", token);
		ClientManager::getInstance()->sendUDP(res, user, key);
	}
}

void SearchManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
	Lock l(cs);
	for (auto i = searchKeys.begin(); i != searchKeys.end();) {
		if (i->second + 1000 * 60 * 5 < aTick) {
			searchKeys.erase(i);
			i = searchKeys.begin();
		} else {
			++i;
		}
	}
}

} // namespace dcpp
