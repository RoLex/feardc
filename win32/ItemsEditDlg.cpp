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

#include "stdafx.h"
#include "ItemsEditDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/format.h>
#include <dcpp/version.h>

#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "ParamDlg.h"

ItemsEditDlg::ItemsEditDlg(
	dwt::Widget* parent, const tstring& name_, const TStringList& lst, bool ensureUniqueness) :
StringListDlg(parent, lst, ensureUniqueness),
name(name_)
{
}

tstring ItemsEditDlg::getTitle() const {
	return _title;
}

tstring ItemsEditDlg::getEditTitle() const {
	return _edittitle;
}

tstring ItemsEditDlg::getEditDescription() const {
	return _description;
}

void ItemsEditDlg::setTitle(const tstring& newTitle)
{
	_title = newTitle;
}

void ItemsEditDlg::setEditTitle(const tstring& title)
{
	_edittitle = title;
}

void ItemsEditDlg::setDescription(const tstring& desc)
{
	_description = desc;
}

void ItemsEditDlg::add(const tstring& s) {
	StringTokenizer<tstring> t(s, ';');
	for(auto& i: t.getTokens()) {
		if(!i.empty()) {
			insert(i);
		}
	}
}

void ItemsEditDlg::edit(unsigned row, const tstring& s) {
	ParamDlg dlg(this, getEditTitle(), getEditDescription(), s);
	if(dlg.run() == IDOK) {
		bool modified = false;
		StringTokenizer<tstring> t(dlg.getValue(), ';');
		for(auto& i: t.getTokens()) {
			if(!i.empty()) {
				if(!modified) {
					modify(row, i);
					modified = true;
				} else {
					insert(i, ++row);
				}
			}
		}
	}
}
