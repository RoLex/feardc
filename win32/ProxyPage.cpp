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
#include "ProxyPage.h"

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/SettingsManager.h>
#include <dcpp/Socket.h>

#include <dwt/widgets/Label.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/RadioButton.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::RadioButton;

ProxyPage::ProxyPage(dwt::Widget* parent) :
PropPage(parent, 2, 1),
autoGroup(0),
autoDetect(0),
directOut(0),
socks5(0),
socksSettings(0)
{
	setHelpId(IDH_PROXYPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	{
		autoGroup = grid->addChild(GroupBox::Seed(T_("Automatic connectivity setup")));
		autoGroup->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);

		autoDetect = autoGroup->addChild(CheckBox::Seed(T_("Let FearDC determine the best connectivity settings")));
		autoDetect->onClicked([this] { handleAutoClicked(); });
	}

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Proxy settings for outgoing connections")))->addChild(Grid::Seed(1, 2));
		cur->row(0).align = GridInfo::TOP_LEFT;
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		{
			GridPtr outChoice = cur->addChild(Grid::Seed(2, 1));

			directOut = outChoice->addChild(RadioButton::Seed(T_("Direct connection")));
			directOut->setHelpId(IDH_SETTINGS_PROXY_DIRECT_OUT);

			socks5 = outChoice->addChild(RadioButton::Seed(T_("SOCKS5")));
			socks5->setHelpId(IDH_SETTINGS_PROXY_SOCKS5);
		}

		socksSettings = cur->addChild(GroupBox::Seed(T_("SOCKS5")));
		cur = socksSettings->addChild(Grid::Seed(5, 2));

		cur->addChild(Label::Seed(T_("IP")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_SERVER);

		cur->addChild(Label::Seed(T_("Port")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PORT);

		auto box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(box, SettingsManager::SOCKS_SERVER, PropPage::T_STR);
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_SERVER);
		box->setTextLimit(250);
		WinUtil::preventSpaces(box);

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.emplace_back(box, SettingsManager::SOCKS_PORT, PropPage::T_INT);
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PORT);
		box->setTextLimit(250);

		cur->addChild(Label::Seed(T_("Login")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_USER);

		cur->addChild(Label::Seed(T_("Password")))->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PASSWORD);

		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(box, SettingsManager::SOCKS_USER, PropPage::T_STR);
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_USER);
		box->setTextLimit(250);

		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(box, SettingsManager::SOCKS_PASSWORD, PropPage::T_STR);
		box->setHelpId(IDH_SETTINGS_PROXY_SOCKS_PASSWORD);
		box->setTextLimit(250);

		auto socksResolve = cur->addChild(CheckBox::Seed(T_("Use SOCKS5 server to resolve host names")));
		cur->setWidget(socksResolve, 4, 0, 1, 2);
		items.emplace_back(socksResolve, SettingsManager::SOCKS_RESOLVE, PropPage::T_BOOL);
		socksResolve->setHelpId(IDH_SETTINGS_PROXY_SOCKS_RESOLVE);
	}

	read();

	updateAuto();

	fixControlsOut();
	directOut->onClicked([this] { fixControlsOut(); });
	socks5->onClicked([this] { fixControlsOut(); });

	ConnectivityManager::getInstance()->addListener(this);
}

ProxyPage::~ProxyPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void ProxyPage::write() {
	PropPage::write(items);

	SettingsManager::getInstance()->set(SettingsManager::OUTGOING_CONNECTIONS,
		socks5->getChecked() ? SettingsManager::OUTGOING_SOCKS5 : SettingsManager::OUTGOING_DIRECT);
}

void ProxyPage::read() {
	switch(SETTING(OUTGOING_CONNECTIONS)) {
	case SettingsManager::OUTGOING_SOCKS5: socks5->setChecked(); break;
	default: directOut->setChecked(); break;
	}

	PropPage::read(items);
}

void ProxyPage::handleAutoClicked() {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, autoDetect->getChecked());
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
}

void ProxyPage::updateAuto() {
	bool setting = SETTING(AUTO_DETECT_CONNECTION);
	autoDetect->setChecked(setting);

	auto controls = grid->getChildren<Control>();
	boost::for_each(controls, [this, setting](Control* control) {
		if(control != autoGroup) {
			control->setEnabled(!setting);
		}
	});
}

void ProxyPage::fixControlsOut() {
	socksSettings->setEnabled(socks5->getEnabled() && socks5->getChecked());
}

void ProxyPage::on(SettingChanged) noexcept {
	callAsync([this] {
		updateAuto();

		// reload settings in case they have been changed (eg by the "Edit detected settings" feature).

		directOut->setChecked(false);
		socks5->setChecked(false);

		read();
		fixControlsOut();
	});
}
