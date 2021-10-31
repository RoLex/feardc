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
#include "HistoryPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/WindowManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Spinner.h>

#include "DirectoryListingFrame.h"
#include "HubFrame.h"
#include "PrivateFrame.h"
#include "WinUtil.h"
#include "resource.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Spinner;

template<typename T>
GridPtr addSubGrid(T parent, size_t rows) {
	GridPtr grid = parent->addChild(Grid::Seed(rows, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(0).align = GridInfo::BOTTOM_RIGHT;
	grid->column(1).size = 40;
	grid->column(1).mode = GridInfo::STATIC;
	return grid;
}

TextBoxPtr addBox(GridPtr grid, const tstring& text, unsigned helpId) {
	grid->addChild(Label::Seed(text))->setHelpId(helpId);

	TextBoxPtr box = grid->addChild(WinUtil::Seeds::Dialog::intTextBox);
	box->setHelpId(helpId);

	SpinnerPtr spin = grid->addChild(Spinner::Seed(0, UD_MAXVAL, box));
	grid->setWidget(spin);
	spin->setHelpId(helpId);

	return box;
}

HistoryPage::HistoryPage(dwt::Widget* parent) :
PropPage(parent, 3, 1),
hub_recents(0),
pm_recents(0),
fl_recents(0),
hub_recents_init(WindowManager::getInstance()->getMaxRecentItems(HubFrame::id)),
pm_recents_init(WindowManager::getInstance()->getMaxRecentItems(PrivateFrame::id)),
fl_recents_init(WindowManager::getInstance()->getMaxRecentItems(DirectoryListingFrame::id))
{
	setHelpId(IDH_HISTORYPAGE);

	GroupBox::Seed gs;
	gs.style |= BS_RIGHT;

	{
		gs.caption = T_("Chat lines to recall from history when opening a window");
		GridPtr cur = addSubGrid(grid->addChild(gs), 2);

		items.emplace_back(addBox(cur, T_("Hubs"), IDH_SETTINGS_HISTORY_CHAT_HUBS), SettingsManager::HUB_LAST_LOG_LINES, PropPage::T_INT_WITH_SPIN);
		items.emplace_back(addBox(cur, T_("Private messages"), IDH_SETTINGS_HISTORY_CHAT_PM), SettingsManager::PM_LAST_LOG_LINES, PropPage::T_INT_WITH_SPIN);
	}

	{
		gs.caption = T_("Maximum number of recent items to save");
		GridPtr cur = addSubGrid(grid->addChild(gs), 3);

		hub_recents = addBox(cur, T_("Recent hubs"), IDH_SETTINGS_HISTORY_RECENT_HUBS);
		hub_recents->setText(Text::toT(Util::toString(hub_recents_init)));

		pm_recents = addBox(cur, T_("Recent PMs"), IDH_SETTINGS_HISTORY_RECENT_PMS);
		pm_recents->setText(Text::toT(Util::toString(pm_recents_init)));

		fl_recents = addBox(cur, T_("Recent file lists"), IDH_SETTINGS_HISTORY_RECENT_FLS);
		fl_recents->setText(Text::toT(Util::toString(fl_recents_init)));
	}

	gs.caption = T_("Search history");
	items.emplace_back(addBox(addSubGrid(grid->addChild(gs), 1), T_("Search history"), IDH_SETTINGS_HISTORY_SEARCH_HISTORY), SettingsManager::SEARCH_HISTORY, PropPage::T_INT_WITH_SPIN);

	PropPage::read(items);
}

HistoryPage::~HistoryPage() {
}

void HistoryPage::write() {
	PropPage::write(items);

	unsigned max = Util::toUInt(Text::fromT(hub_recents->getText()));
	if(max != hub_recents_init)
		WindowManager::getInstance()->setMaxRecentItems(HubFrame::id, max);

	max = Util::toUInt(Text::fromT(pm_recents->getText()));
	if(max != pm_recents_init)
		WindowManager::getInstance()->setMaxRecentItems(PrivateFrame::id, max);

	max = Util::toUInt(Text::fromT(fl_recents->getText()));
	if(max != fl_recents_init)
		WindowManager::getInstance()->setMaxRecentItems(DirectoryListingFrame::id, max);
}
