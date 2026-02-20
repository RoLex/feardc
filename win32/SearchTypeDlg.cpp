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

#include "stdafx.h"
#include "SearchTypeDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/format.h>
#include <dcpp/version.h>

#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "ParamDlg.h"

SearchTypeDlg::SearchTypeDlg(dwt::Widget* parent, const tstring& name_, const TStringList& extList) :
StringListDlg(parent, extList),
name(name_)
{
}

tstring SearchTypeDlg::getTitle() const {
	return str(TF_("Extensions for the %1% search type") % name);
}

tstring SearchTypeDlg::getEditTitle() const {
	return T_("Extension edition");
}

tstring SearchTypeDlg::getEditDescription() const {
	return T_("Extension");
}

void SearchTypeDlg::add(const tstring& s) {
	StringTokenizer<tstring> t(s, ';');
	for(auto& i: t.getTokens()) {
		if(!i.empty()) {
			if(Util::checkExtension(Text::fromT(i))) {
				insert(i);
			} else {
				showError(i);
			}
		}
	}
}

void SearchTypeDlg::edit(unsigned row, const tstring& s) {
	ParamDlg dlg(this, getEditTitle(), getEditDescription(), s);
	if(dlg.run() == IDOK) {
		bool modified = false;
		StringTokenizer<tstring> t(dlg.getValue(), ';');
		for(auto& i: t.getTokens()) {
			if(!i.empty()) {
				if(Util::checkExtension(Text::fromT(i))) {
					if(!modified) {
						modify(row, i);
						modified = true;
					} else {
						insert(i, ++row);
					}
				} else {
					showError(i);
				}
			}
		}
	}
}

void SearchTypeDlg::showError(const tstring& badExt) {
	dwt::MessageBox(this).show(str(TF_("Invalid extension: %1%") % badExt), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
		dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
}
