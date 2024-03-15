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
#include "ConnectivityManualPage.h"

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/format.h>
#include <dcpp/MappingManager.h>
#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/RadioButton.h>

#include "WinUtil.h"
#include "resource.h"

using dwt::Control;
using dwt::Grid;
using dwt::GridInfo;
using dwt::RadioButton;

ConnectivityManualPage::ConnectivityManualPage(dwt::Widget* parent) :
PropPage(parent, 4, 1),
autoGroup(0),
autoDetect(0),
portGrid(0),
v4Grid(0),
v6Grid(0),
transferBox(0),
tlstransferBox(0)
{
	setHelpId(IDH_CONNECTIVITYMANUALPAGE);

	grid->column(0).mode = GridInfo::FILL;

	autoGroup = grid->addChild(GroupBox::Seed(T_("Automatic connectivity setup")));
	autoGroup->setHelpId(IDH_SETTINGS_CONNECTIVITY_AUTODETECT);

	autoDetect = autoGroup->addChild(CheckBox::Seed(T_("Let FearDC determine the best connectivity settings")));
	autoDetect->onClicked([this] { handleAutoClicked(); });

	portGrid = grid->addChild(Grid::Seed(1, 3));
	portGrid->setSpacing(grid->getSpacing());

	auto addPortBox = [this](const tstring& text, int setting, unsigned helpId) {
		auto group = portGrid->addChild(GroupBox::Seed(str(TF_("%1% port") % text)));
		group->setHelpId(helpId);

		auto boxGrid = group->addChild(Grid::Seed(1, 1));
		boxGrid->column(0).size = 40;
		boxGrid->column(0).mode = GridInfo::STATIC;

		auto inputBox = boxGrid->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.emplace_back(inputBox, setting, PropPage::T_INT);

		return inputBox;
	};

	transferBox = addPortBox(T_("Transfer"), SettingsManager::TCP_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_TCP);
	transferBox->onUpdated([this] { onTransferPortUpdated(); });

	tlstransferBox = addPortBox(T_("Encrypted transfer"), SettingsManager::TLS_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_TLS);
	tlstransferBox->onUpdated([this] { onTLSTransferPortUpdated(); });

	addPortBox(T_("Search"), SettingsManager::UDP_PORT, IDH_SETTINGS_CONNECTIVITY_PORT_UDP);
	
	v4Grid = grid->addChild(GroupBox::Seed(T_("IPv4")))->addChild(Grid::Seed(4, 1));
	v4Grid->column(0).mode = GridInfo::FILL;

	v6Grid = grid->addChild(GroupBox::Seed(T_("IPv6")))->addChild(Grid::Seed(4, 1));
	v6Grid->column(0).mode = GridInfo::FILL;

	v4Content.reset(new PageContentV4(v4Grid));
	v6Content.reset(new PageContentV6(v6Grid));

	read();

	updateAuto();

	ConnectivityManager::getInstance()->addListener(this);
}

ConnectivityManualPage::~ConnectivityManualPage() {
	ConnectivityManager::getInstance()->removeListener(this);
}

void ConnectivityManualPage::write() {
	if(transferBox->getText() == tlstransferBox->getText())
	{
		tlstransferBox->setText(Util::emptyStringT);
	}

	PropPage::write(items);

	v4Content->write();
	v6Content->write();
}

void ConnectivityManualPage::handleAutoClicked() {
	// apply immediately so that ConnectivityManager updates.
	SettingsManager::getInstance()->set(SettingsManager::AUTO_DETECT_CONNECTION, autoDetect->getChecked());
	ConnectivityManager::getInstance()->fire(ConnectivityManagerListener::SettingChanged());
}

void ConnectivityManualPage::updateAuto() {
	bool setting = SETTING(AUTO_DETECT_CONNECTION);
	autoDetect->setChecked(setting);

	portGrid->setEnabled(!setting);
	v4Grid->setEnabled(!setting);
	v6Grid->setEnabled(!setting);
}

void ConnectivityManualPage::read() {
	autoDetect->setChecked(SETTING(AUTO_DETECT_CONNECTION));

	PropPage::read(items);

	v4Content->read();
	v6Content->read();
}

void ConnectivityManualPage::onTransferPortUpdated() {
	validatePort(transferBox, tlstransferBox, T_("Transfer"), T_("encrypted transfer"));
}

void ConnectivityManualPage::onTLSTransferPortUpdated() {
	validatePort(tlstransferBox, transferBox, T_("Encrypted transfer"), T_("transfer"));
}

void ConnectivityManualPage::validatePort(
	TextBoxPtr sourcebox, TextBoxPtr otherbox, const tstring& source, const tstring& other
) {
	if(sourcebox->getText() == otherbox->getText()) {
		sourcebox->showPopup(T_("Invalid value"), str(TF_("%1% port cannot be the same as the %2% port") % source % other), TTI_ERROR);
	}
}

void ConnectivityManualPage::on(SettingChanged) noexcept {
	callAsync([this] {
		updateAuto();

		// reload settings in case they have been changed (eg by the "Edit detected settings" feature).
		v4Content->clear();
		v6Content->clear();
		read();
	});
}

PageContent::PageContent(dwt::GridPtr grid_) :
	active(0),
	upnp(0),
	passive(0),
	inactive(0),
	mapper(0),
	bind(0),
	grid(grid_)
{
}

void PageContent::initializeUI()
{

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		active = cur->addChild(RadioButton::Seed(T_("Active mode (I have no router / I have configured my router)")));
		active->setHelpId(IDH_SETTINGS_CONNECTIVITY_ACTIVE);

		upnp = cur->addChild(RadioButton::Seed(T_("Active mode (let FearDC configure my router with NAT-PMP / UPnP)")));
		upnp->setHelpId(IDH_SETTINGS_CONNECTIVITY_UPNP);

		passive = cur->addChild(RadioButton::Seed(T_("Passive mode (last resort - has serious limitations)")));
		passive->setHelpId(IDH_SETTINGS_CONNECTIVITY_PASSIVE);

		inactive = cur->addChild(RadioButton::Seed(T_("Disable connectivity")));
		inactive->setHelpId(IDH_SETTINGS_CONNECTIVITY_DISABLE);
	}


	{
		auto group = grid->addChild(GroupBox::Seed(T_("External / WAN IP address")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_EXTERNAL_IP);

		auto cur = group->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		auto externalIP = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		externalIP->setCue(T_("Enter your external address here"));
		items.emplace_back(externalIP, settingExternalIP, PropPage::T_STR);
		WinUtil::preventSpaces(externalIP);

		auto overrideIP = cur->addChild(CheckBox::Seed(T_("Don't allow hubs/NAT-PMP/UPnP to override")));
		items.emplace_back(overrideIP, settingNoIPOverride, PropPage::T_BOOL);
		overrideIP->setHelpId(IDH_SETTINGS_CONNECTIVITY_OVERRIDE);
	}

	{
		auto cur = grid->addChild(Grid::Seed(2, 1));
		cur->setSpacing(grid->getSpacing());

		{
			auto group = cur->addChild(GroupBox::Seed(T_("Preferred port mapper")));
			group->setHelpId(IDH_SETTINGS_CONNECTIVITY_MAPPER);
			mapper = group->addChild(WinUtil::Seeds::Dialog::comboBox);
		}

		auto group = cur->addChild(GroupBox::Seed(T_("Bind Address")));
		group->setHelpId(IDH_SETTINGS_CONNECTIVITY_BIND_ADDRESS);
		bind = group->addChild(WinUtil::Seeds::Dialog::comboBox);
	}
}

void PageContent::read() {
	switch(SettingsManager::getInstance()->get(settingIncomingConnections)) {
		case SettingsManager::INCOMING_DISABLED: inactive->setChecked(); break;
		case SettingsManager::INCOMING_ACTIVE: active->setChecked(); break;
		case SettingsManager::INCOMING_ACTIVE_UPNP: upnp->setChecked(); break;
		case SettingsManager::INCOMING_PASSIVE: passive->setChecked(); break;
		default: break;
	}

	PropPage::read(items);

	{
		const auto& setting = SettingsManager::getInstance()->get(settingMapper);
		int sel = 0;

		auto mappers = MappingManager::getInstance()->getMappers(isV6);
		for(const auto& name : mappers) {
			auto pos = mapper->addValue(Text::toT(name));
			if(!sel && name == setting)
				sel = pos;
		}
		mapper->setSelected(sel);
	}
	
	{
		const auto& setting = SettingsManager::getInstance()->get(settingBindAddress);
		int sel = 0;

		bind->addValue(T_("Default"));

		if (!setting.empty()) {
			const auto& addresses = Util::getIpAddresses(isV6);
				for (const auto& ipstr : addresses) {
					auto valStr = Text::toT(ipstr.adapterName) + _T(" - ") + Text::toT(ipstr.ip);
					auto pos = bind->addValue(valStr);
					if (!sel && (ipstr.ip == setting)) {
						sel = pos;
					}
				}
			}
		bind->setSelected(sel);
	}
}

void PageContent::write() {
	PropPage::write(items);

	// Set the connection mode
	int c = inactive->getChecked() ? SettingsManager::INCOMING_DISABLED :
		upnp->getChecked() ? SettingsManager::INCOMING_ACTIVE_UPNP :
		passive->getChecked() ? SettingsManager::INCOMING_PASSIVE :
		SettingsManager::INCOMING_ACTIVE;
	if(SettingsManager::getInstance()->get(settingIncomingConnections) != c) {
		SettingsManager::getInstance()->set(settingIncomingConnections, c);
	}

	SettingsManager::getInstance()->set(settingMapper, Text::fromT(mapper->getText()));

	{
		auto setting = Text::fromT(bind->getText());
		size_t found = setting.rfind(" - "); // "friendly_name - ip_address"
		if (found == string::npos) {
			setting = Util::emptyString;
		} else {
			setting.erase(0, found + 3);
		}
		
		SettingsManager::getInstance()->set(settingBindAddress, setting);
	}
}

void PageContent::clear()
{
	active->setChecked(false);
	upnp->setChecked(false);
	passive->setChecked(false);
	inactive->setChecked(false);

	mapper->clear();

	bind->clear();
}

PageContentV4::PageContentV4(dwt::GridPtr grid) : PageContent(grid)
{
	settingIncomingConnections = SettingsManager::INCOMING_CONNECTIONS;
	settingExternalIP = SettingsManager::EXTERNAL_IP;
	settingNoIPOverride = SettingsManager::NO_IP_OVERRIDE;
	settingBindAddress = SettingsManager::BIND_ADDRESS;
	settingMapper = SettingsManager::MAPPER;

	isV6 = false;

	initializeUI();
}

PageContentV6::PageContentV6(dwt::GridPtr grid) : PageContent(grid)
{
	settingIncomingConnections = SettingsManager::INCOMING_CONNECTIONS6;
	settingExternalIP = SettingsManager::EXTERNAL_IP6;
	settingNoIPOverride = SettingsManager::NO_IP_OVERRIDE6;
	settingBindAddress = SettingsManager::BIND_ADDRESS6;
	settingMapper = SettingsManager::MAPPER6;

	isV6 = true;

	initializeUI();
}
