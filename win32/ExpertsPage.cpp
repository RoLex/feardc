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

#include "stdafx.h"
#include "ExpertsPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>

#include "resource.h"
#include "WinUtil.h"
#include "ItemsEditDlg.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

ExpertsPage::ExpertsPage(dwt::Widget* parent) :
PropPage(parent, 7, 2),
modifyWhitelistButton(nullptr)
{
	setHelpId(IDH_EXPERTSPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;

	addItem(T_("Max hash speed"), SettingsManager::MAX_HASH_SPEED, true, IDH_SETTINGS_EXPERT_MAX_HASH_SPEED, T_("MiB/s"));
	addItem(T_("Write buffer size"), SettingsManager::BUFFER_SIZE, true, IDH_SETTINGS_EXPERT_BUFFERSIZE, T_("KiB"));
	addItem(T_("Auto-search limit"), SettingsManager::AUTO_SEARCH_LIMIT, true, IDH_SETTINGS_EXPERT_AUTO_SEARCH_LIMIT);
	addItem(T_("Auto-search interval"), SettingsManager::AUTO_SEARCH_INTERVAL, true, IDH_SETTINGS_EXPERT_AUTO_SEARCH_INTERVAL, T_("seconds"));
	addItem(T_("Mini slot size"), SettingsManager::SET_MINISLOT_SIZE, true, IDH_SETTINGS_EXPERT_MINISLOT_SIZE, T_("KiB"));
	addItem(T_("Max filelist size"), SettingsManager::MAX_FILELIST_SIZE, true, IDH_SETTINGS_EXPERT_MAX_FILELIST_SIZE, T_("MiB"));
	addItem(T_("PID"), SettingsManager::PRIVATE_ID, false, IDH_SETTINGS_EXPERT_PRIVATE_ID);
	addItem(T_("Auto refresh time"), SettingsManager::AUTO_REFRESH_TIME, true, IDH_SETTINGS_EXPERT_AUTO_REFRESH_TIME, T_("minutes"));
	addItem(T_("Settings save interval"), SettingsManager::SETTINGS_SAVE_INTERVAL, true, IDH_SETTINGS_EXPERT_SETTINGS_SAVE_INTERVAL, T_("minutes"));
	addItem(T_("Socket read buffer"), SettingsManager::SOCKET_IN_BUFFER, true, IDH_SETTINGS_EXPERT_SOCKET_IN_BUFFER, T_("B"));
	addItem(T_("Socket write buffer"), SettingsManager::SOCKET_OUT_BUFFER, true, IDH_SETTINGS_EXPERT_SOCKET_OUT_BUFFER, T_("B"));
	addItem(T_("Max PM windows"), SettingsManager::MAX_PM_WINDOWS, true, IDH_SETTINGS_EXPERT_MAX_PM_WINDOWS);
	addItem(T_("Max protocol command length"), SettingsManager::MAX_COMMAND_LENGTH, true, IDH_SETTINGS_EXPERT_MAX_COMMAND_LENGTH, T_("B"));

	AddWhitelistUI();

	PropPage::read(items);
}

ExpertsPage::~ExpertsPage() {
}

void ExpertsPage::write() {
	PropPage::write(items);

	SettingsManager* settings = SettingsManager::getInstance();
	if(SETTING(SET_MINISLOT_SIZE) < 64)
		settings->set(SettingsManager::SET_MINISLOT_SIZE, 64);
	if(SETTING(AUTO_SEARCH_LIMIT) > 5)
		settings->set(SettingsManager::AUTO_SEARCH_LIMIT, 5);
	else if(SETTING(AUTO_SEARCH_LIMIT) < 1)
		settings->set(SettingsManager::AUTO_SEARCH_LIMIT, 1);

	if(SETTING(AUTO_SEARCH_INTERVAL) < 120)
		settings->set(SettingsManager::AUTO_SEARCH_INTERVAL, 120);
}

void ExpertsPage::addItem(const tstring& text, int setting, bool isInt, unsigned helpId, const tstring& text2) {
	auto group = grid->addChild(GroupBox::Seed(text));
	group->setHelpId(helpId);

	auto cur = group->addChild(Grid::Seed(1, 2));
	cur->column(0).mode = GridInfo::FILL;

	auto box = cur->addChild(isInt ? WinUtil::Seeds::Dialog::intTextBox : WinUtil::Seeds::Dialog::textBox);
	items.emplace_back(box, setting, isInt ? PropPage::T_INT : PropPage::T_STR);

	if(text2.empty())
		cur->setWidget(box, 0, 0, 1, 2);
	else
		cur->addChild(Label::Seed(text2));
}

void ExpertsPage::AddWhitelistUI()
{
	auto group = grid->addChild(GroupBox::Seed(T_("Whitelisted URIs to open")));
	group->setHelpId(IDH_SETTINGS_EXPERT_WHITELIST_OPEN_URIS);

	auto cur = group->addChild(Grid::Seed(1, 2));
	cur->column(0).mode = GridInfo::FILL;

	TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
	items.emplace_back(box, SettingsManager::WHITELIST_OPEN_URIS, PropPage::T_STR);

	box->setText(Text::toT(SETTING(WHITELIST_OPEN_URIS)));

	modifyWhitelistButton = cur->addChild(Button::Seed(T_("M&odify")));
	modifyWhitelistButton->onClicked([this, box] 
	{ 
		tstring strName = T_("Whitelisted URIs to open");

		TStringList lst = StringTokenizer<tstring>(box->getText(), ';').getTokens();

		ItemsEditDlg dlg(this, strName, lst);
		dlg.setTitle(strName);
		dlg.setDescription(strName);
		dlg.setEditTitle(strName);

		if(dlg.run() == IDOK)
		{
			StringList lstValue;
			Text::fromT(dlg.getValues(), lstValue);

			box->setText(Text::toT(Util::toString(";", lstValue)));
		}
	});
}
