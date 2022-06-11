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

#ifndef DCPLUSPLUS_DCPP_NMDC_HUB_H
#define DCPLUSPLUS_DCPP_NMDC_HUB_H

#include <list>

#include "TimerManager.h"
#include "SettingsManager.h"

#include "forward.h"
#include "CriticalSection.h"
#include "Text.h"
#include "Client.h"

namespace dcpp {

using std::list;

class NmdcHub : public Client, private Flags
{
public:
	enum SupportFlags {
		SUPPORTS_USERCOMMAND = 0x01,
		SUPPORTS_NOGETINFO = 0x02,
		SUPPORTS_USERIP2 = 0x04,
		SUPPORTS_TTHS = 0x08,
		SUPPORTS_TLS = 0x10,
		SUPPORTS_BOTLIST = 0x20,
		SUPPORTS_HUBTOPIC = 0x40,
		SUPPORTS_MCTO = 0x80
	};

	using Client::send;
	using Client::connect;

	virtual void connect(const OnlineUser& aUser, const string&, ConnectionType);

	virtual void hubMessage(const string& aMessage, bool /*thirdPerson*/ = false);
	virtual void hubMCTo(const string& aNick, const string& aMessage);
	virtual void privateMessage(const OnlineUser& aUser, const string& aMessage, bool /*thirdPerson*/ = false);
	virtual void sendUserCmd(const UserCommand& command, const ParamMap& params);
	virtual void search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList);
	virtual void password(const string& aPass) { send("$MyPass " + fromUtf8(aPass) + "|"); }
	virtual void infoImpl() { myInfo(false); }
	virtual bool haveSupports(int flag) { return (supportFlags & flag); }

	virtual size_t getUserCount() const { Lock l(cs); return users.size(); }
	virtual int64_t getAvailable() const;

	static string escape(const string& str) { return validateMessage(str, false); }
	static string unescape(const string& str) { return validateMessage(str, true); }

	void emulateCommand(const string& cmd) { onLine(cmd); }
	virtual void send(const AdcCommand&) { dcassert(0); }

	static string validateMessage(string tmp, bool reverse);
	
private:
	friend class ClientManager;

	mutable CriticalSection cs;

	unordered_map<string, OnlineUser*, noCaseStringHash, noCaseStringEq> users;

	int supportFlags;

	uint64_t lastUpdate;
	string lastMyInfoA;
	string lastMyInfoB;
	string lastMyInfoC;
	string lastMyInfoD;
	string localIp;

	typedef list<pair<string, uint32_t> > FloodMap;
	typedef FloodMap::iterator FloodIter;
	FloodMap seekers;
	FloodMap flooders;

	uint64_t lastProtectedIPsUpdate;
	StringList protectedIPs;

	NmdcHub(const string& aHubURL, bool secure = false);
	virtual ~NmdcHub();

	void clearUsers();
	void onLine(const string& aLine) noexcept;

	OnlineUser& getUser(const string& aNick);
	OnlineUser* findUser(const string& aNick);
	void putUser(const string& aNick);

	string toUtf8(const string& str) const { return Text::validateUtf8(str) ? str : Text::toUtf8(str, getEncoding()); }
	string fromUtf8(const string& str) const { return Text::fromUtf8(str, getEncoding()); }

	void privateMessage(const string& nick, const string& aMessage);
	void validateNick(const string& aNick) { send("$ValidateNick " + fromUtf8(aNick) + "|"); }
	void key(const string& aKey) { send("$Key " + aKey + "|"); }
	void version() { send("$Version 1,0091|"); }
	void getNickList() { send("$GetNickList|"); }
	void connectToMe(const OnlineUser& aUser);
	void revConnectToMe(const OnlineUser& aUser);
	void myInfo(bool alwaysSend);
	void supports(const StringList& feat);
	void clearFlooders(uint64_t tick);
	bool isProtectedIP(const string& ip);

	void updateFromTag(Identity& id, const string& tag);
	void refreshLocalIp() noexcept;

	/** split an "IP:port" string in 2. */
	static pair<string, string> parseIpPort(const string& aIpPort);

	virtual void checkNick(string& nick);
	virtual bool v4only() const { return true; }

	// TimerManagerListener
	virtual void on(Second, uint64_t aTick) noexcept;
	virtual void on(Minute, uint64_t aTick) noexcept;

	virtual void on(Connected) noexcept;
	virtual void on(Line, const string& l) noexcept;
	virtual void on(Failed, const string&) noexcept;

	OnlineUserList getUsers() const;
};

} // namespace dcpp

#endif // !defined(NMDC_HUB_H)
