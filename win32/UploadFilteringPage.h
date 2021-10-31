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

#ifndef DCPLUSPLUS_WIN32_UPLOAD_FILTERING_PAGE_H
#define DCPLUSPLUS_WIN32_UPLOAD_FILTERING_PAGE_H

#include <dcpp/typedefs.h>

#include "PropPage.h"

class UploadFilteringPage : public PropPage
{
public:
	UploadFilteringPage(dwt::Widget* parent);
	virtual ~UploadFilteringPage();

	virtual void layout();
	virtual void write();

private:
	
	ItemList items;

	CheckBoxPtr shareHiddenCheckBox;

	TextBoxPtr minSizeControl;
	TextBoxPtr maxSizeControl;

	TextBoxPtr regexTextBox;
	TextBoxPtr extensionsTextBox;
	TextBoxPtr pathsTextBox;

	ButtonPtr modifyRegExButton;
	ButtonPtr modifyExtensionsButton;
	ButtonPtr modifyPathsButton;

	void addItem(dwt::Grid* parent, dwt::Control*& control, const tstring& text, int setting, PropPage::Type t, unsigned helpId, const tstring& text2 = _T(""));

	void handleModButtonClicked(const tstring& strName, TextBoxPtr textBox );
	void handleModButtonClicked(const tstring& strDialogName, const tstring& strTitle, const tstring& strDescription, const tstring& strEditTitle, TextBoxPtr textBox );

	bool isModified();

	void handleShareHiddenClicked(CheckBoxPtr checkBox, int setting);

};

#endif // !defined(DCPLUSPLUS_WIN32_UPLOAD_FILTERING_PAGE_H)
