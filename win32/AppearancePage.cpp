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
#include "AppearancePage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/File.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/Spinner.h>

#include "WinUtil.h"
#include "resource.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Spinner;

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::MINIMIZE_TRAY, N_("Minimize to tray"), IDH_SETTINGS_APPEARANCE_MINIMIZE_TRAY },
	{ SettingsManager::ALWAYS_TRAY, N_("Always display tray icon"), IDH_SETTINGS_APPEARANCE_ALWAYS_TRAY },
	{ SettingsManager::TIME_STAMPS, N_("Show timestamps in chat by default"), IDH_SETTINGS_APPEARANCE_TIME_STAMPS },
	{ SettingsManager::STATUS_IN_CHAT, N_("View status messages in chat"), IDH_SETTINGS_APPEARANCE_STATUS_IN_CHAT },
	{ SettingsManager::FILTER_MESSAGES, N_("Filter spam messages"), IDH_SETTINGS_APPEARANCE_FILTER_MESSAGES },
	{ SettingsManager::SHOW_JOINS, N_("Show joins / parts in chat by default"), IDH_SETTINGS_APPEARANCE_SHOW_JOINS },
	{ SettingsManager::FAV_SHOW_JOINS, N_("Only show joins / parts for favorite users"), IDH_SETTINGS_APPEARANCE_FAV_SHOW_JOINS },
	{ SettingsManager::SORT_FAVUSERS_FIRST, N_("Sort favorite users first"), IDH_SETTINGS_APPEARANCE_SORT_FAVUSERS_FIRST },
	{ SettingsManager::GET_USER_COUNTRY, N_("Guess user country from IP"), IDH_SETTINGS_APPEARANCE_GET_USER_COUNTRY },
	{ SettingsManager::SHOW_USER_IP, N_("Show user IP in chat"), IDH_SETTINGS_APPEARANCE_SHOW_USER_IP },
	{ SettingsManager::SHOW_USER_COUNTRY, N_("Show user country in chat"), IDH_SETTINGS_APPEARANCE_SHOW_USER_COUNTRY },
	{ SettingsManager::GEO_CITY, N_("City-level geolocation database (allows parameters such as %[city])"), IDH_SETTINGS_APPEARANCE_GEO_CITY },
	{ SettingsManager::GEO_REGION, N_("Region name geolocation database (allows %[region])"), IDH_SETTINGS_APPEARANCE_GEO_REGION },
	{ 0, 0 }
};

AppearancePage::AppearancePage(dwt::Widget* parent) :
PropPage(parent, 5, 1),
options(0),
languages(0)
{
	setHelpId(IDH_APPEARANCEPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	options = grid->addChild(GroupBox::Seed(T_("Options")))->addChild(WinUtil::Seeds::Dialog::optionsTable);

	{
		auto cur = grid->addChild(Grid::Seed(1, 2));
		cur->setSpacing(grid->getSpacing());

		auto group = cur->addChild(GroupBox::Seed(T_("Timestamp format")));
		group->setHelpId(IDH_SETTINGS_APPEARANCE_TIMESTAMP_FORMAT);

		items.emplace_back(group->addChild(WinUtil::Seeds::Dialog::textBox), SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR);

		group = cur->addChild(GroupBox::Seed(T_("Country format")));
		group->setHelpId(IDH_SETTINGS_APPEARANCE_COUNTRY_FORMAT);

		items.emplace_back(group->addChild(WinUtil::Seeds::Dialog::textBox), SettingsManager::COUNTRY_FORMAT, PropPage::T_STR);
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Height of the message editing box")));
		group->setHelpId(IDH_SETTINGS_APPEARANCE_MESSAGE_LINES);

		auto cur = group->addChild(Grid::Seed(1, 5));
		cur->column(1).size = 40;
		cur->column(1).mode = GridInfo::STATIC;
		cur->column(3).size = 40;
		cur->column(3).mode = GridInfo::STATIC;

		cur->addChild(Label::Seed(T_("Keep it between")));

		auto box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.emplace_back(box, SettingsManager::MIN_MESSAGE_LINES, PropPage::T_INT_WITH_SPIN);
		auto spin = cur->addChild(Spinner::Seed(1, UD_MAXVAL, box));
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("and")));

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.emplace_back(box, SettingsManager::MAX_MESSAGE_LINES, PropPage::T_INT_WITH_SPIN);
		spin = cur->addChild(Spinner::Seed(1, UD_MAXVAL, box));
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("lines")));
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Language")));
		group->setHelpId(IDH_SETTINGS_APPEARANCE_LANGUAGE);

		languages = group->addChild(WinUtil::Seeds::Dialog::comboBox);
		languages->setHelpId(IDH_SETTINGS_APPEARANCE_LANGUAGE);
	}

	auto label = grid->addChild(Label::Seed(T_("Note: some of these options require that you restart DC++")));
	label->setHelpId(IDH_SETTINGS_APPEARANCE_REQUIRES_RESTART);

	PropPage::read(items);
	PropPage::read(listItems, options);

	typedef map<string, string, noCaseStringLess> lang_map;
	lang_map langs;
	langs["en"] = "English (United States)";

	StringList dirs = File::findFiles(Util::getPath(Util::PATH_LOCALE), "*");
	for(auto& i: dirs) {
		string dir = i + "LC_MESSAGES" PATH_SEPARATOR_STR;
		StringList files = File::findFiles(dir, "*.mo");
		if(find(files.begin(), files.end(), dir + "dcpp.mo") == files.end() && find(files.begin(), files.end(), dir + "dcpp-win32.mo") == files.end()) {
			continue;
		}

		string text = Util::getLastDir(i);
		try {
			langs[text] = File(i + "name.txt", File::READ, File::OPEN).read();
		} catch(const FileException&) {
			langs[text] = "";
		}
	}

	languages->addValue(T_("Default"));

	int selected = 0, j = 1;
	const string& cur = SETTING(LANGUAGE);
	for(auto& i: langs) {
		string text = i.first;
		if(!i.second.empty())
			text += ": " + i.second;
		languages->addValue(Text::toT(text));

		if(selected == 0 && (i.first == cur || (i.first == "en" && cur == "C"))) {
			selected = j;
		} else {
			++j;
		}
	}

	languages->setSelected(selected);
}

AppearancePage::~AppearancePage() {
}

void AppearancePage::write()
{
	PropPage::write(items);
	PropPage::write(options);

	tstring lang = languages->getText();
	size_t col = lang.find(':');
	if(col != string::npos)
		lang = lang.substr(0, col);

	if(lang == T_("Default")) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE, "");
	} else if(lang == _T("en")) {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE, "C");
	} else {
		SettingsManager::getInstance()->set(SettingsManager::LANGUAGE, Text::fromT(lang));
	}
}
