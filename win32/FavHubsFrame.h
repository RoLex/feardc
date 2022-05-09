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

#ifndef DCPLUSPLUS_WIN32_FAV_HUBS_FRAME_H
#define DCPLUSPLUS_WIN32_FAV_HUBS_FRAME_H

#include <dcpp/FavoriteManagerListener.h>
#include <dcpp/ClientManagerListener.h>

#include "StaticFrame.h"

class FavHubsFrame :
	public StaticFrame<FavHubsFrame>,
	private FavoriteManagerListener,
	private ClientManagerListener
{
	typedef StaticFrame<FavHubsFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	friend class StaticFrame<FavHubsFrame>;
	friend class MDIChildFrame<FavHubsFrame>;

	FavHubsFrame(TabViewPtr parent);
	virtual ~FavHubsFrame();

	void layout();

	bool preClosing();
	void postClosing();

private:
	enum {
		COLUMN_FIRST,
		COLUMN_STATUS = COLUMN_FIRST,
		COLUMN_NAME,
		COLUMN_DESCRIPTION,
		COLUMN_NICK,
		COLUMN_PASSWORD,
		COLUMN_SERVER,
		COLUMN_USERDESCRIPTION,
		COLUMN_GROUP,
		COLUMN_LAST
	};

	struct StateKeeper {
		StateKeeper(TablePtr hubs_, bool ensureVisible_ = true);
		~StateKeeper();

		const FavoriteHubEntryList& getSelection() const;

	private:
		TablePtr hubs;
		bool ensureVisible;
		FavoriteHubEntryList selected;
		int scroll;
	};

	GridPtr grid;
	TablePtr hubs;

	static dwt::ImageListPtr hubIcons;

	void handleAdd();
	void handleProperties();
	void handleMove(bool up);
	void handleRemove();
	void handleGroup();
	void handleGroup(const string& group);
	void handleGroups();
	void handleDoubleClick();
	bool handleKeyDown(int c);
	bool handleContextMenu(dwt::ScreenCoordinate pt);

	TStringList getSortedGroups() const;
	void fillGroupMenu(Menu* menu);
	void fillList();
	void refresh();
	FavoriteHubEntryList getSelection() const;
	void select(const FavoriteHubEntryList& entries);
	void openSelected();

	// FavoriteManagerListener
	virtual void on(FavoriteAdded, const FavoriteHubEntryPtr e) noexcept;
	virtual void on(FavoriteRemoved, const FavoriteHubEntryPtr e) noexcept;

	// ClientManagerListener
	virtual void on(ClientManagerListener::ClientConnected, Client*) noexcept;
	virtual void on(ClientManagerListener::ClientDisconnected, Client*) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_FAV_HUBS_FRAME_H)
