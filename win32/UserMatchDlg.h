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

#ifndef DCPLUSPLUS_WIN32_USER_MATCH_DLG_H
#define DCPLUSPLUS_WIN32_USER_MATCH_DLG_H

#include <dcpp/UserMatch.h>

#include "GridDialog.h"

class UserMatchDlg : public GridDialog
{
public:
	UserMatchDlg(dwt::Widget* parent, const UserMatch* initialMatcher = 0);
	virtual ~UserMatchDlg();

	const UserMatch& getResult() const { return result; }

private:
	TextBoxPtr name;
	CheckBoxPtr favs, ops, bots;
	GridPtr rules;
	CheckBoxPtr forceChat, ignoreChat;

	UserMatch result;

	bool handleInitDialog(const UserMatch* initialMatcher);
	void handleOKClicked();

	void layoutRules();
	void addRow(const UserMatch::Rule* rule = 0);
};

#endif
