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
#include "NmdcHub.h"

#include "ChatMessage.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "ConnectivityManager.h"
#include "CryptoManager.h"
#include "format.h"
#include "SearchManager.h"
#include "ShareManager.h"
#include "Socket.h"
#include "StringTokenizer.h"
#include "ThrottleManager.h"
#include "PluginManager.h"
#include "UserCommand.h"
#include "UploadManager.h"
#include "version.h"

namespace dcpp {

NmdcHub::NmdcHub(const string& aHubURL, bool secure):
	Client(aHubURL, '|', secure),
	supportFlags(0),
	lastUpdate(0),
	lastProtectedIPsUpdate(0),
	sinceConnect(0)
{}

NmdcHub::~NmdcHub() {
	clearUsers();
}

#define checkstate() if(state != STATE_NORMAL) return

void NmdcHub::connect(const OnlineUser& aUser, const string&, ConnectionType) {
	checkstate();
	dcdebug("NmdcHub::connect %s\n", aUser.getIdentity().getNick().c_str());
	if(ClientManager::getInstance()->isActive()) {
		connectToMe(aUser);
	} else {
		revConnectToMe(aUser);
	}
}

int64_t NmdcHub::getAvailable() const {
	Lock l(cs);
	int64_t x = 0;
	for(auto& i: users) {
		x += i.second->getIdentity().getBytesShared();
	}
	return x;
}

OnlineUser& NmdcHub::getUser(const string& aNick) {
	OnlineUser* u = NULL;
	{
		Lock l(cs);

		auto i = users.find(aNick);
		if(i != users.end())
			return *i->second;
	}

	UserPtr p;
	if(aNick == get(Nick)) {
		p = ClientManager::getInstance()->getMe();
	} else {
		p = ClientManager::getInstance()->getUser(aNick, getHubUrl());
	}

	{
		Lock l(cs);
		u = users.emplace(aNick, new OnlineUser(p, *this, 0)).first->second;
		u->getIdentity().setNick(aNick);
		if(u->getUser() == getMyIdentity().getUser()) {
			setMyIdentity(u->getIdentity());
		}
	}

	ClientManager::getInstance()->putOnline(u);
	return *u;
}

void NmdcHub::supports(const StringList& feat) {
	string x;
	for(auto& i: feat) {
		x += i + ' ';
	}
	send("$Supports " + x + '|');
}

OnlineUser* NmdcHub::findUser(const string& aNick) {
	Lock l(cs);
	auto i = users.find(aNick);
	return i == users.end() ? NULL : i->second;
}

void NmdcHub::putUser(const string& aNick) {
	OnlineUser* ou = NULL;
	{
		Lock l(cs);
		auto i = users.find(aNick);
		if(i == users.end())
			return;
		ou = i->second;
		users.erase(i);
	}
	ClientManager::getInstance()->putOffline(ou);
	delete ou;
}

void NmdcHub::clearUsers() {
	decltype(users) u2;

	{
		Lock l(cs);
		u2.swap(users);
	}

	for(auto& i: u2) {
		ClientManager::getInstance()->putOffline(i.second);
		delete i.second;
	}
}

void NmdcHub::updateFromTag(Identity& id, const string& tag) {
	StringTokenizer<string> tok(tag, ',');
	for(auto& i: tok.getTokens()) {
		if(i.size() < 2)
			continue;

		if(i.compare(0, 2, "H:") == 0) {
			StringTokenizer<string> t(i.substr(2), '/');
			if(t.getTokens().size() != 3)
				continue;
			id.set("HN", t.getTokens()[0]);
			id.set("HR", t.getTokens()[1]);
			id.set("HO", t.getTokens()[2]);
		} else if(i.compare(0, 2, "S:") == 0) {
			id.set("SL", i.substr(2));
		} else if(i.find("V:") != string::npos) {
			string::size_type j = i.find("V:");
			i.erase(i.begin() + j, i.begin() + j + 2);
			id.set("VE", i);
		} else if(i.compare(0, 2, "M:") == 0) {
			if(i.size() == 3) {
				if(i[2] == 'A')
					id.getUser()->unsetFlag(User::PASSIVE);
				else
					id.getUser()->setFlag(User::PASSIVE);
			}
		}
	}
	/// @todo Think about this
	id.set("TA", '<' + tag + '>');
}

void NmdcHub::onLine(const string& aLine) noexcept {
	if(aLine.length() == 0)
		return;

	if(aLine[0] != '$') {
		// Check if we're being banned...
		if(state != STATE_NORMAL) {
			if(Util::findSubString(aLine, "banned") != string::npos) {
				setAutoReconnect(false);
			}
		}
		string line = toUtf8(aLine);
		if(line[0] != '<') {
			fire(ClientListener::StatusMessage(), this, unescape(line));
			return;
		}
		string::size_type i = line.find('>', 2);
		if(i == string::npos) {
			fire(ClientListener::StatusMessage(), this, unescape(line));
			return;
		}
		string nick = line.substr(1, i-1);
		string message;
		if((line.length()-1) > i) {
			message = line.substr(i+2);
		} else {
			fire(ClientListener::StatusMessage(), this, unescape(line));
			return;
		}

		if((line.find("Hub-Security") != string::npos) && (line.find("was kicked by") != string::npos)) {
			fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_IS_SPAM);
			return;
		} else if((line.find("is kicking") != string::npos) && (line.find("because:") != string::npos)) {
			fire(ClientListener::StatusMessage(), this, unescape(line), ClientListener::FLAG_IS_SPAM);
			return;
		}

		auto from = findUser(nick);
		if(from && from->getIdentity().noChat())
			return;

		if(!from) {
			OnlineUser& o = getUser(nick);
			// Assume that messages from unknown users come from the hub
			o.getIdentity().setHub(true);
			o.getIdentity().setHidden(true);
			updated(o);
			from = &o;
		}

		auto chatMessage = unescape(message);
		if(PluginManager::getInstance()->runHook(HOOK_CHAT_IN, this, chatMessage))
			return;

		fire(ClientListener::Message(), this, ChatMessage(chatMessage, from));
		return;
	}

	string cmd;
	string param;
	string::size_type x;

	if( (x = aLine.find(' ')) == string::npos) {
		cmd = aLine;
	} else {
		cmd = aLine.substr(0, x);
		param = toUtf8(aLine.substr(x+1));
	}

	if(cmd == "$Search") {
		if(state != STATE_NORMAL) {
			return;
		}
		string::size_type i = 0;
		string::size_type j = param.find(' ', i);
		if(j == string::npos || i == j)
			return;

		string seeker = param.substr(i, j-i);

		const auto isPassive = seeker.size() > 4 && seeker.compare(0, 4, "Hub:") == 0;

		// Filter own searches
		if(!isPassive && ClientManager::getInstance()->isActive()) {
			if(seeker == localIp + ":" + SearchManager::getInstance()->getPort()) {
				return;
			}
		} else if(
			isPassive &&
			Util::stricmp(seeker.c_str() + 4 /* Hub:seeker */, getMyNick().c_str()) == 0
		) {
			return;
		}

		i = j + 1;

		uint64_t tick = GET_TICK();
		clearFlooders(tick);

		seekers.emplace_back(seeker, tick);

		// First, check if it's a flooder
		for(auto& fi: flooders) {
			if(fi.first == seeker) {
				return;
			}
		}

		int count = 0;
		for(auto& fi: seekers) {
			if(fi.first == seeker)
				count++;

			if(count > 7) {
				if(seeker.compare(0, 4, "Hub:") == 0)
					fire(ClientListener::SearchFlood(), this, seeker.substr(4));
				else
					fire(ClientListener::SearchFlood(), this, str(F_("%1% (Nick unknown)") % seeker));

				flooders.emplace_back(seeker, tick);
				return;
			}
		}

		int a;
		if(param[i] == 'F') {
			a = SearchManager::SIZE_DONTCARE;
		} else if(param[i+2] == 'F') {
			a = SearchManager::SIZE_ATLEAST;
		} else {
			a = SearchManager::SIZE_ATMOST;
		}
		i += 4;
		j = param.find('?', i);
		if(j == string::npos || i == j)
			return;
		string size = param.substr(i, j-i);
		i = j + 1;
		j = param.find('?', i);
		if(j == string::npos || i == j)
			return;
		int type = Util::toInt(param.substr(i, j-i)) - 1;
		i = j + 1;
		string terms = unescape(param.substr(i));

		// without terms, this is an invalid search.
		if(!terms.empty()) {

			if(isPassive) {
				// mark the user as passive.

				auto u = findUser(seeker.substr(4));
				if(!u) {
					return;
				}

				if(!u->getUser()->isSet(User::PASSIVE)) {
					u->getUser()->setFlag(User::PASSIVE);
					updated(*u);
				}
			}

			fire(ClientListener::NmdcSearch(), this, seeker, a, Util::toInt64(size), type, terms);
		}

	} else if (cmd == "$SA") { // tths extension, active search
		checkstate();

		if (!haveSupports(SUPPORTS_TTHS))
			return;

		string::size_type pos = param.find(' ');

		if (pos == string::npos)
			return;

		string terms = param.substr(0, pos);

		if (terms.size() != 39) // validate tth length
			return;

		string seeker = param.substr(pos + 1);

		if (seeker.size() < 9 || seeker.size() > 21) // 1.2.3.4:5 - 111.222.333.444:55555
			return;

		// verlihub never sends our searches to us, but filter own searches anyway
		if (ClientManager::getInstance()->isActive() && (seeker == localIp + ':' + SearchManager::getInstance()->getPort()))
			return;

		uint64_t tick = GET_TICK();
		clearFlooders(tick);
		seekers.emplace_back(seeker, tick);

		for (auto& fi: flooders) {
			if (fi.first == seeker)
				return;
		}

		int count = 0;

		for (auto& fi: seekers) {
			if (fi.first == seeker)
				count++;

			if (count > 7) {
				fire(ClientListener::SearchFlood(), this, str(F_("%1% (Nick unknown)") % seeker));
				flooders.emplace_back(seeker, tick);
				return;
			}
		}

		fire(ClientListener::NmdcSearch(), this, seeker, SearchManager::SIZE_DONTCARE, 0, SearchManager::TYPE_TTH, terms);

	} else if (cmd == "$SP") { // tths extension, passive search
		checkstate();

		if (!haveSupports(SUPPORTS_TTHS))
			return;

		if (!ClientManager::getInstance()->isActive()) // we are also passive
			return;

		string::size_type pos = param.find(' ');

		if (pos == string::npos)
			return;

		string terms = param.substr(0, pos);

		if (terms.size() != 39) // validate tth length
			return;

		string seeker = param.substr(pos + 1);

		if (seeker.empty() || seeker.find(' ') != string::npos)
			return;

		// verlihub never sends our searches to us, but filter own searches anyway
		if (Util::stricmp(seeker.c_str(), getMyNick().c_str()) == 0)
			return;

		string seekpas = "Hub:" + seeker;
		uint64_t tick = GET_TICK();
		clearFlooders(tick);
		seekers.emplace_back(seekpas, tick);

		for (auto& fi: flooders) {
			if (fi.first == seekpas)
				return;
		}

		int count = 0;

		for (auto& fi: seekers) {
			if (fi.first == seekpas)
				count++;

			if (count > 7) {
				fire(ClientListener::SearchFlood(), this, seeker);
				flooders.emplace_back(seekpas, tick);
				return;
			}
		}

		auto u = findUser(seeker);

		if (!u)
			return;

		if (!u->getUser()->isSet(User::PASSIVE)) {
			u->getUser()->setFlag(User::PASSIVE);
			updated(*u);
		}

		fire(ClientListener::NmdcSearch(), this, seekpas, SearchManager::SIZE_DONTCARE, 0, SearchManager::TYPE_TTH, terms);

	} else if(cmd == "$MyINFO") {
		string::size_type i, j;
		i = 5;
		j = param.find(' ', i);
		if( (j == string::npos) || (j == i) )
			return;
		string nick = param.substr(i, j-i);

		if(nick.empty())
			return;

		i = j + 1;

		OnlineUser& u = getUser(nick);

		// If he is already considered to be the hub (thus hidden), probably should appear in the UserList
		if(u.getIdentity().isHidden()) {
			u.getIdentity().setHidden(false);
			u.getIdentity().setHub(false);
		}

		j = param.find('$', i);
		if(j == string::npos)
			return;

		string tmpDesc = unescape(param.substr(i, j-i));
		// Look for a tag...
		if(!tmpDesc.empty() && tmpDesc[tmpDesc.size()-1] == '>') {
			x = tmpDesc.rfind('<');
			if(x != string::npos) {
				// Hm, we have something...disassemble it...
				updateFromTag(u.getIdentity(), tmpDesc.substr(x + 1, tmpDesc.length() - x - 2));
				tmpDesc.erase(x);
			}
		}
		u.getIdentity().setDescription(tmpDesc);

		i = j + 3;
		j = param.find('$', i);
		if(j == string::npos)
			return;

		string connection = ((i == j) ? Util::emptyString : param.substr(i, j - i - 1));
		u.getIdentity().setBot(connection.empty()); // No connection = bot...
		u.getIdentity().setHub(false);
		u.getIdentity().set("CO", connection);
		u.getIdentity().setStatus(Util::toString(param[j - 1]));

		/*
		if (u.getIdentity().getStatus() & Identity::AWAY) // added in OnlineUser.h instead
			u.getIdentity().set("AW", "1");
		else
			u.getIdentity().set("AW", Util::emptyString);
		*/

		if (u.getIdentity().getStatus() & Identity::TLS)
			u.getUser()->setFlag(User::TLS);
		else
			u.getUser()->unsetFlag(User::TLS);

		i = j + 1;
		j = param.find('$', i);

		if(j == string::npos)
			return;

		u.getIdentity().setEmail(unescape(param.substr(i, j-i)));

		i = j + 1;
		j = param.find('$', i);
		if(j == string::npos)
			return;
		u.getIdentity().setBytesShared(param.substr(i, j-i));

		if(u.getUser() == getMyIdentity().getUser()) {
			setMyIdentity(u.getIdentity());
		}

		updated(u);

	} else if(cmd == "$Quit") {
		if(!param.empty()) {
			const string& nick = param;
			OnlineUser* u = findUser(nick);
			if(!u)
				return;

			fire(ClientListener::UserRemoved(), this, *u);

			putUser(nick);
		}

	} else if (cmd == "$ConnectToMe") {
		checkstate();
		string::size_type i = param.find(' ');
		string::size_type j;

		if ((i == string::npos) || ((i + 1) >= param.size()))
			return;

		i++;
		j = param.find(':', i);

		if (j == string::npos)
			return;

		string server = Socket::resolve(param.substr(i, j - i), AF_INET);

		if (isProtectedIP(server))
			return;

		if (j + 1 >= param.size())
			return;

		string port = param.substr(j + 1);

		if (port.empty())
			return;

		bool secure = false;

		if (port[port.size() - 1] == 'S') {
			// sadly we have this disabled, we should never get such requests
			// even from old clients because we did not state tls flag in myinfo
			if (get(HubSettings::DisableCtmTLS))
				return;

			port.erase(port.size() - 1);

			if (CryptoManager::getInstance()->TLSOk())
				secure = true;
		}

		// for simplicity, we make the assumption that users on a hub have the same character encoding
		ConnectionManager::getInstance()->nmdcConnect(server, port, getMyNick(), getHubUrl(), getEncoding(), secure);

	} else if(cmd == "$RevConnectToMe") {
		if(state != STATE_NORMAL) {
			return;
		}

		string::size_type j = param.find(' ');
		if(j == string::npos) {
			return;
		}

		OnlineUser* u = findUser(param.substr(0, j));
		if(u == NULL)
			return;

		if(ClientManager::getInstance()->isActive()) {
			connectToMe(*u);
		} else {
			if(!u->getUser()->isSet(User::PASSIVE)) {
				u->getUser()->setFlag(User::PASSIVE);
				// Notify the user that we're passive too...
				revConnectToMe(*u);
				updated(*u);

				return;
			}
		}
	} else if(cmd == "$SR") {
		SearchManager::getInstance()->onData(aLine);

	} else if (cmd == "$HubName") {
		// If " - " found, the first part goes to hub name, rest to description
		// If no " - " found, first word goes to hub name, rest to description

		string::size_type i = param.find(" - ");

		if (i == string::npos) {
			i = param.find(' ');

			if (i == string::npos) {
				getHubIdentity().setNick(unescape(param));
				getHubIdentity().setDescription(Util::emptyString);

			} else {
				getHubIdentity().setNick(unescape(param.substr(0, i)));
				getHubIdentity().setDescription(unescape(param.substr(i + 1)));
			}

		} else {
			getHubIdentity().setNick(unescape(param.substr(0, i)));
			getHubIdentity().setDescription(unescape(param.substr(i + 3)));
		}

		fire(ClientListener::HubUpdated(), this);

	} else if (cmd == "$HubTopic") {
		if (haveSupports(SUPPORTS_HUBTOPIC) && param.size())
			getHubIdentity().setDescription(unescape(param));

	} else if (cmd == "$Supports") {
		StringTokenizer<string> st(param, ' ');
		StringList& sl = st.getTokens();

		for (auto& i: sl) {
			if (i == "UserCommand")
				supportFlags |= SUPPORTS_USERCOMMAND;
			else if (i == "NoGetINFO")
				supportFlags |= SUPPORTS_NOGETINFO;
			else if (i == "UserIP2") // note: not used anywhere, $UserIP can be both v1 and v2
				supportFlags |= SUPPORTS_USERIP2;
			else if (i == "TTHS")
				supportFlags |= SUPPORTS_TTHS;
			else if (i == "TLS")
				supportFlags |= SUPPORTS_TLS;
			else if (i == "BotList")
				supportFlags |= SUPPORTS_BOTLIST;
			else if (i == "HubTopic")
				supportFlags |= SUPPORTS_HUBTOPIC;
			else if (i == "MCTo")
				supportFlags |= SUPPORTS_MCTO;
		}

	} else if (cmd == "$UserCommand") {
		// we dont want this or hub didnt state support for this
		if (!SETTING(HUB_USER_COMMANDS) || !haveSupports(SUPPORTS_USERCOMMAND))
			return;

		string::size_type i = 0;
		string::size_type j = param.find(' ');
		if(j == string::npos)
			return;

		int type = Util::toInt(param.substr(0, j));
		i = j+1;
 		if(type == UserCommand::TYPE_SEPARATOR || type == UserCommand::TYPE_CLEAR) {
			int ctx = Util::toInt(param.substr(i));
			fire(ClientListener::HubUserCommand(), this, type, ctx, Util::emptyString, Util::emptyString);
		} else if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			j = param.find(' ', i);
			if(j == string::npos)
				return;
			int ctx = Util::toInt(param.substr(i));
			i = j+1;
			j = param.find('$');
			if(j == string::npos)
				return;
			string name = unescape(param.substr(i, j-i));
			// NMDC uses '\' as a separator but both ADC and our internal representation use '/'
			Util::replace("/", "//", name);
			Util::replace("\\", "/", name);
			i = j+1;
			string command = unescape(param.substr(i, param.length() - i));
			fire(ClientListener::HubUserCommand(), this, type, ctx, name, command);
		}
	} else if(cmd == "$Lock") {
		if(state != STATE_PROTOCOL) {
			return;
		}
		state = STATE_IDENTIFY;

		// Param must not be toUtf8'd...
		param = aLine.substr(6);

		if(!param.empty()) {
			string::size_type j = param.find(" Pk=");
			string lock, pk;
			if( j != string::npos ) {
				lock = param.substr(0, j);
				pk = param.substr(j + 4);
			} else {
				// Workaround for faulty linux hubs...
				j = param.find(" ");
				if(j != string::npos)
					lock = param.substr(0, j);
				else
					lock = param;
			}

			if(CryptoManager::getInstance()->isExtended(lock)) {
				StringList feat;

				if (SETTING(HUB_USER_COMMANDS)) // dont waste hub resources
					feat.push_back("UserCommand");

				feat.push_back("NoGetINFO");
				feat.push_back("NoHello");
				feat.push_back("UserIP2");
				feat.push_back("BotList");
				feat.push_back("TTHSearch");
				feat.push_back("HubTopic");
				feat.push_back("MCTo");
				feat.push_back("TTHS"); // https://www.te-home.net/?do=work&id=verlihub&page=nmdc#tths
				feat.push_back("ZPipe0");

				if (!get(HubSettings::DisableCtmTLS) && CryptoManager::getInstance()->TLSOk()) // if not disabled by user
					feat.push_back("TLS");

				supports(feat);
			}

			key(CryptoManager::getInstance()->makeKey(lock));
			OnlineUser& ou = getUser(get(Nick));
			validateNick(ou.getIdentity().getNick());
		}
	} else if(cmd == "$Hello") {
		if(!param.empty()) {
			OnlineUser& u = getUser(param);

			if(u.getUser() == getMyIdentity().getUser()) {
				if(ClientManager::getInstance()->isActive())
					u.getUser()->unsetFlag(User::PASSIVE);
				else
					u.getUser()->setFlag(User::PASSIVE);
			}

			if(state == STATE_IDENTIFY && u.getUser() == getMyIdentity().getUser()) {
				state = STATE_NORMAL;
				updateCounts(false);

				version();
				getNickList();
				myInfo(true);
			}

			updated(u);
		}
	} else if(cmd == "$ForceMove") {
		disconnect(false);
		fire(ClientListener::Redirect(), this, param);
	} else if(cmd == "$HubIsFull") {
		fire(ClientListener::HubFull(), this);
	} else if(cmd == "$ValidateDenide") {		// Mind the spelling...
		disconnect(false);
		fire(ClientListener::NickTaken(), this);
	} else if(cmd == "$UserIP") {
		if(!param.empty()) {
			OnlineUserList v;
			StringTokenizer<string> t(param, "$$");
			StringList& l = t.getTokens();
			for(auto& it: l) {
				string::size_type j = 0;
				if((j = it.find(' ')) == string::npos)
					continue;
				if((j+1) == it.length())
					continue;

				OnlineUser* u = findUser(it.substr(0, j));

				if(!u)
					continue;

				u->getIdentity().setIp4(it.substr(j+1));
				if(u->getUser() == getMyIdentity().getUser()) {
					setMyIdentity(u->getIdentity());
					refreshLocalIp();
				}
				v.push_back(u);
			}

			updated(v);
		}
	} else if(cmd == "$NickList") {
		if(!param.empty()) {
			OnlineUserList v;
			StringTokenizer<string> t(param, "$$");
			StringList& sl = t.getTokens();

			for(auto& it: sl) {
				if(it.empty())
					continue;

				v.push_back(&getUser(it));
			}

			if(!haveSupports(SUPPORTS_NOGETINFO)) {
				string tmp;
				// Let's assume 10 characters per nick...
				tmp.reserve(v.size() * (11 + 10 + getMyNick().length()));
				string n = ' ' + fromUtf8(getMyNick()) + '|';
				for(auto& i: v) {
					tmp += "$GetINFO ";
					tmp += fromUtf8(i->getIdentity().getNick());
					tmp += n;
				}
				if(!tmp.empty()) {
					send(tmp);
				}
			}

			updated(v);
		}

	} else if(cmd == "$OpList") {
		if(!param.empty()) {
			OnlineUserList v;
			StringTokenizer<string> t(param, "$$");
			StringList& sl = t.getTokens();
			for(auto& it: sl) {
				if(it.empty())
					continue;
				OnlineUser& ou = getUser(it);
				ou.getIdentity().setOp(true);
				if(ou.getUser() == getMyIdentity().getUser()) {
					setMyIdentity(ou.getIdentity());
				}
				v.push_back(&ou);
			}

			updated(v);
			updateCounts(false);

			// Special...to avoid op's complaining that their count is not correctly
			// updated when they log in (they'll be counted as registered first...)
			myInfo(false);
		}

	} else if (cmd == "$BotList") { // botlist extension
		if (!haveSupports(SUPPORTS_BOTLIST) || param.empty())
			return;

		OnlineUserList v;
		StringTokenizer<string> t(param, "$$");
		StringList& sl = t.getTokens();

		for (auto& it: sl) {
			if (it.empty())
				continue;

			OnlineUser& ou = getUser(it);
			ou.getIdentity().setBot(true);

			if (ou.getUser() == getMyIdentity().getUser()) // this is wrong, but keep anyway
				setMyIdentity(ou.getIdentity());

			v.push_back(&ou);
		}

		updated(v);
		updateCounts(false);
		myInfo(false);

	} else if(cmd == "$To:") {
		string::size_type i = param.find("From:");
		if(i == string::npos)
			return;

		i+=6;
		string::size_type j = param.find('$', i);
		if(j == string::npos)
			return;

		string rtNick = param.substr(i, j - 1 - i);
		if(rtNick.empty())
			return;
		i = j + 1;

		if(param.size() < i + 3 || param[i] != '<')
			return;

		j = param.find('>', i);
		if(j == string::npos)
			return;

		string fromNick = param.substr(i+1, j-i-1);
		if(fromNick.empty())
			return;

		if(param.size() < j + 2) {
			return;
		}

		auto from = findUser(fromNick);
		if(from && from->getIdentity().noChat())
			return;

		auto replyTo = findUser(rtNick);

		if(!replyTo || !from) {
			if(!replyTo) {
				// Assume it's from the hub
				OnlineUser& ou = getUser(rtNick);
				ou.getIdentity().setHub(true);
				ou.getIdentity().setHidden(true);
				updated(ou);
				replyTo = &ou;
			}
			if(!from) {
				// Assume it's from the hub
				OnlineUser& ou = getUser(fromNick);
				ou.getIdentity().setHub(true);
				ou.getIdentity().setHidden(true);
				updated(ou);
				from = &ou;
			}
		}

		auto chatMessage = unescape(param.substr(j + 2));
		if(PluginManager::getInstance()->runHook(HOOK_CHAT_PM_IN, replyTo, chatMessage))
			return;

		fire(ClientListener::Message(), this, ChatMessage(chatMessage, from, &getUser(getMyNick()), replyTo));

	} else if (cmd == "$MCTo:") {
		if (!haveSupports(SUPPORTS_MCTO) || param.empty())
			return;

		string::size_type i = param.find(" $");

		if (i == string::npos)
			return;

		string nick = param.substr(0, i);

		if (nick.empty() || (Util::stricmp(nick.c_str(), getMyNick().c_str()) != 0)) // not for us
			return;

		i += 2;
		string::size_type j = param.find(' ', i);

		if (j == string::npos)
			return;

		nick = param.substr(i, j - i);

		if (nick.empty())
			return;

		string msg = param.substr(j + 1);

		if (msg.empty())
			return;

		auto from = findUser(nick);

		if (from && from->getIdentity().noChat())
			return;

		if (!from) { // assume that messages from unknown users come from the hub
			OnlineUser& ou = getUser(nick);
			ou.getIdentity().setHub(true);
			ou.getIdentity().setHidden(true);
			updated(ou);
			from = &ou;
		}

		auto chatMessage = unescape(msg);

		if (PluginManager::getInstance()->runHook(HOOK_CHAT_IN, this, chatMessage))
			return;

		//fire(ClientListener::Message(), this, ChatMessage(chatMessage, from));
		// above looks more correct, be we want user to know that this is different type of message
		// the only disadvantage is that we cant see user ip directly in chat as in other public messages
		// todo: any better ideas are welcome
		fire(ClientListener::HubMCTo(), this, nick, chatMessage);

	} else if(cmd == "$GetPass") {
		OnlineUser& ou = getUser(getMyNick());
		ou.getIdentity().set("RG", "1");
		setMyIdentity(ou.getIdentity());
		fire(ClientListener::GetPassword(), this);
	} else if(cmd == "$BadPass") {
		setPassword(Util::emptyString);
	} else if(cmd == "$ZOn") {
		try {
			sock->setMode(BufferedSocket::MODE_ZPIPE);
		} catch (const Exception& e) {
			dcdebug("NmdcHub::onLine %s failed with error: %s\n", cmd.c_str(), e.getError().c_str());
		}
	} else {
		dcassert(cmd[0] == '$');
		dcdebug("NmdcHub::onLine Unknown command %s\n", aLine.c_str());
	}
}

void NmdcHub::checkNick(string& nick) {
	for(size_t i = 0, n = nick.size(); i < n; ++i) {
		if(static_cast<uint8_t>(nick[i]) <= 32 || nick[i] == '|' || nick[i] == '$' || nick[i] == '<' || nick[i] == '>') {
			nick[i] = '_';
		}
	}
}

void NmdcHub::connectToMe(const OnlineUser& aUser) {
	checkstate();
	string nick = aUser.getIdentity().getNick();
	dcdebug("NmdcHub::connectToMe %s\n", nick.c_str());
	bool secure = false;

	if (!get(HubSettings::DisableCtmTLS)) // if not disabled by user
		secure = (CryptoManager::getInstance()->TLSOk() && aUser.getUser()->isSet(User::TLS));

	const string port = (secure ? ConnectionManager::getInstance()->getSecurePort() : ConnectionManager::getInstance()->getPort());

	if (port.size()) {
		nick = fromUtf8(nick);
		ConnectionManager::getInstance()->nmdcExpect(nick, getMyNick(), getHubUrl());
		send("$ConnectToMe " + nick + " " + localIp + ":" + port + (secure ? "S" : "") + "|");
	}
}

void NmdcHub::revConnectToMe(const OnlineUser& aUser) {
	checkstate();
	dcdebug("NmdcHub::revConnectToMe %s\n", aUser.getIdentity().getNick().c_str());
	send("$RevConnectToMe " + fromUtf8(getMyNick()) + " " + fromUtf8(aUser.getIdentity().getNick()) + "|");
}

void NmdcHub::hubMessage(const string& aMessage, bool thirdPerson) {
	checkstate();
	if(!PluginManager::getInstance()->runHook(HOOK_CHAT_OUT, this, aMessage))
		send(fromUtf8( "<" + getMyNick() + "> " + escape(thirdPerson ? "/me " + aMessage : aMessage) + "|" ) );
}

void NmdcHub::hubMCTo(const string& aNick, const string& aMessage) {
	checkstate();

	if (!PluginManager::getInstance()->runHook(HOOK_CHAT_OUT, this, aMessage))
		send(fromUtf8("$MCTo: " + aNick + " $" + getMyNick() + " " + escape(aMessage) + "|"));
}

void NmdcHub::myInfo(bool alwaysSend) {
	checkstate();
	reloadSettings(false);
	char statusChar = Identity::NORMAL;
	char modeChar = 'P'; // '?' is unknown mode, use passive as default

	if (CONNSETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)
		modeChar = '5';
	else if (ClientManager::getInstance()->isActive())
		modeChar = 'A';

	string uploadSpeed;
	int upLimit = ThrottleManager::getInstance()->getUpLimit();

	if (upLimit > 0)
		uploadSpeed = Util::toString(upLimit) + " KiB/s";
	else
		uploadSpeed = SETTING(UPLOAD_SPEED);

	if (Util::getAway())
		statusChar |= Identity::AWAY;

	if (UploadManager::getInstance()->getFileServerStatus())
		statusChar |= Identity::SERVER;

	if (UploadManager::getInstance()->getFireballStatus())
		statusChar |= Identity::FIREBALL;

	if (!get(HubSettings::DisableCtmTLS) && CryptoManager::getInstance()->TLSOk()) // if not disabled by user
		statusChar |= Identity::TLS;

	string uMin = (SETTING(MIN_UPLOAD_SPEED) == 0) ? Util::emptyString : ",O:" + Util::toString(SETTING(MIN_UPLOAD_SPEED));

	string myInfoA =
		"$MyINFO $ALL " + fromUtf8(getMyNick()) + ' ' + fromUtf8(escape(get(Description))) +
		'<' + TAGNAME + " V:" + VERSIONSTRING + ",M:" + modeChar + ",H:" + getCounts();

	string myInfoB = ",S:" + Util::toString(SETTING(SLOTS));

	string myInfoC = uMin +
		">$ $" + uploadSpeed + statusChar + '$' + fromUtf8(escape(get(Email))) + '$';

	string myInfoD = ShareManager::getInstance()->getShareSizeString() + "$|";

	// we always send A and C, however, B (slots) and D (share size) can frequently change so we delay them if needed
 	if (
		lastMyInfoA != myInfoA ||
		lastMyInfoC != myInfoC ||
		alwaysSend ||
		(
			(lastMyInfoB != myInfoB || lastMyInfoD != myInfoD) &&
			lastUpdate + 15 * 60 * 1000 < GET_TICK()
		)
	) {
 		dcdebug("MyInfo %s...\n", getMyNick().c_str());
 		send(myInfoA + myInfoB + myInfoC + myInfoD);
 		lastMyInfoA = myInfoA;
 		lastMyInfoB = myInfoB;
		lastMyInfoC = myInfoC;
		lastMyInfoD = myInfoD;
 		lastUpdate = GET_TICK();
	}
}

void NmdcHub::search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string&, const StringList&) {
	checkstate();

	if ((aFileType == SearchManager::TYPE_TTH) && haveSupports(SUPPORTS_TTHS)) { // tths extension
		if (aString.size() != 39) // validate tth length
			return;

		if (ClientManager::getInstance()->isActive())
			send("$SA " + aString + ' ' + localIp + ':' + SearchManager::getInstance()->getPort() + '|');
		else
			send("$SP " + aString + ' ' + fromUtf8(getMyNick()) + '|');

		return;
	}

	char c1 = (aSizeType == SearchManager::SIZE_DONTCARE) ? 'F' : 'T';
	char c2 = (aSizeType == SearchManager::SIZE_ATLEAST) ? 'F' : 'T';
	string tmp = ((aFileType == SearchManager::TYPE_TTH) ? "TTH:" + aString : fromUtf8(escape(aString)));
	string::size_type i;

	while ((i = tmp.find(' ')) != string::npos)
		tmp[i] = '$';

	string tmp2;

	if (ClientManager::getInstance()->isActive())
		tmp2 = localIp + ':' + SearchManager::getInstance()->getPort();
	else
		tmp2 = "Hub:" + fromUtf8(getMyNick());

	send("$Search " + tmp2 + ' ' + c1 + '?' + c2 + '?' + Util::toString(aSize) + '?' + Util::toString(aFileType + 1) + '?' + tmp + '|');
}

string NmdcHub::validateMessage(string tmp, bool reverse) {
	string::size_type i = 0;

	if(reverse) {
		while( (i = tmp.find("&#36;", i)) != string::npos) {
			tmp.replace(i, 5, "$");
			i++;
		}
		i = 0;
		while( (i = tmp.find("&#124;", i)) != string::npos) {
			tmp.replace(i, 6, "|");
			i++;
		}
		i = 0;
		while( (i = tmp.find("&amp;", i)) != string::npos) {
			tmp.replace(i, 5, "&");
			i++;
		}
	} else {
		i = 0;
		while( (i = tmp.find("&amp;", i)) != string::npos) {
			tmp.replace(i, 1, "&amp;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find("&#36;", i)) != string::npos) {
			tmp.replace(i, 1, "&amp;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find("&#124;", i)) != string::npos) {
			tmp.replace(i, 1, "&amp;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find('$', i)) != string::npos) {
			tmp.replace(i, 1, "&#36;");
			i += 4;
		}
		i = 0;
		while( (i = tmp.find('|', i)) != string::npos) {
			tmp.replace(i, 1, "&#124;");
			i += 5;
		}
	}
	return tmp;
}

void NmdcHub::privateMessage(const string& nick, const string& message) {
	send("$To: " + fromUtf8(nick) + " From: " + fromUtf8(getMyNick()) + " $" + fromUtf8(escape("<" + getMyNick() + "> " + message)) + "|");
}

void NmdcHub::privateMessage(const OnlineUser& aUser, const string& aMessage, bool /*thirdPerson*/) {
	checkstate();

	privateMessage(aUser.getIdentity().getNick(), aMessage);
	// Emulate a returning message...
	Lock l(cs);
	auto ou = findUser(getMyNick());
	if(ou) {
		fire(ClientListener::Message(), this, ChatMessage(aMessage, ou, &aUser, ou));
	}
}

void NmdcHub::sendUserCmd(const UserCommand& command, const ParamMap& params) {
	checkstate();
	string cmd = Util::formatParams(command.getCommand(), params, escape);
	if(command.isChat()) {
		if(command.getTo().empty()) {
			hubMessage(cmd);
		} else {
			privateMessage(command.getTo(), cmd);
		}
	} else {
		send(fromUtf8(cmd));
	}
}

void NmdcHub::clearFlooders(uint64_t aTick) {
	while(!seekers.empty() && seekers.front().second + (5 * 1000) < aTick) {
		seekers.pop_front();
	}

	while(!flooders.empty() && flooders.front().second + (120 * 1000) < aTick) {
		flooders.pop_front();
	}
}

bool NmdcHub::isProtectedIP(const string& ip) {
	if(find(protectedIPs.begin(), protectedIPs.end(), ip) != protectedIPs.end()) {
		fire(ClientListener::StatusMessage(), this, str(F_("This hub is trying to use your client to spam %1%, please urge hub owner to fix this") % ip));
		return true;
	}
	return false;
}

void NmdcHub::refreshLocalIp() noexcept {
	if((!CONNSETTING(NO_IP_OVERRIDE) || getUserIp4().empty()) && !getMyIdentity().getIp().empty()) {
		// Best case - the server detected it
		localIp = getMyIdentity().getIp();
	} else {
		localIp.clear();
	}
	if(localIp.empty()) {
		localIp = getUserIp4();
		if(!localIp.empty()) {
			localIp = Socket::resolve(localIp, AF_INET);
		}
		if(localIp.empty()) {
			localIp = sock->getLocalIp();
			if(localIp.empty()) {
				localIp = Util::getLocalIp(false);
			}
		}
	}
}

pair<string, string> NmdcHub::parseIpPort(const string& aIpPort) {
	string ip, port;

	auto i = aIpPort.rfind(':');
	if (i == string::npos) {
		ip = aIpPort;
	} else {
		ip = aIpPort.substr(0, i);
		port = aIpPort.substr(i + 1);
	}

	return make_pair(move(ip), move(port));
}

void NmdcHub::on(Connected) noexcept {
	Client::on(Connected());

	if(state != STATE_PROTOCOL) {
		return;
	}

	supportFlags = 0;
	lastMyInfoA.clear();
	lastMyInfoB.clear();
	lastMyInfoC.clear();
	lastMyInfoD.clear();
	lastUpdate = 0;
	sinceConnect = GET_TICK();
	refreshLocalIp();
}

void NmdcHub::on(Line, const string& aLine) noexcept {
	Client::on(Line(), aLine);

	if(PluginManager::getInstance()->runHook(HOOK_NETWORK_HUB_IN, this, validateMessage(aLine, true)))
		return;

	onLine(aLine);
}

void NmdcHub::on(Failed, const string& aLine) noexcept {
	clearUsers();
	Client::on(Failed(), aLine);
}

void NmdcHub::on(Second, uint64_t aTick) noexcept {
	Client::on(Second(), aTick);

	/*
		after 2 minutes we force disconnect due to unfinished login
		this check was missing in all clients since the beginning
	*/
	if ((state != STATE_NORMAL) && (sinceConnect > 0) && (aTick >= (sinceConnect + 120 * 1000))) {
		sinceConnect = 0;
		disconnect(true);
		fire(ClientListener::LoginTimeout(), this);
		return;
	}

	if(state == STATE_NORMAL && (aTick > (getLastActivity() + 120*1000)) ) {
		send("|", 1);
	}
}

void NmdcHub::on(Minute, uint64_t aTick) noexcept {
	if (!sock)
		return;

	refreshLocalIp();

	if(aTick > (lastProtectedIPsUpdate + 24*3600*1000)) {
		protectedIPs = {
			"dchublist.org",
			"dcbase.org"
		};
		for(auto i = protectedIPs.begin(); i != protectedIPs.end();) {
			*i = Socket::resolve(*i, AF_INET);
			if(Util::isPrivateIp(*i, false))
				i = protectedIPs.erase(i);
			else
				i++;
		}

		lastProtectedIPsUpdate = aTick;
	}
}

OnlineUserList NmdcHub::getUsers() const {
	Lock l(cs);
	OnlineUserList ret;
	ret.reserve(users.size());
	for(auto& i: users)
		ret.push_back(i.second);
	return ret;
}

} // namespace dcpp
