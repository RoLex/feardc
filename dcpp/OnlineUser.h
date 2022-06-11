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

#ifndef DCPLUSPLUS_DCPP_ONLINEUSER_H_
#define DCPLUSPLUS_DCPP_ONLINEUSER_H_

#include <map>

#include <boost/core/noncopyable.hpp>

#include "forward.h"
#include "Flags.h"
#include "FastAlloc.h"
#include "GetSet.h"
#include "Style.h"
#include "Util.h"
#include "User.h"
#include "PluginEntity.h"

namespace dcpp {

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity : public Flags {
public:
	enum ClientType {
		CT_BOT = 1,
		CT_REGGED = 2,
		CT_OP = 4,
		CT_SU = 8,
		CT_OWNER = 16,
		CT_HUB = 32,
		CT_HIDDEN = 64
	};

	enum StatusFlags {
		NORMAL		= 0x01,
		AWAY		= 0x02,
		SERVER		= 0x04,
		FIREBALL	= 0x08,
		TLS			= 0x10//,
		//NAT			= 0x20
	};

	Identity() : sid(0) { }
	Identity(const UserPtr& ptr, uint32_t aSID) : user(ptr), sid(aSID) { }
	Identity(const Identity& rhs) : Flags(), sid(0) { *this = rhs; } // Use operator= since we have to lock before reading...
	Identity& operator=(const Identity& rhs) {
		FastLock l(cs);
		*static_cast<Flags*>(this) = rhs;
		user = rhs.user;
		sid = rhs.sid;
		style = rhs.style;
		info = rhs.info;
		return *this;
	}

#define GETSET_FIELD(n, x) string get##n() const { return get(x); } void set##n(const string& v) { set(x, v); }
	GETSET_FIELD(Nick, "NI")
	GETSET_FIELD(Description, "DE")
	GETSET_FIELD(Ip4, "I4")
	GETSET_FIELD(Ip6, "I6")
	GETSET_FIELD(Udp4Port, "U4")
	GETSET_FIELD(Udp6Port, "U6")
	GETSET_FIELD(Email, "EM")
#undef GETSET_FIELD

	void setBytesShared(const string& bs) { set("SS", bs); }
	int64_t getBytesShared() const { return Util::toInt64(get("SS")); }

	void setStatus(const string& st) { set("ST", st); }
	StatusFlags getStatus() const { return static_cast<StatusFlags>(Util::toInt(get("ST"))); }

	void setOp(bool op) { set("OP", op ? "1" : Util::emptyString); }
	void setHub(bool hub) { set("HU", hub ? "1" : Util::emptyString); }
	void setBot(bool bot) { set("BO", bot ? "1" : Util::emptyString); }
	void setHidden(bool hidden) { set("HI", hidden ? "1" : Util::emptyString); }
	string getTag() const;
	string getApplication() const;
	string getConnection() const;
	const string& getCountry() const;
	bool supports(const string& name) const;
	bool isHub() const { return isClientType(CT_HUB) || isSet("HU"); }
	bool isOp() const { return isClientType(CT_OP) || isClientType(CT_SU) || isClientType(CT_OWNER) || isSet("OP"); }
	bool isRegistered() const { return isClientType(CT_REGGED) || isSet("RG"); }
	bool isHidden() const { return isClientType(CT_HIDDEN) || isSet("HI"); }
	bool isBot() const { return isClientType(CT_BOT) || isSet("BO"); }
	bool isAway() const { return isSet("AW"); }
	bool isTcpActive() const;
	bool isTcp4Active() const;
	bool isTcp6Active() const;
	bool isUdpActive() const;
	bool isUdp4Active() const;
	bool isUdp6Active() const;
	string getIp() const;
	string getUdpPort() const;

	std::map<string, string> getInfo() const;
	string get(const char* name) const;
	void set(const char* name, const string& val);
	bool isSet(const char* name) const;
	string getSIDString() const { return string((const char*)&sid, 4); }

	bool isClientType(ClientType ct) const;

	void getParams(ParamMap& params, const string& prefix, bool compatibility) const;

	const UserPtr& getUser() const { return user; }
	UserPtr& getUser() { return user; }
	uint32_t getSID() const { return sid; }

	bool isSelf() const;
	void setSelf();

	bool noChat() const;
	void setNoChat(bool ignoreChat);

	Style getStyle() const;
	void setStyle(Style&& style);

private:
	enum {
		// This identity corresponds to this client's user.
		SELF_ID = 1 << 0,

		// Chat messages from this identity shall be ignored.
		IGNORE_CHAT = 1 << 1
	};

	UserPtr user;
	uint32_t sid;

	typedef std::unordered_map<short, string> InfMap;
	typedef InfMap::iterator InfIter;
	InfMap info;

	Style style;

	static FastCriticalSection cs;
};

class OnlineUser : public FastAlloc<OnlineUser>, private boost::noncopyable, public PluginEntity<UserData> {
public:
	typedef vector<OnlineUser*> List;
	typedef List::iterator Iter;

	OnlineUser(const UserPtr& ptr, Client& client_, uint32_t sid_);

	operator UserPtr&() { return getUser(); }
	operator const UserPtr&() const { return getUser(); }

	UserPtr& getUser() { return getIdentity().getUser(); }
	const UserPtr& getUser() const { return getIdentity().getUser(); }
	Identity& getIdentity() { return identity; }
	Client& getClient() { return client; }
	const Client& getClient() const { return client; }

	UserData* getPluginObject() noexcept;

	GETSET(Identity, identity, Identity);
private:
	Client& client;
};

}

#endif /* ONLINEUSER_H_ */
