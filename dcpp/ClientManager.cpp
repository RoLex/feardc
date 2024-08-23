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
#include "ClientManager.h"

#include "AdcHub.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "FavoriteManager.h"
#include "File.h"
#include "NmdcHub.h"
#include "SearchManager.h"
#include "SearchResult.h"
#include "ShareManager.h"
#include "PluginManager.h"
#include "SimpleXML.h"
#include "UserCommand.h"

namespace dcpp {

ClientManager::ClientManager():
udp(Socket::TYPE_UDP)
{
	TimerManager::getInstance()->addListener(this);
}

ClientManager::~ClientManager() {
	TimerManager::getInstance()->removeListener(this);
}

Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;

	if (Util::strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(aHubURL, false);
	} else if (Util::strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(aHubURL, true);
	} else if (Util::strnicmp("nmdcs://", aHubURL.c_str(), 8) == 0) {
		c = new NmdcHub(aHubURL, true);
	} else {
		c = new NmdcHub(aHubURL);
	}

	{
		Lock l(cs);
		clients.insert(c);
	}

	c->addListener(this);
	return c;
}

void ClientManager::putClient(Client* aClient) {
	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);
		clients.erase(aClient);
	}
	aClient->shutdown();
	delete aClient;
}

size_t ClientManager::getUserCount() const {
	Lock l(cs);
	return onlineUsers.size();
}

StringList ClientManager::getNicks(const CID& cid) const {
	Lock l(cs);
	StringSet ret;

	OnlinePairC op = onlineUsers.equal_range(cid);
	for(auto i = op.first; i != op.second; ++i) {
		ret.insert(i->second->getIdentity().getNick());
	}

	if(ret.empty()) {
		ret.insert(getOfflineNick(cid));
	}

	return StringList(ret.begin(), ret.end());
}

StringPairList ClientManager::getHubs(const CID& cid) const {
	Lock l(cs);
	StringPairList lst;
	auto op = onlineUsers.equal_range(cid);
	for(auto i = op.first; i != op.second; ++i) {
		lst.push_back(make_pair(i->second->getClient().getHubUrl(), i->second->getClient().getHubName()));
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(auto i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubName());
	}
	return lst;
}

StringList ClientManager::getHubUrls(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(auto i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubUrl());
	}
	return lst;
}

vector<Identity> ClientManager::getIdentities(const UserPtr &u) const {
	Lock l(cs);
	auto op = onlineUsers.equal_range(u->getCID());
	auto ret = vector<Identity>();
	for(auto i = op.first; i != op.second; ++i) {
		ret.push_back(i->second->getIdentity());
	}

	return ret;
}

string ClientManager::getNick(const HintedUser& user) const {
	Lock l(cs);
	auto ou = findOnlineUserHint(user);
	return ou ? ou->getIdentity().getNick() : getOfflineNick(user.user->getCID());
}

string ClientManager::getHubName(const HintedUser& user) const {
	Lock l(cs);
	auto ou = findOnlineUserHint(user);
	return ou ? ou->getClient().getHubName() : _("Offline");
}

string ClientManager::getField(const CID& cid, const string& hint, const char* field) const {
	Lock l(cs);

	OnlinePairC p;
	auto u = findOnlineUserHint(cid, hint, p);
	if(u) {
		auto value = u->getIdentity().get(field);
		if(!value.empty()) {
			return value;
		}
	}

	for(auto i = p.first; i != p.second; ++i) {
		auto value = i->second->getIdentity().get(field);
		if(!value.empty()) {
			return value;
		}
	}

	return Util::emptyString;
}

string ClientManager::getConnection(const CID& cid) const {
	Lock l(cs);
	auto i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		return i->second->getIdentity().getConnection();
	}
	return _("Offline");
}

string ClientManager::getOfflineNick(const CID& cid) const {
	auto i = nicks.find(cid);
	return i != nicks.end() ? i->second.first : '{' + cid.toBase32() + '}';
}

int64_t ClientManager::getAvailable() const {
	Lock l(cs);
	int64_t bytes = 0;
	for(auto& i: onlineUsers) {
		bytes += i.second->getIdentity().getBytesShared();
	}

	return bytes;
}

bool ClientManager::isConnected(const string& aUrl) const {
	Lock l(cs);

	for(auto i: clients) {
		if(i->getHubUrl() == aUrl) {
			return true;
		}
	}
	return false;
}

bool ClientManager::isHubConnected(const string& aUrl) const {
	Lock l(cs);

	for(auto i: clients) {
		if(i->getHubUrl() == aUrl) {
			return i->isConnected();
		}
	}
	return false;
}

string ClientManager::findHub(const string& ipPort) const {
	Lock l(cs);

	string url;
	for(auto c: clients) {
		auto ipPortPair = NmdcHub::parseIpPort(ipPort);
		if(c->getIp() == ipPortPair.first) {
			// If exact match is found, return it
			if(c->getPort() == ipPortPair.second)
				return c->getHubUrl();

			// Port is not always correct, so use this as a best guess...
			url = c->getHubUrl();
		}
	}

	return url;
}

string ClientManager::findHubEncoding(const string& aUrl) const {
	Lock l(cs);

	for(auto i: clients) {
		if(i->getHubUrl() == aUrl) {
			return i->getEncoding();
		}
	}
	return Text::systemCharset;
}

HintedUser ClientManager::findLegacyUser(const string& nick) const noexcept {
	if(nick.empty())
		return HintedUser();

	Lock l(cs);

	for(auto i: clients) {
		auto nmdc = dynamic_cast<NmdcHub*>(i);
		if(nmdc) {
			/** @todo run the search directly on non-UTF-8 nicks when we store them. */
			auto ou = nmdc->findUser(nmdc->toUtf8(nick));
			if(ou) {
				return HintedUser(*ou);
			}
		}
	}

	return HintedUser();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) noexcept {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	auto ui = users.find(cid);
	if(ui != users.end()) {
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	UserPtr p(new User(cid));
	p->setFlag(User::NMDC);
	users.emplace(cid, p);

	return p;
}

UserPtr ClientManager::getUser(const CID& cid) noexcept {
	Lock l(cs);
	auto ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}

	if(cid == getMe()->getCID()) {
		return getMe();
	}

	UserPtr p(new User(cid));
	users.emplace(cid, p);
	return p;
}

UserPtr ClientManager::findUser(const CID& cid) const noexcept {
	Lock l(cs);
	auto ui = users.find(cid);
	return ui == users.end() ? nullptr : ui->second;
}

bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
	Lock l(cs);
	OnlinePairC p = onlineUsers.equal_range(user->getCID());
	for(auto i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getIdentity().isOp();
		}
	}
	return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) const noexcept {
	string n = Text::toLower(aNick);
	TigerHash th;
	th.update(n.c_str(), n.length());
	th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
	// Construct hybrid CID from the bits of the tiger hash - should be
	// fairly random, and hopefully low-collision
	return CID(th.finalize());
}

void ClientManager::updateUsers() {
	Lock l(cs);
	for(auto client: clients) {
		client->updateUsers();
	}
}

void ClientManager::putOnline(OnlineUser* ou) noexcept {
	{
		Lock l(cs);
		onlineUsers.emplace(ou->getUser()->getCID(), ou);
	}

	if(!ou->getUser()->isOnline()) {
		ou->getUser()->setFlag(User::ONLINE);
		fire(ClientManagerListener::UserConnected(), ou->getUser());
	}
}

void ClientManager::putOffline(OnlineUser* ou, bool disconnect) noexcept {
	OnlineIter::difference_type diff = 0;
	{
		Lock l(cs);
		auto op = onlineUsers.equal_range(ou->getUser()->getCID());
		dcassert(op.first != op.second);
		for(auto i = op.first; i != op.second; ++i) {
			auto ou2 = i->second;
			if(ou == ou2) {
				diff = distance(op.first, op.second);
				onlineUsers.erase(i);
				break;
			}
		}
	}

	if(diff == 1) { //last user
		UserPtr& u = ou->getUser();
		u->unsetFlag(User::ONLINE);
		if(disconnect)
			ConnectionManager::getInstance()->disconnect(u);
		fire(ClientManagerListener::UserDisconnected(), u);
	} else if(diff > 1) {
			fire(ClientManagerListener::UserUpdated(), *ou);
	}
}

OnlineUser* ClientManager::findOnlineUserHint(const CID& cid, const string& hintUrl, OnlinePairC& p) const {
	p = onlineUsers.equal_range(cid);
	if(p.first == p.second) // no user found with the given CID.
		return 0;

	if(!hintUrl.empty()) {
		for(auto i = p.first; i != p.second; ++i) {
			OnlineUser* u = i->second;
			if(u->getClient().getHubUrl() == hintUrl) {
				return u;
			}
		}
	}

	return 0;
}

OnlineUser* ClientManager::findOnlineUser(const HintedUser& user) const {
	return findOnlineUser(user.user->getCID(), user.hint);
}

OnlineUser* ClientManager::findOnlineUserHint(const HintedUser& user) const {
	return findOnlineUserHint(user.user->getCID(), user.hint);
}

OnlineUser* ClientManager::findOnlineUser(const CID& cid, const string& hintUrl) const {
	OnlinePairC p;
	OnlineUser* u = findOnlineUserHint(cid, hintUrl, p);
	if(u) // found an exact match (CID + hint).
		return u;

	if(p.first == p.second) // no user found with the given CID.
		return 0;

	// return a random user that matches the given CID but not the hint.
	return p.first->second;
}

OnlineUser* ClientManager::findOnlineUserHint(const CID& cid, const string& hintUrl) const {
	OnlinePairC p;
	return findOnlineUserHint(cid, hintUrl, p);
}

void ClientManager::connect(const HintedUser& user, const string& token, ConnectionType type) {
	Lock l(cs);
	OnlineUser* u = findOnlineUser(user);

	if(u) {
		u->getClient().connect(*u, token, type);
	}
}

void ClientManager::privateMessage(const HintedUser& user, const string& msg, bool thirdPerson) {
	Lock l(cs);
	OnlineUser* u = findOnlineUser(user);

	if(u && !PluginManager::getInstance()->runHook(HOOK_CHAT_PM_OUT, u, msg)) {
		u->getClient().privateMessage(*u, msg, thirdPerson);
	}
}

void ClientManager::userCommand(const HintedUser& user, const UserCommand& uc, ParamMap& params, bool compatibility) {
	Lock l(cs);
	OnlineUser* ou = findOnlineUserHint(user.user->getCID(), user.hint.empty() ? uc.getHub() : user.hint);
	if(!ou)
		return;

	ou->getIdentity().getParams(params, "user", compatibility);
	ou->getClient().getHubIdentity().getParams(params, "hub", false);
	ou->getClient().getMyIdentity().getParams(params, "my", compatibility);
	ou->getClient().sendUserCmd(uc, params);
}

void ClientManager::sendUDP(AdcCommand& cmd, const OnlineUser& user, const string& aKey) {
	dcassert(cmd.getType() == AdcCommand::TYPE_UDP);
	if(!user.getIdentity().isUdpActive()) {
		cmd.setType(AdcCommand::TYPE_DIRECT);
		cmd.setTo(user.getIdentity().getSID());
		const_cast<Client&>(user.getClient()).send(cmd);
	} else {
		auto state = CONNSTATE(INCOMING_CONNECTIONS6);
		sendUDP(state ? user.getIdentity().getIp() : user.getIdentity().getIp4(), state ? user.getIdentity().getUdpPort() : user.getIdentity().getUdp4Port(), cmd.toString(getMe()->getCID()), aKey);
	}
}

void ClientManager::sendUDP(const string& ip, const string& port, const string& data, const string& aKey) {
	if(PluginManager::getInstance()->onUDP(true, ip, port, data))
		return;

	string encryptedData;

	if(SETTING(ENABLE_SUDP) && !aKey.empty() && Encoder::isBase32(aKey.c_str())) {
		uint8_t keyChar[16];
		Encoder::fromBase32(aKey.c_str(), keyChar, 16);
		encryptedData = CryptoManager::getInstance()->encryptSUDP(keyChar, data);
	}

	try {
		udp.writeTo(ip, port, encryptedData.empty() ? data : encryptedData);
	} catch(const SocketException&) {
		dcdebug("Socket exception when sending UDP data to %s:%s\n", ip.c_str(), port.c_str());
	}
}

void ClientManager::infoUpdated() {
	Lock l(cs);
	for(auto client: clients) {
		client->info();
	}
}

void ClientManager::on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize,
									int aFileType, const string& aString) noexcept
{
	bool isPassive = (aSeeker.compare(0, 4, "Hub:") == 0);

	// We don't wan't to answer passive searches if we're in passive mode...
	if(isPassive && !ClientManager::getInstance()->isActive()) {
		return;
	}

	auto l = ShareManager::getInstance()->search(aString, aSearchType, aSize, aFileType, isPassive ? 5 : 10);
//		dcdebug("Found %d items (%s)\n", l.size(), aString.c_str());
	if(!l.empty()) {
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(const auto& sr: l) {
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += Text::fromUtf8(name, aClient->getEncoding());
				str += '|';
			}

			if(!str.empty())
				aClient->send(str);

		} else {
			string ip, port;

			auto ipPortPair = NmdcHub::parseIpPort(aSeeker);

			port = ipPortPair.second;
			ip = Socket::resolve(ipPortPair.first, AF_INET);

			if(static_cast<NmdcHub*>(aClient)->isProtectedIP(ip))
				return;

			if(port.empty())
				port = "412";

			for(const auto& sr: l) {
				sendUDP(ip, port, sr->toSR(*aClient));
			}
		}
	}
}

void ClientManager::on(AdcSearch, Client*, const AdcCommand& cmd, const OnlineUser& from) noexcept {
	SearchManager::getInstance()->respond(cmd, from);
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const string& aKey) {
	Lock l(cs);

	for(auto i: clients) {
		if(i->isConnected()) {
			i->search(aSizeMode, aSize, aFileType, aString, aToken, StringList() /*ExtList*/, aKey);
		}
	}
}

void ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, const string& aKey) {
	Lock l(cs);

	for(auto& client: who) {
		for(auto c: clients) {
			if(c->isConnected() && c->getHubUrl() == client) {
				c->search(aSizeMode, aSize, aFileType, aString, aToken, aExtList, aKey);
			}
		}
	}
}

void ClientManager::on(TimerManagerListener::Minute, uint64_t /* aTick */) noexcept {
	Lock l(cs);

	// Collect some garbage...
	auto i = users.begin();
	while(i != users.end()) {
		if(i->second->unique()) {
			users.erase(i++);
		} else {
			++i;
		}
	}

	for(auto client: clients) {
		client->info();
	}
}

UserPtr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			users.emplace(me->getCID(), me);
		}
	}
	return me;
}

bool ClientManager::isActive() const {
	return CONNSETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_PASSIVE;
}

const CID& ClientManager::getMyPID() {
	if(!pid)
		pid = CID(SETTING(PRIVATE_ID));
	return pid;
}

CID ClientManager::getMyCID() {
	TigerHash tiger;
	tiger.update(getMyPID().data(), CID::SIZE);
	return CID(tiger.finalize());
}

void ClientManager::updateUser(const OnlineUser& user) noexcept {
	if(!user.getIdentity().getNick().empty()) {
		Lock l(cs);
		auto i = nicks.find(user.getUser()->getCID());
		if(i == nicks.end()) {
			nicks[user.getUser()->getCID()] = std::make_pair(user.getIdentity().getNick(), false);
		} else {
			i->second.first = user.getIdentity().getNick();
		}
	}

	fire(ClientManagerListener::UserUpdated(), user);
}

void ClientManager::loadUsers() {
	try {
		SimpleXML xml;
		xml.fromXML(File(getUsersFile(), File::READ, File::OPEN).read());

		if(xml.findChild("Users")) {
			xml.stepIn();

			{
				Lock l(cs);
				while(xml.findChild("User")) {
					nicks[CID(xml.getChildAttrib("CID"))] = std::make_pair(xml.getChildAttrib("Nick"), false);
				}
			}

			xml.stepOut();
		}
	} catch(const Exception&) { }
}

void ClientManager::saveUsers() const {
	try {
		SimpleXML xml;
		xml.addTag("Users");
		xml.stepIn();

		{
			Lock l(cs);
			for(auto& i: nicks) {
				if(i.second.second) {
					xml.addTag("User");
					xml.addChildAttrib("CID", i.first.toBase32());
					xml.addChildAttrib("Nick", i.second.first);
				}
			}
		}

		xml.stepOut();

		const string fName = getUsersFile();
		File out(fName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(fName);
		File::renameFile(fName + ".tmp", fName);
	} catch(const Exception&) { }
}

void ClientManager::saveUser(const CID& cid) {
	Lock l(cs);
	auto i = nicks.find(cid);
	if(i != nicks.end())
		i->second.second = true;
}

void ClientManager::on(Connected, Client* c) noexcept {
	fire(ClientManagerListener::ClientConnected(), c);
}

void ClientManager::on(UserUpdated, Client*, const OnlineUser& user) noexcept {
	updateUser(user);
}

void ClientManager::on(UsersUpdated, Client*, const OnlineUserList& l) noexcept {
	for(auto& i: l) {
		updateUser(*i);
	}
}

void ClientManager::on(HubUpdated, Client* c) noexcept {
	fire(ClientManagerListener::ClientUpdated(), c);
}

void ClientManager::on(Failed, Client* client, const string&) noexcept {
	fire(ClientManagerListener::ClientDisconnected(), client);
}

void ClientManager::on(HubUserCommand, Client* client, int aType, int ctx, const string& name, const string& command) noexcept {
	if(SETTING(HUB_USER_COMMANDS)) {
		if(aType == UserCommand::TYPE_REMOVE) {
			int cmd = FavoriteManager::getInstance()->findUserCommand(name, client->getHubUrl());
			if(cmd != -1)
				FavoriteManager::getInstance()->removeUserCommand(cmd);
		} else if(aType == UserCommand::TYPE_CLEAR) {
 			FavoriteManager::getInstance()->removeHubUserCommands(ctx, client->getHubUrl());
 		} else {
 			FavoriteManager::getInstance()->addUserCommand(aType, ctx, UserCommand::FLAG_NOSAVE, name, command, "", client->getHubUrl());
 		}
	}
}

} // namespace dcpp
