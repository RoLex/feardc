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

#ifndef DCPLUSPLUS_DCPP_SEARCH_MANAGER_H
#define DCPLUSPLUS_DCPP_SEARCH_MANAGER_H

#include "SettingsManager.h"

#include "AdcCommand.h"
#include "Socket.h"
#include "Thread.h"
#include "Singleton.h"

#include "SearchManagerListener.h"
#include "TimerManager.h"

namespace dcpp {

class SearchManager;
class SocketException;

class SearchManager : public Speaker<SearchManagerListener>, public Singleton<SearchManager>, public Thread, private CommandHandler<SearchManager>, private TimerManagerListener
{
public:
	enum SizeModes {
		SIZE_DONTCARE = 0x00,
		SIZE_ATLEAST = 0x01,
		SIZE_ATMOST = 0x02
	};

	enum TypeModes {
		TYPE_ANY = 0,
		TYPE_AUDIO,
		TYPE_COMPRESSED,
		TYPE_DOCUMENT,
		TYPE_EXECUTABLE,
		TYPE_PICTURE,
		TYPE_VIDEO,
		TYPE_DIRECTORY,
		TYPE_TTH,
		TYPE_LAST
	};
private:
	static const char* types[TYPE_LAST];
public:
	static const char* getTypeStr(int type);

	void search(const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken);
	void search(const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken) {
		search(aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken);
	}

	void search(StringList& who, const string& aName, int64_t aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const StringList& aExtList);
	void search(StringList& who, const string& aName, const string& aSize, TypeModes aTypeMode, SizeModes aSizeMode, const string& aToken, const StringList& aExtList) {
		search(who, aName, Util::toInt64(aSize), aTypeMode, aSizeMode, aToken, aExtList);
	}

	void respond(const AdcCommand& cmd, const OnlineUser& user);

	const string& getPort() const { return port; }

	void listen();
	void disconnect() noexcept;

	void onData(const string& data, const string& remoteIp = Util::emptyString);
	void onRES(const AdcCommand& cmd, const UserPtr& from, const string& removeIp = Util::emptyString);
	void onSR(const string& x, const string& remoteIP = Util::emptyString);

	int32_t timeToSearch() {
		return 5 - (static_cast<int64_t>(GET_TICK() - lastSearch) / 1000);
	}

	bool okToSearch() {
		return timeToSearch() <= 0;
	}

	void genSUDPKey(string& aKey);
	bool decryptPacket(string& x, size_t aLen, const uint8_t* aBuf);

private:
	friend class CommandHandler<SearchManager>;

	void handle(AdcCommand::RES, AdcCommand& c, const string& remoteIp) noexcept;

	// Ignore any other ADC commands for now
	template<typename T> void handle(T, AdcCommand&, const string&) { }

	std::unique_ptr<Socket> socket;
	string port;
	bool stop;
	uint64_t lastSearch;
	friend class Singleton<SearchManager>;

	SearchManager();

	static std::string normalizeWhitespace(const std::string& aString);
	virtual int run();

	virtual ~SearchManager();

	vector<pair<std::unique_ptr<uint8_t[]>, uint64_t>> searchKeys;
	
	CriticalSection cs;

	// TimerManagerListener
	virtual void on(Minute, uint64_t aTick) noexcept; 
};

} // namespace dcpp

#endif // !defined(SEARCH_MANAGER_H)
