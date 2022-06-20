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
#include "AdvancedPage.h"

#include <dwt/widgets/Grid.h>
#include <dcpp/SettingsManager.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;

AdvancedPage::ListItem AdvancedPage::listItems[] = {
	{ SettingsManager::AUTO_FOLLOW, N_("Automatically follow redirects"), IDH_SETTINGS_ADVANCED_AUTO_FOLLOW },
	{ SettingsManager::CLEAR_SEARCH, N_("Clear search box after each search"), IDH_SETTINGS_ADVANCED_CLEAR_SEARCH },
	{ SettingsManager::LIST_DUPES, N_("Keep duplicate files in your file list"), IDH_SETTINGS_ADVANCED_LIST_DUPES },
	{ SettingsManager::URL_HANDLER, N_("Register with Windows to handle dchub://, adc:// and adcs:// URL links"), IDH_SETTINGS_ADVANCED_URL_HANDLER },
	{ SettingsManager::MAGNET_REGISTER, N_("Register with Windows to handle magnet: URI links"), IDH_SETTINGS_ADVANCED_MAGNET_REGISTER },
	{ SettingsManager::DCEXT_REGISTER, N_("Register with Windows to handle .dcext files"), IDH_SETTINGS_ADVANCED_DCEXT_REGISTER },
	{ SettingsManager::KEEP_LISTS, N_("Don't delete file lists when exiting"), IDH_SETTINGS_ADVANCED_KEEP_LISTS },
	{ SettingsManager::AUTO_KICK, N_("Automatically disconnect users who leave the hub"), IDH_SETTINGS_ADVANCED_AUTO_KICK },
	{ SettingsManager::SFV_CHECK, N_("Enable automatic SFV checking"), IDH_SETTINGS_ADVANCED_SFV_CHECK },
	{ SettingsManager::NO_AWAYMSG_TO_BOTS, N_("Don't send the away message to bots"), IDH_SETTINGS_ADVANCED_NO_AWAYMSG_TO_BOTS },
	{ SettingsManager::ADLS_BREAK_ON_FIRST, N_("Break on first ADLSearch match"), IDH_SETTINGS_ADVANCED_ADLS_BREAK_ON_FIRST },
	{ SettingsManager::COMPRESS_TRANSFERS, N_("Enable safe and compressed transfers"), IDH_SETTINGS_ADVANCED_COMPRESS_TRANSFERS },
	{ SettingsManager::HUB_USER_COMMANDS, N_("Accept custom user commands from hub"), IDH_SETTINGS_ADVANCED_HUB_USER_COMMANDS },
	{ SettingsManager::SEND_UNKNOWN_COMMANDS, N_("Send unknown /commands to the hub"), IDH_SETTINGS_ADVANCED_SEND_UNKNOWN_COMMANDS },
	{ SettingsManager::ADD_FINISHED_INSTANTLY, N_("Add finished files to share instantly (if shared)"), IDH_SETTINGS_ADVANCED_ADD_FINISHED_INSTANTLY },
	{ SettingsManager::USE_CTRL_FOR_LINE_HISTORY, N_("Use CTRL for line history"), IDH_SETTINGS_ADVANCED_USE_CTRL_FOR_LINE_HISTORY },
	{ SettingsManager::AUTO_KICK_NO_FAVS, N_("Don't automatically disconnect favorite users who leave the hub"), IDH_SETTINGS_ADVANCED_AUTO_KICK_NO_FAVS },
	{ SettingsManager::OWNER_DRAWN_MENUS, N_("Use extended menus with icons and titles"), IDH_SETTINGS_ADVANCED_OWNER_DRAWN_MENUS },
	{ SettingsManager::USE_SYSTEM_ICONS, N_("Use system icons when browsing files (slows browsing down a bit)"), IDH_SETTINGS_ADVANCED_USE_SYSTEM_ICONS },
	{ SettingsManager::CLICKABLE_CHAT_LINKS, N_("Clickable chat links (disable on Wine)"), IDH_SETTINGS_CLICKABLE_CHAT_LINKS },
	{ SettingsManager::SEGMENTED_DL, N_("Enable segmented downloads"), IDH_SETTINGS_ADVANCED_SEGMENTED_DL },
	{ SettingsManager::REGISTER_SYSTEM_STARTUP, N_("Start FearDC when Windows starts"), IDH_SETTINGS_ADVANCED_REGISTER_SYSTEM_STARTUP },
	{ SettingsManager::TESTING_STATUS, N_("Display testing nags"), IDH_SETTINGS_ADVANCED_DISPLAY_TESTING_NAGS,
		[]() { return SETTING(TESTING_STATUS) != SettingsManager::TESTING_DISABLED; }, // custom read
		[](bool checked) { // custom write
			if(checked) {
				SettingsManager::getInstance()->unset(SettingsManager::TESTING_STATUS); // back to defaults
			} else {
				SettingsManager::getInstance()->set(SettingsManager::TESTING_STATUS, SettingsManager::TESTING_DISABLED);
			}
		}
	},
	{ 0, 0 }
};

AdvancedPage::AdvancedPage(dwt::Widget* parent) : PropPage(parent, 1, 1) {
	setHelpId(IDH_ADVANCEDPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	options = grid->addChild(WinUtil::Seeds::Dialog::optionsTable);
	PropPage::read(listItems, options);
}

AdvancedPage::~AdvancedPage() {
}

void AdvancedPage::write() {
	PropPage::write(options);
}
