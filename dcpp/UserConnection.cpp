/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "UserConnection.h"

#include "AdcCommand.h"
#include "ChatMessage.h"
#include "ClientManager.h"
#include "format.h"
#include "PluginManager.h"
#include "SettingsManager.h"
#include "StringTokenizer.h"
#include "Transfer.h"

namespace dcpp {

const string UserConnection::FEATURE_MINISLOTS = "MiniSlots";
const string UserConnection::FEATURE_XML_BZLIST = "XmlBZList";
const string UserConnection::FEATURE_ADCGET = "ADCGet";
const string UserConnection::FEATURE_ZLIB_GET = "ZLIG";
const string UserConnection::FEATURE_TTHL = "TTHL";
const string UserConnection::FEATURE_TTHF = "TTHF";
const string UserConnection::FEATURE_ADC_BAS0 = "BAS0";
const string UserConnection::FEATURE_ADC_BASE = "BASE";
const string UserConnection::FEATURE_ADC_BZIP = "BZIP";
const string UserConnection::FEATURE_ADC_TIGR = "TIGR";

const string UserConnection::FILE_NOT_AVAILABLE = "File Not Available";

const string UserConnection::UPLOAD = "Upload";
const string UserConnection::DOWNLOAD = "Download";

void UserConnection::on(BufferedSocketListener::Line, const string& aLine) noexcept {

	if(aLine.length() < 2) {
		fire(UserConnectionListener::ProtocolError(), this, _("Invalid data"));
		return;
	}

	if(aLine[0] == '$')
		setFlag(FLAG_NMDC);

	if(PluginManager::getInstance()->runHook(HOOK_NETWORK_CONN_IN, this, aLine))
		return;

	if(aLine[0] == 'C' && !isSet(FLAG_NMDC)) {
		if(!Text::validateUtf8(aLine)) {
			fire(UserConnectionListener::ProtocolError(), this, _("Non-UTF-8 data in an ADC connection"));
			return;
		}
		dispatch(aLine);
		return;
	} else if(!isSet(FLAG_NMDC)) {
		fire(UserConnectionListener::ProtocolError(), this, _("Invalid data"));
		return;
	}

	string cmd;
	string param;

	string::size_type x;

	if( (x = aLine.find(' ')) == string::npos) {
		cmd = aLine;
	} else {
		cmd = aLine.substr(0, x);
		param = aLine.substr(x+1);
	}

	if(cmd == "$MyNick") {
		if(!param.empty())
			fire(UserConnectionListener::MyNick(), this, param);
	} else if(cmd == "$Direction") {
		x = param.find(" ");
		if(x != string::npos) {
			fire(UserConnectionListener::Direction(), this, param.substr(0, x), param.substr(x+1));
		}
	} else if(cmd == "$Error") {
		if(Util::stricmp(param.c_str(), FILE_NOT_AVAILABLE) == 0 ||
			param.rfind(/*path/file*/" no more exists") != string::npos) {
			fire(UserConnectionListener::FileNotAvailable(), this);
		} else {
			fire(UserConnectionListener::ProtocolError(), this, param);
		}
	} else if(cmd == "$GetListLen") {
		fire(UserConnectionListener::GetListLength(), this);
	} else if(cmd == "$Get") {
		x = param.find('$');
		if(x != string::npos) {
			fire(UserConnectionListener::Get(), this, Text::toUtf8(param.substr(0, x), encoding), Util::toInt64(param.substr(x+1)) - (int64_t)1);
		}
	} else if(cmd == "$Key") {
		if(!param.empty())
			fire(UserConnectionListener::Key(), this, param);
	} else if(cmd == "$Lock") {
		if(!param.empty()) {
			x = param.find(" Pk=");
			if(x != string::npos) {
				fire(UserConnectionListener::CLock(), this, param.substr(0, x), param.substr(x + 4));
			} else {
				// Workaround for faulty linux clients...
				x = param.find(' ');
				if(x != string::npos) {
					setFlag(FLAG_INVALIDKEY);
					fire(UserConnectionListener::CLock(), this, param.substr(0, x), Util::emptyString);
				} else {
					fire(UserConnectionListener::CLock(), this, param, Util::emptyString);
				}
			}
		}
	} else if(cmd == "$Send") {
		fire(UserConnectionListener::Send(), this);
	} else if(cmd == "$MaxedOut") {
		fire(UserConnectionListener::MaxedOut(), this, param);
	} else if(cmd == "$Supports") {
		if(!param.empty()) {
			fire(UserConnectionListener::Supports(), this, StringTokenizer<string>(param, ' ').getTokens());
		}
	} else if(cmd.compare(0, 4, "$ADC") == 0) {
		dispatch(aLine, true);
	} else {
		fire(UserConnectionListener::ProtocolError(), this, _("Invalid data"));
	}
}

void UserConnection::connect(const string& aServer, const string& aPort, const string& localPort, BufferedSocket::NatRoles natRole, UserPtr user) {
	dcassert(!socket);

	port = aPort;
	socket = BufferedSocket::getSocket(0);
	socket->addListener(this);

	// @see UserConnection::accept, additionally opt to treat connections in both directions identically to avoid unforseen issues
	/*string expKP;
	if(user) {
		// TODO: verify that this KeyPrint was mediated by a trusted hub?
		expKP = ClientManager::getInstance()->getField(user->getCID(), hubUrl, "KP");
		setUser(user);
	}*/

	socket->connect(aServer, aPort, localPort, natRole, secure, true, true);
}

void UserConnection::accept(const Socket& aServer) {
	dcassert(!socket);
	socket = BufferedSocket::getSocket(0);
	socket->addListener(this);

	// Technically only one side needs to verify KeyPrint, also since we most likely requested to be connected to (and we have insufficent info otherwise) deal with TLS options check post handshake
	// -> SSLSocket::verifyKeyprint does full certificate verification after INF
	setPort(Util::toString(socket->accept(aServer, secure, true)));
}

void UserConnection::inf(bool withToken) {
	AdcCommand c(AdcCommand::CMD_INF);
	c.addParam("ID", ClientManager::getInstance()->getMyCID().toBase32());
	if(withToken) {
		c.addParam("TO", getToken());
	}
	if(isSet(FLAG_PM)) {
		c.addParam("PM", "1");
	}
	send(c);
}

void UserConnection::sup(const StringList& features) {
	AdcCommand c(AdcCommand::CMD_SUP);
	for(auto& i: features)
		c.addParam(i);
	send(c);
}

void UserConnection::supports(const StringList& feat) {
	string x;
	for(auto& i: feat) {
		x+= i + ' ';
	}
	send("$Supports " + x + '|');
}

void UserConnection::get(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) {
	send(AdcCommand(AdcCommand::CMD_GET).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes)));
}

void UserConnection::snd(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) {
	send(AdcCommand(AdcCommand::CMD_SND).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes)));
}

void UserConnection::pm(const string& message, bool thirdPerson) {
	{
		auto lock = ClientManager::getInstance()->lock();
		auto ou = ClientManager::getInstance()->findOnlineUser(getHintedUser());

		if(PluginManager::getInstance()->runHook(HOOK_CHAT_PM_OUT, ou, message))
			return;
	}

	AdcCommand c(AdcCommand::CMD_MSG);
	c.addParam(message);
	if(thirdPerson)
		c.addParam("ME", "1");
	send(c);

	// simulate an echo message.
	handlePM(c, true);
}

void UserConnection::handle(AdcCommand::MSG t, const AdcCommand& c) {
	handlePM(c, false);

	fire(t, this, c);
}

void UserConnection::handle(AdcCommand::STA t, const AdcCommand& c) {
	if(c.getParameters().size() >= 2) {
		const string& code = c.getParam(0);
		if(!code.empty() && code[0] - '0' == AdcCommand::SEV_FATAL) {
			fire(UserConnectionListener::ProtocolError(), this, c.getParam(1));
			return;
		}
	}

	fire(t, this, c);
}

void UserConnection::handlePM(const AdcCommand& c, bool echo) noexcept {
	auto message = c.getParam(0);

	auto cm = ClientManager::getInstance();
	auto lock = cm->lock();
	auto peer = cm->findOnlineUser(user->getCID(), hubUrl);
	auto me = cm->findOnlineUser(cm->getMe()->getCID(), hubUrl);
	// null pointers allowed here as the conn may be going on without hubs.

	if(echo) {
		std::swap(peer, me);
	}

	if(peer && peer->getIdentity().noChat())
		return;

	if(PluginManager::getInstance()->runHook(HOOK_CHAT_PM_IN, peer, message))
		return;

	string tmp;
	fire(UserConnectionListener::PrivateMessage(), this, ChatMessage(message, peer, me, peer,
		c.hasFlag("ME", 1), c.getParam("TS", 1, tmp) ? Util::toInt64(tmp) : 0));
}

void UserConnection::on(Connected) noexcept {
	lastActivity = GET_TICK();
	fire(UserConnectionListener::Connected(), this);
}

void UserConnection::on(Data, uint8_t* data, size_t len) noexcept {
	lastActivity = GET_TICK();
	fire(UserConnectionListener::Data(), this, data, len);
}

void UserConnection::on(BytesSent, size_t bytes, size_t actual) noexcept {
	lastActivity = GET_TICK();
	fire(UserConnectionListener::BytesSent(), this, bytes, actual);
}

void UserConnection::on(ModeChange) noexcept {
	lastActivity = GET_TICK();
	fire(UserConnectionListener::ModeChange(), this);
}

void UserConnection::on(TransmitDone) noexcept {
	fire(UserConnectionListener::TransmitDone(), this);
}

void UserConnection::on(Failed, const string& aLine) noexcept {
	setState(STATE_UNCONNECTED);
	fire(UserConnectionListener::Failed(), this, aLine);

	delete this;
}

// # ms we should aim for per segment
static const int64_t SEGMENT_TIME = 120*1000;
static const int64_t MIN_CHUNK_SIZE = 64*1024;

void UserConnection::updateChunkSize(int64_t leafSize, int64_t lastChunk, uint64_t ticks) {

	if(chunkSize == 0) {
		chunkSize = std::max(MIN_CHUNK_SIZE, std::min(lastChunk, (int64_t)1024*1024));
		return;
	}

	if(ticks <= 10) {
		// Can't rely on such fast transfers - double
		chunkSize *= 2;
		return;
	}

	double lastSpeed = (1000. * lastChunk) / ticks;

	int64_t targetSize = chunkSize;

	// How long current chunk size would take with the last speed...
	double msecs = 1000 * targetSize / lastSpeed;

	if(msecs < SEGMENT_TIME / 4) {
		targetSize *= 2;
	} else if(msecs < SEGMENT_TIME / 1.25) {
		targetSize += leafSize;
	} else if(msecs < SEGMENT_TIME * 1.25) {
		// We're close to our target size - don't change it
	} else if(msecs < SEGMENT_TIME * 4) {
		targetSize = std::max(MIN_CHUNK_SIZE, targetSize - chunkSize);
	} else {
		targetSize = std::max(MIN_CHUNK_SIZE, targetSize / 2);
	}

	chunkSize = targetSize;
}

ConnectionData* UserConnection::getPluginObject() noexcept {
	resetEntity();

	pod.ip = pluginString(getRemoteIp());
	pod.object = this;
	pod.port = Util::toInt(port);
	pod.protocol = isSet(UserConnection::FLAG_NMDC) ? PROTOCOL_NMDC : PROTOCOL_ADC; // TODO: isSet(...) not practical if more than two protocols
	pod.isOp = isSet(UserConnection::FLAG_OP) ? True : False;
	pod.isSecure = isSecure() ? True : False;

	return &pod;
}

void UserConnection::send(const string& aString) {
	if(PluginManager::getInstance()->runHook(HOOK_NETWORK_CONN_OUT, this, aString))
		return;

	lastActivity = GET_TICK();
	socket->write(aString);
}

} // namespace dcpp
