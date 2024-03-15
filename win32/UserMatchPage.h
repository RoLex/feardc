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

#ifndef DCPLUSPLUS_WIN32_USER_MATCH_PAGE_H
#define DCPLUSPLUS_WIN32_USER_MATCH_PAGE_H

#include <dcpp/UserMatch.h>

#include "PropPage.h"

class UserMatchPage : public PropPage
{
public:
	UserMatchPage(dwt::Widget* parent);
	virtual ~UserMatchPage();

	virtual void layout();
	virtual void write();

	void setDirty();
	void updateStyles();

private:
	TablePtr table;
	ButtonPtr edit, up, down, remove;

	std::vector<UserMatch> list;
	bool dirty;

	void handleDoubleClick();
	bool handleKeyDown(int c);
	void handleSelectionChanged();
	tstring handleTooltip(int i);

	void handleAddClicked();
	void handleEditClicked();
	void handleMoveUpClicked();
	void handleMoveDownClicked();
	void handleRemoveClicked();

	void addEntry(const UserMatch& matcher, int index = -1);
};

#endif
