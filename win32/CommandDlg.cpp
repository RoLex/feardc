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
#include "CommandDlg.h"

#include <dcpp/UserCommand.h>
#include <dcpp/AdcCommand.h>
#include <dcpp/NmdcHub.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/RadioButton.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::RadioButton;

CommandDlg::CommandDlg(dwt::Widget* parent, int type_, int ctx_, const tstring& name_, const tstring& command_,
					   const tstring& to_, const tstring& hub_) :
GridDialog(parent, 298, DS_CONTEXTHELP),
separator(0),
raw(0),
chat(0),
PM(0),
hubMenu(0),
userMenu(0),
searchMenu(0),
fileListMenu(0),
nameBox(0),
commandBox(0),
hubBox(0),
nick(0),
once(0),
openHelp(0),
type(type_),
ctx(ctx_),
name(name_),
command(command_),
to(to_),
hub(hub_)
{
	onInitDialog([this] { return handleInitDialog(); });
	onHelp(&WinUtil::help);
}

CommandDlg::~CommandDlg() {
}

bool CommandDlg::handleInitDialog() {
	setHelpId(IDH_USER_COMMAND);

	grid = addChild(Grid::Seed(5, 3));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Command Type")));
		grid->setWidget(group, 0, 0, 1, 3);

		GridPtr cur = group->addChild(Grid::Seed(2, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		separator = cur->addChild(RadioButton::Seed(T_("Separator")));
		separator->setHelpId(IDH_USER_COMMAND_SEPARATOR);
		separator->onClicked([this] { handleTypeChanged(); });

		chat = cur->addChild(RadioButton::Seed(T_("Chat")));
		chat->setHelpId(IDH_USER_COMMAND_CHAT);
		chat->onClicked([this] { handleTypeChanged(); });

		raw = cur->addChild(RadioButton::Seed(T_("Raw")));
		raw->setHelpId(IDH_USER_COMMAND_RAW);
		raw->onClicked([this] { handleTypeChanged(); });

		PM = cur->addChild(RadioButton::Seed(T_("PM")));
		PM->setHelpId(IDH_USER_COMMAND_PM);
		PM->onClicked([this] { handleTypeChanged(); });
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Context")));
		grid->setWidget(group, 1, 0, 1, 3);
		group->setHelpId(IDH_USER_COMMAND_CONTEXT);

		GridPtr cur = group->addChild(Grid::Seed(2, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		hubMenu = cur->addChild(CheckBox::Seed(T_("Hub Menu")));
		hubMenu->setHelpId(IDH_USER_COMMAND_HUB_MENU);

		searchMenu = cur->addChild(CheckBox::Seed(T_("Search Menu")));
		searchMenu->setHelpId(IDH_USER_COMMAND_SEARCH_MENU);

		userMenu = cur->addChild(CheckBox::Seed(T_("User Menu")));
		userMenu->setHelpId(IDH_USER_COMMAND_USER_MENU);

		fileListMenu = cur->addChild(CheckBox::Seed(T_("Filelist Menu")));
		fileListMenu->setHelpId(IDH_USER_COMMAND_FILELIST_MENU);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Parameters")));
		grid->setWidget(group, 2, 0, 1, 3);

		GridPtr cur = group->addChild(Grid::Seed(9, 1));
		cur->column(0).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Name")))->setHelpId(IDH_USER_COMMAND_NAME);
		nameBox = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		nameBox->setHelpId(IDH_USER_COMMAND_NAME);

		cur->addChild(Label::Seed(T_("Command")))->setHelpId(IDH_USER_COMMAND_COMMAND);
		TextBox::Seed seed = WinUtil::Seeds::Dialog::textBox;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;
		commandBox = cur->addChild(seed);
		commandBox->setHelpId(IDH_USER_COMMAND_COMMAND);
		commandBox->onUpdated([this] { updateCommand(); });

		cur->addChild(Label::Seed(T_("Hub address (see help for usage)")))->setHelpId(IDH_USER_COMMAND_HUB);
		hubBox = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		hubBox->setHelpId(IDH_USER_COMMAND_HUB);
		hubBox->onUpdated([this] { updateHub(); });

		cur->addChild(Label::Seed(T_("To")))->setHelpId(IDH_USER_COMMAND_NICK);
		nick = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		nick->setHelpId(IDH_USER_COMMAND_NICK);
		nick->onUpdated([this] { updateCommand(); });

		once = cur->addChild(CheckBox::Seed(T_("Send once per nick")));
		once->setHelpId(IDH_USER_COMMAND_ONCE);
	}

	openHelp = grid->addChild(CheckBox::Seed(T_("Always open help file with this dialog")));
	grid->setWidget(openHelp, 3, 0, 1, 3);
	bool bOpenHelp = SETTING(OPEN_USER_CMD_HELP);
	openHelp->setChecked(bOpenHelp);

	WinUtil::addDlgButtons(grid,
		[this] { handleOKClicked(); },
		[this] { endDialog(IDCANCEL); });

	WinUtil::addHelpButton(grid)->onClicked([this] { WinUtil::help(this); });

	if(bOpenHelp) {
		// launch the help file, instead of having the help in the dialog
		WinUtil::help(this);
	}

	int newType = -1;
	if(type == UserCommand::TYPE_SEPARATOR) {
		separator->setChecked(true);
		newType = 0;
	} else {
		commandBox->setText(Text::toDOS(command));
		if(type == UserCommand::TYPE_RAW || type == UserCommand::TYPE_RAW_ONCE) {
			raw->setChecked(true);
			newType = 1;
		} else if(type == UserCommand::TYPE_CHAT || type == UserCommand::TYPE_CHAT_ONCE) {
			if(to.empty()) {
				chat->setChecked(true);
				newType = 2;
			} else {
				PM->setChecked(true);
				nick->setText(to);
				newType = 3;
			}
		}
		if(type == UserCommand::TYPE_RAW_ONCE || type == UserCommand::TYPE_CHAT_ONCE) {
			once->setChecked(true);
		}
	}
	type = newType;

	hubBox->setText(hub);
	nameBox->setText(name);

	if(ctx & UserCommand::CONTEXT_HUB)
		hubMenu->setChecked(true);
	if(ctx & UserCommand::CONTEXT_USER)
		userMenu->setChecked(true);
	if(ctx & UserCommand::CONTEXT_SEARCH)
		searchMenu->setChecked(true);
	if(ctx & UserCommand::CONTEXT_FILELIST)
		fileListMenu->setChecked(true);

	updateControls();
	updateCommand();

	setText(T_("Create / Modify User Command"));

	layout();
	centerWindow();

	return false;
}

void CommandDlg::handleTypeChanged() {
	updateType();
	updateCommand();
	updateControls();
}

void CommandDlg::handleOKClicked() {
	name = nameBox->getText();
	if((type != 0) && (name.empty() || commandBox->getText().empty())) {
		dwt::MessageBox(this).show(T_("Name and command must not be empty"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return;
	}

	ctx = 0;
	if(hubMenu->getChecked())
		ctx |= UserCommand::CONTEXT_HUB;
	if(userMenu->getChecked())
		ctx |= UserCommand::CONTEXT_USER;
	if(searchMenu->getChecked())
		ctx |= UserCommand::CONTEXT_SEARCH;
	if(fileListMenu->getChecked())
		ctx |= UserCommand::CONTEXT_FILELIST;

	switch(type) {
	case 0:
		type = UserCommand::TYPE_SEPARATOR;
		break;
	case 1:
		type = once->getChecked() ? UserCommand::TYPE_RAW_ONCE : UserCommand::TYPE_RAW;
		break;
	case 2:
		type = UserCommand::TYPE_CHAT;
		break;
	case 3:
		type = once->getChecked() ? UserCommand::TYPE_CHAT_ONCE : UserCommand::TYPE_CHAT;
		to = nick->getText();
		break;
	}

	SettingsManager::getInstance()->set(SettingsManager::OPEN_USER_CMD_HELP, openHelp->getChecked());

	endDialog(IDOK);
}

void CommandDlg::updateType() {
	if(separator->getChecked()) {
		type = 0;
	} else if(raw->getChecked()) {
		type = 1;
	} else if(chat->getChecked()) {
		type = 2;
	} else if(PM->getChecked()) {
		type = 3;
	}
}

void CommandDlg::updateCommand() {
	if(type == 0) {
		command.clear();
	} else {
		command = commandBox->getText();
	}
	if(type == 1 && UserCommand::adc(Text::fromT(hub)) && !command.empty() && *(command.end() - 1) != '\n')
		command += '\n';
}

void CommandDlg::updateHub() {
	hub = hubBox->getText();
	updateCommand();
}

void CommandDlg::updateControls() {
	switch(type) {
		case 0:
			nameBox->setEnabled(false);
			commandBox->setEnabled(false);
			nick->setEnabled(false);
			break;
		case 1:
		case 2:
			nameBox->setEnabled(true);
			commandBox->setEnabled(true);
			nick->setEnabled(false);
			break;
		case 3:
			nameBox->setEnabled(true);
			commandBox->setEnabled(true);
			nick->setEnabled(true);
			break;
	}
}
