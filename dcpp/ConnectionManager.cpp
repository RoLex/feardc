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
#include "ConnectionManager.h"

#include "Client.h"
#include "ClientManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "DownloadManager.h"
#include "LogManager.h"
#include "QueueManager.h"
#include "UploadManager.h"
#include "UserConnection.h"

namespace dcpp {

ConnectionManager::ConnectionManager() :
	downloads(cqis[CONNECTION_TYPE_DOWNLOAD]),
	floodCounter(0),
	shuttingDown(false)
{
	TimerManager::getInstance()->addListener(this);

	features = {
		UserConnection::FEATURE_MINISLOTS,
		UserConnection::FEATURE_XML_BZLIST,
		UserConnection::FEATURE_ADCGET,
		UserConnection::FEATURE_TTHL,
		UserConnection::FEATURE_TTHF
	};

	adcFeatures = {
		"AD" + UserConnection::FEATURE_ADC_BAS0,
		"AD" + UserConnection::FEATURE_ADC_BASE,
		"AD" + UserConnection::FEATURE_ADC_TIGR,
		"AD" + UserConnection::FEATURE_ADC_BZIP
	};
}

string ConnectionManager::makeToken() const {
	string token;

	Lock l(cs);
	do { token = Util::toString(Util::rand()); }
	while(tokens.find(token) != tokens.end());

	return token;
}

void ConnectionManager::addToken(const string& token, const OnlineUser& user, ConnectionType type) {
	Lock l(cs);
	tokens[token] = make_pair(user.getUser()->getCID(), type);
}

void ConnectionManager::listen() {
	server.reset(new Server(false, Util::toString(CONNSETTING(TCP_PORT)), CONNSETTING(BIND_ADDRESS), CONNSETTING(BIND_ADDRESS6)));

	if(!CryptoManager::getInstance()->TLSOk()) {
		dcdebug("Skipping secure port: %d\n", CONNSETTING(TLS_PORT));
		return;
	}
	if(CONNSETTING(TCP_PORT) != 0 && (CONNSETTING(TCP_PORT) == CONNSETTING(TLS_PORT)))
	{
		LogManager::getInstance()->message(_("The encrypted transfer port cannot be the same as the transfer port, encrypted transfers will be disabled"));
		return;
	}
	secureServer.reset(new Server(true, Util::toString(CONNSETTING(TLS_PORT)), CONNSETTING(BIND_ADDRESS), CONNSETTING(BIND_ADDRESS6)));
}

ConnectionQueueItem::ConnectionQueueItem(const HintedUser& user, ConnectionType type) :
	token(ConnectionManager::getInstance()->makeToken()),
	lastAttempt(0),
	errors(0),
	state(WAITING),
	type(type),
	user(user)
{
}

bool ConnectionQueueItem::operator==(const ConnectionQueueItem& rhs) const {
	return rhs.getType() == getType() && rhs.getUser() == getUser();
}

bool ConnectionQueueItem::operator==(const UserPtr& user) const {
	return this->user == user;
}

/**
 * Request a connection for downloading.
 * DownloadManager::addConnection will be called as soon as the connection is ready
 * for downloading.
 * @param aUser The user to connect to.
 */
void ConnectionManager::getDownloadConnection(const HintedUser& aUser) {
	dcassert((bool)aUser.user);
	{
		Lock l(cs);
		auto i = find(downloads.begin(), downloads.end(), aUser.user);
		if(i == downloads.end()) {
			getCQI(aUser,  CONNECTION_TYPE_DOWNLOAD);
		} else {
			DownloadManager::getInstance()->checkIdle(aUser.user);
		}
	}
}

ConnectionQueueItem& ConnectionManager::getCQI(const HintedUser& user, ConnectionType type) {
	auto& container = cqis[type];
	dcassert(find(container.begin(), container.end(), user.user) == container.end());

	container.emplace_back(user, type);
	auto& cqi = container.back();

	fire(ConnectionManagerListener::Added(), &cqi);
	return cqi;
}

void ConnectionManager::putCQI(ConnectionQueueItem& cqi) {
	fire(ConnectionManagerListener::Removed(), &cqi);

	auto& container = cqis[cqi.getType()];
	dcassert(find(container.begin(), container.end(), cqi) != container.end());
	container.erase(remove(container.begin(), container.end(), cqi), container.end());
}

UserConnection* ConnectionManager::getConnection(bool aNmdc, bool secure) noexcept {
	UserConnection* uc = new UserConnection(secure);
	uc->addListener(this);
	{
		Lock l(cs);
		userConnections.push_back(uc);
	}
	if(aNmdc)
		uc->setFlag(UserConnection::FLAG_NMDC);
	return uc;
}

void ConnectionManager::putConnection(UserConnection* aConn) {
	aConn->removeListener(this);
	aConn->disconnect();

	Lock l(cs);
	userConnections.erase(remove(userConnections.begin(), userConnections.end(), aConn), userConnections.end());
}

void ConnectionManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	UserList passiveUsers;
	vector<UserPtr> removed;

	{
		Lock l(cs);

		bool attemptDone = false;

		for(auto& cqi: downloads) {

			if(cqi.getState() != ConnectionQueueItem::ACTIVE) {
				if(!cqi.getUser().user->isOnline()) {
					// Not online anymore...remove it from the pending...
					removed.push_back(cqi.getUser());
					continue;
				}

				if(cqi.getUser().user->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive()) {
					passiveUsers.push_back(cqi.getUser());
					removed.push_back(cqi.getUser());
					continue;
				}

				if(cqi.getErrors() == -1 && cqi.getLastAttempt() != 0) {
					// protocol error, don't reconnect except after a forced attempt
					continue;
				}

				if(cqi.getLastAttempt() == 0 || (!attemptDone &&
					cqi.getLastAttempt() + 60 * 1000 * max(1, cqi.getErrors()) < aTick))
				{
					cqi.setLastAttempt(aTick);

					QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(cqi.getUser());

					if(prio == QueueItem::PAUSED) {
						removed.push_back(cqi.getUser());
						continue;
					}

					bool startDown = DownloadManager::getInstance()->startDownload(prio);

					if(cqi.getState() == ConnectionQueueItem::WAITING) {
						if(startDown) {
							cqi.setState(ConnectionQueueItem::CONNECTING);
							ClientManager::getInstance()->connect(cqi.getUser(), cqi.getToken());
							fire(ConnectionManagerListener::StatusChanged(), &cqi);
							attemptDone = true;
						} else {
							cqi.setState(ConnectionQueueItem::NO_DOWNLOAD_SLOTS);
							fire(ConnectionManagerListener::Failed(), &cqi, _("All download slots taken"));
						}
					} else if(cqi.getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS && startDown) {
						cqi.setState(ConnectionQueueItem::WAITING);
					}
				} else if(cqi.getState() == ConnectionQueueItem::CONNECTING && cqi.getLastAttempt() + 50 * 1000 < aTick) {
					cqi.setErrors(cqi.getErrors() + 1);
					fire(ConnectionManagerListener::Failed(), &cqi, _("Connection timeout"));
					cqi.setState(ConnectionQueueItem::WAITING);
				}
			}
		}

		// imitate putCQI but for multiple items at once.
		downloads.erase(std::remove_if(downloads.begin(), downloads.end(), [this, &removed](ConnectionQueueItem& cqi) {
			if(std::find(removed.begin(), removed.end(), cqi.getUser().user) != removed.end()) {
				fire(ConnectionManagerListener::Removed(), &cqi);
				return true;
			}
			return false;
		}), downloads.end());
	}

	for(auto& ui: passiveUsers) {
		QueueManager::getInstance()->removeSource(ui, QueueItem::Source::FLAG_PASSIVE);
	}
}

void ConnectionManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
	Lock l(cs);

	// remove tokens associated with offline users.
	for(auto i = tokens.begin(); i != tokens.end();) {
		auto user = ClientManager::getInstance()->findUser(i->second.first);
		if(user && user->isOnline()) {
			++i;
		} else {
			i = tokens.erase(i);
		}
	}

	// disconnect connections that have timed out.
	for(auto& conn: userConnections) {
		if(!conn->isSet(UserConnection::FLAG_PM) && (conn->getLastActivity() + 180*1000) < aTick) {
			conn->disconnect(true);
		}
	}
}

const string& ConnectionManager::getPort() const {
	return server.get() ? server->getPort() : Util::emptyString;
}

const string& ConnectionManager::getSecurePort() const {
	return secureServer.get() ? secureServer->getPort() : Util::emptyString;
}

static const uint32_t FLOOD_TRIGGER = 20000;
static const uint32_t FLOOD_ADD = 2000;

ConnectionManager::Server::Server(bool secure, const string& port_, const string& ipv4, const string& ipv6) :
sock(Socket::TYPE_TCP), secure(secure), die(false)
{
	sock.setLocalIp4(ipv4);
	sock.setLocalIp6(ipv6);
	sock.setV4only(false);
	port = sock.listen(port_);

	start();
}

static const uint32_t POLL_TIMEOUT = 250;

int ConnectionManager::Server::run() noexcept {
	while(!die) {
		try {
			while(!die) {
				auto ret = sock.wait(POLL_TIMEOUT, true, false);
				if(ret.first) {
					ConnectionManager::getInstance()->accept(sock, secure);
				}
			}
		} catch(const Exception& e) {
			dcdebug("ConnectionManager::Server::run Error: %s\n", e.getError().c_str());
		}

		bool failed = false;
		while(!die) {
			try {
				sock.disconnect();
				port = sock.listen(port);

				if(failed) {
					LogManager::getInstance()->message(_("Connectivity restored"));
					failed = false;
				}
				break;
			} catch(const SocketException& e) {
				dcdebug("ConnectionManager::Server::run Stopped listening: %s\n", e.getError().c_str());

				if(!failed) {
					LogManager::getInstance()->message(str(F_("Connectivity error: %1%") % e.getError()));
					failed = true;
				}

				// Spin for 60 seconds
				for(auto i = 0; i < 60 && !die; ++i) {
					Thread::sleep(1000);
				}
			}
		}
	}
	return 0;
}

/**
 * Someone's connecting, accept the connection and wait for identification...
 * It's always the other fellow that starts sending if he made the connection.
 */
void ConnectionManager::accept(const Socket& sock, bool secure) noexcept {
	uint64_t now = GET_TICK();

	if(now > floodCounter) {
		floodCounter = now + FLOOD_ADD;
	} else {
		if(false && now + FLOOD_TRIGGER < floodCounter) {
			Socket s(Socket::TYPE_TCP);
			try {
				s.accept(sock);
			} catch(const SocketException&) {
				// ...
			}
			dcdebug("Connection flood detected!\n");
			return;
		} else {
			floodCounter += FLOOD_ADD;
		}
	}
	UserConnection* uc = getConnection(false, secure);
	uc->setFlag(UserConnection::FLAG_INCOMING);
	uc->setState(UserConnection::STATE_SUPNICK);
	uc->setLastActivity(GET_TICK());
	try {
		uc->accept(sock);
	} catch(const Exception&) {
		putConnection(uc);
		delete uc;
	}
}

void ConnectionManager::nmdcConnect(const string& aServer, const string& aPort, const string& aNick, const string& hubUrl, const string& encoding, bool secure) {
	if(shuttingDown)
		return;

	if (checkHubCCBlock(aServer, aPort, hubUrl))
		return;

	UserConnection* uc = getConnection(true, secure);
	uc->setToken(aNick);
	uc->setHubUrl(hubUrl);
	uc->setEncoding(encoding);
	uc->setState(UserConnection::STATE_CONNECT);
	uc->setFlag(UserConnection::FLAG_NMDC);
	try {
		uc->connect(aServer, aPort, Util::emptyString, BufferedSocket::NAT_NONE);
	} catch(const Exception&) {
		putConnection(uc);
		delete uc;
	}
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, const string& aPort, const string& aToken, bool secure) {
	adcConnect(aUser, aPort, Util::emptyString, BufferedSocket::NAT_NONE, aToken, secure);
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, const string& aToken, bool secure) {
	if(shuttingDown)
		return;

	UserConnection* uc = getConnection(false, secure);
	uc->setToken(aToken);
	uc->setEncoding(Text::utf8);
	uc->setState(UserConnection::STATE_CONNECT);
	uc->setHubUrl(aUser.getClient().getHubUrl());
	if(aUser.getIdentity().isOp()) {
		uc->setFlag(UserConnection::FLAG_OP);
	}

	{
		Lock l(cs);
		auto t = tokens.find(aToken);
		if(t != tokens.end() && t->second.second == CONNECTION_TYPE_PM) {
			uc->setFlag(UserConnection::FLAG_PM);
		}
	}

	try {
		uc->connect(ConnectivityManager::getInstance()->getConnectivityStatus(true /*V6*/) ? aUser.getIdentity().getIp() : aUser.getIdentity().getIp4(), aPort, localPort, natRole, aUser.getUser());
	} catch(const Exception&) {
		putConnection(uc);
		delete uc;
	}
}

void ConnectionManager::disconnect() noexcept {
	server.reset();
	secureServer.reset();
}

void ConnectionManager::on(AdcCommand::SUP, UserConnection* aSource, const AdcCommand& cmd) noexcept {
	if(aSource->getState() != UserConnection::STATE_SUPNICK) {
		// Already got this once, ignore...@todo fix support updates
		dcdebug("CM::onSUP %p sent sup twice\n", (void*)aSource);
		return;
	}

	bool baseOk = false;

	for(auto& i: cmd.getParameters()) {
		if(i.compare(0, 2, "AD") == 0) {
			string feat = i.substr(2);
			if(feat == UserConnection::FEATURE_ADC_BASE || feat == UserConnection::FEATURE_ADC_BAS0) {
				baseOk = true;
				// ADC clients must support all these...
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
				// For compatibility with older clients...
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
			} else if(feat == UserConnection::FEATURE_ZLIB_GET) {
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
			} else if(feat == UserConnection::FEATURE_ADC_BZIP) {
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
			}
		}
	}

	if(!baseOk) {
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Invalid SUP"));
		aSource->disconnect();
		return;
	}

	if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
		StringList defFeatures = adcFeatures;
		if(SETTING(COMPRESS_TRANSFERS)) {
			defFeatures.push_back("AD" + UserConnection::FEATURE_ZLIB_GET);
		}
		aSource->sup(defFeatures);
	} else {
		aSource->inf(true);
	}
	aSource->setState(UserConnection::STATE_INF);
}

void ConnectionManager::on(AdcCommand::STA, UserConnection*, const AdcCommand& cmd) noexcept {

}

void ConnectionManager::on(UserConnectionListener::Connected, UserConnection* aSource) noexcept {
	dcassert(aSource->getState() == UserConnection::STATE_CONNECT);
	if(aSource->isSet(UserConnection::FLAG_NMDC)) {
		aSource->myNick(aSource->getToken());
		aSource->lock(CryptoManager::getInstance()->getLock(), CryptoManager::getInstance()->getPk() + "Ref=" + aSource->getHubUrl());
	} else {
		StringList defFeatures = adcFeatures;
		if(SETTING(COMPRESS_TRANSFERS)) {
			defFeatures.push_back("AD" + UserConnection::FEATURE_ZLIB_GET);
		}
		aSource->sup(defFeatures);
		aSource->send(AdcCommand(AdcCommand::SEV_SUCCESS, AdcCommand::SUCCESS, Util::emptyString).addParam("RF", aSource->getHubUrl()));
	}
	aSource->setState(UserConnection::STATE_SUPNICK);
}

void ConnectionManager::on(UserConnectionListener::MyNick, UserConnection* aSource, const string& aNick) noexcept {
	if(aSource->getState() != UserConnection::STATE_SUPNICK) {
		// Already got this once, ignore...
		dcdebug("CM::onMyNick %p sent nick twice\n", (void*)aSource);
		return;
	}

	dcassert(!aNick.empty());
	dcdebug("ConnectionManager::onMyNick %p, %s\n", (void*)aSource, aNick.c_str());
	dcassert(!aSource->getUser());

	if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
		// Try to guess where this came from...
		pair<string, string> i = expectedConnections.remove(aNick);
		if(i.second.empty()) {
			dcassert(i.first.empty());
			dcdebug("Unknown incoming connection from %s\n", aNick.c_str());
			putConnection(aSource);
			return;
		}
		aSource->setToken(i.first);
		aSource->setHubUrl(i.second);
		aSource->setEncoding(ClientManager::getInstance()->findHubEncoding(i.second));
	}

	string nick = Text::toUtf8(aNick, aSource->getEncoding());
	CID cid = ClientManager::getInstance()->makeCid(nick, aSource->getHubUrl());

	// First, we try looking in the pending downloads...hopefully it's one of them...
	{
		Lock l(cs);
		for(auto& cqi: downloads) {
			cqi.setErrors(0);
			if((cqi.getState() == ConnectionQueueItem::CONNECTING || cqi.getState() == ConnectionQueueItem::WAITING) &&
				cqi.getUser().user->getCID() == cid)
			{
				aSource->setUser(cqi.getUser());
				// Indicate that we're interested in this file...
				aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
				break;
			}
		}
	}

	if(!aSource->getUser()) {
		// Make sure we know who it is, i e that he/she is connected...

		aSource->setUser(ClientManager::getInstance()->findUser(cid));
		if(!aSource->getUser() || !ClientManager::getInstance()->isOnline(aSource->getUser())) {
			dcdebug("CM::onMyNick Incoming connection from unknown user %s\n", nick.c_str());
			putConnection(aSource);
			return;
		}
		// We don't need this connection for downloading...make it an upload connection instead...
		aSource->setFlag(UserConnection::FLAG_UPLOAD);
	}

	if(ClientManager::getInstance()->isOp(aSource->getUser(), aSource->getHubUrl()))
		aSource->setFlag(UserConnection::FLAG_OP);

	if( aSource->isSet(UserConnection::FLAG_INCOMING) ) {
		aSource->myNick(aSource->getToken());
		aSource->lock(CryptoManager::getInstance()->getLock(), CryptoManager::getInstance()->getPk());
	}

	aSource->setState(UserConnection::STATE_LOCK);
}

void ConnectionManager::on(UserConnectionListener::CLock, UserConnection* aSource, const string& aLock, const string& aPk) noexcept {
	if(aSource->getState() != UserConnection::STATE_LOCK) {
		dcdebug("CM::onLock %p received lock twice, ignoring\n", (void*)aSource);
		return;
	}

	if( CryptoManager::getInstance()->isExtended(aLock) ) {
		StringList defFeatures = features;
		if(SETTING(COMPRESS_TRANSFERS)) {
			defFeatures.push_back(UserConnection::FEATURE_ZLIB_GET);
		}

		aSource->supports(defFeatures);
	}

	aSource->setState(UserConnection::STATE_DIRECTION);
	aSource->direction(aSource->getDirectionString(), aSource->getNumber());
	aSource->key(CryptoManager::getInstance()->makeKey(aLock));
}

void ConnectionManager::on(UserConnectionListener::Direction, UserConnection* aSource, const string& dir, const string& num) noexcept {
	if(aSource->getState() != UserConnection::STATE_DIRECTION) {
		dcdebug("CM::onDirection %p received direction twice, ignoring\n", (void*)aSource);
		return;
	}

	dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));
	if(dir == "Upload") {
		// Fine, the other fellow want's to send us data...make sure we really want that...
		if(aSource->isSet(UserConnection::FLAG_UPLOAD)) {
			// Huh? Strange...disconnect...
			putConnection(aSource);
			return;
		}
	} else {
		if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
			int number = Util::toInt(num);
			// Damn, both want to download...the one with the highest number wins...
			if(aSource->getNumber() < number) {
				// Damn! We lost!
				aSource->unsetFlag(UserConnection::FLAG_DOWNLOAD);
				aSource->setFlag(UserConnection::FLAG_UPLOAD);
			} else if(aSource->getNumber() == number) {
				putConnection(aSource);
				return;
			}
		}
	}

	dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));

	aSource->setState(UserConnection::STATE_KEY);
}

void ConnectionManager::addDownloadConnection(UserConnection* uc) {
	bool addConn = false;
	{
		Lock l(cs);

		auto i = find(downloads.begin(), downloads.end(), uc->getUser());
		if(i != downloads.end()) {
			auto& cqi = *i;

			if(cqi.getState() == ConnectionQueueItem::WAITING || cqi.getState() == ConnectionQueueItem::CONNECTING) {
				cqi.setState(ConnectionQueueItem::ACTIVE);
				uc->setFlag(UserConnection::FLAG_ASSOCIATED);

				fire(ConnectionManagerListener::Connected(), &cqi, uc);

				dcdebug("ConnectionManager::addDownloadConnection, leaving to downloadmanager\n");
				addConn = true;
			}
		}
	}

	if(addConn) {
		DownloadManager::getInstance()->addConnection(uc);
	} else {
		putConnection(uc);
	}
}

void ConnectionManager::addNewConnection(UserConnection* uc, ConnectionType type) {
	bool addConn = false;
	if(type != CONNECTION_TYPE_PM || SETTING(ENABLE_CCPM)) {
		Lock l(cs);

		auto& container = cqis[type];
		auto i = find(container.begin(), container.end(), uc->getUser());
		if(i == container.end()) {
			auto& cqi = getCQI(uc->getHintedUser(), type);

			cqi.setState(ConnectionQueueItem::ACTIVE);
			uc->setFlag(UserConnection::FLAG_ASSOCIATED);

			fire(ConnectionManagerListener::Connected(), &cqi, uc);

			dcdebug("ConnectionManager::addNewConnection, leaving to uploadmanager or PM handler\n");
			addConn = true;
		}
	}

	if(addConn) {
		if(type == CONNECTION_TYPE_UPLOAD) {
			UploadManager::getInstance()->addConnection(uc);
		}
	} else {
		putConnection(uc);
	}
}

void ConnectionManager::on(UserConnectionListener::Key, UserConnection* aSource, const string&/* aKey*/) noexcept {
	if(aSource->getState() != UserConnection::STATE_KEY) {
		dcdebug("CM::onKey Bad state, ignoring");
		return;
	}

	dcassert(aSource->getUser());

	if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
		addDownloadConnection(aSource);
	} else {
		addNewConnection(aSource, CONNECTION_TYPE_UPLOAD);
	}
}

void ConnectionManager::on(AdcCommand::INF, UserConnection* aSource, const AdcCommand& cmd) noexcept {
	if(aSource->getState() != UserConnection::STATE_INF) {
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Expecting INF"));
		aSource->disconnect();
		return;
	}

	// Leaks CSUPs, other client's CINF, and ADC connection's presence. Allows removing
	// user from queue by waiting long enough for aSource->getUser() to function.
	if(SETTING(REQUIRE_TLS) && !aSource->isSet(UserConnection::FLAG_NMDC) && !aSource->isSecure()) {
		putConnection(aSource);
		QueueManager::getInstance()->removeSource(aSource->getUser(), QueueItem::Source::FLAG_UNENCRYPTED);
		return;
	}

	string cid;
	if(!cmd.getParam("ID", 0, cid)) {
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID missing").addParam("FM", "ID"));
		dcdebug("CM::onINF missing ID\n");
		aSource->disconnect();
		return;
	}

	if(!aSource->getUser()) {
		aSource->setUser(ClientManager::getInstance()->findUser(CID(cid)));

		if(!aSource->getUser()) {
			aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF ID: user not found").addParam("FB", "ID"));
			putConnection(aSource);
			return;
		}
	}

	// without a valid KeyPrint this degrades into normal turst check
	if(!checkKeyprint(aSource)) {
		QueueManager::getInstance()->removeSource(aSource->getUser(), QueueItem::Source::FLAG_UNTRUSTED);
		putConnection(aSource);
		return;
	}

	auto type = CONNECTION_TYPE_LAST;

	if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
		string token;
		if(!cmd.getParam("TO", 0, token)) {
			aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF TO missing").addParam("FM", "TO"));
			putConnection(aSource);
			return;
		}
		aSource->setToken(token);

		auto tokCheck = checkToken(aSource);
		if(!tokCheck.first) {
			aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_FIELD, "INF TO: invalid token").addParam("FB", "TO"));
			putConnection(aSource);
			return;
		}
		type = tokCheck.second;

		// set the PM flag now in order to send a INF with PM1
		if(type == CONNECTION_TYPE_PM || cmd.hasFlag("PM", 0)) {
			if(!aSource->isSet(UserConnection::FLAG_PM)) {
				aSource->setFlag(UserConnection::FLAG_PM);
			}

			if (!aSource->getUser()->isSet(User::TLS)) {
				aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_GENERIC, "Unencrypted CCPM connections aren't allowed"));
				putConnection(aSource);
				return;
			}
		}

		aSource->inf(false);
	}

	if(type == CONNECTION_TYPE_DOWNLOAD || checkDownload(aSource)) {
		if(!aSource->isSet(UserConnection::FLAG_DOWNLOAD)) { aSource->setFlag(UserConnection::FLAG_DOWNLOAD); }
		addDownloadConnection(aSource);

	} else if(type == CONNECTION_TYPE_PM || aSource->isSet(UserConnection::FLAG_PM) || cmd.hasFlag("PM", 0)) {
		if(!aSource->isSet(UserConnection::FLAG_PM)) { aSource->setFlag(UserConnection::FLAG_PM); }
		addNewConnection(aSource, CONNECTION_TYPE_PM);

	} else {
		if(!aSource->isSet(UserConnection::FLAG_UPLOAD)) { aSource->setFlag(UserConnection::FLAG_UPLOAD); }
		addNewConnection(aSource, CONNECTION_TYPE_UPLOAD);
	}
}

void ConnectionManager::force(const UserPtr& aUser) {
	Lock l(cs);

	auto i = find(downloads.begin(), downloads.end(), aUser);
	if(i != downloads.end()) {
		i->setLastAttempt(0);
	}
}

bool ConnectionManager::checkKeyprint(UserConnection* aSource) {
	dcassert(aSource->getUser());

	if(!aSource->isSecure() || aSource->isTrusted())
		return true;

	string kp = ClientManager::getInstance()->getField(aSource->getUser()->getCID(), aSource->getHubUrl(), "KP");
	return aSource->verifyKeyprint(kp, SETTING(ALLOW_UNTRUSTED_CLIENTS));
}

pair<bool, ConnectionType> ConnectionManager::checkToken(const UserConnection* uc) const {
	Lock l(cs);

	auto t = tokens.find(uc->getToken());
	if(t != tokens.end() && t->second.first == uc->getUser()->getCID()) {
		return make_pair(true, t->second.second);
	}

	return make_pair(false, CONNECTION_TYPE_LAST);
}

bool ConnectionManager::checkDownload(const UserConnection* uc) const {
	Lock l(cs);

	auto d = find(downloads.begin(), downloads.end(), uc->getUser());
	if(d != downloads.end() && d->getToken() == uc->getToken()) {
		d->setErrors(0);
		return true;
	}

	return false;
}

void ConnectionManager::failed(UserConnection* aSource, const string& aError, bool protocolError) {
	Lock l(cs);

	if(aSource->isSet(UserConnection::FLAG_ASSOCIATED)) {

		if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
			auto i = find(downloads.begin(), downloads.end(), aSource->getUser());
			dcassert(i != downloads.end());

			auto& cqi = *i;
			cqi.setState(ConnectionQueueItem::WAITING);
			cqi.setLastAttempt(GET_TICK());
			cqi.setErrors(protocolError ? -1 : (cqi.getErrors() + 1));
			fire(ConnectionManagerListener::Failed(), &cqi, aError);

		} else {
			auto type = aSource->isSet(UserConnection::FLAG_UPLOAD) ? CONNECTION_TYPE_UPLOAD :
				aSource->isSet(UserConnection::FLAG_PM) ? CONNECTION_TYPE_PM : CONNECTION_TYPE_LAST;
			if(type != CONNECTION_TYPE_LAST) {
				auto& container = cqis[type];
				auto i = find(container.begin(), container.end(), aSource->getUser());
				dcassert(i != container.end());
				putCQI(*i);
			}
		}
	}

	putConnection(aSource);
}

bool ConnectionManager::checkHubCCBlock(const string& aServer, const string& aPort, const string& aHubUrl)
{
	const auto server_lower = Text::toLower(aServer);
	dcassert(server_lower == aServer);

	bool cc_blocked = false;

	{
		Lock l(cs);
		cc_blocked = !hubsBlockingCC.empty() && hubsBlockingCC.find(server_lower) != hubsBlockingCC.end();
	}

    if(cc_blocked)
	{
		LogManager::getInstance()->message(str(F_("Blocked a C-C connection to a hub ('%1%:%2%'; request from '%3%')") % aServer % aPort % aHubUrl));
		return true;
	}

	return false;
}

void ConnectionManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) noexcept {
	failed(aSource, aError, false);
}

void ConnectionManager::on(UserConnectionListener::ProtocolError, UserConnection* aSource, const string& aError) noexcept {
	if(aError.compare(0, 7, "CTM2HUB", 7) == 0) {
		{
			Lock l(cs);
			hubsBlockingCC.insert(Text::toLower(aSource->getRemoteIp()));
		}

		string aServerPort = aSource->getRemoteIp() + ":" + aSource->getPort();
		LogManager::getInstance()->message(str(F_("Blocking '%1%', potential DDoS detected (originating hub '%2%')") % aServerPort % aSource->getHubUrl() ));
	}

	failed(aSource, aError, true);
}

void ConnectionManager::disconnect(const UserPtr& user) {
	Lock l(cs);
	for(auto uc: userConnections) {
		if(uc->getUser() == user)
			uc->disconnect(true);
	}
}

void ConnectionManager::disconnect(const UserPtr& user, ConnectionType type) {
	Lock l(cs);
	for(auto uc: userConnections) {
		if(uc->getUser() == user && uc->isSet(type == CONNECTION_TYPE_DOWNLOAD ? UserConnection::FLAG_DOWNLOAD :
			type == CONNECTION_TYPE_UPLOAD ? UserConnection::FLAG_UPLOAD : UserConnection::FLAG_PM))
		{
			uc->disconnect(true);
			break;
		}
	}
}

void ConnectionManager::shutdown() {
	TimerManager::getInstance()->removeListener(this);

	shuttingDown = true;
	disconnect();
	{
		Lock l(cs);
		for(auto j: userConnections) {
			j->disconnect(true);
		}
	}
	// Wait until all connections have died out...
	while(true) {
		{
			Lock l(cs);
			if(userConnections.empty()) {
				break;
			}
		}
		Thread::sleep(50);
	}
}

// UserConnectionListener
void ConnectionManager::on(UserConnectionListener::Supports, UserConnection* conn, const StringList& feat) noexcept {
	for(auto& i: feat) {
		if(i == UserConnection::FEATURE_MINISLOTS) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
		} else if(i == UserConnection::FEATURE_XML_BZLIST) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
		} else if(i == UserConnection::FEATURE_ADCGET) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
		} else if(i == UserConnection::FEATURE_ZLIB_GET) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
		} else if(i == UserConnection::FEATURE_TTHL) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
		} else if(i == UserConnection::FEATURE_TTHF) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
		}
	}
}

} // namespace dcpp
