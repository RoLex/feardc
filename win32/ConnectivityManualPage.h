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

#ifndef DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H
#define DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H

#include <dcpp/ConnectivityManager.h>

#include "PropPage.h"

class ConnectivityManualPage;

class PageContent
{
public:
	PageContent(dwt::GridPtr grid);

	void read();
	void write();
	void clear();

protected:
	void initializeUI();

	SettingsManager::IntSetting settingIncomingConnections;
	SettingsManager::StrSetting settingExternalIP;
	SettingsManager::BoolSetting settingNoIPOverride;
	SettingsManager::StrSetting settingBindAddress;
	SettingsManager::StrSetting settingMapper;

	bool isV6;

private:
	RadioButtonPtr active;
	RadioButtonPtr upnp;
	RadioButtonPtr passive;
	RadioButtonPtr inactive;

	ComboBoxPtr mapper;
	ComboBoxPtr bind;

	PropPage::ItemList items;

	dwt::GridPtr grid;
};

class PageContentV4 : public PageContent
{
public:
	PageContentV4(dwt::GridPtr grid);
};

class PageContentV6 : public PageContent
{
public:
	PageContentV6(dwt::GridPtr grid);
};

class ConnectivityManualPage : public PropPage, private ConnectivityManagerListener
{
public:
	ConnectivityManualPage(dwt::Widget* parent);
	virtual ~ConnectivityManualPage();

	void write();

private:
	ItemList items;

	GroupBoxPtr autoGroup;
	CheckBoxPtr autoDetect;

	dwt::GridPtr portGrid, v4Grid, v6Grid;

	TextBoxPtr transferBox;
	TextBoxPtr tlstransferBox;

	std::unique_ptr<PageContent> v4Content, v6Content;

	void handleAutoClicked();

	void read();
	void updateAuto();

	void onTransferPortUpdated();
	void onTLSTransferPortUpdated();
	void validatePort(TextBoxPtr sourceBox, TextBoxPtr otherBox, const tstring& source, const tstring& other);

	// ConnectivityManagerListener
	void on(SettingChanged) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_CONNECTIVITY_MANUAL_PAGE_H)
