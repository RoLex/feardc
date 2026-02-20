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

#ifndef DCPLUSPLUS_WIN32_LOG_PAGE_H
#define DCPLUSPLUS_WIN32_LOG_PAGE_H

#include "PropPage.h"

class LogPage : public PropPage
{
public:
	LogPage(dwt::Widget* parent);
	virtual ~LogPage();

	virtual void write();

private:
	ItemList items;

	TextBoxPtr dir;

	static ListItem listItems[];
	TablePtr options;

	TextBoxPtr logFormat;
	TextBoxPtr logFile;

	CheckBoxPtr dontLogCCPMCheckBox; 

	int oldSelection;

	//store all log options here so we can discard them
	//if the user cancels the dialog.
	//.first is filename and .second is format
	TStringPairList logOptions;

	void handleSelectionChanged();

	void getValues();
};

#endif // !defined(DCPLUSPLUS_WIN32_LOG_PAGE_H)
