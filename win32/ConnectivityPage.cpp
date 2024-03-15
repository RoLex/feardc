/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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
#include "ConnectivityPage.h"

#include <dcpp/format.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/MessageBox.h>

#include "ConnectivityManualPage.h"
#include "resource.h"
#include "SettingsDialog.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::GroupBox;

ConnectivityPage::ConnectivityPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
autoDetect(0),
detectNow(0),
log(0),
edit(0)
{
	setHelpId(IDH_CONNECTIVITYPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Automatic connectivity setup")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);

		auto cur = group->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(1).mode = GridInfo::FILL;
		cur->row(1).align = GridInfo::STRETCH;

		auto cur2 = cur->addChild(Grid::Seed(1, 2));
		cur2->column(1).mode = GridInfo::FILL;
		cur2->column(1).align = GridInfo::BOTTOM_RIGHT;

		autoDetect = cur2->addChild(CheckBox::Seed(T_("Let FearDC determine the best connectivity settings")));
		items.emplace_back(autoDetect, SettingsManager::AUTO_DETECT_CONNECTION, PropPage::T_BOOL);
		autoDetect->onClicked([this] { handleAutoClicked(); });

		detectNow = cur2->addChild(Button::Seed(T_("Detect now")));
		detectNow->setHelpId(IDH_SETTINGS_CONNECTIVITY_DETECT_NOW);
		detectNow->onClicked([] { ConnectivityManager::getInstance()->detectConnection(); });

		group = cur->addChild(GroupBox::Seed(T_("Detection log")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_DETECTION_LOG);

		log = group->addChild(WinUtil::Seeds::Dialog::richTextBox);

		cur2 = cur->addChild(Grid::Seed(1, 1));
		cur2->column(0).mode = GridInfo::FILL;
		cur2->column(0).align = GridInfo::BOTTOM_RIGHT;

		edit = cur2->addChild(Button::Seed(T_("Edit detected settings")));
		edit->setHelpId(IDH_SETTINGS_CONNECTIVITY_EDIT);
		edit->setImage(WinUtil::buttonIcon(IDI_CONN_GREY));
		edit->onClicked([this] { handleEdit(); });
	}

	PropPage::read(items);

	callAsync([this] {
		updateAuto();
	});

	if(SETTING(AUTO_DETECT_CONNECTION)) {
		const auto& status4 = ConnectivityManager::getInstance()->getStatus(false);
		const auto& status6 = ConnectivityManager::getInstance()->getStatus(true);
		if(!status4.empty()) {
			addLogLine(Text::toT(str(F_("IPv4: %1%") % status4)));
		}
		if(!status6.empty()) {
			addLogLine(Text::toT(str(F_("IPv6: %1%") % status6)));
		}
	} else {
		addLogLine(T_("Automatic connectivity setup is currently disabled"));
	}

	ConnectivityManager::getInstance()->addListener(this);
}

ConnectivityPage::~ConnectivityPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void ConnectivityPage::handleAutoClicked() {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, autoDetect->getChecked());
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
}

void ConnectivityPage::handleEdit() {
	if(dwt::MessageBox(this).show(T_(
		"Automatic connectivity setup will be disabled.\n\n"
		"Manually configured settings will be overwritten by automatically detected ones.\n\n"
		"Proceed?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)
	{
		ConnectivityManager::getInstance()->editAutoSettings();
		static_cast<SettingsDialog*>(getRoot())->activatePage<ConnectivityManualPage>();
	}
}

void ConnectivityPage::updateAuto() {
	bool enable = SETTING(AUTO_DETECT_CONNECTION);
	autoDetect->setChecked(enable);

	enable = enable && !ConnectivityManager::getInstance()->isRunning();
	detectNow->setEnabled(enable);
	edit->setEnabled(enable && ConnectivityManager::getInstance()->ok());
}

void ConnectivityPage::addLogLine(const tstring& msg) {
	/// @todo factor out to dwt
	log->addTextSteady(Text::toT("{\\urtf1\n") + log->rtfEscape(msg + Text::toT("\r\n")) + Text::toT("}\n"));
}

void ConnectivityPage::on(Message, const string& message) noexcept {
	callAsync([this, message] { addLogLine(Text::toT(message)); });
}

void ConnectivityPage::on(Started) noexcept {
	callAsync([this] {
		detectNow->setEnabled(false);
		this->log->setText(Util::emptyStringT);
		edit->setEnabled(false);
		autoDetect->setEnabled(false);
	});
}

void ConnectivityPage::on(Finished) noexcept {
	callAsync([this] {
		detectNow->setEnabled(true);
		edit->setEnabled(true);
		autoDetect->setEnabled(true);
	});
}

void ConnectivityPage::on(SettingChanged) noexcept {
	callAsync([this] { updateAuto(); });
}
