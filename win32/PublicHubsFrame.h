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

#ifndef DCPLUSPLUS_WIN32_PUBLIC_HUBS_FRAME_H
#define DCPLUSPLUS_WIN32_PUBLIC_HUBS_FRAME_H

#include <dcpp/ClientManagerListener.h>
#include <dcpp/FavoriteManagerListener.h>
#include <dcpp/HubEntry.h>

#include "ListFilter.h"
#include "StaticFrame.h"

class PublicHubsFrame :
	public StaticFrame<PublicHubsFrame>,
	public FavoriteManagerListener,
	private ClientManagerListener
{
	typedef StaticFrame<PublicHubsFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_HUBS,
		STATUS_USERS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

private:
	friend class StaticFrame<PublicHubsFrame>;
	friend class MDIChildFrame<PublicHubsFrame>;

	PublicHubsFrame(TabViewPtr parent);
	virtual ~PublicHubsFrame();

	enum {
		COLUMN_FIRST,
		COLUMN_STATUS = COLUMN_FIRST,
		COLUMN_NAME,
		COLUMN_DESCRIPTION,
		COLUMN_USERS,
		COLUMN_SERVER,
		COLUMN_COUNTRY,
		COLUMN_SHARED,
		COLUMN_MINSHARE,
		COLUMN_MINSLOTS,
		COLUMN_MAXHUBS,
		COLUMN_MAXUSERS,
		COLUMN_RELIABILITY,
		COLUMN_RATING,
		COLUMN_LAST
	};

	class HubInfo {
	public:
		HubInfo(const HubEntry* entry_, const tstring& statusText);

		void setText(int column, const tstring& value) { columns[column] = value; }
		const tstring& getText(int column) const { return columns[column]; }
		static int compareItems(const HubInfo* a, const HubInfo* b, int col);

		const HubEntry* entry;

	private:
		tstring columns[COLUMN_LAST];
	};

	static dwt::ImageListPtr hubIcons;

	GridPtr grid;

	GridPtr upper;
	TextBoxPtr blacklist;

	typedef TypedTable<HubInfo> WidgetHubs;
	typedef WidgetHubs* WidgetHubsPtr;
	WidgetHubsPtr hubs;

	ListFilter filter;

	ComboBoxPtr lists;
	GridPtr listsGrid;

	size_t visibleHubs;
	int users;

	HubEntryList entries;

	void layout();
	bool preClosing();
	void postClosing();
	void handleConfigure();
	void handleRefresh();
	void handleConnect();
	void handleAdd();
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	bool handleKeyDown(int c);
	void handleListSelChanged();

	void updateStatus();
	void updateList();
	void updateDropDown();
	void openSelected();
	string getText(const HubEntry& entry, size_t column) const;

	void onFinished(const tstring& s, bool success, bool isCached);

	void changeHubStatus(const string& aUrl);

	virtual void on(DownloadStarting, const string& l) noexcept;
	virtual void on(DownloadFailed, const string& l, bool isCached) noexcept;
	virtual void on(DownloadFinished, const string& l) noexcept;
	virtual void on(LoadedFromCache, const string& l, const string& d, bool isForced) noexcept;
	virtual void on(Corrupted, const string& l, bool isCached) noexcept;

	// ClientManagerListener
	virtual void on(ClientManagerListener::ClientConnected, Client*) noexcept;
	virtual void on(ClientManagerListener::ClientUpdated, Client*) noexcept;
	virtual void on(ClientManagerListener::ClientDisconnected, Client*) noexcept;
};

#endif // !defined(PUBLIC_HUBS_FRM_H)
