/*
 * Copyright (C) 2001-2026 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef USERINFOBASE_H_
#define USERINFOBASE_H_

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/forward.h>
#include <dcpp/Flags.h>
#include <dcpp/Text.h>
#include <dcpp/User.h>

#include <dwt/widgets/Menu.h>

#include "resource.h"
#include "WinUtil.h"

using boost::range::for_each;

class UserInfoBase {
public:
	UserInfoBase(const HintedUser& u) : user(u) { }
	virtual ~UserInfoBase() { }

	virtual void getList(TabViewPtr);
	virtual void browseList(TabViewPtr);
	virtual void getOwnList(TabViewPtr);
	virtual void matchQueue();
	virtual void pm(TabViewPtr);
	virtual void pubMsg();
	virtual void grant();
	virtual void addFav();
	virtual void removeFromQueue();
	virtual void connectFav(TabViewPtr);
	virtual void ignoreChat(bool ignore);

	enum {
		INFO_WITH_CID = 1 << 0,
		INFO_ALL = INFO_WITH_CID
	};
	tstring getInfo(int flags = 0) const;
	tstring getTooltip() const;

	const HintedUser& getUser() const { return user; }
	HintedUser& getUser() { return user; }

protected:
	HintedUser user;
};

struct UserTraits : Flags {
	enum {
		adcOnly = 1 << 1,
		favOnly = 1 << 2,
		nonFavOnly = 1 << 3,
		chatIgnoredOnly = 1 << 4,
		chatNotIgnoredOnly = 1 << 5
	};

	UserTraits();

	void parse(const UserInfoBase* ui);
};

template<typename T>
class AspectUserInfo {
	typedef AspectUserInfo<T> ThisType;

	T& t() { return *static_cast<T*>(this); }
	const T& t() const { return *static_cast<const T*>(this); }

protected:
	typedef std::vector<UserInfoBase*> UserInfoList;

private:
	struct UserCollector {
		void operator()(UserInfoBase* data) {
			users.push_back(data);
		}
		UserInfoList users;
	};

protected:
	template<typename TableType>
	UserInfoList usersFromTable(TableType* table) const {
		return table->forEachSelectedT(UserCollector()).users;
	}

private:
	template<typename FunctionType>
	void handleUserFunction(const FunctionType& userFunction) {
		for_each(t().selectedUsersImpl(), userFunction);
	}

protected:
	void handleMatchQueue() {
		handleUserFunction([](UserInfoBase* u) { u->matchQueue(); });
	}
	void handleGetList(TabViewPtr parent) {
		handleUserFunction([&](UserInfoBase* u) { u->getList(parent); });
	}
	void handleBrowseList(TabViewPtr parent) {
		handleUserFunction([&](UserInfoBase* u) { u->browseList(parent); });
	}
	void handleAddFavorite() {
		handleUserFunction([](UserInfoBase* u) { u->addFav(); });
	}

	void handlePrivateMessage(TabViewPtr parent) {
		handleUserFunction([&](UserInfoBase* u) { u->pm(parent); });
	}

	void handlePublicMessage() {
		handleUserFunction([](UserInfoBase* u) { u->pubMsg(); });
	}

	void handleGrantSlot() {
		handleUserFunction([](UserInfoBase* u) { u->grant(); });
	}
	void handleRemoveFromQueue() {
		handleUserFunction([](UserInfoBase* u) { u->removeFromQueue(); });
	}
	void handleConnectFav(TabViewPtr parent) {
		handleUserFunction([&](UserInfoBase* u) { u->connectFav(parent); });
	}
	void handleIgnoreChat(bool ignore) {
		handleUserFunction([ignore](UserInfoBase* u) { u->ignoreChat(ignore); });
	}
	void handleCopyUserInfo() {
		tstring text;
		handleUserFunction([&](UserInfoBase* u) {
			if(!text.empty()) { text += _T("\r\n\r\n"); }
			text += u->getInfo(UserInfoBase::INFO_ALL);
		});
		WinUtil::setClipboard(text);
	}

	void appendUserItems(TabViewPtr parent, Menu* menu, bool defaultIsGetList = true, bool includeSendPM = true) {
		auto users = t().selectedUsersImpl();
		if(users.empty())
			return;

		UserTraits traits;
		for_each(users, [&](const UserInfoBase* u) { traits.parse(u); });

		menu->appendItem(T_("&Get file list"), [this, parent] { this->t().handleGetList(parent); }, dwt::IconPtr(), true, defaultIsGetList);
		menu->appendItem(T_("&Browse file list"), [this, parent] { this->t().handleBrowseList(parent); });
		menu->appendItem(T_("&Match queue"), [this] { this->t().handleMatchQueue(); });

		if (includeSendPM) {
			menu->appendItem(T_("&Send private message"), [this, parent] { this->t().handlePrivateMessage(parent); }, dwt::IconPtr(), true, !defaultIsGetList);
			menu->appendItem(T_("Send public message"), [this] { this->t().handlePublicMessage(); }, dwt::IconPtr(), true, false);
		}

		if(!traits.isSet(UserTraits::favOnly))
			menu->appendItem(T_("Add To &Favorites"), [this] { this->t().handleAddFavorite(); }, WinUtil::menuIcon(IDI_FAVORITE_USER_ON));
		menu->appendItem(T_("Grant &extra slot"), [this] { this->t().handleGrantSlot(); });
		if(!traits.isSet(UserTraits::nonFavOnly))
			menu->appendItem(T_("Connect to hub"), [this, parent] { this->t().handleConnectFav(parent); }, WinUtil::menuIcon(IDI_HUB));
		menu->appendSeparator();
		menu->appendItem(T_("Remove user from queue"), [this] { this->t().handleRemoveFromQueue(); });

		menu->appendSeparator();
		if(!traits.isSet(UserTraits::chatIgnoredOnly))
			menu->appendItem(T_("Ignore chat"), [this] { this->t().handleIgnoreChat(true); });
		if(!traits.isSet(UserTraits::chatNotIgnoredOnly))
			menu->appendItem(T_("Un-ignore chat"), [this] { this->t().handleIgnoreChat(false); });

		menu->appendSeparator();
		menu->appendItem(T_("Copy user information"), [this] { this->t().handleCopyUserInfo(); });
	}
};

#endif /*USERINFOBASE_H_*/
