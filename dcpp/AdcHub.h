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

#ifndef DCPLUSPLUS_DCPP_ADC_HUB_H
#define DCPLUSPLUS_DCPP_ADC_HUB_H

#include "typedefs.h"

#include "Client.h"
#include "AdcCommand.h"
#include "Socket.h"

namespace dcpp {

class ClientManager;

class AdcHub : public Client, public CommandHandler<AdcHub> {
public:
	using Client::send;
	using Client::connect;

	virtual void connect(const OnlineUser& user, const string& token, ConnectionType type);
	void connect(const OnlineUser& user, const string& token, ConnectionType type, bool secure);

	virtual void hubMessage(const string& aMessage, bool thirdPerson = false);
	virtual void privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson = false);
	virtual void sendUserCmd(const UserCommand& command, const ParamMap& params);
	virtual void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, const string& aKey = Util::emptyString);
	virtual void password(const string& pwd);
	virtual void infoImpl();

	virtual size_t getUserCount() const { Lock l(cs); return users.size(); }
	virtual int64_t getAvailable() const;

	static string escape(const string& str) { return AdcCommand::escape(str, false); }
	void emulateCommand(const string& cmd) { dispatch(cmd); }
	virtual void send(const AdcCommand& cmd);

	string getMySID() { return AdcCommand::fromSID(sid); }

	static const vector<StringList>& getSearchExts();
	static StringList parseSearchExts(int flag);

	static const string CLIENT_PROTOCOL;
	static const string SECURE_CLIENT_PROTOCOL;
	static const string ADCS_FEATURE;
	static const string TCP4_FEATURE;
	static const string TCP6_FEATURE;
	static const string UDP4_FEATURE;
	static const string UDP6_FEATURE;
	static const string NAT0_FEATURE;
	static const string SEGA_FEATURE;
	static const string CCPM_FEATURE;
	static const string SUDP_FEATURE;
	static const string BASE_SUPPORT;
	static const string BAS0_SUPPORT;
	static const string TIGR_SUPPORT;
	static const string UCM0_SUPPORT;
	static const string BLO0_SUPPORT;
	static const string ZLIF_SUPPORT;

private:
	friend class ClientManager;
	friend class CommandHandler<AdcHub>;
	friend class Identity;

	AdcHub(const string& aHubURL, bool secure);
	virtual ~AdcHub();

	bool oldPassword;
	Socket udp;
	unordered_map<uint32_t, OnlineUser*> users; /** Map session id to OnlineUser */
	StringMap lastInfoMap;
	mutable CriticalSection cs;

	string salt;
	uint32_t sid;

	uint64_t sinceConnect;

	std::unordered_set<uint32_t> forbiddenCommands;

	static const vector<StringList> searchExts;

	virtual void checkNick(string& nick);

	OnlineUser& getUser(const uint32_t aSID, const CID& aCID);
	OnlineUser* findUser(const uint32_t sid) const;
	OnlineUser* findUser(const CID& cid) const;
	void putUser(const uint32_t sid, bool disconnect);

	void clearUsers();
	void appendConnectivity(StringMap& lastInfoMap, AdcCommand& c, bool v4, bool v6);

	void handle(AdcCommand::SUP, AdcCommand& c) noexcept;
	void handle(AdcCommand::SID, AdcCommand& c) noexcept;
	void handle(AdcCommand::MSG, AdcCommand& c) noexcept;
	void handle(AdcCommand::INF, AdcCommand& c) noexcept;
	void handle(AdcCommand::GPA, AdcCommand& c) noexcept;
	void handle(AdcCommand::QUI, AdcCommand& c) noexcept;
	void handle(AdcCommand::CTM, AdcCommand& c) noexcept;
	void handle(AdcCommand::RCM, AdcCommand& c) noexcept;
	void handle(AdcCommand::STA, AdcCommand& c) noexcept;
	void handle(AdcCommand::SCH, AdcCommand& c) noexcept;
	void handle(AdcCommand::CMD, AdcCommand& c) noexcept;
	void handle(AdcCommand::RES, AdcCommand& c) noexcept;
	void handle(AdcCommand::GET, AdcCommand& c) noexcept;
	void handle(AdcCommand::NAT, AdcCommand& c) noexcept;
	void handle(AdcCommand::RNT, AdcCommand& c) noexcept;
	void handle(AdcCommand::ZON, AdcCommand& c) noexcept;
	void handle(AdcCommand::ZOF, AdcCommand& c) noexcept;

	template<typename T> void handle(T, AdcCommand&) { }

	void sendSearch(AdcCommand& c);
	void sendUDP(const AdcCommand& cmd) noexcept;
	void unknownProtocol(uint32_t target, const string& protocol, const string& token);
	bool secureAvail(uint32_t target, const string& protocol, const string& token);

	virtual bool v4only() const { return false; }
	virtual void on(Connecting) noexcept { fire(ClientListener::Connecting(), this); }
	virtual void on(Connected) noexcept;
	virtual void on(Line, const string& aLine) noexcept;
	virtual void on(Failed, const string& aLine) noexcept;

	virtual void on(Second, uint64_t aTick) noexcept;

	OnlineUserList getUsers() const;
};

} // namespace dcpp

#endif // !defined(ADC_HUB_H)
