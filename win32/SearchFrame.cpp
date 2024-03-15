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
#include "SearchFrame.h"

#include <boost/range/adaptor/reversed.hpp>

#include <dcpp/ClientManager.h>
#include <dcpp/Encoder.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/GeoManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/SearchResult.h>
#include <dcpp/ShareManager.h>

#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/SplitterContainer.h>
#include <dwt/widgets/TableTree.h>

#include "HoldRedraw.h"
#include "resource.h"
#include "TypedTable.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::FolderDialog;
using dwt::SplitterContainer;

const string SearchFrame::id = "Search";
const string& SearchFrame::getId() const { return id; }

static const ColumnInfo resultsColumns[] = {
	{ N_("File"), 200, false },
	{ N_("Hits"), 50, true },
	{ N_("User"), 100, false },
	{ N_("Type"), 60, false },
	{ N_("Size"), 80, true },
	{ N_("Path"), 100, false },
	{ N_("Slots"), 50, true },
	{ N_("Connection"), 70, false },
	{ N_("Hub"), 150, false },
	{ N_("Exact size"), 100, true },
	{ N_("IP"), 100, false },
	{ N_("TTH"), 300, false },
	{ N_("CID"), 300, false }
};

TStringList SearchFrame::lastSearches;

SearchFrame::FrameSet SearchFrame::frames;

SearchFrame::SearchInfo::~SearchInfo() { }

int SearchFrame::SearchInfo::getImage(int col) const {
	if(col != 0) {
		return -1;
	}

	const SearchResultPtr& sr = srs[0];
	return sr->getType() == SearchResult::TYPE_FILE ? WinUtil::getFileIcon(sr->getFile()) : static_cast<int>(WinUtil::DIR_ICON);
}

int SearchFrame::SearchInfo::getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int col) const {
		auto style = srs[0]->getStyle();

		if (!style.font.empty()) {
			auto cached = WinUtil::getUserMatchFont(style.font);
			if (cached.get()) {
				font = cached->handle();
			}
		}

		if (style.textColor != -1) {
			textColor = style.textColor;
		}

		if (style.bgColor != -1) {
			bgColor = style.bgColor;
		}

	return CDRF_NEWFONT;
}

int SearchFrame::SearchInfo::compareItems(const SearchInfo* a, const SearchInfo* b, int col) {
	switch(col) {
	case COLUMN_HITS: return compare(a->srs.size(), b->srs.size());
	case COLUMN_TYPE:
		if(a->srs[0]->getType() == b->srs[0]->getType())
			return compare(a->columns[COLUMN_TYPE], b->columns[COLUMN_TYPE]);
		else
			return(a->srs[0]->getType() == SearchResult::TYPE_DIRECTORY) ? -1 : 1;
	case COLUMN_SIZE: // Fall through
	case COLUMN_EXACT_SIZE: return compare(a->srs[0]->getSize(), b->srs[0]->getSize());
	default: return compare(a->getText(col), b->getText(col));
	}
}

void SearchFrame::openWindow(TabViewPtr parent, const tstring& str, SearchManager::TypeModes type, const tstring& url) {
	frames.insert(new SearchFrame(parent, str, type, url));
}

void SearchFrame::closeAll() {
	for(auto i: frames)
		i->close(true);
}

SearchFrame::SearchFrame(TabViewPtr parent, const tstring& initialString, SearchManager::TypeModes initialType_, const tstring& hubUrl) :
BaseType(parent, T_("Search"), IDH_SEARCH, IDI_SEARCH, false),
paned(0),
options(0),
searchBox(0),
isHash(false),
mode(0),
size(0),
sizeMode(0),
fileType(0),
onlyFree(SETTING(SEARCH_ONLY_FREE_SLOTS)),
hideShared(SETTING(SEARCH_FILTER_SHARED)),
hubs(0),
results(0),
filter(resultsColumns, COLUMN_LAST, [this] { updateList(); }),
initialType(initialType_),
droppedResults(0)
{
	paned = addChild(SplitterContainer::Seed(SETTING(SEARCH_PANED_POS)));

	{
		options = paned->addChild(Grid::Seed(5, 1));
		options->column(0).mode = GridInfo::FILL;
		options->row(4).mode = GridInfo::FILL;
		options->row(4).align = GridInfo::STRETCH;

		auto gs = WinUtil::Seeds::group;

		gs.caption = T_("Search for");
		auto group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_SEARCH_FOR);

		auto cur = group->addChild(Grid::Seed(2, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(1).mode = GridInfo::FILL;

		searchBox = cur->addChild(WinUtil::Seeds::comboBoxEdit);
		cur->setWidget(searchBox, 0, 0, 1, 2);
		searchBox->getTextBox()->setCue(T_("Enter search here"));
		addWidget(searchBox);

		for(auto& i: lastSearches | boost::adaptors::reversed)
			searchBox->addValue(i);
		searchBox->getTextBox()->onKeyDown([this](int c) { return handleSearchKeyDown(c); });
		searchBox->getTextBox()->onChar([this] (int c) { return handleSearchChar(c); });

		Button::Seed bs = WinUtil::Seeds::button;
		bs.caption = T_("Purge");
		ButtonPtr button = cur->addChild(bs);
		button->setHelpId(IDH_SEARCH_PURGE);
		button->onClicked([this] { handlePurgeClicked(); });

		bs.style |= BS_DEFPUSHBUTTON;
		bs.caption = T_("Search");
		button = cur->addChild(bs);
		button->setHelpId(IDH_SEARCH_SEARCH);
		button->onClicked([this] { runSearch(); });

		gs.caption = T_("Size");
		group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_SIZE);

		cur = group->addChild(Grid::Seed(1, 3));
		cur->column(1).mode = GridInfo::FILL;

		mode = cur->addChild(WinUtil::Seeds::comboBox);
		addWidget(mode);

		mode->addValue(T_("Normal"));
		mode->addValue(T_("At least"));
		mode->addValue(T_("At most"));

		TextBox::Seed ts = WinUtil::Seeds::textBox;
		ts.style |= ES_AUTOHSCROLL | ES_NUMBER;
		size = cur->addChild(ts);
		addWidget(size);

		sizeMode = cur->addChild(WinUtil::Seeds::comboBox);
		addWidget(sizeMode);

		sizeMode->addValue(T_("B"));
		sizeMode->addValue(T_("KiB"));
		sizeMode->addValue(T_("MiB"));
		sizeMode->addValue(T_("GiB"));
		sizeMode->setSelected(2);

		gs.caption = T_("File type");
		group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_TYPE);

		{
			ComboBox::Seed cs = WinUtil::Seeds::comboBox;
			cs.style |= CBS_SORT;
			fileType = group->addChild(cs);
			addWidget(fileType);
			fillFileType(Text::toT(initialString.empty() ? SETTING(LAST_SEARCH_TYPE) : SearchManager::getTypeStr(initialType)));
		}

		gs.caption = T_("Search options");
		cur = options->addChild(gs)->addChild(Grid::Seed(2, 1));

		CheckBox::Seed cs = WinUtil::Seeds::checkBox;
		
		cs.caption = T_("Only users with free slots");
		auto box = cur->addChild(cs);
		box->setHelpId(IDH_SEARCH_SLOTS);
		box->setChecked(onlyFree);
		box->onClicked([this, box] { onlyFree = box->getChecked(); });

		cs.caption = T_("Hide files already in share");
		box = cur->addChild(cs);
		box->setHelpId(IDH_SEARCH_SHARE);
		box->setChecked(hideShared);
		box->onClicked([this, box] { hideShared = box->getChecked(); });

		gs.caption = T_("Hubs");
		group = options->addChild(gs);
		group->setHelpId(IDH_SEARCH_HUBS);

		WidgetHubs::Seed ls(WinUtil::Seeds::table);
		ls.style |= LVS_NOCOLUMNHEADER;
		ls.lvStyle |= LVS_EX_CHECKBOXES;
		hubs = group->addChild(ls);
		addWidget(hubs);

		hubs->addColumn();

		hubs->onRaw([this](WPARAM w, LPARAM l) { return handleHubItemChanged(w, l); }, dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));

		hubs->insert(new HubInfo(Util::emptyStringT, T_("Hubs where I'm op"), true, true));
		hubs->setChecked(OPTION_ONLY_AS_OP, false);
		hubs->insert(new HubInfo(Util::emptyStringT, T_("Hubs with secure transfers"), true, true));
		hubs->setChecked(OPTION_ONLY_SECURE, false);
	}

	{
		auto cur = paned->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;

		results = cur->addChild(WidgetResults::Seed(WinUtil::Seeds::table));
		addWidget(results);

		results->setSmallImageList(WinUtil::fileImages);

		WinUtil::makeColumns(results, resultsColumns, COLUMN_LAST, SETTING(SEARCHFRAME_ORDER), SETTING(SEARCHFRAME_WIDTHS));
		WinUtil::setTableSort(results, COLUMN_LAST, SettingsManager::SEARCHFRAME_SORT, COLUMN_HITS, false);

		results->onDblClicked([this] { handleDownload(); });
		results->onKeyDown([this](int c) { return handleKeyDown(c); });
		results->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleContextMenu(sc); });

		cur = cur->addChild(Grid::Seed(1, 4));
		cur->column(1).mode = GridInfo::FILL;
		cur->setHelpId(IDH_SEARCH_FILTER);

		auto ls = WinUtil::Seeds::label;
		ls.caption = T_("Filter search results:");
		cur->addChild(ls);

		filter.createTextBox(cur);
		filter.text->setCue(T_("Filter search results"));
		addWidget(filter.text);

		filter.createColumnBox(cur);
		addWidget(filter.column);

		filter.createMethodBox(cur);
		addWidget(filter.method);
	}

	initStatus();

	{
		auto showUI = addChild(WinUtil::Seeds::splitCheckBox);
		showUI->setChecked(true);
		showUI->onClicked([this, showUI] { paned->maximize(showUI->getChecked() ? nullptr : results->getParent()); });
		status->setWidget(STATUS_SHOW_UI, showUI);
	}

	addAccel(FALT, 'I', [this] { filter.text->setFocus(); });
	initAccels();

	layout();
	activate();

	ClientManager* clientMgr = ClientManager::getInstance();
	{
		auto lock = clientMgr->lock();
		clientMgr->addListener(this);
		auto& clients = clientMgr->getClients();
		for(auto client: clients) {
			if(!client->isConnected())
				continue;

			auto hubInfo = new HubInfo(client);

			bool checkedHub = true;
			if(!hubUrl.empty())
			{
				checkedHub = hubInfo->url == hubUrl;
			}

			onHubAdded(hubInfo, checkedHub);
		}
	}

	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);

	SearchManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	if(!initialString.empty()) {
		lastSearches.push_back(initialString);
		searchBox->insertValue(0, initialString);
		searchBox->setSelected(0);
		runSearch();
	} else {
		mode->setSelected(1);
	}
	searchBox->setFocus();
}

SearchFrame::~SearchFrame() {
}

void SearchFrame::fillFileType(const tstring& toSelect) {
	vector<pair<string, int>> values;
	for(int type = SearchManager::TYPE_ANY; type != SearchManager::TYPE_LAST; ++type)
		values.emplace_back(SearchManager::getTypeStr(type), type);

	for(auto& i: SettingsManager::getInstance()->getSearchTypes()) {
		if(i.first.size() > 1 || i.first[0] < '1' || i.first[0] > '6') { //Custom type
			values.emplace_back(i.first, SearchManager::TYPE_ANY);
		}
	}

	bool selected = false;
	for(auto& i: values) {
		tstring value = Text::toT(i.first);
		int pos = fileType->addValue(value);
		if(i.second != SearchManager::TYPE_ANY)
			fileType->setData(pos, i.second);
		if(!selected && value == toSelect) {
			fileType->setSelected(pos);
			selected = true;
		}
	}
	if(!selected)
		fileType->setSelected(0);
}

void SearchFrame::searchTypesChanged() {
	tstring sel = fileType->getText();
	fileType->clear();
	fillFileType(sel);
}

void SearchFrame::layout() {
	dwt::Rectangle r { getClientSize() };

	r.size.y -= status->refresh();

	paned->resize(r);
}

bool SearchFrame::preClosing() {
	SettingsManager::getInstance()->removeListener(this);
	SearchManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);

	frames.erase(this);
	return true;
}

void SearchFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::SEARCH_PANED_POS, paned->getSplitterPos(0));

	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_ORDER, WinUtil::toString(results->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_WIDTHS, WinUtil::toString(results->getColumnWidths()));
	SettingsManager::getInstance()->set(SettingsManager::SEARCHFRAME_SORT, WinUtil::getTableSort(results));
}

void SearchFrame::SearchInfo::view() {
	if(srs[0]->getType() == SearchResult::TYPE_FILE) {
		QueueManager::getInstance()->add(Util::getTempPath() + srs[0]->getFileName(),
			srs[0]->getSize(), srs[0]->getTTH(), srs[0]->getUser(),
			QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_TEXT);
	}
}

void SearchFrame::SearchInfo::Download::operator()(SearchInfo* si) {
	if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
		addFile(si, Text::fromT(tgt + si->columns[COLUMN_FILENAME]));
	} else {
		addDir(si);
	}
}

void SearchFrame::SearchInfo::Download::addFile(SearchInfo* si, const string& target) {
	for(const auto& sr: si->srs) {
		total++;
		try {
			QueueManager::getInstance()->add(target, sr->getSize(), sr->getTTH(), sr->getUser());
		} catch(const Exception& e) {
			ignored++;
			error = e.getError();
		}
	}

	if(WinUtil::isShift())
		QueueManager::getInstance()->setPriority(target, QueueItem::HIGHEST);
}

void SearchFrame::SearchInfo::Download::addDir(SearchInfo* si, const string& target) {
	total++;
	// TODO Add all users...
	QueueManager::getInstance()->addDirectory(target.empty() ? si->srs[0]->getFile() : target,
		si->srs[0]->getUser(), Text::fromT(tgt),
		WinUtil::isShift() ? QueueItem::HIGHEST : QueueItem::DEFAULT);
}

void SearchFrame::SearchInfo::DownloadWhole::operator()(SearchInfo* si) {
	if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
		addDir(si, Text::fromT(si->columns[COLUMN_PATH]));
	} else {
		addDir(si);
	}
}

void SearchFrame::SearchInfo::DownloadTarget::operator()(SearchInfo* si) {
	if(si->srs[0]->getType() == SearchResult::TYPE_FILE) {
		addFile(si, Text::fromT(tgt));
	} else {
		addDir(si);
	}
}

void SearchFrame::SearchInfo::CheckTTH::operator()(SearchInfo* si) {
	const SearchResultPtr& sr = si->srs[0];

	if(firstTTH) {
		hasTTH = true;
		firstTTH = false;
		tth = sr->getTTH();
		name = si->getText(COLUMN_FILENAME);
		size = sr->getSize();
	} else if(hasTTH) {
		if(tth != sr->getTTH()) {
			hasTTH = false;
		}
	}

	if(firstHubs && hubs.empty()) {
		hubs = ClientManager::getInstance()->getHubUrls(sr->getUser());
		firstHubs = false;
	} else if(!hubs.empty()) {
		Util::intersect(hubs, ClientManager::getInstance()->getHubUrls(sr->getUser()));
	}
}

SearchFrame::SearchInfo::SearchInfo(const SearchResultPtr& aSR) : parent(nullptr) {
	srs.push_back(aSR);
	update();
}

void SearchFrame::SearchInfo::update() {
	// TODO Use most common value, not the first one...
	const SearchResultPtr& sr = srs[0];
	if(sr->getType() == SearchResult::TYPE_FILE) {
		if(sr->getFile().rfind(_T('\\')) == tstring::npos) {
			columns[COLUMN_FILENAME] = Text::toT(sr->getFile());
		} else {
			columns[COLUMN_FILENAME] = Text::toT(Util::getFileName(sr->getFile()));
			columns[COLUMN_PATH] = Text::toT(Util::getFilePath(sr->getFile()));
		}

		columns[COLUMN_TYPE] = Text::toT(Util::getFileExt(Text::fromT(columns[COLUMN_FILENAME])));
		if(!columns[COLUMN_TYPE].empty() && columns[COLUMN_TYPE][0] == _T('.'))
			columns[COLUMN_TYPE].erase(0, 1);
		columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(sr->getSize()));
		columns[COLUMN_EXACT_SIZE] = Text::toT(Util::formatExactSize(sr->getSize()));
        columns[COLUMN_TTH] = Text::toT(sr->getTTH().toBase32());
	} else {
		columns[COLUMN_FILENAME] = Text::toT(sr->getFileName());
		columns[COLUMN_PATH] = Text::toT(sr->getFile());
		columns[COLUMN_TYPE] = T_("Directory");
		if(sr->getSize() > 0) {
			columns[COLUMN_SIZE] = Text::toT(Util::formatBytes(sr->getSize()));
			columns[COLUMN_EXACT_SIZE] = Text::toT(Util::formatExactSize(sr->getSize()));
		}
	}

	if(parent) {
		columns[COLUMN_HITS].clear();
	} else {
		columns[COLUMN_HITS] = Text::toT(std::to_string(srs.size()));
	}

	if(srs.size() > 1) {
		columns[COLUMN_CONNECTION].clear();
		columns[COLUMN_IP].clear();
		columns[COLUMN_CID].clear();

		std::set<std::string> nicks, hubs;
		int freeSlots = 0, slots = 0;
		for(auto& i: srs) {
			nicks.insert(ClientManager::getInstance()->getNicks(i->getUser())[0]);
			hubs.insert(i->getHubName());
			freeSlots += i->getFreeSlots();
			slots += i->getSlots();
		}
		columns[COLUMN_NICK] = Text::toT(Util::toString(StringList(nicks.begin(), nicks.end())));
		columns[COLUMN_HUB] = Text::toT(Util::toString(StringList(hubs.begin(), hubs.end())));
		columns[COLUMN_SLOTS] = Text::toT(std::to_string(freeSlots) + '/' + std::to_string(slots));

	} else {
		columns[COLUMN_NICK] = WinUtil::getNicks(sr->getUser());
		columns[COLUMN_CONNECTION] = Text::toT(ClientManager::getInstance()->getConnection(sr->getUser().user->getCID()));
		columns[COLUMN_SLOTS] = Text::toT(sr->getSlotString());
		const auto& ip = sr->getIP();
		const auto& country = GeoManager::getInstance()->getCountry(ip);
		columns[COLUMN_IP] = Text::toT(country.empty() ? ip : str(F_("%1% (%2%)") % country % ip));
		columns[COLUMN_HUB] = Text::toT(sr->getHubName());
		columns[COLUMN_CID] = Text::toT(sr->getUser().user->getCID().toBase32());
	}
}

void SearchFrame::addResult(SearchResultPtr psr) {
	auto& sr = *psr;

	// Check that this is really a relevant search result...
	if(currentSearch.empty()) {
		return;
	}
	if(!sr.getToken().empty() /* NMDC searches have no token */ && sr.getToken() != token) {
		addDropped();
		return;
	}
	if(isHash) {
		if(sr.getType() != SearchResult::TYPE_FILE || TTHValue(Text::fromT(currentSearch[0])) != sr.getTTH()) {
			addDropped();
			return;
		}
	} else {
		for(auto& i: currentSearch) {
			if((*i.begin() != _T('-') && Util::findSubString(sr.getFile(), Text::fromT(i)) == tstring::npos) ||
				(*i.begin() == _T('-') && i.size() != 1 && Util::findSubString(sr.getFile(), Text::fromT(i.substr(1))) != tstring::npos))
			{
				addDropped();
				return;
			}
		}
	}

	// Drop results for already shared files
	if(hideShared && ShareManager::getInstance()->isTTHShared(sr.getTTH())) {
		addDropped();
		return;
	}

	// Reject results without free slots
	if(onlyFree && sr.getFreeSlots() < 1) {
		addDropped();
		return;
	}

	SearchInfo* parent = nullptr;
	SearchInfo* dupChild = nullptr;

	// Check previous search results for dupes
	for(auto& si2: searchResults) {
		if(si2.parent) {
			continue;
		}

		for(auto& psr2: si2.srs) {
			auto& sr2 = *psr2;

			bool sameUser = sr.getUser() == sr2.getUser();
			if(sameUser && sr.getFile() == sr2.getFile()) {
				return; // dupe
			}
			if(sr.getType() == SearchResult::TYPE_FILE && sr2.getType() == SearchResult::TYPE_FILE && sr.getTTH() == sr2.getTTH()) {
				if(sameUser || sr.getSize() != sr2.getSize()) {
					return; // dupe
				}

				/* found a parent for the new result! if this is the 2nd child, duplicate the
				previous result so it appears among this parent's children. */
				parent = &si2;
				if(si2.srs.size() == 1) {
					searchResults.emplace_back(psr2);
					dupChild = &searchResults.back();
					dupChild->parent = parent;
				}
				si2.srs.push_back(psr);
				si2.update();

				break;
			}
		}
		if(parent) {
			break;
		}
	}

	searchResults.emplace_back(psr);
	auto si = &searchResults.back();
	si->parent = parent;

	{
		HoldRedraw hold { results };

		if(dupChild) {
			addToList(dupChild);
		}
		addToList(si);

		/* single results get inserted at the right position; children, however, may alter their
		parent's sorting. */
		if(parent) {
			results->resort();
		}
	}

	updateStatusCount();
	setDirty(SettingsManager::BOLD_SEARCH);
}

void SearchFrame::addToList(SearchInfo* si) {
	auto i = results->find(si);
	if(filter.empty() || filter.match(filter.prepare(), [this, si](int column) { return Text::fromT(si->getText(column)); })) {
		if(i == -1) {
			if(si->parent) {
				results->insertChild(reinterpret_cast<LPARAM>(si->parent), reinterpret_cast<LPARAM>(si));
			} else {
				results->insert(si);
			}
		} else {
			results->update(i);
		}
	} else if(i != -1) {
		results->erase(i);
	}
}

void SearchFrame::updateList() {
	auto i = searchResults.begin();

	auto filterPrep = filter.prepare();
	auto filterInfoF = [this, &i](int column) { return Text::fromT(i->getText(column)); };

	HoldRedraw hold { results };
	results->clear();
	for(; i != searchResults.end(); ++i) {
		if(filter.empty() || filter.match(filterPrep, filterInfoF)) {
			if(i->parent) {
				results->insertChild(reinterpret_cast<LPARAM>(i->parent), reinterpret_cast<LPARAM>(&*i));
			} else {
				results->insert(&*i);
			}
		}
	}
	results->resort();

	updateStatusCount();
}

void SearchFrame::handlePurgeClicked() {
	searchBox->clear();
	lastSearches.clear();
}

LRESULT SearchFrame::handleHubItemChanged(WPARAM wParam, LPARAM lParam) {
	LPNMLISTVIEW lv = (LPNMLISTVIEW)lParam;

	if((lv->iItem == OPTION_ONLY_AS_OP) && (lv->uNewState ^ lv->uOldState) & LVIS_STATEIMAGEMASK) {
		if (((lv->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) {
			for(int iItem = 0; (iItem = hubs->getNext(iItem, LVNI_ALL)) != -1; ) {
				HubInfo* client = hubs->getData(iItem);
				if (!client->op)
					hubs->setChecked(iItem, false);
			}
		}

		// Treat the option as a radio button
		if(hubs->isChecked(OPTION_ONLY_AS_OP))
			hubs->setChecked(OPTION_ONLY_SECURE, false);
	}

	if((lv->iItem == OPTION_ONLY_SECURE) && (lv->uNewState ^ lv->uOldState) & LVIS_STATEIMAGEMASK) {
		if (((lv->uNewState & LVIS_STATEIMAGEMASK) >> 12) - 1) {
			for(int iItem = 0; (iItem = hubs->getNext(iItem, LVNI_ALL)) != -1; ) {
				HubInfo* client = hubs->getData(iItem);
				if (!client->secure)
					hubs->setChecked(iItem, false);
			}
		}

		// Treat the option as a radio button
		if(hubs->isChecked(OPTION_ONLY_SECURE))
			hubs->setChecked(OPTION_ONLY_AS_OP, false);
	}

	return 0;
}

bool SearchFrame::handleKeyDown(int c) {
	if(c == VK_DELETE) {
		handleRemove();
		return true;
	}
	return false;
}

bool SearchFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(results->countSelected() > 0) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = results->getContextMenuPos();
		}

		makeMenu()->open(pt);
		return true;
	}
	return false;
}

void SearchFrame::handleDownload() {
	treat(results->forEachSelectedT(SearchInfo::Download(Text::toT(SETTING(DOWNLOAD_DIRECTORY)))));
}

void SearchFrame::handleDownloadFavoriteDirs(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if(index < spl.size()) {
		treat(results->forEachSelectedT(SearchInfo::Download(Text::toT(spl[index].first))));
	} else {
		dcassert((index - spl.size()) < targets.size());
		treat(results->forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[index - spl.size()]))));
	}
}

void SearchFrame::handleDownloadTo() {
	if(results->countSelected() == 1) {
		int i = results->getNext(-1, LVNI_SELECTED);
		dcassert(i != -1);
		SearchInfo* si = results->getData(i);
		const SearchResultPtr& sr = si->srs[0];

		if(sr->getType() == SearchResult::TYPE_FILE) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + si->columns[COLUMN_FILENAME];
			if(WinUtil::browseSaveFile(this, target)) {
				WinUtil::addLastDir(Util::getFilePath(target));
				treat(results->forEachSelectedT(SearchInfo::DownloadTarget(target)));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(FolderDialog(this).open(target)) {
				WinUtil::addLastDir(target);
				treat(results->forEachSelectedT(SearchInfo::Download(target)));
			}
		}
	} else {
		tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
		if(FolderDialog(this).open(target)) {
			WinUtil::addLastDir(target);
			treat(results->forEachSelectedT(SearchInfo::Download(target)));
		}
	}
}

void SearchFrame::handleDownloadTarget(unsigned index) {
	if(index < WinUtil::lastDirs.size()) {
		treat(results->forEachSelectedT(SearchInfo::Download(WinUtil::lastDirs[index])));
	} else {
		dcassert((index - WinUtil::lastDirs.size()) < targets.size());
		treat(results->forEachSelectedT(SearchInfo::DownloadTarget(Text::toT(targets[index - WinUtil::lastDirs.size()]))));
	}
}

void SearchFrame::handleDownloadDir() {
	treat(results->forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(SETTING(DOWNLOAD_DIRECTORY)))));
}

void SearchFrame::handleDownloadWholeFavoriteDirs(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	dcassert(index < spl.size());
	treat(results->forEachSelectedT(SearchInfo::DownloadWhole(Text::toT(spl[index].first))));
}

void SearchFrame::handleDownloadWholeTarget(unsigned index) {
	dcassert(index < WinUtil::lastDirs.size());
	treat(results->forEachSelectedT(SearchInfo::DownloadWhole(WinUtil::lastDirs[index])));
}

void SearchFrame::handleDownloadDirTo() {
	tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
	if(FolderDialog(this).open(target)) {
		WinUtil::addLastDir(target);
		treat(results->forEachSelectedT(SearchInfo::DownloadWhole(target)));
	}
}

void SearchFrame::handleViewAsText() {
	try {
		results->forEachSelected(&SearchInfo::view);
	} catch(const Exception& e) {
		status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
}

void SearchFrame::handleRemove() {
	int i = -1;
	while((i = results->getNext(-1, LVNI_SELECTED)) != -1) {
		auto data = results->getData(i);
		results->erase(i);
		if(data) {
			searchResults.remove_if([data](const SearchInfo& si) { return &si == data || si.parent == data; });
		}
	}

	updateStatusCount();
}

struct UserCollector {
	template<typename T>
	void operator()(T* si) {
		for(const auto& sr: si->srs) {
			if(std::find(users.begin(), users.end(), sr->getUser()) == users.end()) {
				users.emplace_back(sr->getUser());
				dirs.push_back(Util::getFilePath(sr->getFile()));
			}
		}
	}
	HintedUserList users;
	StringList dirs;
};

MenuPtr SearchFrame::makeMenu() {
	auto menu = addChild(WinUtil::Seeds::menu);

	StringPairList favoriteDirs = FavoriteManager::getInstance()->getFavoriteDirs();
	SearchInfo::CheckTTH checkTTH = results->forEachSelectedT(SearchInfo::CheckTTH());

	menu->setTitle(checkTTH.hasTTH ? escapeMenu(checkTTH.name) : str(TF_("%1% results") % results->countSelected()),
		getParent()->getIcon(this));

	menu->appendItem(T_("&Download"), [this] { handleDownload(); }, WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargetMenu(menu.get(), favoriteDirs, checkTTH);
	menu->appendItem(T_("Download whole directory"), [this] { handleDownloadDir(); });
	addTargetDirMenu(menu.get(), favoriteDirs);
	menu->appendItem(T_("&View as text"), [this] { handleViewAsText(); });

	if(checkTTH.hasTTH) {
		menu->appendSeparator();
		WinUtil::addHashItems(menu.get(), checkTTH.tth, checkTTH.name, checkTTH.size);
	}

	menu->appendSeparator();
	UserCollector users = results->forEachSelectedT(UserCollector());
	WinUtil::addUserItems(menu.get(), users.users, getParent(), users.dirs);

	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), [this] { handleRemove(); });

	WinUtil::addCopyMenu(menu.get(), results);

	prepareMenu(menu.get(), UserCommand::CONTEXT_SEARCH, checkTTH.hubs);

	return menu;
}

void SearchFrame::addTargetMenu(Menu* menu, const StringPairList& favoriteDirs, const SearchInfo::CheckTTH& checkTTH) {
	auto sub = menu->appendPopup(T_("Download to..."));

	int n = 0;
	if(!favoriteDirs.empty()) {
		for(auto i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i, ++n)
			sub->appendItem(Text::toT(i->second), [=, this] { handleDownloadFavoriteDirs(n); });
		sub->appendSeparator();
	}

	n = 0;
	sub->appendItem(T_("&Browse..."), [this] { handleDownloadTo(); });
	if(!WinUtil::lastDirs.empty()) {
		sub->appendSeparator();
		for(auto i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i, ++n)
			sub->appendItem(*i, [=, this] { handleDownloadTarget(n); });
	}

	if(checkTTH.hasTTH) {
		targets = QueueManager::getInstance()->getTargets(checkTTH.tth);
		if(!targets.empty()) {
			sub->appendSeparator();
			for(auto i = targets.begin(); i != targets.end(); ++i, ++n)
				sub->appendItem(Text::toT(*i), [=, this] { handleDownloadTarget(n); });
		}
	}
}

void SearchFrame::addTargetDirMenu(Menu* menu, const StringPairList& favoriteDirs) {
	auto sub = menu->appendPopup(T_("Download whole directory to..."));

	int n = 0;
	if(!favoriteDirs.empty()) {
		for(auto i = favoriteDirs.begin(); i != favoriteDirs.end(); ++i, ++n)
			sub->appendItem(Text::toT(i->second), [=, this] { handleDownloadWholeFavoriteDirs(n); });
		sub->appendSeparator();
	}

	n = 0;
	sub->appendItem(T_("&Browse..."), [this] { handleDownloadDirTo(); });
	if(!WinUtil::lastDirs.empty()) {
		sub->appendSeparator();
		for(auto i = WinUtil::lastDirs.begin(); i != WinUtil::lastDirs.end(); ++i, ++n)
			sub->appendItem(*i, [=, this] { handleDownloadWholeTarget(n); });
	}
}

void SearchFrame::on(SearchManagerListener::SR, const SearchResultPtr& sr) noexcept {
	callAsync([this, sr] { addResult(sr); });
}

void SearchFrame::onHubAdded(HubInfo* info, bool defaultHubState) {
	int nItem = hubs->insert(info);

	if (hubs->isChecked(OPTION_ONLY_AS_OP) && hubs->isChecked(OPTION_ONLY_SECURE))
		hubs->setChecked(nItem, info->op && info->secure);
	else if(hubs->isChecked(OPTION_ONLY_AS_OP))
		hubs->setChecked(nItem, info->op);
	else if(hubs->isChecked(OPTION_ONLY_SECURE))
		hubs->setChecked(nItem, info->secure);
	else
		hubs->setChecked(nItem, defaultHubState);

	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubChanged(HubInfo* info) {
	int nItem = 0;
	int n = hubs->size();
	for(; nItem < n; nItem++) {
		if(hubs->getData(nItem)->url == info->url)
			break;
	}
	if (nItem == n)
		return;

	hubs->setData(nItem, info);
	hubs->update(nItem);

	if (hubs->isChecked(OPTION_ONLY_AS_OP) && hubs->isChecked(OPTION_ONLY_SECURE))
		hubs->setChecked(nItem, info->op && info->secure);
	else if(hubs->isChecked(OPTION_ONLY_AS_OP))
		hubs->setChecked(nItem, info->op);
	else if(hubs->isChecked(OPTION_ONLY_SECURE))
		hubs->setChecked(nItem, info->secure);

	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::onHubRemoved(HubInfo* info) {
	int nItem = 0;
	int n = hubs->size();
	for(; nItem < n; nItem++) {
		if(hubs->getData(nItem)->url == info->url)
			break;
	}
	if (nItem == n)
		return;

	hubs->erase(nItem);
	hubs->setColumnWidth(0, LVSCW_AUTOSIZE);
}

void SearchFrame::runSearch() {
	// change settings if changed
	if(onlyFree != SETTING(SEARCH_ONLY_FREE_SLOTS))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_ONLY_FREE_SLOTS, onlyFree);
	if(hideShared != SETTING(SEARCH_FILTER_SHARED))
		SettingsManager::getInstance()->set(SettingsManager::SEARCH_FILTER_SHARED, hideShared);
	if(initialType == SearchManager::TYPE_ANY) {
		string text = Text::fromT(fileType->getText());
		if(text != SETTING(LAST_SEARCH_TYPE))
			SettingsManager::getInstance()->set(SettingsManager::LAST_SEARCH_TYPE, text);
	}

	auto s = searchBox->getText();
	if(s.empty())
		return;

	int ftype = fileType->getData(fileType->getSelected());
	if(ftype == SearchManager::TYPE_TTH) {
		auto s8 = Text::fromT(s);
		s8.erase(std::remove_if(s8.begin(), s8.end(), [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }), s8.end());
		if(s8.size() != 39 || !Encoder::isBase32(s8)) {
			auto text = T_("Invalid input value for TTH search");
			status->setText(STATUS_STATUS, text);
			setText(str(TF_("Search - %1%") % text));
			return;
		}
		s = Text::toT(s8);
	}

	StringList clients;
	for(size_t i = 0, n = hubs->size(); i < n; ++i) {
		if(hubs->isChecked(i)) {
			clients.push_back(Text::fromT(hubs->getData(i)->url));
		}
	}
	if(clients.empty())
		return;

	auto lsize = Util::toDouble(Text::fromT(size->getText()));
	switch(sizeMode->getSelected()) {
	case 1: lsize *= 1024.0; break;
	case 2: lsize *= 1024.0 * 1024.0; break;
	case 3: lsize *= 1024.0 * 1024.0 * 1024.0; break;
	}
	auto llsize = static_cast<int64_t>(lsize);

	results->clear();
	searchResults.clear();

	// Add new searches to the last-search dropdown list
	if(find(lastSearches.begin(), lastSearches.end(), s) == lastSearches.end())
	{
		size_t i = max(SETTING(SEARCH_HISTORY)-1, 0);

		if(searchBox->size() > i)
			searchBox->erase(i);
		searchBox->insertValue(0, s);

		while(lastSearches.size() > i) {
			lastSearches.erase(lastSearches.begin());
		}
		lastSearches.push_back(s);
	}

	currentSearch = StringTokenizer<tstring>(s, ' ').getTokens();
	s.clear();
	//strip out terms beginning with -
	for(auto si = currentSearch.begin(); si != currentSearch.end();) {
		if(si->empty()) {
			si = currentSearch.erase(si);
			continue;
		}
		if((*si)[0] != _T('-'))
			s += *si + _T(' ');
		++si;
	}

	s = s.substr(0, max(s.size(), static_cast<tstring::size_type>(1)) - 1);
	token = std::to_string(Util::rand());

	SearchManager::SizeModes searchMode((SearchManager::SizeModes)mode->getSelected());
	if(llsize == 0)
		searchMode = SearchManager::SIZE_DONTCARE;

	// Get ADC searchtype extensions if any is selected
	StringList extList;
	try {
		if(ftype == SearchManager::TYPE_ANY) {
			// Custom searchtype
			extList = SettingsManager::getInstance()->getExtensions(Text::fromT(fileType->getText()));
		} else if(ftype > SearchManager::TYPE_ANY && ftype < SearchManager::TYPE_DIRECTORY) {
			// Predefined searchtype
			extList = SettingsManager::getInstance()->getExtensions(string(1, '0' + ftype));
		}
	} catch(const SearchTypeException&) {
		ftype = SearchManager::TYPE_ANY;
	}

	status->setText(STATUS_STATUS, str(TF_("Searching for %1%...") % s));
	status->setText(STATUS_COUNT, Util::emptyStringT);
	status->setText(STATUS_DROPPED, Util::emptyStringT);
	droppedResults = 0;
	isHash = (ftype == SearchManager::TYPE_TTH);

	setText(str(TF_("Search - %1%") % s));

	if(SearchManager::getInstance()->okToSearch()) {
		SearchManager::getInstance()->search(clients, Text::fromT(s), llsize,
			(SearchManager::TypeModes)ftype, searchMode, token, extList);
		if(SETTING(CLEAR_SEARCH)) // Only clear if the search was sent
			searchBox->setText(Util::emptyStringT);

	} else {
		auto waitFor = SearchManager::getInstance()->timeToSearch();
		tstring msg = str(TFN_("Searching too soon, next search in %1% second", "Searching too soon, next search in %1% seconds", waitFor) % waitFor);
		status->setText(STATUS_STATUS, msg);
		setText(str(TF_("Search - %1%") % msg));
		// Start the countdown timer
		initSecond();
	}
}

void SearchFrame::updateStatusCount() {
	auto current = results->size();
	auto total = searchResults.size();
	if(current >= total) {
		status->setText(STATUS_COUNT, str(TFN_("%1% item", "%1% items", current) % current));
	} else {
		status->setText(STATUS_COUNT, str(TFN_("%1% / %2% item", "%1% / %2% items", current) % current % total));
	}
}

void SearchFrame::addDropped() {
	++droppedResults;
	status->setText(STATUS_DROPPED, str(TF_("%1% dropped") % droppedResults));
}

void SearchFrame::initSecond() {
	setTimer([this] { return eachSecond(); }, 1000);
}

bool SearchFrame::eachSecond() {
	int32_t waitFor = SearchManager::getInstance()->timeToSearch();
	if(waitFor > 0) {
		tstring msg = str(TFN_("Searching too soon, next search in %1% second", "Searching too soon, next search in %1% seconds", waitFor) % waitFor);
		status->setText(STATUS_STATUS, msg);
		setText(str(TF_("Search - %1%") % msg));
		return true;
	}

	status->setText(STATUS_STATUS, T_("Ready to search..."));
	setText(T_("Search - Ready to search..."));

	return false;
}

void SearchFrame::treat(const SearchInfo::Download& dl) {
	if(dl.total) {
		status->setText(STATUS_STATUS,
			dl.ignored ? str(TF_("%1% / %2% sources ignored: %3%") % dl.ignored % dl.total % Text::toT(dl.error))
			: str(TF_("%1% sources added") % dl.total));
	}
}

void SearchFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	set<CID> users;

	int sel = -1;
	while((sel = results->getNext(sel, LVNI_SELECTED)) != -1) {
		auto si = results->getData(sel);
		for(auto sr: si->srs) {

			if(!sr->getUser().user->isOnline())
				continue;

			if(uc.once()) {
				if(users.find(sr->getUser().user->getCID()) != users.end())
					continue;
				users.insert(sr->getUser().user->getCID());
			}

			ucParams["fileFN"] = [sr] { return sr->getFile(); };
			ucParams["fileSI"] = [sr] { return std::to_string(sr->getSize()); };
			ucParams["fileSIshort"] = [sr] { return Util::formatBytes(sr->getSize()); };
			if(sr->getType() == SearchResult::TYPE_FILE) {
				ucParams["fileTR"] = [sr] { return sr->getTTH().toBase32(); };
			}
			ucParams["fileMN"] = [sr] { return WinUtil::makeMagnet(sr->getTTH(), sr->getFile(), sr->getSize()); };

			// compatibility with 0.674 and earlier
			ucParams["file"] = ucParams["fileFN"];
			ucParams["filesize"] = ucParams["fileSI"];
			ucParams["filesizeshort"] = ucParams["fileSIshort"];
			ucParams["tth"] = ucParams["fileTR"];

			auto tmp = ucParams;
			ClientManager::getInstance()->userCommand(sr->getUser(), uc, tmp, true);
		}
	}
}

bool SearchFrame::handleSearchKeyDown(int c) {
	if(c == VK_RETURN && !(WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt())) {
		runSearch();
		return true;
	}
	return false;
}

bool SearchFrame::handleSearchChar(int c) {
	// avoid the "beep" sound when enter is pressed
	return c == VK_RETURN;
}

void SearchFrame::on(ClientConnected, Client* c) noexcept {
	auto hi = new HubInfo(c);
	callAsync([=, this] { onHubAdded(hi); });
}

void SearchFrame::on(ClientUpdated, Client* c) noexcept {
	auto hi = new HubInfo(c);
	callAsync([=, this] { onHubChanged(hi); });
}

void SearchFrame::on(ClientDisconnected, Client* c) noexcept {
	auto hi = new HubInfo(c);
	callAsync([=, this] { onHubRemoved(hi); });
}

void SearchFrame::on(SettingsManagerListener::SearchTypesChanged) noexcept {
	callAsync([this] { searchTypesChanged(); });
}
