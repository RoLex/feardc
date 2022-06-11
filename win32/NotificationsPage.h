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

#ifndef DCPLUSPLUS_WIN32_NOTIFICATIONS_PAGE_H
#define DCPLUSPLUS_WIN32_NOTIFICATIONS_PAGE_H

#include "PropPage.h"
#include "WinUtil.h"

class NotificationsPage : public PropPage
{
public:
	NotificationsPage(dwt::Widget* parent);
	virtual ~NotificationsPage();

	virtual void layout();
	virtual void write();

private:
	struct Option {
		tstring sound;
		int balloon;
		unsigned helpId;
		Option() : balloon(0), helpId(0) { }
	};
	Option options[WinUtil::NOTIFICATION_LAST];

	enum {
		COLUMN_DUMMY, // so that the first column doesn't have a blank space.
		COLUMN_TEXT,
		COLUMN_SOUND,
		COLUMN_BALLOON,
		COLUMN_LAST
	};

	enum {
		IMAGE_DISABLED,
		IMAGE_SOUND,
		IMAGE_BALLOON
	};

	TablePtr table;
	CheckBoxPtr sound;
	CheckBoxPtr balloon;
	GroupBoxPtr soundGroup;
	TextBoxPtr soundFile;
	GroupBoxPtr balloonGroup;
	CheckBoxPtr balloonBg;

	void handleSelectionChanged();
	void handleDblClicked();
	void handleTableHelpId(unsigned& id);
	void handleSoundClicked();
	void handleBalloonClicked();
	void handlePlayClicked();
	void handleSoundChanged();
	void handleBrowseClicked();
	void handleBalloonBgClicked();
	void handleExampleClicked();

	void updateSound(size_t row);
	void updateBalloon(size_t row);
};

#endif // !defined(DCPLUSPLUS_WIN32_NOTIFICATIONS_PAGE_H)
