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

#ifndef DCPLUSPLUS_WIN32_COMMAND_DLG_H
#define DCPLUSPLUS_WIN32_COMMAND_DLG_H

#include <dcpp/Util.h>

#include "GridDialog.h"

class CommandDlg : public GridDialog
{
public:
	CommandDlg(dwt::Widget* parent, int type_ = 0, int ctx_ = 0, const tstring& name_ = Util::emptyStringT,
		const tstring& command_ = Util::emptyStringT, const tstring& to_ = Util::emptyStringT,
		const tstring& hub_ = Util::emptyStringT);
	virtual ~CommandDlg();

	int getType() const { return type; }
	int getCtx() const { return ctx; }
	const tstring& getName() const { return name; }
	const tstring& getCommand() const { return command; }
	const tstring& getTo() const { return to; }
	const tstring& getHub() const { return hub; }

private:
	RadioButtonPtr separator;
	RadioButtonPtr raw;
	RadioButtonPtr chat;
	RadioButtonPtr PM;
	CheckBoxPtr hubMenu;
	CheckBoxPtr userMenu;
	CheckBoxPtr searchMenu;
	CheckBoxPtr fileListMenu;
	TextBoxPtr nameBox;
	TextBoxPtr commandBox;
	TextBoxPtr hubBox;
	TextBoxPtr nick;
	CheckBoxPtr once;
	CheckBoxPtr openHelp;

	int type;
	int ctx;
	tstring name;
	tstring command;
	tstring to;
	tstring hub;

	bool handleInitDialog();
	void handleTypeChanged();
	void handleOKClicked();

	void updateType();
	void updateCommand();
	void updateHub();
	void updateControls();
};

#endif // !defined(DCPLUSPLUS_WIN32_COMMAND_DLG_H)
