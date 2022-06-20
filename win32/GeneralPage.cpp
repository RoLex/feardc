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
#include "GeneralPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Spinner.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Spinner;

GeneralPage::GeneralPage(dwt::Widget* parent) :
PropPage(parent, 2, 1),
nick(0),
connections(0)
{
	setHelpId(IDH_GENERALPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Personal information")));
		group->setHelpId(IDH_SETTINGS_GENERAL_PERSONAL_INFORMATION);

		auto cur = group->addChild(Grid::Seed(4, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		cur->addChild(Label::Seed(T_("Nick")))->setHelpId(IDH_SETTINGS_GENERAL_NICK);
		nick = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(nick, SettingsManager::NICK, PropPage::T_STR);
		nick->setHelpId(IDH_SETTINGS_GENERAL_NICK);
		WinUtil::preventSpaces(nick);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_SETTINGS_GENERAL_DESCRIPTION);
		auto box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(box, SettingsManager::DESCRIPTION, PropPage::T_STR);
		box->setHelpId(IDH_SETTINGS_GENERAL_DESCRIPTION);

		cur->addChild(Label::Seed(T_("Email")))->setHelpId(IDH_SETTINGS_GENERAL_EMAIL);
		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(box, SettingsManager::EMAIL, PropPage::T_STR);
		box->setHelpId(IDH_SETTINGS_GENERAL_EMAIL);

		cur->addChild(Label::Seed(T_("Line speed (upload)")))->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);

		{
			auto conn = cur->addChild(Grid::Seed(1, 2));
			conn->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);

			connections = conn->addChild(WinUtil::Seeds::Dialog::comboBox);

			conn->addChild(Label::Seed(T_("MiBits/s")));
		}
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Away mode")));
		group->setHelpId(IDH_SETTINGS_GENERAL_AWAY_MODE);

		auto cur = group->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		{
			group = cur->addChild(GroupBox::Seed(T_("Away message (empty = no away message)")));
			group->setHelpId(IDH_SETTINGS_GENERAL_DEFAULT_AWAY_MESSAGE);

			auto cur2 = group->addChild(Grid::Seed(2, 1));
			cur2->column(0).mode = GridInfo::FILL;

			auto seed = WinUtil::Seeds::Dialog::textBox;
			seed.style |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;
			items.emplace_back(cur2->addChild(seed), SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR);

			auto box = cur2->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Add timestamp to the end of away messages")));
			box->setHelpId(IDH_SETTINGS_GENERAL_AWAY_TIMESTAMP);
			items.emplace_back(box, SettingsManager::AWAY_TIMESTAMP, PropPage::T_BOOL);
		}

		// dummy grid so that the check-box doesn't fill the whole row.
		auto box = cur->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Enable away mode when FearDC is minimized")));
		box->setHelpId(IDH_SETTINGS_GENERAL_AUTO_AWAY);
		items.emplace_back(box, SettingsManager::AUTO_AWAY, PropPage::T_BOOL);

		box = cur->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Enable away mode when the Windows session is locked")));
		box->setHelpId(IDH_SETTINGS_GENERAL_AWAY_COMP_LOCK);
		items.emplace_back(box, SettingsManager::AWAY_COMP_LOCK, PropPage::T_BOOL);

		{
			auto idle = cur->addChild(Grid::Seed(1, 3));
			idle->setHelpId(IDH_SETTINGS_GENERAL_AWAY_IDLE);
			idle->column(1).size = 40;
			idle->column(1).mode = GridInfo::STATIC;

			idle->addChild(Label::Seed(T_("Enable away mode after")));

			auto box = idle->addChild(WinUtil::Seeds::Dialog::intTextBox);
			items.emplace_back(box, SettingsManager::AWAY_IDLE, PropPage::T_INT_WITH_SPIN);
			idle->setWidget(idle->addChild(Spinner::Seed(0, UD_MAXVAL, box)));

			idle->addChild(Label::Seed(T_("minutes of inactivity (0 = disabled)")));
		}
	}

	PropPage::read(items);

	if(SETTING(NICK).empty()) {
		// fill the Nick field with the Win user account name.
		DWORD size = 0;
		::GetUserName(0, &size);
		if(size > 1) {
			tstring str(size - 1, 0);
			if(::GetUserName(&str[0], &size)) {
				nick->setText(str);
			}
		}
	}

	int selected = 0, j = 0;
	for(auto i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i, ++j) {
		connections->addValue(Text::toT(*i).c_str());
		if(selected == 0 && SETTING(UPLOAD_SPEED) == *i) {
			selected = j;
		}
	}
	connections->setSelected(selected);
}

GeneralPage::~GeneralPage() {
}

void GeneralPage::write() {
	PropPage::write(items);

	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, Text::fromT(connections->getText()));
}
