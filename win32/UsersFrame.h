/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_USERS_FRAME_H
#define DCPLUSPLUS_WIN32_USERS_FRAME_H

#include <dcpp/ClientManagerListener.h>
#include <dcpp/FavoriteManagerListener.h>
#include <dcpp/QueueManagerListener.h>
#include <dcpp/UploadManagerListener.h>

#include "ListFilter.h"
#include "StaticFrame.h"
#include "UserInfoBase.h"

class UsersFrame :
	public StaticFrame<UsersFrame>,
	private FavoriteManagerListener,
	private ClientManagerListener,
	private UploadManagerListener,
	private QueueManagerListener,
	public AspectUserInfo<UsersFrame>
{
	typedef StaticFrame<UsersFrame> BaseType;

	friend class StaticFrame<UsersFrame>;
	friend class MDIChildFrame<UsersFrame>;
	friend class AspectUserInfo<UsersFrame>;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	UsersFrame(TabViewPtr parent);
	virtual ~UsersFrame();

	void layout();

	bool preClosing();
	void postClosing();

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FAVORITE = COLUMN_FIRST,
		COLUMN_SLOT,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_SEEN,
		COLUMN_DESCRIPTION,
		COLUMN_CID,
		COLUMN_LAST
	};

	enum {
		FAVORITE_OFF_ICON,
		FAVORITE_ON_ICON,
		GRANT_OFF_ICON,
		GRANT_ON_ICON
	};

	class UserInfo : public UserInfoBase {
	public:
		UserInfo(const UserPtr& u, bool visible);

		const tstring& getText(int col) const {
			return columns[col];
		}

		int getImage(int col) const {
			switch(col) {
			case COLUMN_FAVORITE: return isFavorite ? FAVORITE_ON_ICON : FAVORITE_OFF_ICON;
			case COLUMN_SLOT: return grantSlot ? GRANT_ON_ICON : GRANT_OFF_ICON;
			default: return -1;
			}
		}

		static int compareItems(const UserInfo* a, const UserInfo* b, int col) {
			switch(col) {
			case COLUMN_FAVORITE: return compare(a->isFavorite, b->isFavorite);
			case COLUMN_SLOT: return compare(a->grantSlot, b->grantSlot);
			default: return compare(a->columns[col], b->columns[col]);
			}
		}

		void remove();

		void update(const UserPtr& u, bool visible);

		tstring columns[COLUMN_LAST];

		bool isFavorite;
		bool grantSlot;
	};

	GridPtr grid;

	SplitterContainerPtr splitter;

	typedef TypedTable<UserInfo, false> WidgetUsers;
	typedef WidgetUsers* WidgetUsersPtr;
	WidgetUsersPtr users;

	dwt::ScrolledContainerPtr scroll;
	GridPtr userInfo;

	ListFilter filter;

	int selected;

	static dwt::ImageListPtr userIcons;

	std::unordered_map<UserPtr, UserInfo, User::Hash> userInfos;

	void addUser(const UserPtr& aUser);
	void updateUser(const UserPtr& aUser);
	void updateUserInfo();

	void handleDescription();
	void handleRemove();
	bool handleKeyDown(int c);
	LRESULT handleItemChanged(LPARAM lParam);
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	bool handleClick(const dwt::MouseEvent &me);

	void updateList();
	bool matches(const UserInfo& ui);
	bool show(const UserPtr& u, bool any) const;

	// AspectUserInfo
	UserInfoList selectedUsersImpl() const;

	// FavoriteManagerListener
	virtual void on(UserAdded, const FavoriteUser& aUser) noexcept;
	virtual void on(FavoriteManagerListener::UserUpdated, const FavoriteUser& aUser) noexcept;
	virtual void on(UserRemoved, const FavoriteUser& aUser) noexcept;
	virtual void on(StatusChanged, const UserPtr& aUser) noexcept;

	// ClientManagerListner
	virtual void on(UserConnected, const UserPtr& aUser) noexcept;
	virtual void on(ClientManagerListener::UserUpdated, const UserPtr& aUser) noexcept;
	virtual void on(UserDisconnected, const UserPtr& aUser) noexcept;

	// UploadManagerListener
	virtual void on(WaitingAddFile, const HintedUser&, const string&) noexcept;
	virtual void on(WaitingRemoveUser, const HintedUser&) noexcept;

	// QueueManagerListener
	virtual void on(Added, QueueItem*) noexcept;
	virtual void on(SourcesUpdated, QueueItem*) noexcept;
	virtual void on(Removed, QueueItem*) noexcept;
};

#endif // !defined(USERS_FRAME_H)
