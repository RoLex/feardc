/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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
#include "PublicHubsFrame.h"

#include <dcpp/ClientManager.h>
#include <dcpp/FavoriteManager.h>
//#include <dcpp/GeoManager.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>

#include "resource.h"
#include "HubFrame.h"
#include "HoldRedraw.h"
#include "HubListsDlg.h"
#include "TypedTable.h"

using dwt::Grid;
using dwt::GridInfo;

const string PublicHubsFrame::id = "PublicHubs";
const string& PublicHubsFrame::getId() const { return id; }

dwt::ImageListPtr PublicHubsFrame::hubIcons;

static const ColumnInfo hubsColumns[] = {
	{ N_("Status"), 25, false },
	{ N_("Name"), 200, false },
	{ N_("Description"), 290, false },
	{ N_("Users"), 50, true },
	{ N_("Address"), 100, false },
	{ N_("Country"), 100, false },
	{ N_("Shared"), 100, true },
	{ N_("Min share"), 100, true },
	{ N_("Min slots"), 100, true },
	{ N_("Max hubs"), 100, true },
	{ N_("Max users"), 100, true },
	{ N_("Reliability"), 100, true },
	{ N_("Rating"), 100, false }
};

PublicHubsFrame::HubInfo::HubInfo(const HubEntry* entry_, const tstring& statusText) : entry(entry_) {
	columns[COLUMN_STATUS] = statusText;
	columns[COLUMN_NAME] = Text::toT(entry->getName());
	columns[COLUMN_DESCRIPTION] = Text::toT(entry->getDescription());
	columns[COLUMN_USERS] = Text::toT(std::to_string(entry->getUsers()));
	columns[COLUMN_SERVER] = Text::toT(entry->getServer());
	columns[COLUMN_COUNTRY] = Text::toT(entry->getCountry());
	columns[COLUMN_SHARED] = Text::toT(Util::formatBytes(entry->getShared()));
	columns[COLUMN_MINSHARE] = Text::toT(Util::formatBytes(entry->getMinShare()));
	columns[COLUMN_MINSLOTS] = Text::toT(std::to_string(entry->getMinSlots()));
	columns[COLUMN_MAXHUBS] = Text::toT(std::to_string(entry->getMaxHubs()));
	columns[COLUMN_MAXUSERS] = Text::toT(std::to_string(entry->getMaxUsers()));
	columns[COLUMN_RELIABILITY] = Text::toT(Util::toString(entry->getReliability()));
	columns[COLUMN_RATING] = Text::toT(entry->getRating());
}

int PublicHubsFrame::HubInfo::compareItems(const HubInfo* a, const HubInfo* b, int col) {
	switch(col) {
	case COLUMN_USERS: return compare(a->entry->getUsers(), b->entry->getUsers());
	case COLUMN_SHARED: return compare(a->entry->getShared(), b->entry->getShared());
	case COLUMN_MINSHARE: return compare(a->entry->getMinShare(), b->entry->getMinShare());
	case COLUMN_MINSLOTS: return compare(a->entry->getMinSlots(), b->entry->getMinSlots());
	case COLUMN_MAXHUBS: return compare(a->entry->getMaxHubs(), b->entry->getMaxHubs());
	case COLUMN_MAXUSERS: return compare(a->entry->getMaxUsers(), b->entry->getMaxUsers());
	case COLUMN_RELIABILITY: return compare(a->entry->getReliability(), b->entry->getReliability());
	default: return compare(a->columns[col], b->columns[col]);
	}
}

PublicHubsFrame::PublicHubsFrame(TabViewPtr parent) :
BaseType(parent, T_("Public Hubs"), IDH_PUBLIC_HUBS, IDI_PUBLICHUBS, false),
grid(0),
upper(0),
blacklist(0),
hubs(0),
filter(hubsColumns, COLUMN_LAST, [this] { updateList(); }),
lists(0),
listsGrid(0),
visibleHubs(0),
users(0)
{
	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		upper = grid->addChild(Grid::Seed(2, 1));
		upper->setSpacing(0);
		upper->column(0).mode = GridInfo::FILL;
		upper->row(0).size = 0;
		upper->row(0).mode = GridInfo::STATIC;
		upper->row(1).mode = GridInfo::FILL;
		upper->row(1).align = GridInfo::STRETCH;

		{
			TextBox::Seed seed = WinUtil::Seeds::textBox;
			seed.style &= ~WS_VISIBLE;
			seed.style |= WS_DISABLED | ES_MULTILINE | WS_VSCROLL | ES_READONLY;
			seed.lines = 8;
			blacklist = upper->addChild(seed);
			addWidget(blacklist, NO_FOCUS);
		}

		{
			WidgetHubs::Seed seed(WinUtil::Seeds::table);
			seed.style |= LVS_SINGLESEL;
			hubs = upper->addChild(seed);
			addWidget(hubs);
		}

		WinUtil::makeColumns(hubs, hubsColumns, COLUMN_LAST, SETTING(PUBLICHUBSFRAME_ORDER), SETTING(PUBLICHUBSFRAME_WIDTHS));
		WinUtil::setTableSort(hubs, COLUMN_LAST, SettingsManager::PUBLICHUBSFRAME_SORT, COLUMN_USERS, false);

		if(!hubIcons) {
			const dwt::Point size(16, 16);
			hubIcons = dwt::ImageListPtr(new dwt::ImageList(size));
			hubIcons->add(dwt::Icon(IDI_HUB, size));
			hubIcons->add(dwt::Icon(IDI_HUB_OFF, size));
		}
		hubs->setSmallImageList(hubIcons);

		hubs->onDblClicked([this] { openSelected(); });
		hubs->onKeyDown([this](int c) { return handleKeyDown(c); });
		hubs->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleContextMenu(sc); });
	}

	{
		GridPtr lower = grid->addChild(Grid::Seed(2, 4));
		// columns 0 and 3, and row 1, are just here to have proper margins.
		lower->column(0).size = 0;
		lower->column(0).mode = GridInfo::STATIC;
		lower->column(1).mode = GridInfo::FILL;
		lower->column(2).mode = GridInfo::FILL;
		lower->column(3).size = 0;
		lower->column(3).mode = GridInfo::STATIC;
		lower->row(1).size = 0;
		lower->row(1).mode = GridInfo::STATIC;
		lower->setSpacing(6);

		GroupBox::Seed gs = WinUtil::Seeds::group;

		gs.caption = T_("F&ilter");
		GroupBoxPtr group = lower->addChild(gs);
		lower->setWidget(group, 0, 1);
		group->setHelpId(IDH_PUBLIC_HUBS_FILTER);

		GridPtr cur = group->addChild(Grid::Seed(1, 3));
		cur->column(0).mode = GridInfo::FILL;

		filter.createTextBox(cur);
		filter.text->setCue(T_("Filter hubs"));
		addWidget(filter.text);

		filter.createColumnBox(cur);
		addWidget(filter.column);

		filter.createMethodBox(cur);
		addWidget(filter.method);

		gs.caption = T_("Configured Public Hub Lists");
		group = lower->addChild(gs);
		lower->setWidget(group, 0, 2);
		group->setHelpId(IDH_PUBLIC_HUBS_LISTS);

		listsGrid = group->addChild(Grid::Seed(1, 3));
		listsGrid->column(0).mode = GridInfo::FILL;

		lists = listsGrid->addChild(WinUtil::Seeds::comboBox);
		addWidget(lists);
		lists->onSelectionChanged([this] { handleListSelChanged(); });

		Button::Seed bs = WinUtil::Seeds::button;

		bs.caption = T_("&Configure");
		ButtonPtr button = listsGrid->addChild(bs);
		addWidget(button);
		button->onClicked([this] { handleConfigure(); });

		bs.caption = T_("&Refresh");
		button = listsGrid->addChild(bs);
		button->setHelpId(IDH_PUBLIC_HUBS_REFRESH);
		addWidget(button);
		button->onClicked([this] { handleRefresh(); });
	}

	initStatus();

	status->setHelpId(STATUS_STATUS, IDH_PUBLIC_HUBS_STATUS);
	status->setHelpId(STATUS_HUBS, IDH_PUBLIC_HUBS_HUBS);
	status->setHelpId(STATUS_USERS, IDH_PUBLIC_HUBS_USERS);

	FavoriteManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	entries	 = FavoriteManager::getInstance()->getPublicHubs();

	// populate with values from the settings
	updateDropDown();
	updateList();

	addAccel(FALT, 'I', [this] { filter.text->setFocus(); });
	initAccels();

	layout();

	if(FavoriteManager::getInstance()->isDownloading()) {
		status->setText(STATUS_STATUS, T_("Downloading public hub list..."));
		listsGrid->setEnabled(false);
	} else if(entries.empty()) {
		FavoriteManager::getInstance()->refresh();
	}
}

PublicHubsFrame::~PublicHubsFrame() {
}

bool PublicHubsFrame::preClosing() {
	FavoriteManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);

	return true;
}

void PublicHubsFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::PUBLICHUBSFRAME_ORDER, WinUtil::toString(hubs->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::PUBLICHUBSFRAME_WIDTHS, WinUtil::toString(hubs->getColumnWidths()));
	SettingsManager::getInstance()->set(SettingsManager::PUBLICHUBSFRAME_SORT, WinUtil::getTableSort(hubs));
}

void PublicHubsFrame::layout() {
	dwt::Rectangle r { getClientSize() };

	r.size.y -= status->refresh();

	grid->resize(r);
}

void PublicHubsFrame::updateStatus() {
	if (entries.size() > visibleHubs)
		status->setText(STATUS_HUBS, str(TF_("Hubs: %1% / %2%") % visibleHubs % entries.size()));
	else
		status->setText(STATUS_HUBS, str(TF_("Hubs: %1%") % visibleHubs));
	status->setText(STATUS_USERS, str(TF_("Users: %1%") % users));
}

void PublicHubsFrame::updateDropDown() {
	lists->clear();
	StringList l(FavoriteManager::getInstance()->getHubLists());
	for(auto& idx: l) {
		lists->addValue(Text::toT(idx).c_str());
	}
	lists->setSelected((FavoriteManager::getInstance()->getSelectedHubList()) % l.size());
}

void PublicHubsFrame::updateList() {
	dcdebug("PublicHubsFrame::updateList\n");

	const string& blacklisted = FavoriteManager::getInstance()->blacklisted();
	if(blacklisted.empty()) {
		if(blacklist->getEnabled()) {
			blacklist->setText(Util::emptyStringT);
			blacklist->setEnabled(false);
			blacklist->setVisible(false);
			upper->row(0).mode = GridInfo::STATIC;
			upper->layout();
		}
	} else {
		blacklist->setText(str(TF_(
			"Warning: fraudulent hub list detected!\n\n"
			"The current hub list (%1%) has been blacklisted by FearDC for the following reason:\n"
			"%2%\n\n"
			"It is strongly recommended that you do not connect to any of the hubs listed here "
			"and that you remove this hub list from your collection by using the \"Configure\" "
			"button below. Alternatively you can go to Settings / Downloads pane and click "
			"\"Reset Public Hub Lists\" button to get the list of latest known safe and working " 
			"public hub list servers."
			) % Text::toT(FavoriteManager::getInstance()->getCurrentHubList()) % Text::toT(blacklisted)));
		blacklist->setEnabled(true);
		blacklist->setVisible(true);
		upper->row(0).mode = GridInfo::AUTO;
		upper->layout();
	}

	users = 0;
	visibleHubs = 0;

	auto i = entries.cbegin(), iend = entries.cend();

	auto filterPrep = filter.prepare();
	auto filterInfoF = [this, &i](int column) { return getText(*i, column); };

	HoldRedraw hold { hubs };
	hubs->clear();
	for(; i != iend; ++i) {
		if(filter.empty() || filter.match(filterPrep, filterInfoF)) {
			auto hubEntry = &*i;

			tstring statusText;
			int statusIcon;
			WinUtil::getHubStatus(hubEntry->getServer(), statusText, statusIcon);

			auto hubInfo = new HubInfo(hubEntry, statusText);

			auto row = hubs->insert(hubs->size(), hubInfo);
			hubs->setIcon(row, COLUMN_STATUS, statusIcon);

			visibleHubs++;
			users += i->getUsers();
		}
	}

	hubs->resort();

	updateStatus();
}

void PublicHubsFrame::changeHubStatus(const string& url)
{
	if(url == Util::emptyString)
	{
		return;
	}

	auto aUrl = Text::toT(url);

	for(unsigned int row = 0; row < hubs->size(); ++row)
	{
		HubInfo* hubInfo = hubs->getData(row);
		if(hubInfo == nullptr)
		{
			continue;
		}

		if(aUrl == hubInfo->getText(COLUMN_SERVER))
		{
			tstring statusText;
			int statusIcon;
			WinUtil::getHubStatus(url, statusText, statusIcon);

			{
				HoldRedraw hold { hubs };

				hubInfo->setText(COLUMN_STATUS, statusText);
				hubs->setIcon(row, COLUMN_STATUS, statusIcon);

				hubs->update(hubInfo);

				if(hubs->getSortColumn() == COLUMN_STATUS)
					hubs->resort();
			}

			break;
		}
	}
}

string PublicHubsFrame::getText(const HubEntry& entry, size_t column) const {
	switch(column) {
	case COLUMN_NAME: return entry.getName();
	case COLUMN_DESCRIPTION: return entry.getDescription();
	case COLUMN_USERS: return std::to_string(entry.getUsers());
	case COLUMN_SERVER: return entry.getServer();
	case COLUMN_COUNTRY: return entry.getCountry();
	case COLUMN_SHARED: return std::to_string(entry.getShared());
	case COLUMN_MINSHARE: return std::to_string(entry.getMinShare());
	case COLUMN_MINSLOTS: return std::to_string(entry.getMinSlots());
	case COLUMN_MAXHUBS: return std::to_string(entry.getMaxHubs());
	case COLUMN_MAXUSERS: return std::to_string(entry.getMaxUsers());
	case COLUMN_RELIABILITY: return Util::toString(entry.getReliability());
	case COLUMN_RATING: return entry.getRating();
	default: return Util::emptyString;
	}
}

bool PublicHubsFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(hubs->hasSelected()) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = hubs->getContextMenuPos();
		}

		auto menu = addChild(WinUtil::Seeds::menu);
		menu->setTitle(escapeMenu(hubs->getSelectedData()->getText(COLUMN_NAME)), getParent()->getIcon(this));
		menu->appendItem(T_("&Connect"), [this] { handleConnect(); }, dwt::IconPtr(), true, true);
		menu->appendItem(T_("Add To &Favorites"), [this] { handleAdd(); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		WinUtil::addCopyMenu(menu.get(), hubs);

		menu->open(pt);
		return true;
	}
	return false;
}

void PublicHubsFrame::handleRefresh() {
	status->setText(STATUS_STATUS, T_("Downloading public hub list..."));
	FavoriteManager::getInstance()->refresh(true);
	updateDropDown();
	listsGrid->setEnabled(false);
}

void PublicHubsFrame::handleConfigure() {
	HubListsDlg dlg(this);
	if(dlg.run() == IDOK) {
		SettingsManager::getInstance()->save();
		updateDropDown();
	}
}

void PublicHubsFrame::handleConnect() {
	if(!WinUtil::checkNick())
		return;

	if(hubs->hasSelected() == 1) {
		HubFrame::openWindow(getParent(), hubs->getSelectedData()->entry->getServer());
	}
}

void PublicHubsFrame::handleAdd() {
	if(!WinUtil::checkNick())
		return;

	if(hubs->hasSelected()) {
		FavoriteManager::getInstance()->addFavorite(*hubs->getSelectedData()->entry);
	}
}

void PublicHubsFrame::openSelected() {
	if(!WinUtil::checkNick())
		return;

	if(hubs->hasSelected()) {
		HubFrame::openWindow(getParent(), hubs->getSelectedData()->entry->getServer());
	}
}

bool PublicHubsFrame::handleKeyDown(int c) {
	if(c == VK_RETURN) {
		openSelected();
	}

	return false;
}

void PublicHubsFrame::handleListSelChanged() {
	listsGrid->setEnabled(false);
	FavoriteManager::getInstance()->setHubList(lists->getSelected());
	entries = FavoriteManager::getInstance()->getPublicHubs();
	updateList();
}

void PublicHubsFrame::onFinished(const tstring& s, bool success, bool isCached) {
	if(success) {
		entries = FavoriteManager::getInstance()->getPublicHubs();
		updateList();
	} else if (isCached) {
		// The HTTP resource has failed but we've got a cached version so let's queue another refresh to load that
		// We don't interact with the user about what to do here so this is a hack.
		// @todo make all the checks, decisions and load in the manager and fire the events only once, depending on the outcome.
		FavoriteManager::getInstance()->refresh(false, true);
	}
	status->setText(STATUS_STATUS, s);
	listsGrid->setEnabled(true);
}

void PublicHubsFrame::on(DownloadStarting, const string& l) noexcept {
	callAsync([=, this] { status->setText(STATUS_STATUS, str(TF_("Downloading public hub list... (%1%)") % Text::toT(l)), false); });
}

void PublicHubsFrame::on(DownloadFailed, const string& l, bool isCached) noexcept {
	callAsync([=, this] { onFinished(str(TF_("Download failed: %1%") % Text::toT(l)), false, isCached); });
}

void PublicHubsFrame::on(DownloadFinished, const string& l) noexcept {
	callAsync([=, this] { onFinished(str(TF_("Hub list downloaded (%1%)") % Text::toT(l)), true, false); });
}

void PublicHubsFrame::on(LoadedFromCache, const string& l, const string& d, bool isForced) noexcept {
	tstring s = isForced ? str(TF_("Download failed")) + _T(" - ") : Util::emptyStringT;
	callAsync([=, this] { onFinished(s + str(TF_("Locally cached (as of %1%) version of the hub list loaded (%2%)") % Text::toT(d) % Text::toT(l)), true, false); });
}

void PublicHubsFrame::on(Corrupted, const string& l, bool isCached) noexcept {
	if(l.empty()) {
		callAsync([this] { onFinished(T_("Cached hub list is corrupted or unsupported"), false, false); });
	} else {
		callAsync([=, this] { onFinished(str(TF_("Downloaded hub list is corrupted or unsupported (%1%)") % Text::toT(l)), false, isCached); });
	}
}

void PublicHubsFrame::on(ClientManagerListener::ClientConnected, Client* client) noexcept
{
	if(client != nullptr)
	{
		auto url = client->getHubUrl();
		callAsync([=, this] { changeHubStatus(url); });
	}
}

void PublicHubsFrame::on(ClientManagerListener::ClientUpdated, Client* client) noexcept
{
	if(client != nullptr)
	{
		auto url = client->getHubUrl();
		callAsync([=, this] { changeHubStatus(url); });
	}
}

void PublicHubsFrame::on(ClientManagerListener::ClientDisconnected, Client* client) noexcept
{
	if(client != nullptr)
	{
		auto url = client->getHubUrl();
		callAsync([=, this] { changeHubStatus(url); });
	}
}
