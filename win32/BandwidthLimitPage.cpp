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

#include "stdafx.h"
#include "BandwidthLimitPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/ThrottleManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Spinner.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Spinner;

BandwidthLimitPage::BandwidthLimitPage(dwt::Widget* parent) :
PropPage(parent, 4, 1),
main(0),
secondaryToggle(0),
secondary(0),
throttleTime(0)
{
	setHelpId(IDH_BWLIMITPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::AUTO;
	grid->row(0).align = GridInfo::STRETCH;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;
	grid->row(2).mode = GridInfo::AUTO;
	grid->row(2).align = GridInfo::STRETCH;
	grid->row(3).mode = GridInfo::FILL;
	grid->row(3).align = GridInfo::STRETCH;

	{
		main = grid->addChild(GroupBox::Seed(T_("Transfer Rate Limiting")));
		GridPtr cur = main->addChild(Grid::Seed(2, 2));
		cur->column(0).size = 70;
		cur->column(0).mode = GridInfo::STATIC;

		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		box->setHelpId(IDH_SETTINGS_BWLIMIT_UPLOAD);
		items.emplace_back(box, SettingsManager::MAX_UPLOAD_SPEED_MAIN, PropPage::T_INT_WITH_SPIN);

		SpinnerPtr spin = cur->addChild(Spinner::Seed(0, ThrottleManager::MAX_LIMIT, box));
		spin->setHelpId(IDH_SETTINGS_BWLIMIT_UPLOAD);
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("Maximum Upload Rate (KiB/s) (0 = infinite)")))->setHelpId(IDH_SETTINGS_BWLIMIT_UPLOAD);

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		box->setHelpId(IDH_SETTINGS_BWLIMIT_DOWNLOAD);
		items.emplace_back(box, SettingsManager::MAX_DOWNLOAD_SPEED_MAIN, PropPage::T_INT_WITH_SPIN);

		spin = cur->addChild(Spinner::Seed(0, ThrottleManager::MAX_LIMIT, box));
		spin->setHelpId(IDH_SETTINGS_BWLIMIT_DOWNLOAD);
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("Maximum Download Rate (KiB/s) (0 = infinite)")))->setHelpId(IDH_SETTINGS_BWLIMIT_DOWNLOAD);
	}

	{
		secondaryToggle = grid->addChild(GroupBox::Seed(T_("Secondary Transfer Rate Limiting")));
		GridPtr cur = secondaryToggle->addChild(Grid::Seed(1, 4));
		cur->column(0).mode = GridInfo::AUTO;
		cur->column(1).mode = GridInfo::AUTO;
		cur->column(1).align = GridInfo::CENTER;
		cur->column(2).mode = GridInfo::FILL;
		cur->column(2).align = GridInfo::CENTER;
		cur->column(3).mode = GridInfo::AUTO;
		cur->column(3).align = GridInfo::CENTER;

		throttleTime = cur->addChild(CheckBox::Seed(T_("Use second set of bandwidth limits from")));
		throttleTime->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_ENABLE);
		items.emplace_back(throttleTime, SettingsManager::TIME_DEPENDENT_THROTTLE, PropPage::T_BOOL);
		throttleTime->onClicked([this] { fixControls(); });

		timeBound[0] = cur->addChild(WinUtil::Seeds::Dialog::comboBox);
		cur->addChild(Label::Seed(T_("to")))->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_ENABLE);
		timeBound[1] = cur->addChild(WinUtil::Seeds::Dialog::comboBox);
	}

	for (int i = 0; i < 2; ++i) {
		timeBound[i]->addValue(T_("Midnight"));
		for (int j = 1; j < 12; ++j)
			timeBound[i]->addValue(Text::toT(Util::toString(j) +" AM").c_str()); ///@todo use the user locale
	 	timeBound[i]->addValue(T_("Noon"));
		for (int j = 1; j < 12; ++j)
			timeBound[i]->addValue(Text::toT(Util::toString(j) +" PM").c_str()); ///@todo use the user locale
		timeBound[i]->setSelected(i?SETTING(BANDWIDTH_LIMIT_END):SETTING(BANDWIDTH_LIMIT_START));
		timeBound[i]->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_ENABLE);
	}

	{
		secondary = grid->addChild(GroupBox::Seed(T_("Secondary Transfer Rate Limiting Settings")));
		GridPtr cur = secondary->addChild(Grid::Seed(3, 2));
		cur->column(0).size = 70;
		cur->column(0).mode = GridInfo::STATIC;

		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		box->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_UPLOAD);
		items.emplace_back(box, SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE, PropPage::T_INT_WITH_SPIN);

		SpinnerPtr spin = cur->addChild(Spinner::Seed(0, ThrottleManager::MAX_LIMIT, box));
		spin->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_UPLOAD);
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("Maximum Upload Rate (KiB/s) (0 = infinite)")))->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_UPLOAD);

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		box->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_DOWNLOAD);
		items.emplace_back(box, SettingsManager::MAX_DOWNLOAD_SPEED_ALTERNATE, PropPage::T_INT_WITH_SPIN);

		spin = cur->addChild(Spinner::Seed(0, ThrottleManager::MAX_LIMIT, box));
		spin->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_DOWNLOAD);
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("Maximum Download Rate (KiB/s) (0 = infinite)")))->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_DOWNLOAD);

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		box->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_SLOTS);
		items.emplace_back(box, SettingsManager::SLOTS_ALTERNATE_LIMITING, PropPage::T_INT_WITH_SPIN);

		spin = cur->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		spin->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_SLOTS);
		cur->setWidget(spin);

		cur->addChild(Label::Seed(T_("Upload Slots")))->setHelpId(IDH_SETTINGS_BWLIMIT_SECONDARY_SLOTS);
	}

	PropPage::read(items);

	fixControls();
}

BandwidthLimitPage::~BandwidthLimitPage() {
}

void BandwidthLimitPage::write() {
	PropPage::write(items);

	SettingsManager::getInstance()->set(SettingsManager::BANDWIDTH_LIMIT_START, timeBound[0]->getSelected());
	SettingsManager::getInstance()->set(SettingsManager::BANDWIDTH_LIMIT_END, timeBound[1]->getSelected());
}

void BandwidthLimitPage::fixControls() {
	secondary->setEnabled(throttleTime->getChecked());
}
