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
#include "AdcHub.h"

#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#include "AdcCommand.h"
#include "ChatMessage.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "format.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "StringTokenizer.h"
#include "ThrottleManager.h"
#include "UploadManager.h"
#include "PluginManager.h"
#include "UserCommand.h"
#include "Util.h"
#include "version.h"

#include <cmath>

namespace dcpp {

using std::make_pair;

const string AdcHub::CLIENT_PROTOCOL("ADC/1.0");
const string AdcHub::SECURE_CLIENT_PROTOCOL("ADCS/0.10");
const string AdcHub::ADCS_FEATURE("ADC0");
const string AdcHub::TCP4_FEATURE("TCP4");
const string AdcHub::TCP6_FEATURE("TCP6");
const string AdcHub::UDP4_FEATURE("UDP4");
const string AdcHub::UDP6_FEATURE("UDP6");
const string AdcHub::NAT0_FEATURE("NAT0");
const string AdcHub::SEGA_FEATURE("SEGA");
const string AdcHub::CCPM_FEATURE("CCPM");
const string AdcHub::SUDP_FEATURE("SUDP");
const string AdcHub::BASE_SUPPORT("ADBASE");
const string AdcHub::BAS0_SUPPORT("ADBAS0");
const string AdcHub::TIGR_SUPPORT("ADTIGR");
const string AdcHub::UCM0_SUPPORT("ADUCM0");
const string AdcHub::BLO0_SUPPORT("ADBLO0");
const string AdcHub::ZLIF_SUPPORT("ADZLIF");

const vector<StringList> AdcHub::searchExts;

AdcHub::AdcHub(const string& aHubURL, bool secure):
	Client(aHubURL, '\n', secure),
	oldPassword(false),
	udp(Socket::TYPE_UDP),
	sid(0),
	sinceConnect(0)
{
	TimerManager::getInstance()->addListener(this);
}

AdcHub::~AdcHub() {
	TimerManager::getInstance()->removeListener(this);
	clearUsers();
}

OnlineUser& AdcHub::getUser(const uint32_t aSID, const CID& aCID) {
	OnlineUser* ou = findUser(aSID);
	if(ou) {
		return *ou;
	}

	UserPtr p = ClientManager::getInstance()->getUser(aCID);

	{
		Lock l(cs);
		ou = users.emplace(aSID, new OnlineUser(p, *this, aSID)).first->second;
	}

	if(aSID != AdcCommand::HUB_SID)
		ClientManager::getInstance()->putOnline(ou);
	return *ou;
}

OnlineUser* AdcHub::findUser(const uint32_t aSID) const {
	Lock l(cs);
	auto i = users.find(aSID);
	return i == users.end() ? NULL : i->second;
}

OnlineUser* AdcHub::findUser(const CID& aCID) const {
	Lock l(cs);
	for(auto& i: users) {
		if(i.second->getUser()->getCID() == aCID) {
			return i.second;
		}
	}
	return 0;
}

void AdcHub::putUser(const uint32_t aSID, bool disconnect) {
	OnlineUser* ou = nullptr;
	{
		Lock l(cs);
		auto i = users.find(aSID);
		if(i == users.end())
			return;
		ou = i->second;
		users.erase(i);
	}

	if(aSID != AdcCommand::HUB_SID)
		ClientManager::getInstance()->putOffline(ou, disconnect);

	fire(ClientListener::UserRemoved(), this, *ou);
	delete ou;
}

void AdcHub::clearUsers() {
	decltype(users) tmp;
	{
		Lock l(cs);
		users.swap(tmp);
	}

	for(auto& i: tmp) {
		if(i.first != AdcCommand::HUB_SID)
			ClientManager::getInstance()->putOffline(i.second);
		delete i.second;
	}
}

void AdcHub::handle(AdcCommand::INF, AdcCommand& c) noexcept {
	if(c.getParameters().empty())
		return;

	string cid;

	OnlineUser* u = 0;
	if(c.getParam("ID", 0, cid)) {
		u = findUser(CID(cid));
		if(u) {
			if(u->getIdentity().getSID() != c.getFrom()) {
				// Same CID but different SID not allowed - buggy hub?
				string nick;
				if(!c.getParam("NI", 0, nick)) {
					nick = "[nick unknown]";
				}
				fire(ClientListener::StatusMessage(), this, str(F_("%1% (%2%) has same CID {%3%} as %4% (%5%), ignoring")
					% u->getIdentity().getNick() % u->getIdentity().getSIDString() % cid % nick % AdcCommand::fromSID(c.getFrom())),
					ClientListener::FLAG_IS_SPAM);
				return;
			}
		} else {
			u = &getUser(c.getFrom(), CID(cid));
		}
	} else if(c.getFrom() == AdcCommand::HUB_SID) {
		u = &getUser(c.getFrom(), CID());
	} else {
		u = findUser(c.getFrom());
	}

	if(!u) {
		dcdebug("AdcHub::INF Unknown user / no ID\n");
		return;
	}

	for(auto& i: c.getParameters()) {
		if(i.length() < 2)
			continue;

		u->getIdentity().set(i.c_str(), i.substr(2));
	}

	if(u->getIdentity().supports(ADCS_FEATURE)) {
		u->getUser()->setFlag(User::TLS);
	}

	if(u->getUser() == getMyIdentity().getUser()) {
		state = STATE_NORMAL;
		setAutoReconnect(true);
		setMyIdentity(u->getIdentity());
		updateCounts(false);
	}

	if(u->getIdentity().isHub()) {
		setHubIdentity(u->getIdentity());
		fire(ClientListener::HubUpdated(), this);
	} else {
		updated(*u);
	}
}

void AdcHub::handle(AdcCommand::SUP, AdcCommand& c) noexcept {
	if(state != STATE_PROTOCOL) /** @todo SUP changes */
		return;
	bool baseOk = false;
	bool tigrOk = false;
	for(auto& i: c.getParameters()) {
		if(i == BAS0_SUPPORT) {
			baseOk = true;
			tigrOk = true;
		} else if(i == BASE_SUPPORT) {
			baseOk = true;
		} else if(i == TIGR_SUPPORT) {
			tigrOk = true;
		}
	}

	if(!baseOk) {
		fire(ClientListener::StatusMessage(), this, _("Failed to negotiate base protocol"));
		disconnect(false);
		return;
	} else if(!tigrOk) {
		oldPassword = true;
		// Some hubs fake BASE support without TIGR support =/
		fire(ClientListener::StatusMessage(), this, _("Hub probably uses an old version of ADC, please encourage the owner to upgrade"));
	}
}

void AdcHub::handle(AdcCommand::SID, AdcCommand& c) noexcept {
	if(state != STATE_PROTOCOL) {
		dcdebug("Invalid state for SID\n");
		return;
	}

	if(c.getParameters().empty())
		return;

	sid = AdcCommand::toSID(c.getParam(0));

	state = STATE_IDENTIFY;
	infoImpl();
}

void AdcHub::handle(AdcCommand::MSG, AdcCommand& c) noexcept {
	if(c.getParameters().empty())
		return;

	auto from = findUser(c.getFrom());
	if(!from || from->getIdentity().noChat())
		return;

	decltype(from) to = nullptr, replyTo = nullptr;

	string temp;
	string chatMessage = c.getParam(0);
	if(c.getParam("PM", 1, temp)) { // add PM<group-cid> as well

		to = findUser(c.getTo());
		if(!to)
			return;

		replyTo = findUser(AdcCommand::toSID(temp));
		if(!replyTo || PluginManager::getInstance()->runHook(HOOK_CHAT_PM_IN, replyTo, chatMessage))
			return;
	} else if(PluginManager::getInstance()->runHook(HOOK_CHAT_IN, this, chatMessage))
		return;

	fire(ClientListener::Message(), this, ChatMessage(chatMessage, from, to, replyTo, c.hasFlag("ME", 1),
		c.getParam("TS", 1, temp) ? Util::toInt64(temp) : 0));
}

void AdcHub::handle(AdcCommand::GPA, AdcCommand& c) noexcept {
	if(c.getParameters().empty())
		return;
	salt = c.getParam(0);
	state = STATE_VERIFY;

	fire(ClientListener::GetPassword(), this);
}

void AdcHub::handle(AdcCommand::QUI, AdcCommand& c) noexcept {
	uint32_t s = AdcCommand::toSID(c.getParam(0));

	OnlineUser* victim = findUser(s);
	if(victim) {

		string tmp;
		if(c.getParam("MS", 1, tmp)) {
			OnlineUser* source = 0;
			string tmp2;
			if(c.getParam("ID", 1, tmp2)) {
				source = findUser(AdcCommand::toSID(tmp2));
			}

			if(source) {
				tmp = str(F_("%1% was kicked by %2%: %3%") % victim->getIdentity().getNick() %
					source->getIdentity().getNick() % tmp);
			} else {
				tmp = str(F_("%1% was kicked: %2%") % victim->getIdentity().getNick() % tmp);
			}
			fire(ClientListener::StatusMessage(), this, tmp, ClientListener::FLAG_IS_SPAM);
		}

		putUser(s, c.getParam("DI", 1, tmp));
	}

	if(s == sid) {
		// this QUI is directed to us

		string tmp;
		if(c.getParam("TL", 1, tmp)) {
			if(tmp == "-1") {
				setAutoReconnect(false);
			} else {
				setAutoReconnect(true);
				setReconnDelay(Util::toUInt32(tmp));
			}
		}
		if(!victim && c.getParam("MS", 1, tmp)) {
			fire(ClientListener::StatusMessage(), this, tmp, ClientListener::FLAG_NORMAL);
		}
		if(c.getParam("RD", 1, tmp)) {
			fire(ClientListener::Redirect(), this, tmp);
		}
	}
}

void AdcHub::handle(AdcCommand::CTM, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 3)
		return;

	OnlineUser* u = findUser(c.getFrom());
	if(!u || u->getUser() == ClientManager::getInstance()->getMe())
		return;

	const string& protocol = c.getParam(0);
	const string& port = c.getParam(1);
	const string& token = c.getParam(2);

	bool secure = false;
	if(protocol == CLIENT_PROTOCOL && !SETTING(REQUIRE_TLS)) {
		// Nothing special
	} else if(protocol == SECURE_CLIENT_PROTOCOL && CryptoManager::getInstance()->TLSOk()) {
		secure = true;
	} else {
		unknownProtocol(c.getFrom(), protocol, token);
		return;
	}

	if(!u->getIdentity().isTcpActive()) {
		send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "IP unknown", AdcCommand::TYPE_DIRECT).setTo(c.getFrom()));
		return;
	}

	ConnectionManager::getInstance()->adcConnect(*u, port, token, secure);
}

void AdcHub::handle(AdcCommand::RCM, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 2) {
		return;
	}

	OnlineUser* u = findUser(c.getFrom());
	if(!u || u->getUser() == ClientManager::getInstance()->getMe())
		return;

	const string& protocol = c.getParam(0);
	const string& token = c.getParam(1);

	bool secure;
	if(protocol == CLIENT_PROTOCOL && !SETTING(REQUIRE_TLS)) {
		secure = false;
	} else if(protocol == SECURE_CLIENT_PROTOCOL && CryptoManager::getInstance()->TLSOk()) {
		secure = true;
	} else {
		unknownProtocol(c.getFrom(), protocol, token);
		return;
	}

	if(ClientManager::getInstance()->isActive()) {
		connect(*u, token, CONNECTION_TYPE_LAST, secure);
		return;
	}

	if (!u->getIdentity().supports(NAT0_FEATURE))
		return;

	// Attempt to traverse NATs and/or firewalls with TCP.
	// If they respond with their own, symmetric, RNT command, both
	// clients call ConnectionManager::adcConnect.
	send(AdcCommand(AdcCommand::CMD_NAT, u->getIdentity().getSID(), AdcCommand::TYPE_DIRECT).
		addParam(protocol).addParam(std::to_string(sock->getLocalPort())).addParam(token));
}

void AdcHub::handle(AdcCommand::CMD, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 1)
		return;
	const string& name = c.getParam(0);
	bool rem = c.hasFlag("RM", 1);
	if(rem) {
		fire(ClientListener::HubUserCommand(), this, (int)UserCommand::TYPE_REMOVE, 0, name, Util::emptyString);
		return;
	}
	bool sep = c.hasFlag("SP", 1);
	string sctx;
	if(!c.getParam("CT", 1, sctx))
		return;
	int ctx = Util::toInt(sctx);
	if(ctx <= 0)
		return;
	if(sep) {
		fire(ClientListener::HubUserCommand(), this, (int)UserCommand::TYPE_SEPARATOR, ctx, name, Util::emptyString);
		return;
	}
	bool once = c.hasFlag("CO", 1);
	string txt;
	if(!c.getParam("TT", 1, txt))
		return;
	fire(ClientListener::HubUserCommand(), this, (int)(once ? UserCommand::TYPE_RAW_ONCE : UserCommand::TYPE_RAW), ctx, name, txt);
}

void AdcHub::sendUDP(const AdcCommand& cmd) noexcept {
	string command;
	string ip;
	string port;
	{
		Lock l(cs);
		auto i = users.find(cmd.getTo());
		if(i == users.end()) {
			dcdebug("AdcHub::sendUDP: invalid user\n");
			return;
		}
		OnlineUser& ou = *i->second;
		if(!ou.getIdentity().isUdpActive()) {
			return;
		}
		ip = CONNSTATE(INCOMING_CONNECTIONS6) ? ou.getIdentity().getIp() : ou.getIdentity().getIp4();
		port = ou.getIdentity().getUdpPort();
		command = cmd.toString(ou.getUser()->getCID());
	}
	try {
		udp.writeTo(ip, port, command);
	} catch(const SocketException& e) {
		dcdebug("AdcHub::sendUDP: write failed: %s\n", e.getError().c_str());
		udp.close();
	}
}

void AdcHub::handle(AdcCommand::STA, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 2)
		return;

	OnlineUser* u = c.getFrom() == AdcCommand::HUB_SID ? &getUser(c.getFrom(), CID()) : findUser(c.getFrom());
	if(!u)
		return;

	//int severity = Util::toInt(c.getParam(0).substr(0, 1));
	if(c.getParam(0).size() != 3) {
		return;
	}

	switch(Util::toInt(c.getParam(0).substr(1))) {

	case AdcCommand::ERROR_BAD_PASSWORD:
		{
			if(c.getType() == AdcCommand::TYPE_INFO) {
				setPassword(Util::emptyString);
			}
			break;
		}

	case AdcCommand::ERROR_COMMAND_ACCESS:
		{
			if(c.getType() == AdcCommand::TYPE_INFO) {
				string tmp;
				if(c.getParam("FC", 1, tmp) && tmp.size() == 4)
					forbiddenCommands.insert(AdcCommand::toFourCC(tmp.c_str()));
			}
			break;
		}

	case AdcCommand::ERROR_PROTOCOL_UNSUPPORTED:
		{
			string tmp;
			if(c.getParam("PR", 1, tmp)) {
				if(tmp == CLIENT_PROTOCOL) {
					u->getUser()->setFlag(User::NO_ADC_1_0_PROTOCOL);
				} else if(tmp == SECURE_CLIENT_PROTOCOL) {
					u->getUser()->setFlag(User::NO_ADCS_0_10_PROTOCOL);
					u->getUser()->unsetFlag(User::TLS);
				}
				// Try again...
				ConnectionManager::getInstance()->force(u->getUser());
			}
			return;
		}
	}

	fire(ClientListener::Message(), this, ChatMessage(c.getParam(1), u));
}

void AdcHub::handle(AdcCommand::SCH, AdcCommand& c) noexcept {
	OnlineUser* ou = findUser(c.getFrom());
	if(!ou) {
		dcdebug("Invalid user in AdcHub::onSCH\n");
		return;
	}

	fire(ClientListener::AdcSearch(), this, c, *ou);
}

void AdcHub::handle(AdcCommand::RES, AdcCommand& c) noexcept {
	OnlineUser* ou = findUser(c.getFrom());
	if(!ou) {
		dcdebug("Invalid user in AdcHub::onRES\n");
		return;
	}
	SearchManager::getInstance()->onRES(c, ou->getUser());
}

void AdcHub::handle(AdcCommand::GET, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 5) {
		if(!c.getParameters().empty()) {
			if(c.getParam(0) == "blom") {
				send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC,
					"Too few parameters for blom", AdcCommand::TYPE_HUB));
			} else {
				send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
					"Unknown transfer type", AdcCommand::TYPE_HUB));
			}
		} else {
			send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC,
				"Too few parameters for GET", AdcCommand::TYPE_HUB));
		}
		return;
	}

	const string& type = c.getParam(0);
	string sk, sh;
	if(type == "blom" && c.getParam("BK", 4, sk) && c.getParam("BH", 4, sh))  {
		ByteVector v;
		size_t m = Util::toUInt32(c.getParam(3)) * 8;
		size_t k = Util::toUInt32(sk);
		size_t h = Util::toUInt32(sh);

		if(k > 8 || k < 1) {
			send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
				"Unsupported k", AdcCommand::TYPE_HUB));
			return;
		}
		if(h > 64 || h < 1) {
			send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
				"Unsupported h", AdcCommand::TYPE_HUB));
			return;
		}

		size_t n = ShareManager::getInstance()->getSharedFiles();

		// Ideal size for m is n * k / ln(2), but we allow some slack
		// When h >= 32, m can't go above 2^h anyway since it's stored in a size_t.
		if(m > static_cast<size_t>(5 * Util::roundUp((int64_t)(n * k / log(2.)), (int64_t)64)) || (h < 32 && m > static_cast<size_t>(1U << h))) {
			send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_TRANSFER_GENERIC,
				"Unsupported m", AdcCommand::TYPE_HUB));
			return;
		}

		if (m > 0) {
			ShareManager::getInstance()->getBloom(v, k, m, h);
		}
		AdcCommand cmd(AdcCommand::CMD_SND, AdcCommand::TYPE_HUB);
		cmd.addParam(c.getParam(0));
		cmd.addParam(c.getParam(1));
		cmd.addParam(c.getParam(2));
		cmd.addParam(c.getParam(3));
		cmd.addParam(c.getParam(4));
		send(cmd);
		if (m > 0) {
			send((char*)&v[0], v.size());
		}
	}
}

void AdcHub::handle(AdcCommand::NAT, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 3)
		return;

	OnlineUser* u = findUser(c.getFrom());
	if(!u || u->getUser() == ClientManager::getInstance()->getMe())
		return;

	const string& protocol = c.getParam(0);
	const string& port = c.getParam(1);
	const string& token = c.getParam(2);

	// bool secure = secureAvail(c.getFrom(), protocol, token);
	bool secure = false;
	if(protocol == CLIENT_PROTOCOL) {
		// Nothing special
	} else if(protocol == SECURE_CLIENT_PROTOCOL && CryptoManager::getInstance()->TLSOk()) {
		secure = true;
	} else {
		unknownProtocol(c.getFrom(), protocol, token);
		return;
	}

	// Trigger connection attempt sequence locally ...
	auto localPort = std::to_string(sock->getLocalPort());
	dcdebug("triggering connecting attempt in NAT: remote port = %s, local port = %d\n", port.c_str(), sock->getLocalPort());
	ConnectionManager::getInstance()->adcConnect(*u, port, localPort, BufferedSocket::NAT_CLIENT, token, secure);

	// ... and signal other client to do likewise.
	send(AdcCommand(AdcCommand::CMD_RNT, u->getIdentity().getSID(), AdcCommand::TYPE_DIRECT).addParam(protocol).
		addParam(localPort).addParam(token));
}

void AdcHub::handle(AdcCommand::RNT, AdcCommand& c) noexcept {
	if(c.getParameters().size() < 3)
		return;

	// Sent request for NAT traversal cooperation, which
	// was acknowledged (with requisite local port information).

	OnlineUser* u = findUser(c.getFrom());
	if(!u || u->getUser() == ClientManager::getInstance()->getMe())
		return;

	const string& protocol = c.getParam(0);
	const string& port = c.getParam(1);
	const string& token = c.getParam(2);

	bool secure = false;
	if(protocol == CLIENT_PROTOCOL) {
		// Nothing special
	} else if(protocol == SECURE_CLIENT_PROTOCOL && CryptoManager::getInstance()->TLSOk()) {
		secure = true;
	} else {
		unknownProtocol(c.getFrom(), protocol, token);
		return;
	}

	// Trigger connection attempt sequence locally
	dcdebug("triggering connecting attempt in RNT: remote port = %s, local port = %d\n", port.c_str(), sock->getLocalPort());
	ConnectionManager::getInstance()->adcConnect(*u, port, std::to_string(sock->getLocalPort()), BufferedSocket::NAT_SERVER, token, secure);
}

void AdcHub::handle(AdcCommand::ZON, AdcCommand& c) noexcept {
	if(c.getType() == AdcCommand::TYPE_INFO) {
		try {
			sock->setMode(BufferedSocket::MODE_ZPIPE);
		} catch (const Exception& e) {
			dcdebug("AdcHub::handleZON failed with error: %s\n", e.getError().c_str());
		}
	}
}

void AdcHub::handle(AdcCommand::ZOF, AdcCommand& c) noexcept {
	if(c.getType() == AdcCommand::TYPE_INFO) {
		try {
			sock->setMode(BufferedSocket::MODE_LINE);
		} catch (const Exception& e) {
			dcdebug("AdcHub::handleZOF failed with error: %s\n", e.getError().c_str());
		}
	}
}

void AdcHub::connect(const OnlineUser& user, const string& token, ConnectionType type) {
	connect(user, token, type, CryptoManager::getInstance()->TLSOk() && user.getUser()->isSet(User::TLS));
}

void AdcHub::connect(const OnlineUser& user, const string& token, ConnectionType type, bool secure) {
	if(state != STATE_NORMAL)
		return;

	const string* proto;
	if(secure) {
		if(user.getUser()->isSet(User::NO_ADCS_0_10_PROTOCOL)) {
			/// @todo log
			return;
		}
		proto = &SECURE_CLIENT_PROTOCOL;
	} else {
		if(user.getUser()->isSet(User::NO_ADC_1_0_PROTOCOL) || SETTING(REQUIRE_TLS)) {
			/// @todo log, consider removing from queue
			return;
		}
		proto = &CLIENT_PROTOCOL;
	}

	ConnectionManager::getInstance()->addToken(token, user, type);

	if(ClientManager::getInstance()->isActive()) {
		const string& port = secure ? ConnectionManager::getInstance()->getSecurePort() : ConnectionManager::getInstance()->getPort();
		if(port.empty()) {
			// Oops?
			LogManager::getInstance()->message(str(F_("Not listening for connections - please restart %1%") % APPNAME));
			return;
		}
		send(AdcCommand(AdcCommand::CMD_CTM, user.getIdentity().getSID(), AdcCommand::TYPE_DIRECT).addParam(*proto).addParam(port).addParam(token));
	} else {
		send(AdcCommand(AdcCommand::CMD_RCM, user.getIdentity().getSID(), AdcCommand::TYPE_DIRECT).addParam(*proto).addParam(token));
	}
}

void AdcHub::hubMessage(const string& aMessage, bool thirdPerson) {
	if(state != STATE_NORMAL)
		return;

	if(PluginManager::getInstance()->runHook(HOOK_CHAT_OUT, this, aMessage))
		return;

	AdcCommand c(AdcCommand::CMD_MSG, AdcCommand::TYPE_BROADCAST);
	c.addParam(aMessage);
	if(thirdPerson)
		c.addParam("ME", "1");
	send(c);
}

void AdcHub::privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson) {
	if(state != STATE_NORMAL)
		return;
	AdcCommand c(AdcCommand::CMD_MSG, user.getIdentity().getSID(), AdcCommand::TYPE_ECHO);
	c.addParam(aMessage);
	if(thirdPerson)
		c.addParam("ME", "1");
	c.addParam("PM", getMySID());
	send(c);
}

void AdcHub::sendUserCmd(const UserCommand& command, const ParamMap& params) {
	if(state != STATE_NORMAL)
		return;
	string cmd = Util::formatParams(command.getCommand(), params, escape);
	if(command.isChat()) {
		if(command.getTo().empty()) {
			hubMessage(cmd);
		} else {
			const string& to = command.getTo();
			Lock l(cs);
			for(auto& i: users) {
				if(i.second->getIdentity().getNick() == to) {
					privateMessage(*i.second, cmd);
					return;
				}
			}
		}
	} else {
		send(cmd);
	}
}

const vector<StringList>& AdcHub::getSearchExts() {
	if(!searchExts.empty())
		return searchExts;

	// the list is always immutable except for this function where it is initially being filled.
	const_cast<vector<StringList>&>(searchExts) = {
		// these extensions *must* be sorted alphabetically!
		{ "ape", "flac", "m4a", "mid", "mp3", "mpc", "ogg", "ra", "wav", "wma" },
		{ "7z", "ace", "arj", "bz2", "gz", "lha", "lzh", "rar", "tar", "z", "zip" },
		{ "doc", "docx", "htm", "html", "nfo", "odf", "odp", "ods", "odt", "pdf", "ppt", "pptx", "rtf", "txt", "xls", "xlsx", "xml", "xps" },
		{ "app", "bat", "cmd", "com", "dll", "exe", "jar", "msi", "ps1", "vbs", "wsf" },
		{ "bmp", "cdr", "eps", "gif", "ico", "img", "jpeg", "jpg", "png", "ps", "psd", "sfw", "tga", "tif", "webp" },
		{ "3gp", "asf", "asx", "avi", "divx", "flv", "mkv", "mov", "mp4", "mpeg", "mpg", "ogm", "pxp", "qt", "rm", "rmvb", "swf", "vob", "webm", "wmv" }
	};

	return searchExts;
}

StringList AdcHub::parseSearchExts(int flag) {
	StringList ret;
	const auto& searchExts = getSearchExts();
	for(auto i = searchExts.cbegin(), ibegin = i, iend = searchExts.cend(); i != iend; ++i) {
		if(flag & (1 << (i - ibegin))) {
			ret.insert(ret.begin(), i->begin(), i->end());
		}
	}
	return ret;
}

void AdcHub::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, const string& aKey) {
	if(state != STATE_NORMAL)
		return;

	AdcCommand c(AdcCommand::CMD_SCH, AdcCommand::TYPE_BROADCAST);

	/* token format: [per-hub unique id] "/" [per-search actual token]
	this allows easily knowing which hub a search was sent on when parsing a search result,
	whithout having to bother maintaining a list of sent tokens. */
	c.addParam("TO", std::to_string(getUniqueId()) + "/" + aToken);

	if(aFileType == SearchManager::TYPE_TTH) {
		c.addParam("TR", aString);

	} else {
		if(aSizeMode == SearchManager::SIZE_ATLEAST) {
			c.addParam("GE", Util::toString(aSize));
		} else if(aSizeMode == SearchManager::SIZE_ATMOST) {
			c.addParam("LE", Util::toString(aSize));
		}

		StringTokenizer<string> st(aString, ' ');
		for(auto& i: st.getTokens()) {
			c.addParam("AN", i);
		}

		if(aFileType == SearchManager::TYPE_DIRECTORY) {
			c.addParam("TY", "2");
		}

		if(aExtList.size() > 2) {
			StringList exts = aExtList;
			sort(exts.begin(), exts.end());

			uint8_t gr = 0;
			StringList rx;

			const auto& searchExts = getSearchExts();
			for(auto i = searchExts.cbegin(), ibegin = i, iend = searchExts.cend(); i != iend; ++i) {
				const StringList& def = *i;

				// gather the exts not present in any of the lists
				StringList temp(def.size() + exts.size());
				temp = StringList(temp.begin(), set_symmetric_difference(def.begin(), def.end(),
					exts.begin(), exts.end(), temp.begin()));

				// figure out whether the remaining exts have to be added or removed from the set
				StringList rx_;
				bool ok = true;
				for(auto diff = temp.begin(); diff != temp.end();) {
					if(find(def.cbegin(), def.cend(), *diff) == def.cend()) {
						++diff; // will be added further below as an "EX"
					} else {
						if(rx_.size() == 2) {
							ok = false;
							break;
						}
						rx_.push_back(*diff);
						diff = temp.erase(diff);
					}
				}
				if(!ok) // too many "RX"s necessary - disregard this group
					continue;

				// let's include this group!
				gr += 1 << (i - ibegin);

				exts = temp; // the exts to still add (that were not defined in the group)

				rx.insert(rx.begin(), rx_.begin(), rx_.end());

				if(exts.size() <= 2)
					break;
				// keep looping to see if there are more exts that can be grouped
			}

			if(gr) {
				// some extensions can be grouped; let's send a command with grouped exts.
				AdcCommand c_gr(AdcCommand::CMD_SCH, AdcCommand::TYPE_FEATURE);
				c_gr.setFeatures('+' + SEGA_FEATURE);

				const auto& params = c.getParameters();
				for(auto& i: params)
					c_gr.addParam(i);

				for(auto& i: exts)
					c_gr.addParam("EX", i);
				c_gr.addParam("GR", Util::toString(gr));
				for(auto& i: rx)
					c_gr.addParam("RX", i);

				sendSearch(c_gr);

				// make sure users with the feature don't receive the search twice.
				c.setType(AdcCommand::TYPE_FEATURE);
				c.setFeatures('-' + SEGA_FEATURE);
			}
		}

		for(auto& i: aExtList)
			c.addParam("EX", i);
	}

	// Verify that it is an active ADCS hub
	if(SETTING(ENABLE_SUDP) && !aKey.empty() && ClientManager::getInstance()->isActive() && isSecure()) {
		c.addParam("KY", aKey);
	}

	sendSearch(c);
}

void AdcHub::sendSearch(AdcCommand& c) {
	if(ClientManager::getInstance()->isActive()) {
		send(c);
	} else {
		c.setType(AdcCommand::TYPE_FEATURE);
		string features = c.getFeatures();

		c.setFeatures(features + '+' + TCP4_FEATURE + '-' + NAT0_FEATURE);
		send(c);
		c.setFeatures(features + '+' + NAT0_FEATURE);
		send(c);
	}
}

void AdcHub::password(const string& pwd) {
	if(state != STATE_VERIFY)
		return;
	if(!salt.empty()) {
		size_t saltBytes = salt.size() * 5 / 8;
		boost::scoped_array<uint8_t> buf(new uint8_t[saltBytes]);
		Encoder::fromBase32(salt.c_str(), &buf[0], saltBytes);
		TigerHash th;
		if(oldPassword) {
			CID cid = getMyIdentity().getUser()->getCID();
			th.update(cid.data(), CID::SIZE);
		}
		th.update(pwd.data(), pwd.length());
		th.update(&buf[0], saltBytes);
		send(AdcCommand(AdcCommand::CMD_PAS, AdcCommand::TYPE_HUB).addParam(Encoder::toBase32(th.finalize(), TigerHash::BYTES)));
		salt.clear();
	}
}

static void addParam(StringMap& lastInfoMap, AdcCommand& c, const string& var, const string& value) {
	auto i = lastInfoMap.find(var);

	if(i != lastInfoMap.end()) {
		if(i->second != value) {
			if(value.empty()) {
				lastInfoMap.erase(i);
			} else {
				i->second = value;
			}
			c.addParam(var, value);
		}
	} else if(!value.empty()) {
		lastInfoMap.emplace(var, value);
		c.addParam(var, value);
	}
}

void AdcHub::appendConnectivity(StringMap& lastInfoMap, AdcCommand& c, bool v4, bool v6) {
	if (v4) {
		if(CONNSETTING(NO_IP_OVERRIDE) && !getUserIp4().empty()) {
			addParam(lastInfoMap, c, "I4", Socket::resolve(getUserIp4(), AF_INET));
		} else {
			addParam(lastInfoMap, c, "I4", "0.0.0.0");
		}

		if(isActiveV4()) {
			addParam(lastInfoMap, c, "U4", SearchManager::getInstance()->getPort());
		} else {
			addParam(lastInfoMap, c, "U4", "");
		}
	} else {
		addParam(lastInfoMap, c, "I4", "");
		addParam(lastInfoMap, c, "U4", "");
	}

	if (v6) {
		if (CONNSETTING(NO_IP_OVERRIDE6) && !getUserIp6().empty()) {
			addParam(lastInfoMap, c, "I6", Socket::resolve(getUserIp6(), AF_INET6));
		} else {
			addParam(lastInfoMap, c, "I6", "::");
		}

		if(isActiveV6()) {
			addParam(lastInfoMap, c, "U6", SearchManager::getInstance()->getPort());
		} else {
			addParam(lastInfoMap, c, "U6", "");
		}
	} else {
		addParam(lastInfoMap, c, "I6", "");
		addParam(lastInfoMap, c, "U6", "");
	}
}

void AdcHub::infoImpl() {
	if(state != STATE_IDENTIFY && state != STATE_NORMAL)
		return;

	reloadSettings(false);

	AdcCommand c(AdcCommand::CMD_INF, AdcCommand::TYPE_BROADCAST);

	if (state == STATE_NORMAL) {
		updateCounts(false);
	}

	addParam(lastInfoMap, c, "ID", ClientManager::getInstance()->getMyCID().toBase32());
	addParam(lastInfoMap, c, "PD", ClientManager::getInstance()->getMyPID().toBase32());
	addParam(lastInfoMap, c, "NI", get(Nick));
	addParam(lastInfoMap, c, "DE", get(Description));
	addParam(lastInfoMap, c, "SL", Util::toString(SETTING(SLOTS)));
	addParam(lastInfoMap, c, "FS", Util::toString(UploadManager::getInstance()->getFreeSlots()));
	addParam(lastInfoMap, c, "SS", ShareManager::getInstance()->getShareSizeString());
	addParam(lastInfoMap, c, "SF", std::to_string(ShareManager::getInstance()->getSharedFiles()));
	addParam(lastInfoMap, c, "EM", get(Email));
	addParam(lastInfoMap, c, "HN", std::to_string(counts[COUNT_NORMAL]));
	addParam(lastInfoMap, c, "HR", std::to_string(counts[COUNT_REGISTERED]));
	addParam(lastInfoMap, c, "HO", std::to_string(counts[COUNT_OP]));
	addParam(lastInfoMap, c, "AP", TAGNAME);
	addParam(lastInfoMap, c, "VE", VERSIONSTRING);
	addParam(lastInfoMap, c, "AW", Util::getAway() ? "1" : Util::emptyString);

	int limit = ThrottleManager::getInstance()->getDownLimit();
	if (limit > 0) {
		addParam(lastInfoMap, c, "DS", Util::toString(limit * 1024));
	} else {
		addParam(lastInfoMap, c, "DS", Util::emptyString);
	}

	limit = ThrottleManager::getInstance()->getUpLimit();
	if (limit > 0) {
		addParam(lastInfoMap, c, "US", Util::toString(limit * 1024));
	} else {
		addParam(lastInfoMap, c, "US", std::to_string((long)(Util::toDouble(SETTING(UPLOAD_SPEED))*1024*1024/8)));
	}

	addParam(lastInfoMap, c, "LC", Util::getIETFLang());

	string su(SEGA_FEATURE);

	if(CryptoManager::getInstance()->TLSOk()) {
		su += "," + ADCS_FEATURE;
		if(SETTING(ENABLE_CCPM)) {
			su += "," + CCPM_FEATURE;
		}
		auto &kp = CryptoManager::getInstance()->getKeyprint();
		addParam(lastInfoMap, c, "KP", CryptoManager::getInstance()->keyprintToString(kp));
	}

	if(SETTING(ENABLE_SUDP) && isSecure()) {
		su += "," + SUDP_FEATURE;
	}

	bool addV4 = !sock->isV6Valid() || (get(HubSettings::Connection) != SettingsManager::INCOMING_DISABLED);
	bool addV6 = sock->isV6Valid() || (get(HubSettings::Connection6) != SettingsManager::INCOMING_DISABLED);
	
	
	if(addV4 && isActiveV4()) {
		su += "," + TCP4_FEATURE;
		su += "," + UDP4_FEATURE;
	}

	if(addV6 && isActiveV6()) {
		su += "," + TCP6_FEATURE;
		su += "," + UDP6_FEATURE;
	}

	if ((addV6 && !isActiveV6() && get(HubSettings::Connection6) != SettingsManager::INCOMING_DISABLED) || 
		(addV4 && !isActiveV4() && get(HubSettings::Connection) != SettingsManager::INCOMING_DISABLED)) {
		su += "," + NAT0_FEATURE;
	}

	addParam(lastInfoMap, c, "SU", su);
	
	appendConnectivity(lastInfoMap, c, addV4, addV6);

	if(!c.getParameters().empty()) {
		send(c);
	}
}

int64_t AdcHub::getAvailable() const {
	Lock l(cs);
	int64_t x = 0;
	for(auto& i: users) {
		x += i.second->getIdentity().getBytesShared();
	}
	return x;
}

void AdcHub::checkNick(string& nick) {
	for(size_t i = 0, n = nick.size(); i < n; ++i) {
		if(static_cast<uint8_t>(nick[i]) <= 32) {
			nick[i] = '_';
		}
	}
}

void AdcHub::send(const AdcCommand& cmd) {
	if(forbiddenCommands.find(AdcCommand::toFourCC(cmd.getFourCC().c_str())) == forbiddenCommands.end()) {
		if(cmd.getType() == AdcCommand::TYPE_UDP)
			sendUDP(cmd);
		send(cmd.toString(sid));
	}
}

void AdcHub::unknownProtocol(uint32_t target, const string& protocol, const string& token) {
	AdcCommand cmd(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_UNSUPPORTED, "Protocol unknown", AdcCommand::TYPE_DIRECT);
	cmd.setTo(target);
	cmd.addParam("PR", protocol);
	cmd.addParam("TO", token);

	send(cmd);
}

void AdcHub::on(Connected c) noexcept {
	Client::on(c);

	if(state != STATE_PROTOCOL) {
		return;
	}

	lastInfoMap.clear();
	sid = 0;
	forbiddenCommands.clear();

	AdcCommand cmd(AdcCommand::CMD_SUP, AdcCommand::TYPE_HUB);
	cmd.addParam(BAS0_SUPPORT).addParam(BASE_SUPPORT).addParam(TIGR_SUPPORT);

	if(SETTING(HUB_USER_COMMANDS)) {
		cmd.addParam(UCM0_SUPPORT);
	}
	if(SETTING(SEND_BLOOM)) {
		cmd.addParam(BLO0_SUPPORT);
	}

	cmd.addParam(ZLIF_SUPPORT);

	send(cmd);
	sinceConnect = GET_TICK();
}

void AdcHub::on(Line l, const string& aLine) noexcept {
	Client::on(l, aLine);

	if(!Text::validateUtf8(aLine)) {
		// @todo report to user?
		return;
	}

	if(PluginManager::getInstance()->runHook(HOOK_NETWORK_HUB_IN, this, aLine))
		return;

	dispatch(aLine);
}

void AdcHub::on(Failed f, const string& aLine) noexcept {
	clearUsers();
	Client::on(f, aLine);
}

void AdcHub::on(Second s, uint64_t aTick) noexcept {
	Client::on(s, aTick);

	/*
		after 2 minutes we force disconnect due to unfinished login
		this check was missing in all clients since the beginning
	*/
	if ((state > STATE_CONNECTING) && (state < STATE_NORMAL) && (sinceConnect > 0) && (aTick >= (sinceConnect + 120 * 1000))) {
		sinceConnect = 0;
		disconnect(true);
		fire(ClientListener::LoginTimeout(), this);
		return;
	}

	if ((state == STATE_NORMAL) && (aTick > (getLastActivity() + 120 * 1000)))
		send("\n", 1);
}

OnlineUserList AdcHub::getUsers() const {
	Lock l(cs);
	OnlineUserList ret;
	ret.reserve(users.size());
	for(auto& i: users)
		ret.push_back(i.second);
	return ret;
}

} // namespace dcpp
