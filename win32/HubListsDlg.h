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

#ifndef DCPLUSPLUS_WIN32_HUB_LISTS_DLG_H
#define DCPLUSPLUS_WIN32_HUB_LISTS_DLG_H

#include "StringListDlg.h"

class HubListsDlg : public StringListDlg
{
public:
	HubListsDlg(dwt::Widget* parent);

	int run();

private:
	tstring getTitle() const;
	tstring getEditTitle() const;
	tstring getEditDescription() const;
	unsigned getHelpId(HelpFields field) const;
	void add(const tstring& s);
	void edit(unsigned row, const tstring& s);

	static TStringList getHubLists();
};

#endif // !defined(DCPLUSPLUS_WIN32_HUB_LISTS_DLG_H)
