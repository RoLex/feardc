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
#include "ConnectivityManager.h"

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "debug.h"
#include "format.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "version.h"

namespace dcpp {

ConnectivityManager::ConnectivityManager() :
autoDetected(false),
running(false)
{
}

bool ConnectivityManager::get(SettingsManager::BoolSetting setting) const {
	if(SETTING(AUTO_DETECT_CONNECTION)) {
		Lock l(cs);
		auto i = autoSettings.find(setting);
		if(i != autoSettings.end()) {
			return boost::get<bool>(i->second);
		}
	}
	return SettingsManager::getInstance()->get(setting);
}

int ConnectivityManager::get(SettingsManager::IntSetting setting) const {
	if(SETTING(AUTO_DETECT_CONNECTION)) {
		Lock l(cs);
		auto i = autoSettings.find(setting);
		if(i != autoSettings.end()) {
			return boost::get<int>(i->second);
		}
	}
	return SettingsManager::getInstance()->get(setting);
}

const string& ConnectivityManager::get(SettingsManager::StrSetting setting) const {
	if(SETTING(AUTO_DETECT_CONNECTION)) {
		Lock l(cs);
		auto i = autoSettings.find(setting);
		if(i != autoSettings.end()) {
			return boost::get<const string&>(i->second);
		}
	}
	return SettingsManager::getInstance()->get(setting);
}

void ConnectivityManager::set(SettingsManager::StrSetting setting, const string& str) {
	if(SETTING(AUTO_DETECT_CONNECTION)) {
		Lock l(cs);
		autoSettings[setting] = str;
	} else {
		SettingsManager::getInstance()->set(setting, str);
	}
}

void ConnectivityManager::clearAutoSettings(bool v6, bool resetDefaults) {
	int settings6[] = { SettingsManager::EXTERNAL_IP6, SettingsManager::BIND_ADDRESS6, 
		SettingsManager::NO_IP_OVERRIDE6, SettingsManager::INCOMING_CONNECTIONS6, 
		SettingsManager::BROAD_DETECTION6 };

	int settings4[] = { SettingsManager::EXTERNAL_IP, SettingsManager::NO_IP_OVERRIDE,
		SettingsManager::BIND_ADDRESS, SettingsManager::INCOMING_CONNECTIONS,
		SettingsManager::BROAD_DETECTION };

	int portSettings[] = { SettingsManager::TCP_PORT, SettingsManager::UDP_PORT,
		SettingsManager::TLS_PORT };


	Lock l(cs);

	//erase the old settings first
	for(const auto setting: v6 ? settings6 : settings4) {
		autoSettings.erase(setting);
	}
	for(const auto setting: portSettings) {
		autoSettings.erase(setting);
	}

	if (resetDefaults) {
		for(const auto setting: v6 ? settings6 : settings4) {
			if(setting >= SettingsManager::STR_FIRST && setting < SettingsManager::STR_LAST) {
				autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::StrSetting>(setting));
			} else if(setting >= SettingsManager::INT_FIRST && setting < SettingsManager::INT_LAST) {
				autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(setting));
			} else if(setting >= SettingsManager::BOOL_FIRST && setting < SettingsManager::BOOL_LAST) {
				autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::BoolSetting>(setting));
			} else {
				dcassert(0);
			}
		}

		for(const auto setting: portSettings)
			autoSettings[setting] = SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(setting));
	}
}

void ConnectivityManager::detectConnection() {
	if(running)
		return;
	running = true;

	MappingManager::getInstance()->close();
	disconnect();

	fire(ConnectivityManagerListener::Started());

	autoDetected = true;

	auto needsPortMapping4 = detectConnection(false);
	auto needsPortMapping6 = detectConnection(true);
	if(needsPortMapping4 || needsPortMapping6) {
		startMapping(needsPortMapping4, needsPortMapping6);
	} else {
		fire(ConnectivityManagerListener::Finished());
	}
}

bool ConnectivityManager::detectConnection(bool v6) {
	(v6 ? statusV6 : statusV4).clear();

	// restore auto settings to their default value.
	clearAutoSettings(v6, true);

	const auto incomingConnSetting = v6 ? SettingsManager::INCOMING_CONNECTIONS6 : SettingsManager::INCOMING_CONNECTIONS;
	const tribool logType = v6 ? true : false;

	log(_("Determining the best connectivity settings..."), logType);

	if(!v6) {
		/** @todo the following is only run once, during ipv4 connectivity detection. think of a
		 * less hacky way. */
		try {
			listen();
		} catch(const Exception& e) {
			{
				Lock l(cs);
				autoSettings[SettingsManager::INCOMING_CONNECTIONS] = SettingsManager::INCOMING_PASSIVE;
				autoSettings[SettingsManager::INCOMING_CONNECTIONS6] = SettingsManager::INCOMING_PASSIVE;
			}
			autoDetected = false;
			log(str(F_("Unable to open %1% port(s); connectivity settings must be configured manually") % e.getError()));
			return false;
		}
	}

	auto ip = Util::getLocalIp(v6);

	if(Util::isPublicIp(ip, v6)) {
		{
			Lock l(cs);
			autoSettings[incomingConnSetting] = SettingsManager::INCOMING_ACTIVE;
		}
		log(_("Public IP address detected, selecting active mode with direct connection"), logType);
		return false;
	}

	// Disable connectivity when local & IPv6.
	if(v6 && Util::isLocalIp(ip, v6)) {
		{
			Lock l(cs);
			autoSettings[incomingConnSetting] = SettingsManager::INCOMING_DISABLED;
		}
		log(_("All IPv6 addresses found are local; disabling connectivity"), logType);
		return false;
	}

	{
		Lock l(cs);
		autoSettings[incomingConnSetting] = SettingsManager::INCOMING_ACTIVE_UPNP;
	}
	log(_("Local network with possible NAT detected, trying to map the ports..."), logType);
	return true;
}

void ConnectivityManager::setup(bool v4SettingsChanged, bool v6SettingsChanged) {
	if(SETTING(AUTO_DETECT_CONNECTION)) {
		if(!autoDetected) {
			detectConnection();
		}
	} else {
		if(autoDetected) {
			Lock l(cs);
			autoSettings.clear();
		}
		auto settingsChanged = v4SettingsChanged || v6SettingsChanged;
		if(autoDetected || settingsChanged) {
			if(v4SettingsChanged || (SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_ACTIVE_UPNP)) {
				MappingManager::getInstance()->close(false);
			}
			if(v6SettingsChanged || (SETTING(INCOMING_CONNECTIONS6) != SettingsManager::INCOMING_ACTIVE_UPNP)) {
				MappingManager::getInstance()->close(true);
			}
			startSocket();
		} else if(!running) {
			// find out whether previous mappings had failed, to try them again.
			startMapping(
				SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_ACTIVE_UPNP,
				SETTING(INCOMING_CONNECTIONS6) == SettingsManager::INCOMING_ACTIVE_UPNP);
		}
	}
}

void ConnectivityManager::editAutoSettings() {
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, false);

	auto sm = SettingsManager::getInstance();
	for(auto& i: autoSettings) {
		if(i.first >= SettingsManager::STR_FIRST && i.first < SettingsManager::STR_LAST) {
			sm->set(static_cast<SettingsManager::StrSetting>(i.first), boost::get<const string&>(i.second));
		} else if(i.first >= SettingsManager::INT_FIRST && i.first < SettingsManager::INT_LAST) {
			sm->set(static_cast<SettingsManager::IntSetting>(i.first), boost::get<int>(i.second));
		} else if(i.first >= SettingsManager::BOOL_FIRST && i.first < SettingsManager::BOOL_LAST) {
			sm->set(static_cast<SettingsManager::BoolSetting>(i.first), boost::get<bool>(i.second));
		} else {
			dcassert(0);
		}
	}
	autoSettings.clear();

	fire(ConnectivityManagerListener::SettingChanged());
}

string ConnectivityManager::getInformation() const {
	if(running) {
		return _("Connectivity settings are being configured; try again later");
	}

	string autoStatus = ok() ? str(F_(
		"enabled\n"
		"\tIPv4 detection: %1%\n"
		"\tIPv6 detection: %2%") % getStatus(false) % getStatus(true)) : _("disabled");

	auto getMode = [&](bool v6) -> string { 
		switch(v6 ? CONNSETTING(INCOMING_CONNECTIONS6) : CONNSETTING(INCOMING_CONNECTIONS)) {
		case SettingsManager::INCOMING_ACTIVE: return _("Active mode");
		case SettingsManager::INCOMING_ACTIVE_UPNP: return str(F_(
			"Active mode behind a router that %1% can configure; port mapping status: %2%") %
				APPNAME % MappingManager::getInstance()->getStatus(v6));
		case SettingsManager::INCOMING_PASSIVE: return _("Passive mode");
		default: return _("Disabled");
		}
	};

	auto field = [](const string& s) { return s.empty() ? _("undefined") : s; };

	return str(F_(
		"Connectivity information:\n\n"
		"Automatic connectivity setup is: %1%\n\n"
		"\tIPv4: %2%\n"
		"\tIPv6: %3%\n"
		"\tExternal IP (v4): %4%\n"
		"\tExternal IP (v6): %5%\n"
		"\tBound interface (v4): %6%\n"
		"\tBound interface (v6): %7%\n"
		"\tTransfer port: %8%\n"
		"\tEncrypted transfer port: %9%\n"
		"\tSearch port: %10%") % autoStatus % getMode(false) % getMode(true) %
		field(CONNSETTING(EXTERNAL_IP)) % field(CONNSETTING(EXTERNAL_IP6)) %
		field(CONNSETTING(BIND_ADDRESS)) % field(CONNSETTING(BIND_ADDRESS6)) %
		field(ConnectionManager::getInstance()->getPort()) % field(ConnectionManager::getInstance()->getSecurePort()) %
		field(SearchManager::getInstance()->getPort()));
}

void ConnectivityManager::startMapping(bool needsPortMapping4, bool needsPortMapping6) {
	if(!needsPortMapping4 && !needsPortMapping6) { running = false; return; }
	running = true;
	if(!MappingManager::getInstance()->open(needsPortMapping4, needsPortMapping6)) {
		running = false;
	}
}

// called when port mapping for both IPv6 & IPv4 is done.
void ConnectivityManager::mappingFinished() {
	if(SETTING(AUTO_DETECT_CONNECTION))
		fire(ConnectivityManagerListener::Finished());
	running = false;
}

// called once per IP version.
void ConnectivityManager::mappingFinished(const string& mapperName, bool v6) {
	if(SETTING(AUTO_DETECT_CONNECTION) && mapperName.empty()) {
		{
			Lock l(cs);
			autoSettings[v6 ? SettingsManager::INCOMING_CONNECTIONS6 : SettingsManager::INCOMING_CONNECTIONS] = SettingsManager::INCOMING_PASSIVE;
		}
		log(_("Active mode could not be achieved; a manual configuration is recommended for better connectivity"), v6 ? true : false);
	} else {
		SettingsManager::getInstance()->set(v6 ? SettingsManager::MAPPER6 : SettingsManager::MAPPER, mapperName);
	}
}

void ConnectivityManager::log(string&& message, tribool v6) {
	if(SETTING(AUTO_DETECT_CONNECTION) && !indeterminate(v6)) {
		auto& status = v6 ? statusV6 : statusV4;
		status = move(message);
		string proto = v6 ? "IPv6" : "IPv4";

		LogManager::getInstance()->message(str(F_("Connectivity (%1%): %2%") % proto % status));
		fire(ConnectivityManagerListener::Message(), str(F_("%1%: %2%") % proto % status));

	} else {
		LogManager::getInstance()->message(message);
	}
}

const string& ConnectivityManager::getStatus(bool v6) const { 
	return v6 ? statusV6 : statusV4; 
}

void ConnectivityManager::startSocket() {
	autoDetected = false;

	disconnect();

	if(ClientManager::getInstance()->isActive()) {
		listen();

		// must be done after listen calls; otherwise ports won't be set
		if(!running) {
			startMapping(
				SETTING(INCOMING_CONNECTIONS) == SettingsManager::INCOMING_ACTIVE_UPNP,
				SETTING(INCOMING_CONNECTIONS6) == SettingsManager::INCOMING_ACTIVE_UPNP);
		}
	}
}

void ConnectivityManager::listen() {
	try {
		ConnectionManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception(_("Transfer (TCP)"));
	}

	try {
		SearchManager::getInstance()->listen();
	} catch(const Exception&) {
		throw Exception(_("Search (UDP)"));
	}
}

void ConnectivityManager::disconnect() {
	SearchManager::getInstance()->disconnect();
	ConnectionManager::getInstance()->disconnect();
}

} // namespace dcpp
