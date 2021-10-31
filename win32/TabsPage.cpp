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
#include "TabsPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/RadioButton.h>
#include <dwt/widgets/Slider.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::RadioButton;
using dwt::Slider;

PropPage::ListItem TabsPage::listItems[] = {
	{ SettingsManager::BOLD_HUB, N_("Hub"), IDH_SETTINGS_TABS_BOLD_HUB },
	{ SettingsManager::BOLD_PM, N_("Private message"), IDH_SETTINGS_TABS_BOLD_PM },
	{ SettingsManager::BOLD_FL, N_("File List"), IDH_SETTINGS_TABS_BOLD_FL },
	{ SettingsManager::BOLD_SEARCH, N_("Search"), IDH_SETTINGS_TABS_BOLD_SEARCH },
	{ SettingsManager::BOLD_SYSTEM_LOG, N_("System Log"), IDH_SETTINGS_TABS_BOLD_SYSTEM_LOG },
	{ SettingsManager::BOLD_QUEUE, N_("Download Queue"), IDH_SETTINGS_TABS_BOLD_QUEUE },
	{ SettingsManager::BOLD_FINISHED_DOWNLOADS, N_("Finished Downloads"), IDH_SETTINGS_TABS_BOLD_FINISHED_DOWNLOADS },
	{ SettingsManager::BOLD_FINISHED_UPLOADS, N_("Finished Uploads"), IDH_SETTINGS_TABS_BOLD_FINISHED_UPLOADS },
	{ 0, 0 }
};

TabsPage::TabsPage(dwt::Widget* parent) :
PropPage(parent, 4, 1),
dcppDraw(0),
buttonStyle(0),
themeGroup(0),
browserTheme(0),
tabWidth(0),
previewGrid(0),
options(0)
{
	setHelpId(IDH_TABSPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(3).mode = GridInfo::FILL;
	grid->row(3).align = GridInfo::STRETCH;

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 3));

		auto cp = [this] { createPreview(); };
		GroupBoxPtr group = cur->addChild(GroupBox::Seed());
		group->setHelpId(IDH_SETTINGS_TABS_DRAW);
		GridPtr cur2 = group->addChild(Grid::Seed(2, 1));
		dcppDraw = cur2->addChild(RadioButton::Seed(T_("Let DC++ draw tabs")));
		dcppDraw->onClicked(cp);
		dcppDraw->onClicked([this]() { themeGroup->setEnabled(true); });
		RadioButtonPtr button = cur2->addChild(RadioButton::Seed(T_("Use standard Windows tabs")));
		button->onClicked(cp);
		button->onClicked([this]() { themeGroup->setEnabled(false); });
		if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_OD)
			dcppDraw->setChecked();
		else
			button->setChecked();

		group = cur->addChild(GroupBox::Seed());
		group->setHelpId(IDH_SETTINGS_TABS_STYLE);
		cur2 = group->addChild(Grid::Seed(2, 1));
		button = cur2->addChild(RadioButton::Seed(T_("Tab style")));
		button->onClicked(cp);
		buttonStyle = cur2->addChild(RadioButton::Seed(T_("Button style")));
		buttonStyle->onClicked(cp);
		if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BUTTONS)
			buttonStyle->setChecked();
		else
			button->setChecked();

		themeGroup = cur->addChild(GroupBox::Seed());
		themeGroup->setHelpId(IDH_SETTINGS_TABS_STYLE);
		cur2 = themeGroup->addChild(Grid::Seed(2, 1));
		button = cur2->addChild(RadioButton::Seed(T_("Default theme")));
		button->onClicked(cp);
		browserTheme = cur2->addChild(RadioButton::Seed(T_("Browser theme")));
		browserTheme->onClicked(cp);
		if(browserTheme->getEnabled() && (SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BROWSER))
			browserTheme->setChecked();
		else
			button->setChecked();
		if(!(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_OD))
			themeGroup->setEnabled(false);
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Tab width")));
		group->setHelpId(IDH_SETTINGS_TAB_WIDTH);

		GridPtr cur = group->addChild(Grid::Seed(1, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).size = 30;
		cur->row(0).mode = GridInfo::STATIC;
		cur->row(0).align = GridInfo::STRETCH;

		tabWidth = cur->addChild(Slider::Seed());
		tabWidth->setRange(100, 1000);
		tabWidth->setPosition(SETTING(TAB_WIDTH));
		tabWidth->onScrollHorz([this] { createPreview(); });
	}

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Preview")));
		group->setHelpId(IDH_SETTINGS_TAB_PREVIEW);

		previewGrid = group->addChild(Grid::Seed(1, 1));
		previewGrid->column(0).mode = GridInfo::FILL;
		previewGrid->row(0).size = 100;
		previewGrid->row(0).mode = GridInfo::STATIC;
		previewGrid->row(0).align = GridInfo::STRETCH;

		createPreview();
	}

	options = grid->addChild(GroupBox::Seed(T_("Tab highlight on content change")))->addChild(WinUtil::Seeds::Dialog::optionsTable);

	PropPage::read(listItems, options);
}

TabsPage::~TabsPage() {
}

void TabsPage::write() {
	int tabStyle = 0;
	if(dcppDraw->getChecked())
		tabStyle |= SettingsManager::TAB_STYLE_OD;
	if(buttonStyle->getChecked())
		tabStyle |= SettingsManager::TAB_STYLE_BUTTONS;
	if(browserTheme->getChecked())
		tabStyle |= SettingsManager::TAB_STYLE_BROWSER;
	SettingsManager::getInstance()->set(SettingsManager::TAB_STYLE, tabStyle);

	SettingsManager::getInstance()->set(SettingsManager::TAB_WIDTH, tabWidth->getPosition());

	PropPage::write(options);
}

void TabsPage::createPreview() {
	auto previous = *previewGrid->getChildren<TabView>().first;
	if(previous)
		previous->close();

	TabView::Seed seed = WinUtil::Seeds::tabs;
	seed.style &= ~TCS_TOOLTIPS;
	seed.widthConfig = tabWidth->getPosition();
	seed.style |= WS_DISABLED;
	if(dcppDraw->getChecked()) {
		if(browserTheme->getChecked())
			seed.tabStyle = TabView::Seed::WinBrowser;
	} else {
		seed.style &= ~TCS_OWNERDRAWFIXED;
		seed.widthConfig = (seed.widthConfig - 100) / 9; // max width to max chars
	}
	if(buttonStyle->getChecked())
		seed.style |= TCS_BUTTONS;
	seed.closeIcon = WinUtil::tabIcon(IDI_EXIT);
	TabViewPtr tabs = previewGrid->addChild(seed);

	Container::Seed cs;
	auto makeTab = [&tabs, &cs](const tstring& text) -> ContainerPtr {
		cs.caption = text;
		ContainerPtr ret = dwt::WidgetCreator<Container>::create(tabs, cs);
		// the tab control sends WM_ACTIVATE messages; catch them, otherwise the dialog gets messed up.
		ret->onRaw([](WPARAM, LPARAM) { return 0; }, dwt::Message(WM_ACTIVATE));
		return ret;
	};

	tabs->add(makeTab(T_("Example hub tab")), WinUtil::tabIcon(IDI_HUB));
	dwt::IconPtr icon = WinUtil::tabIcon(IDI_DCPP);
	tabs->add(makeTab(T_("Selected tab")), icon);
	ContainerPtr highlighted = makeTab(T_("Highlighted tab"));
	tabs->add(highlighted, icon);
	tabs->add(makeTab(T_("Yet another tab")), icon);
	tabs->setSelected(1);
	tabs->mark(highlighted);

	previewGrid->layout();
}
