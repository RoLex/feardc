/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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
#include "User.h"

#include "AdcHub.h"
#include "FavoriteUser.h"
#include "format.h"
#include "GeoManager.h"
#include "StringTokenizer.h"

namespace dcpp {

FastCriticalSection Identity::cs;

OnlineUser::OnlineUser(const UserPtr& ptr, Client& client_, uint32_t sid_) : identity(ptr, sid_), client(client_) {

}

UserData* OnlineUser::getPluginObject() noexcept {
	resetEntity();

	pod.nick = pluginString(getIdentity().getNick());
	pod.hubHint = pluginString(getClient().getHubUrl());
	pod.cid = pluginString(getUser()->getCID().toBase32());
	pod.sid = getIdentity().getSID();
	pod.protocol = getUser()->isNMDC() ? PROTOCOL_NMDC : PROTOCOL_ADC; // TODO: isNMDC() is not practical if more than two protocols
	pod.isOp = getIdentity().isOp() ? True : False;

	return &pod;
}

bool Identity::isTcpActive() const {
	return isTcp4Active() || isTcp6Active();
}

bool Identity::isTcp4Active() const {
	return (!user->isSet(User::NMDC)) ?
		!getIp4().empty() && supports(AdcHub::TCP4_FEATURE) :
		!user->isSet(User::PASSIVE);
}

bool Identity::isTcp6Active() const {
	return !getIp6().empty() && supports(AdcHub::TCP6_FEATURE);
}

bool Identity::isUdpActive() const {
	return isUdp4Active() || isUdp6Active();
}

bool Identity::isUdp4Active() const {
	if(getIp4().empty() || getUdp4Port().empty())
		return false;
	return user->isSet(User::NMDC) ? !user->isSet(User::PASSIVE) : supports(AdcHub::UDP4_FEATURE);
}

bool Identity::isUdp6Active() const {
	if(getIp6().empty() || getUdp6Port().empty())
		return false;
	return user->isSet(User::NMDC) ? false : supports(AdcHub::UDP6_FEATURE);
}

string Identity::getUdpPort() const {
	if(getIp6().empty() || getUdp6Port().empty()) {
		return getUdp4Port();
	}

	return getUdp6Port();
}

string Identity::getIp() const {
	return getIp6().empty() ? getIp4() : getIp6();
}

void Identity::getParams(ParamMap& params, const string& prefix, bool compatibility) const {
	{
		FastLock l(cs);
		for(auto& i: info) {
			params[prefix + string((char*)(&i.first), 2)] = i.second;
		}
	}
	if(user) {
		params[prefix + "SID"] = [this] { return getSIDString(); };
		params[prefix + "CID"] = [this] { return user->getCID().toBase32(); };
		params[prefix + "TAG"] = [this] { return getTag(); };
		params[prefix + "SSshort"] = [this] { return Util::formatBytes(get("SS")); };

		if(compatibility) {
			if(prefix == "my") {
				params["mynick"] = [this] { return getNick(); };
				params["mycid"] = [this] { return user->getCID().toBase32(); };
			} else {
				params["nick"] = [this] { return getNick(); };
				params["cid"] = [this] { return user->getCID().toBase32(); };
				params["ip"] = [this] { return get("I4"); };
				params["tag"] = [this] { return getTag(); };
				params["description"] = [this] { return get("DE"); };
				params["email"] = [this] { return get("EM"); };
				params["share"] = [this] { return get("SS"); };
				params["shareshort"] = [this] { return Util::formatBytes(get("SS")); };
			}
		}
	}
}

bool Identity::isClientType(ClientType ct) const {
	int type = Util::toInt(get("CT"));
	return (type & ct) == ct;
}

string Identity::getTag() const {
	if(!get("TA").empty())
		return get("TA");
	if(get("VE").empty() || get("HN").empty() || get("HR").empty() ||get("HO").empty() || get("SL").empty())
		return Util::emptyString;
	return "<" + getApplication() + ",M:" + string(isTcpActive() ? "A" : "P") +
		",H:" + get("HN") + "/" + get("HR") + "/" + get("HO") + ",S:" + get("SL") + ">";
}

string Identity::getApplication() const {
	auto application = get("AP");
	auto version = get("VE");

	if(version.empty()) {
		return application;
	}

	if(application.empty()) {
		// AP is an extension, so we can't guarantee that the other party supports it, so default to VE.
		return version;
	}

	return application + ' ' + version;
}

string Identity::getConnection() const {
	if(!get("US").empty())
		return str(F_("%1%/s") % Util::formatBytes(get("US")));

	return get("CO");
}

string Identity::getCountry() const noexcept {
	bool v6 = !getIp6().empty();
	return GeoManager::getInstance()->getCountry(v6 ? getIp6() : getIp4());
}

string Identity::get(const char* name) const {
	FastLock l(cs);
	auto i = info.find(*(short*)name);
	return i == info.end() ? Util::emptyString : i->second;
}

bool Identity::isSet(const char* name) const {
	FastLock l(cs);
	auto i = info.find(*(short*)name);
	return i != info.end();
}


void Identity::set(const char* name, const string& val) {
	FastLock l(cs);
	if(val.empty())
		info.erase(*(short*)name);
	else
		info[*(short*)name] = val;
}

bool Identity::supports(const string& name) const {
	string su = get("SU");
	StringTokenizer<string> st(su, ',');
	for(auto& i: st.getTokens()) {
		if(i == name)
			return true;
	}
	return false;
}

std::map<string, string> Identity::getInfo() const {
	std::map<string, string> ret;

	FastLock l(cs);
	for(auto& i: info) {
		ret[string((char*)(&i.first), 2)] = i.second;
	}

	return ret;
}

bool Identity::isSelf() const {
	FastLock l(cs);
	return Flags::isSet(SELF_ID);
}

void Identity::setSelf() {
	FastLock l(cs);
	if(!Flags::isSet(SELF_ID))
		Flags::setFlag(SELF_ID);
}

bool Identity::noChat() const {
	FastLock l(cs);
	return Flags::isSet(IGNORE_CHAT);
}

void Identity::setNoChat(bool ignoreChat) {
	FastLock l(cs);
	if(ignoreChat) {
		if(!Flags::isSet(IGNORE_CHAT))
			Flags::setFlag(IGNORE_CHAT);
	} else {
		if(Flags::isSet(IGNORE_CHAT))
			Flags::unsetFlag(IGNORE_CHAT);
	}
}

Style Identity::getStyle() const {
	FastLock l(cs);
	return style;
}

void Identity::setStyle(Style&& style) {
	FastLock l(cs);
	this->style = move(style);
}

void FavoriteUser::update(const OnlineUser& info) {
	setNick(info.getIdentity().getNick());
	setUrl(info.getClient().getHubUrl());
}

} // namespace dcpp
