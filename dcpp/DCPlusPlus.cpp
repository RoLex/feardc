/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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
#include "DCPlusPlus.h"

#include "ADLSearch.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "DownloadManager.h"
#include "FavoriteManager.h"
#include "FinishedManager.h"
#include "GeoManager.h"
#include "HashManager.h"
#include "HttpManager.h"
#include "LogManager.h"
#include "MappingManager.h"
#include "PluginApiImpl.h"
#include "QueueManager.h"
#include "ResourceManager.h"
#include "SearchManager.h"
#include "SettingsManager.h"
#include "ShareManager.h"
#include "ThrottleManager.h"
#include "UploadManager.h"
#include "PluginManager.h"
#include "UserMatchManager.h"
#include "WindowManager.h"

extern "C" int _nl_msg_cat_cntr;

namespace dcpp {

void startup() {
	// "Dedicated to the near-memory of Nev. Let's start remembering people while they're still alive."
	// Nev's great contribution to dc++
	while(1) break;


#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	bindtextdomain(PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");

	ResourceManager::newInstance();
	SettingsManager::newInstance();

	LogManager::newInstance();
	TimerManager::newInstance();
	HashManager::newInstance();
	CryptoManager::newInstance();
	SearchManager::newInstance();
	ClientManager::newInstance();
	ConnectionManager::newInstance();
	DownloadManager::newInstance();
	UploadManager::newInstance();
	ThrottleManager::newInstance();
	QueueManager::newInstance();
	ShareManager::newInstance();
	HttpManager::newInstance();
	FavoriteManager::newInstance();
	FinishedManager::newInstance();
	ADLSearchManager::newInstance();
	ConnectivityManager::newInstance();
	MappingManager::newInstance();
	GeoManager::newInstance();
	UserMatchManager::newInstance();
	WindowManager::newInstance();
	PluginManager::newInstance();
	PluginApiImpl::init();
}

void load(function<void (const string&)> stepF, function<void (float)> progressF) {
	SettingsManager::getInstance()->load();

#ifdef _WIN32
	if(!SETTING(LANGUAGE).empty()) {
		string language = "LANGUAGE=" + SETTING(LANGUAGE);
		putenv(language.c_str());

		// Apparently this is supposted to make gettext reload the message catalog...
		_nl_msg_cat_cntr++;
	}
#endif

	auto announce = [&stepF](const string& str) {
		if(stepF) {
			stepF(str);
		}
	};

	announce(_("Users"));
	ClientManager::getInstance()->loadUsers();
	FavoriteManager::getInstance()->load();

	PluginManager::getInstance()->loadPlugins(stepF);

	announce(_("Security certificates"));
	CryptoManager::getInstance()->loadCertificates();

	announce(_("Hash database"));
	HashManager::getInstance()->startup(progressF);

	announce(_("Shared Files"));
	ShareManager::getInstance()->refresh(true, false, true, progressF);

	announce(_("Download Queue"));
	QueueManager::getInstance()->loadQueue(progressF);

	if(SETTING(GET_USER_COUNTRY)) {
		announce(_("Country information"));
		GeoManager::getInstance()->init();
	}
}

void shutdown() {
	PluginManager::getInstance()->unloadPlugins();
	TimerManager::getInstance()->shutdown();
	HashManager::getInstance()->shutdown();
	ThrottleManager::getInstance()->shutdown();
	ConnectionManager::getInstance()->shutdown();
	HttpManager::getInstance()->shutdown();
	MappingManager::getInstance()->close();
	GeoManager::getInstance()->close();
	BufferedSocket::waitShutdown();
	FavoriteManager::getInstance()->shutdown();

	WindowManager::getInstance()->prepareSave();
	QueueManager::getInstance()->saveQueue(true);
	ClientManager::getInstance()->saveUsers();
	SettingsManager::getInstance()->save();

	PluginApiImpl::shutdown();
	PluginManager::deleteInstance();
	WindowManager::deleteInstance();
	UserMatchManager::deleteInstance();
	GeoManager::deleteInstance();
	MappingManager::deleteInstance();
	ConnectivityManager::deleteInstance();
	ADLSearchManager::deleteInstance();
	FinishedManager::deleteInstance();
	HttpManager::deleteInstance();
	ShareManager::deleteInstance();
	CryptoManager::deleteInstance();
	ThrottleManager::deleteInstance();
	DownloadManager::deleteInstance();
	UploadManager::deleteInstance();
	QueueManager::deleteInstance();
	ConnectionManager::deleteInstance();
	SearchManager::deleteInstance();
	FavoriteManager::deleteInstance();
	ClientManager::deleteInstance();
	HashManager::deleteInstance();
	LogManager::deleteInstance();
	SettingsManager::deleteInstance();
	TimerManager::deleteInstance();
	ResourceManager::deleteInstance();

#ifdef _WIN32
	::WSACleanup();
#endif
}

} // namespace dcpp
