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

#ifndef DCPLUSPLUS_WIN32_SEARCH_TYPE_DLG_H
#define DCPLUSPLUS_WIN32_SEARCH_TYPE_DLG_H

#include "StringListDlg.h"

class SearchTypeDlg : public StringListDlg
{
public:
	SearchTypeDlg(dwt::Widget* parent, const tstring& name_, const TStringList& extList);

private:
	tstring getTitle() const;
	tstring getEditTitle() const;
	tstring getEditDescription() const;
	void add(const tstring& s);
	void edit(unsigned row, const tstring& s);

	void showError(const tstring& badExt);

	const tstring name;
};

#endif // !defined(DCPLUSPLUS_WIN32_SEARCH_TYPE_DLG_H)
