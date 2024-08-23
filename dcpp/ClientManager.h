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

#ifndef DCPLUSPLUS_DCPP_CLIENT_MANAGER_H
#define DCPLUSPLUS_DCPP_CLIENT_MANAGER_H

#include <unordered_map>
#include <unordered_set>

#include "CID.h"
#include "ClientListener.h"
#include "ClientManagerListener.h"
#include "ConnectionType.h"
#include "HintedUser.h"
#include "OnlineUser.h"
#include "Singleton.h"
#include "Socket.h"
#include "TimerManager.h"

namespace dcpp {

using std::pair;
using std::unordered_map;
using std::unordered_multimap;
using std::unordered_set;

class UserCommand;

class ClientManager : public Speaker<ClientManagerListener>,
	private ClientListener, public Singleton<ClientManager>,
	private TimerManagerListener
{
	typedef unordered_set<Client*> ClientList;

	typedef unordered_map<CID, UserPtr> UserMap;

	typedef unordered_multimap<CID, OnlineUser*> OnlineMap;
	typedef OnlineMap::iterator OnlineIter;
	typedef OnlineMap::const_iterator OnlineIterC;
	typedef pair<OnlineIter, OnlineIter> OnlinePair;
	typedef pair<OnlineIterC, OnlineIterC> OnlinePairC;

public:
	Client* getClient(const string& aHubURL);
	void putClient(Client* aClient);

	size_t getUserCount() const;
	int64_t getAvailable() const;

	StringList getNicks(const CID& cid) const;
	StringPairList getHubs(const CID& cid) const;
	StringList getHubNames(const CID& cid) const;
	StringList getHubUrls(const CID& cid) const;

	vector<Identity> getIdentities(const UserPtr &u) const;

	string getNick(const HintedUser& user) const;
	string getHubName(const HintedUser& user) const;

	string getField(const CID& cid, const string& hintUrl, const char* field) const;
	string getConnection(const CID& cid) const;

	string getOfflineNick(const CID& cid) const;

	bool isConnected(const string& aUrl) const;
	bool isHubConnected(const string& aUrl) const;

	void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const string& aKey);
	void search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, const string& aKey);
	void infoUpdated();

	UserPtr getUser(const string& aNick, const string& aHubUrl) noexcept;
	UserPtr getUser(const CID& cid) noexcept;

	string findHub(const string& ipPort) const;
	string findHubEncoding(const string& aUrl) const;

	/** Get an OnlineUser object - lock it with lock()!
	* @return OnlineUser* found by CID, using the hub URL as a hint.
	*/
	OnlineUser* findOnlineUser(const HintedUser& user) const;
	OnlineUser* findOnlineUser(const CID& cid, const string& hintUrl) const;
	/// @return OnlineUser* found by CID and hint; discard any user that doesn't match the hint.
	OnlineUser* findOnlineUserHint(const HintedUser& user) const;
	OnlineUser* findOnlineUserHint(const CID& cid, const string& hintUrl) const;

	UserPtr findUser(const string& aNick, const string& aHubUrl) const noexcept { return findUser(makeCid(aNick, aHubUrl)); }
	UserPtr findUser(const CID& cid) const noexcept;
	/** Find an online NMDC user by nick only (random user returned if multiple hubs share users
	with the same nick). The nick is given in hub-dependant encoding. */
	HintedUser findLegacyUser(const string& nick) const noexcept;

	bool isOnline(const UserPtr& aUser) const {
		Lock l(cs);
		return onlineUsers.find(aUser->getCID()) != onlineUsers.end();
	}

	bool isOp(const UserPtr& aUser, const string& aHubUrl) const;

	/** Constructs a synthetic, hopefully unique CID */
	CID makeCid(const string& nick, const string& hubUrl) const noexcept;

	/** Send a ClientListener::Updated signal for every connected user. */
	void updateUsers();
	void putOnline(OnlineUser* ou) noexcept;
	void putOffline(OnlineUser* ou, bool disconnect = false) noexcept;

	UserPtr& getMe();

	void sendUDP(AdcCommand& cmd, const OnlineUser& user, const string& aKey = Util::emptyString);

	void connect(const HintedUser& user, const string& token, ConnectionType type = CONNECTION_TYPE_LAST);
	void privateMessage(const HintedUser& user, const string& msg, bool thirdPerson);
	void userCommand(const HintedUser& user, const UserCommand& uc, ParamMap& params, bool compatibility);
	bool isActive() const;

	Lock lock() { return Lock(cs); }

	/// Access current hubs - lock this with lock()!
	const ClientList& getClients() const { return clients; }

	/// Access known users (online and offline) - lock this with lock()!
	const UserMap& getUsers() const { return users; }
	UserMap& getUsers() { return users; }

	/// Access online users - lock this with lock()!
	const OnlineMap& getOnlineUsers() const { return onlineUsers; }
	OnlineMap& getOnlineUsers() { return onlineUsers; }

	CID getMyCID();
	const CID& getMyPID();

	void loadUsers();
	void saveUsers() const;
	void saveUser(const CID& cid);

private:
	typedef unordered_map<string, UserPtr> LegacyMap;
	typedef LegacyMap::iterator LegacyIter;

	typedef UserMap::iterator UserIter;

	typedef std::pair<std::string, bool> NickMapEntry; // the boolean being true means "save this".
	typedef unordered_map<CID, NickMapEntry> NickMap;

	ClientList clients;
	mutable CriticalSection cs;

	UserMap users;
	OnlineMap onlineUsers;
	NickMap nicks;

	UserPtr me;

	Socket udp;

	CID pid;

	friend class Singleton<ClientManager>;

	ClientManager();
	virtual ~ClientManager();

	void updateUser(const OnlineUser& user) noexcept;

	/**
	* @param p OnlinePair of all the users found by CID, even those who don't match the hint.
	* @return OnlineUser* found by CID and hint; discard any user that doesn't match the hint.
	*/
	OnlineUser* findOnlineUserHint(const CID& cid, const string& hintUrl, OnlinePairC& p) const;

	void sendUDP(const string& ip, const string& port, const string& data, const string& aKey = Util::emptyString);

	string getUsersFile() const { return Util::getPath(Util::PATH_USER_LOCAL) + "Users.xml"; }

	// ClientListener
	virtual void on(Connected, Client* c) noexcept;
	virtual void on(UserUpdated, Client*, const OnlineUser& user) noexcept;
	virtual void on(UsersUpdated, Client* c, const OnlineUserList&) noexcept;
	virtual void on(Failed, Client*, const string&) noexcept;
	virtual void on(HubUpdated, Client* c) noexcept;
	virtual void on(HubUserCommand, Client*, int, int, const string&, const string&) noexcept;
	virtual void on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize,
		int aFileType, const string& aString) noexcept;
	virtual void on(AdcSearch, Client*, const AdcCommand& cmd, const OnlineUser& from) noexcept;
	// TimerManagerListener
	virtual void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;
};

} // namespace dcpp

#endif // !defined(CLIENT_MANAGER_H)
