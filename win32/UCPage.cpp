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
#include "UCPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/FavoriteManager.h>

#include <dwt/widgets/Grid.h>

#include "resource.h"

#include "CommandDlg.h"
#include "HoldRedraw.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;

static const ColumnInfo columns[] = {
	{ N_("Name"), 100, false },
	{ N_("Command"), 100, false },
	{ N_("Hub"), 100, false }
};

UCPage::UCPage(dwt::Widget* parent) :
PropPage(parent, 2, 5),
commands(0)
{
	setHelpId(IDH_UCPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;
	grid->column(3).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->setHelpId(IDH_SETTINGS_UC_LIST);

	commands = grid->addChild(WinUtil::Seeds::Dialog::table);
	grid->setWidget(commands, 0, 0, 1, 5);

	auto button = grid->addChild(Button::Seed(T_("&Add")));
	button->onClicked([this] { handleAddClicked(); });
	button->setHelpId(IDH_SETTINGS_UC_ADD);

	button = grid->addChild(Button::Seed(T_("&Change")));
	button->onClicked([this] { handleChangeClicked(); });
	button->setHelpId(IDH_SETTINGS_UC_CHANGE);

	button = grid->addChild(Button::Seed(T_("Move &Up")));
	button->onClicked([this] { handleMoveUpClicked(); });
	button->setHelpId(IDH_SETTINGS_UC_MOVE_UP);

	button = grid->addChild(Button::Seed(T_("Move &Down")));
	button->onClicked([this] { handleMoveDownClicked(); });
	button->setHelpId(IDH_SETTINGS_UC_MOVE_DOWN);

	button = grid->addChild(Button::Seed(T_("&Remove")));
	button->onClicked([this] { handleRemoveClicked(); });
	button->setHelpId(IDH_SETTINGS_UC_REMOVE);

	WinUtil::makeColumns(commands, columns, 3);

	for(const auto& uc: FavoriteManager::getInstance()->getUserCommands()) {
		if(!uc.isSet(UserCommand::FLAG_NOSAVE))
			addEntry(uc);
	}

	commands->onDblClicked([this] { handleDoubleClick(); });
	commands->onKeyDown([this](int c) { return handleKeyDown(c); });
}

UCPage::~UCPage() {
}

void UCPage::layout() {
	PropPage::layout();

	commands->setColumnWidth(1, commands->getWindowSize().x - 220);
}

void UCPage::handleDoubleClick() {
	if(commands->hasSelected()) {
		handleChangeClicked();
	} else {
		handleAddClicked();
	}
}

bool UCPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddClicked();
		return true;
	case VK_DELETE:
		handleRemoveClicked();
		return true;
	}
	return false;
}

void UCPage::handleAddClicked() {
	CommandDlg dlg(this);
	if(dlg.run() == IDOK) {
		addEntry(FavoriteManager::getInstance()->addUserCommand(dlg.getType(), dlg.getCtx(), 0, Text::fromT(dlg.getName()),
			Text::fromT(dlg.getCommand()), Text::fromT(dlg.getTo()), Text::fromT(dlg.getHub())));
	}
}

void UCPage::handleChangeClicked() {
	if(commands->countSelected() == 1) {
		int i = commands->getSelected();
		UserCommand uc;
		FavoriteManager::getInstance()->getUserCommand(commands->getData(i), uc);

		CommandDlg dlg(this, uc.getType(), uc.getCtx(), Text::toT(uc.getName()), Text::toT(uc.getCommand()), Text::toT(uc.getTo()), Text::toT(uc.getHub()));
		if(dlg.run() == IDOK) {
			commands->setText(i, 0, (dlg.getType() == UserCommand::TYPE_SEPARATOR) ? T_("Separator") : dlg.getName());
			commands->setText(i, 1, dlg.getCommand());
			commands->setText(i, 2, dlg.getHub());

			uc.setName(Text::fromT(dlg.getName()));
			uc.setCommand(Text::fromT(dlg.getCommand()));
			uc.setTo(Text::fromT(dlg.getTo()));
			uc.setHub(Text::fromT(dlg.getHub()));
			uc.setType(dlg.getType());
			uc.setCtx(dlg.getCtx());
			FavoriteManager::getInstance()->updateUserCommand(uc);
		}
	}
}

void UCPage::handleMoveUpClicked() {
	if(commands->countSelected() == 1) {
		auto i = commands->getSelected();
		if(i == 0)
			return;
		auto n = commands->getData(i);
		FavoriteManager::getInstance()->moveUserCommand(n, -1);
		HoldRedraw hold { commands };
		commands->erase(i);
		UserCommand uc;
		FavoriteManager::getInstance()->getUserCommand(n, uc);
		addEntry(uc, --i);
		commands->setSelected(i);
		commands->ensureVisible(i);
	}
}

void UCPage::handleMoveDownClicked() {
	if(commands->countSelected() == 1) {
		auto i = commands->getSelected();
		if(i == static_cast<int>(commands->size()) - 1)
			return;
		auto n = commands->getData(i);
		FavoriteManager::getInstance()->moveUserCommand(n, 1);
		HoldRedraw hold { commands };
		commands->erase(i);
		UserCommand uc;
		FavoriteManager::getInstance()->getUserCommand(n, uc);
		addEntry(uc, ++i);
		commands->setSelected(i);
		commands->ensureVisible(i);
	}
}

void UCPage::handleRemoveClicked() {
	if(commands->countSelected() == 1) {
		int i = commands->getSelected();
		FavoriteManager::getInstance()->removeUserCommand(commands->getData(i));
		commands->erase(i);
	}
}

void UCPage::addEntry(const UserCommand& uc, int index) {
	commands->insert({
		(uc.getType() == UserCommand::TYPE_SEPARATOR) ? T_("Separator") : Text::toT(uc.getName()),
		Text::toT(uc.getCommand()),
		Text::toT(uc.getHub())
	}, static_cast<LPARAM>(uc.getId()), index);
}
