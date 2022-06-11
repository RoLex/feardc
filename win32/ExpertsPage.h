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

#ifndef DCPLUSPLUS_WIN32_EXPERTS_PAGE_H
#define DCPLUSPLUS_WIN32_EXPERTS_PAGE_H

#include "PropPage.h"

class ExpertsPage : public PropPage
{
public:
	ExpertsPage(dwt::Widget* parent);
	virtual ~ExpertsPage();

	virtual void write();

private:

	ButtonPtr modifyWhitelistButton;

	ItemList items;

private:

	void addItem(const tstring& text, int setting, bool isInt, unsigned helpId, const tstring& text2 = _T(""));

	void AddWhitelistUI();

};

#endif // !defined(DCPLUSPLUS_WIN32_EXPERTS_PAGE_H)
