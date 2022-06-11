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

#ifndef DCPLUSPLUS_DCPP_CONNECTIVITY_MANAGER_H
#define DCPLUSPLUS_DCPP_CONNECTIVITY_MANAGER_H

#include "CriticalSection.h"
#include "SettingsManager.h"
#include "Singleton.h"
#include "Speaker.h"
#include "tribool.h"

#include <string>
#include <unordered_map>
#include <boost/variant.hpp>

namespace dcpp {

using std::string;
using std::unordered_map;

class ConnectivityManagerListener {
public:
	virtual ~ConnectivityManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Message;
	typedef X<1> Started;
	typedef X<2> Finished;
	typedef X<3> SettingChanged; // auto-detection has been enabled / disabled

	virtual void on(Message, const string&) noexcept { }
	virtual void on(Started) noexcept { }
	virtual void on(Finished) noexcept { }
	virtual void on(SettingChanged) noexcept { }
};

class ConnectivityManager : public Singleton<ConnectivityManager>, public Speaker<ConnectivityManagerListener>
{
public:
	bool get(SettingsManager::BoolSetting setting) const;
	int get(SettingsManager::IntSetting setting) const;
	const string& get(SettingsManager::StrSetting setting) const;
	void set(SettingsManager::StrSetting setting, const string& str);

	void detectConnection();
	void setup(bool v4SettingsChanged, bool v6SettingsChanged);
	void editAutoSettings();
	bool ok() const { return autoDetected; }
	bool isRunning() const { return running; }
	const string& getStatus(bool v6) const;
	string getInformation() const;

private:
	friend class Singleton<ConnectivityManager>;
	friend class MappingManager;
	
	ConnectivityManager();
	virtual ~ConnectivityManager() { }

	/** Utility funciton to launch detection for one IP version.
	 * @return Whether port mapping is needed. */
	bool detectConnection(bool v6);
	void startMapping(bool needsPortMapping4, bool needsPortMapping6);
	void mappingFinished();
	void mappingFinished(const string& mapperName, bool v6);
	void clearAutoSettings(bool v6, bool resetDefaults);
	void log(string&& message, tribool v6 = indeterminate);

	void startSocket();
	void listen();
	void disconnect();

	bool autoDetected;
	bool running;

	string statusV4;
	string statusV6;

	/* contains auto-detected settings. they are stored separately from manual connectivity
	settings (stored in SettingsManager) in case the user wants to keep the manually set ones for
	future use. */
	unordered_map<int, boost::variant<bool, int, string>> autoSettings;

	mutable CriticalSection cs;
};

#define CONNSETTING(k) ConnectivityManager::getInstance()->get(SettingsManager::k)

} // namespace dcpp

#endif // !defined(CONNECTIVITY_MANAGER_H)
