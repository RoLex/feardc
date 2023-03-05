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
#include "Client.h"

#include "BufferedSocket.h"
#include "ClientManager.h"
#include "ConnectivityManager.h"
#include "FavoriteManager.h"
#include "TimerManager.h"
#include "UserMatchManager.h"
#include "PluginManager.h"

#include "AdcHub.h" // for dynamic_cast

namespace dcpp {

std::atomic_long Client::counts[COUNT_UNCOUNTED];

uint32_t idCounter = 0;

Client::Client(const string& hubURL, char separator_, bool secure_) :
	myIdentity(ClientManager::getInstance()->getMe(), 0), uniqueId(++idCounter),
	reconnDelay(120), lastActivity(GET_TICK()), registered(false), autoReconnect(false),
	encoding(Text::systemCharset), state(STATE_DISCONNECTED), sock(0),
	hubUrl(hubURL),separator(separator_),
	secure(secure_), countType(COUNT_UNCOUNTED)
{
	string file, proto, query, fragment;
	Util::decodeUrl(hubURL, proto, address, port, file, query, fragment);
	keyprint = Util::decodeQuery(query)["kp"];

	TimerManager::getInstance()->addListener(this);
}

Client::~Client() {
	dcassert(!sock);

	// In case we were deleted before we Failed
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	TimerManager::getInstance()->removeListener(this);
	updateCounts(true);
}

void Client::reconnect() {
	disconnect(true);
	setAutoReconnect(true);
	setReconnDelay(0);
}

void Client::shutdown() {
	if(sock) {
		BufferedSocket::putSocket(sock);
		sock = 0;
	}
}

void Client::reloadSettings(bool updateNick) {
	/// @todo update the nick in ADC hubs?
	string prevNick;
	if(!updateNick)
		prevNick = get(Nick);

	*static_cast<HubSettings*>(this) = SettingsManager::getInstance()->getHubSettings();

	auto fav = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());
	if(fav) {
		FavoriteManager::getInstance()->mergeHubSettings(*fav, *this);

		if(!fav->getPassword().empty())
			setPassword(fav->getPassword());

		if(!fav->getEncoding().empty())
			setEncoding(fav->getEncoding());
	}

	if(updateNick)
		checkNick(get(Nick));
	else
		get(Nick) = prevNick;
}

const string& Client::getUserIp4() const {
	if(!get(UserIp).empty()) {
		return get(UserIp);
	}
	return CONNSETTING(EXTERNAL_IP);
}

const string& Client::getUserIp6() const {
	if(!get(UserIp6).empty()) {
		return get(UserIp6);
	}
	return CONNSETTING(EXTERNAL_IP6);
}

bool Client::isActive() const {
	return isActiveV4() || isActiveV6();
}

bool Client::isActiveV4() const {
	return get(HubSettings::Connection) != SettingsManager::INCOMING_PASSIVE && get(HubSettings::Connection) != SettingsManager::INCOMING_DISABLED;
}

bool Client::isActiveV6() const {
	return !v4only() && get(HubSettings::Connection6) != SettingsManager::INCOMING_PASSIVE && get(HubSettings::Connection6) != SettingsManager::INCOMING_DISABLED;
}

void Client::connect() {
	if(sock) {
		BufferedSocket::putSocket(sock);
		sock = 0;
	}

	setAutoReconnect(true);
	setReconnDelay(120 + Util::rand(0, 60));
	reloadSettings(true);
	setRegistered(false);
	setMyIdentity(Identity(ClientManager::getInstance()->getMe(), 0));
	setHubIdentity(Identity());

	state = STATE_CONNECTING;

	try {
		sock = BufferedSocket::getSocket(separator, v4only());
		sock->addListener(this);
		sock->connect(address, port, secure, SETTING(ALLOW_UNTRUSTED_HUBS), true, keyprint);
	} catch(const Exception& e) {
		state = STATE_DISCONNECTED;
		fire(ClientListener::Failed(), this, e.getError());
	}
	updateActivity();
}

void Client::info() {
	if(isConnected()) {
		sock->callAsync([this] { infoImpl(); });
	}
}

void Client::send(const char* aMessage, size_t aLen) {
	if(!isConnected()) {
		dcassert(0);
		return;
	}

	if(PluginManager::getInstance()->runHook(HOOK_NETWORK_HUB_OUT, this, aMessage))
		return;

	updateActivity();
	sock->write(aMessage, aLen);
}

HubData* Client::getPluginObject() noexcept {
	resetEntity();

	pod.url = pluginString(hubUrl);
	pod.ip = pluginString(ip);
	pod.object = this;
	pod.port = Util::toInt(port);
	pod.protocol = dynamic_cast<AdcHub*>(this) ? PROTOCOL_ADC : PROTOCOL_NMDC; // TODO: dynamic_cast not practical if more than two protocols
	pod.isOp = isOp() ? True : False;
	pod.isSecure = isSecure() ? True : False;

	return &pod;
}

void Client::on(Connected) noexcept {
	updateActivity();
	ip = sock->getIp();

	fire(ClientListener::Connected(), this);
	state = STATE_PROTOCOL;
}

void Client::on(Failed, const string& aLine) noexcept {
	state = STATE_DISCONNECTED;
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	fire(ClientListener::Failed(), this, aLine);
}

void Client::disconnect(bool graceLess) {
	if(sock)
		sock->disconnect(graceLess);
}

bool Client::isConnected() const {
	State s = state;
	return s != STATE_CONNECTING && s != STATE_DISCONNECTED;
}

bool Client::isSecure() const {
	return isConnected() && sock->isSecure();
}

bool Client::isTrusted() const {
	return isConnected() && sock->isTrusted();
}

string Client::getCipherName() const {
	return isConnected() ? sock->getCipherName() : Util::emptyString;
}

ByteVector Client::getKeyprint() const {
	return isConnected() ? sock->getKeyprint() : vector<uint8_t>();
}

void Client::updateCounts(bool aRemove) {
	// We always remove the count and then add the correct one if requested...
	if(countType != COUNT_UNCOUNTED) {
		--counts[countType];
		countType = COUNT_UNCOUNTED;
	}

	if(!aRemove) {
		if(getMyIdentity().isOp()) {
			countType = COUNT_OP;
		} else if(getMyIdentity().isRegistered()) {
			countType = COUNT_REGISTERED;
		} else {
			countType = COUNT_NORMAL;
		}
		++counts[countType];
	}
}

string Client::getCounts() {
	char buf[128];
	return string(buf, snprintf(buf, sizeof(buf), "%ld/%ld/%ld",
		counts[COUNT_NORMAL].load(), counts[COUNT_REGISTERED].load(), counts[COUNT_OP].load()));
}

void Client::updateUsers() {
	// this is a public call, can come from any thread. bring it to this hub's thread.
	if(sock) sock->callAsync([this] { auto users = getUsers(); updated(users); });
}

void Client::updated(OnlineUser& user) {
	UserMatchManager::getInstance()->match(user);

	fire(ClientListener::UserUpdated(), this, user);
}

void Client::updated(OnlineUserList& users) {
	std::for_each(users.begin(), users.end(), [](OnlineUser* user) { UserMatchManager::getInstance()->match(*user); });

	fire(ClientListener::UsersUpdated(), this, users);
}

void Client::on(Line, const string& aLine) noexcept {
	updateActivity();
}

void Client::on(Second, uint64_t aTick) noexcept {
	if(state == STATE_DISCONNECTED && getAutoReconnect() && (aTick > (getLastActivity() + getReconnDelay() * 1000)) ) {
		// Try to reconnect...
		connect();
	}
}


} // namespace dcpp
