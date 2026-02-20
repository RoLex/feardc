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

#ifndef DCPLUSPLUS_WIN32_ITEMS_EDIT_DLG_H
#define DCPLUSPLUS_WIN32_ITEMS_EDIT_DLG_H

#include "StringListDlg.h"

/** Dialog box that allows editing a list of strings serialized as 1 single string split on
 * semi-columns. */
class ItemsEditDlg : public StringListDlg
{
public:
	/// @param ensureUniqueness whether items in the list are unique (case insensitive)
	ItemsEditDlg(
		dwt::Widget* parent, const tstring& name_, const TStringList& extList,
		bool ensureUniqueness = true);

	void setTitle(const tstring& title);
	void setEditTitle(const tstring& title);
	void setDescription(const tstring& description);

private:
	tstring getTitle() const;
	tstring getEditTitle() const;
	tstring getEditDescription() const;

	void add(const tstring& s);
	void edit(unsigned row, const tstring& s);

	const tstring name;
	tstring _title;
	tstring _edittitle;
	tstring _description;
};

#endif // !defined(DCPLUSPLUS_WIN32_ITEMS_EDIT_DLG_H)
