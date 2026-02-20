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
#include "DirectoryListingFrame.h"

#include <atomic>
#include <boost/range/adaptor/reversed.hpp>

#include <dcpp/ADLSearch.h>
#include <dcpp/ClientManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/File.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/ScopedFunctor.h>
#include <dcpp/Thread.h>
#include <dcpp/WindowInfo.h>

#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Rebar.h>
#include <dwt/widgets/SaveDialog.h>
#include <dwt/widgets/SplitterContainer.h>
#include <dwt/widgets/ToolBar.h>
#include <dwt/widgets/VirtualTree.h>

#include "TypedTable.h"
#include "TypedTree.h"
#include "ShellMenu.h"
#include "MainWindow.h"
#include "TextFrame.h"
#include "HoldRedraw.h"

#include "resource.h"

using dwt::FolderDialog;
using dwt::Grid;
using dwt::GridInfo;
using dwt::SplitterContainer;
using dwt::Rebar;
using dwt::SaveDialog;
using dwt::ToolBar;

const string DirectoryListingFrame::id = "DirectoryListing";
const string& DirectoryListingFrame::getId() const { return id; }

static const ColumnInfo filesColumns[] = {
	{ N_("File"), 300, false },
	{ N_("Type"), 60, false },
	{ N_("Size"), 80, true },
	{ N_("Exact size"), 100, true },
	{ N_("TTH"), 300, false }
};

DirectoryListingFrame::UserMap DirectoryListingFrame::lists;

DirectoryListingFrame::LastSearches DirectoryListingFrame::lastSearches;

int DirectoryListingFrame::ItemInfo::getImage(int col) const {
	if(col != 0) {
		return -1;
	}

	if(type == DIRECTORY || type == USER) {
		return dir->getComplete() ? WinUtil::DIR_ICON : WinUtil::DIR_ICON_INCOMPLETE;
	}

	return WinUtil::getFileIcon(getText(COLUMN_FILENAME));
}

int DirectoryListingFrame::ItemInfo::compareItems(const ItemInfo* a, const ItemInfo* b, int col) {
	if(a->type == DIRECTORY) {
		if(b->type == DIRECTORY) {
			switch(col) {
			case COLUMN_EXACTSIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
			case COLUMN_SIZE: return compare(a->dir->getTotalSize(), b->dir->getTotalSize());
			default: return compare(a->columns[col], b->columns[col]);
			}
		} else {
			return -1;
		}
	} else if(b->type == DIRECTORY) {
		return 1;
	} else {
		switch(col) {
		case COLUMN_EXACTSIZE: return compare(a->file->getSize(), b->file->getSize());
		case COLUMN_SIZE: return compare(a->file->getSize(), b->file->getSize());
		default: return compare(a->columns[col], b->columns[col]);
		}
	}
}

void DirectoryListingFrame::openWindow(TabViewPtr parent, const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed, Activation activate) {
	auto prev = lists.find(aUser);
	if(prev == lists.end()) {
		openWindow_(parent, aFile, aDir, aUser, aSpeed, activate);
	} else {
		if(activate != FORCE_ACTIVE && prev->second->isActive())
			activate = FORCE_ACTIVE;
		prev->second->close();
		parent->callAsync([=] { openWindow_(parent, aFile, aDir, aUser, aSpeed, activate); });
	}
}

void DirectoryListingFrame::openWindow_(TabViewPtr parent, const tstring& aFile, const tstring& aDir, const HintedUser& aUser, int64_t aSpeed, Activation activate) {
	DirectoryListingFrame* frame = new DirectoryListingFrame(parent, aUser, aSpeed);

	/* save the path now in case the tab is closed without having been loaded (the file list will
	only be loaded upon tab activation), to still have it in WindowManager. */
	auto& path = frame->path;
	path = Text::fromT(aFile);
	auto n = path.size();
	if(n > 4 && Util::stricmp(path.substr(n - 4), ".bz2") == 0) {
		// strip the .bz2 ext - the file list loader will now where to find the file list anyway.
		path.erase(n - 4);
	}

	if(activate == FORCE_ACTIVE || (activate == FOLLOW_SETTING && !SETTING(POPUNDER_FILELIST))) {
		frame->loadFile(aDir);
		frame->activate();
	} else {
		frame->setDirty(SettingsManager::BOLD_FL);
		frame->onVisibilityChanged([frame, aDir](bool b) {
			if(b && !frame->loaded && !frame->loading && !WinUtil::mainWindow->closing())
				frame->loadFile(aDir);
		});
	}
}

void DirectoryListingFrame::openOwnList(TabViewPtr parent, const tstring& dir, Activation activate) {
	openWindow(parent, Text::toT(ShareManager::getInstance()->getOwnListFile()), dir,
		HintedUser(ClientManager::getInstance()->getMe(), Util::emptyString), 0, activate);
}

void DirectoryListingFrame::closeAll() {
	for(auto& i: lists)
		i.second->close(true);
}

WindowParams DirectoryListingFrame::getWindowParams() const {
	WindowParams ret;
	addRecentParams(ret);
	ret["CID"] = WindowParam(dl->getUser().user->getCID().toBase32(), WindowParam::FLAG_CID);
	ret["FileList"] = WindowParam((dl->getUser() == ClientManager::getInstance()->getMe()) ? "" : path,
		WindowParam::FLAG_IDENTIFIES | WindowParam::FLAG_FILELIST);
	ItemInfo* ii = dirs->getSelectedData();
	if(ii && ii->type == ItemInfo::DIRECTORY)
		ret["Directory"] = WindowParam(dl->getPath(ii->dir));
	ret["Hub"] = WindowParam(dl->getUser().hint);
	ret["Speed"] = WindowParam(std::to_string(speed));
	return ret;
}

void DirectoryListingFrame::parseWindowParams(TabViewPtr parent, const WindowParams& params) {
	auto path = params.find("FileList");
	auto dir = params.find("Directory");
	auto hub = params.find("Hub");
	auto speed = params.find("Speed");
	if(path != params.end() && speed != params.end()) {
		Activation activate = parseActivateParam(params) ? FORCE_ACTIVE : FORCE_INACTIVE;
		if(path->second.empty()) {
			openOwnList(parent, (dir == params.end()) ? Util::emptyStringT : Text::toT(dir->second), activate);
		} else if(File::getSize(path->second) != -1) {
			UserPtr u = DirectoryListing::getUserFromFilename(path->second);
			if(u) {
				openWindow(parent, Text::toT(path->second),
					(dir == params.end()) ? Util::emptyStringT : Text::toT(dir->second),
					HintedUser(u, (hub == params.end()) ? Util::emptyString : hub->second.content),
					Util::toInt64(speed->second), activate);
			}
		}
	}
}

bool DirectoryListingFrame::isFavorite(const WindowParams& params) {
	auto path = params.find("FileList");
	if(path != params.end() && !(path->second == "OwnList") && File::getSize(path->second) != -1) {
		UserPtr u = DirectoryListing::getUserFromFilename(path->second);
		if(u)
			return FavoriteManager::getInstance()->isFavoriteUser(u);
	}
	return false;
}

void DirectoryListingFrame::openWindow(TabViewPtr parent, const HintedUser& aUser, const string& txt, int64_t aSpeed) {
	auto i = lists.find(aUser);
	if(i != lists.end()) {
		i->second->speed = aSpeed;
		i->second->loadXML(txt);
		i->second->setDirty(SettingsManager::BOLD_FL);
	} else {
		DirectoryListingFrame* frame = new DirectoryListingFrame(parent, aUser, aSpeed);
		frame->loadXML(txt);
		if(SETTING(POPUNDER_FILELIST))
			frame->setDirty(SettingsManager::BOLD_FL);
		else
			frame->activate();
	}
}

void DirectoryListingFrame::activateWindow(const HintedUser& aUser) {
	auto i = lists.find(aUser);
	if(i != lists.end()) {
		i->second->activate();
	}
}

DirectoryListingFrame::DirectoryListingFrame(TabViewPtr parent, const HintedUser& aUser, int64_t aSpeed) :
	BaseType(parent, _T(""), IDH_FILE_LIST, IDI_DIRECTORY, false),
	loader(nullptr),
	loading(0),
	rebar(0),
	pathBox(0),
	grid(0),
	searchGrid(0),
	searchBox(0),
	filterMethod(0),
	paned(0),
	dirs(0),
	files(0),
	speed(aSpeed),
	dl(new DirectoryListing(aUser)),
	user(aUser),
	usingDirMenu(false),
	historyIndex(0),
	treeRoot(nullptr),
	curDir(nullptr),
	loaded(false),
	updating(false),
	searching(false)
{
	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).size = 0;
	grid->row(0).mode = GridInfo::STATIC;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;
	grid->setSpacing(0);

	{
		searchGrid = grid->addChild(Grid::Seed(1, 4));
		searchGrid->column(0).mode = GridInfo::FILL;

		searchBox = searchGrid->addChild(WinUtil::Seeds::comboBoxEdit);
		searchBox->setHelpId(IDH_FILE_LIST_SEARCH_BOX);
		searchBox->getTextBox()->setCue(T_("Find files"));
		addWidget(searchBox);
		searchBox->getTextBox()->onKeyDown([this](int c) { return handleSearchKeyDown(c); });
		searchBox->getTextBox()->onChar([this] (int c) { return handleSearchChar(c); });

		searchBox->getTextBox()->addRemoveStyle(WS_CLIPCHILDREN, true);
		WinUtil::addSearchIcon(searchBox->getTextBox());

		filterMethod = searchGrid->addChild(WinUtil::Seeds::comboBox);
		filterMethod->setHelpId(IDH_FILE_LIST_SEARCH_BOX);
		addWidget(filterMethod);

		WinUtil::addFilterMethods(filterMethod);
		filterMethod->setSelected(StringMatch::PARTIAL);

		searchBox->onSelectionChanged([this] { handleSearchSelChanged(); });

		auto seed = WinUtil::Seeds::button;
		seed.caption = T_("Find previous");
		ButtonPtr button = searchGrid->addChild(seed);
		button->setHelpId(IDH_FILE_LIST_FIND_PREV);
		button->setImage(WinUtil::buttonIcon(IDI_LEFT));
		button->onClicked([this] { handleFind(true); });
		addWidget(button);

		seed.caption = T_("Find next");
		seed.style |= BS_DEFPUSHBUTTON;
		button = searchGrid->addChild(seed);
		button->setHelpId(IDH_FILE_LIST_FIND_NEXT);
		button->setImage(WinUtil::buttonIcon(IDI_RIGHT));
		button->onClicked([this] { handleFind(false); });
		addWidget(button);
	}

	searchGrid->setEnabled(false);
	searchGrid->setVisible(false);

	paned = grid->addChild(SplitterContainer::Seed(SETTING(FILE_LIST_PANED_POS)));

	{
		dirs = paned->addChild(WidgetDirs::Seed(WinUtil::Seeds::treeView));
		dirs->setHelpId(IDH_FILE_LIST_DIRS);
		addWidget(dirs);

		dirs->setNormalImageList(WinUtil::fileImages);
		dirs->onSelectionChanged([this] { handleSelectionChanged(); });
		dirs->onKeyDown([this](int c) { return handleKeyDownDirs(c); });
		dirs->onSysKeyDown([this](int c) { return handleKeyDownDirs(c); });
		dirs->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleDirsContextMenu(sc); });
		dirs->onXMouseUp([this](const dwt::MouseEvent &me) { return handleXMouseUp(me); });

		files = paned->addChild(WidgetFiles::Seed(WinUtil::Seeds::table));
		files->setHelpId(IDH_FILE_LIST_FILES);
		addWidget(files);

		files->setSmallImageList(WinUtil::fileImages);

		WinUtil::makeColumns(files, filesColumns, COLUMN_LAST, SETTING(DIRECTORYLISTINGFRAME_ORDER), SETTING(DIRECTORYLISTINGFRAME_WIDTHS));
		WinUtil::setTableSort(files, COLUMN_LAST, SettingsManager::DIRECTORYLISTINGFRAME_SORT, COLUMN_FILENAME);

		files->onDblClicked([this] { handleDoubleClickFiles(); });
		files->onKeyDown([this](int c) { return handleKeyDownFiles(c); });
		files->onSysKeyDown([this](int c) { return handleKeyDownFiles(c); });
		files->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleFilesContextMenu(sc); });
		files->onXMouseUp([this](const dwt::MouseEvent &me) { return handleXMouseUp(me); });
	}

	{
		// create the rebar after the rest to make sure it doesn't grab the default focus.
		rebar = addChild(Rebar::Seed());

		// create the first toolbar (on the left of the address bar).
		auto seed = ToolBar::Seed();
		seed.style &= ~CCS_ADJUSTABLE;
		ToolBarPtr toolbar = addChild(seed);

		StringList ids;
		auto addButton = [&toolbar, &ids](unsigned icon, const tstring& text, bool showText,
			unsigned helpId, const dwt::Dispatchers::VoidVoid<>::F& f)
		{
			ids.emplace_back(1, '0' + ids.size());
			toolbar->addButton(ids.back(), icon ? WinUtil::toolbarIcon(icon) : 0, 0, text, showText, helpId, f);
		};

		addButton(IDI_LEFT, T_("Back"), false, IDH_FILE_LIST_BACK, [this] { back(); });
		addButton(IDI_RIGHT, T_("Forward"), false, IDH_FILE_LIST_FORWARD, [this] { this->forward(); }); // explicit ns (vs std::forward)
		ids.emplace_back();
		addButton(IDI_UP, T_("Up one level"), false, IDH_FILE_LIST_UP, [this] { up(); });
		toolbar->setLayout(ids);

		rebar->add(toolbar, RBBS_NOGRIPPER);

		// create the address bar.
		pathBox = addChild(WinUtil::Seeds::comboBoxEdit);
		pathBox->getTextBox()->setReadOnly();
		addWidget(pathBox);
		pathBox->onSelectionChanged([this] { selectItem(Text::toT(history[pathBox->getSelected()])); });

		rebar->add(pathBox, RBBS_NOGRIPPER);
		rebar->sendMessage(RB_MAXIMIZEBAND, 1); // the address bar will occupy all the space it can.

		// create the second toolbar (on the right of the address bar).
		toolbar = addChild(seed);

		ids.clear();
		addButton(0, T_("Subtract list"), true, IDH_FILE_LIST_SUBSTRACT, [this] { handleListDiff(); });
		addButton(0, T_("Match queue"), true, IDH_FILE_LIST_MATCH_QUEUE, [this] { handleMatchQueue(); });
		addButton(0, T_("Download full list"), true, IDH_FILE_LIST_DOWNLOAD_FULL, [this] { user.getList(); });
		ids.push_back("Find");
		auto findId = ids.back();
		toolbar->addButton(findId, WinUtil::toolbarIcon(IDI_SEARCH), 0, T_("Find"), true, IDH_FILE_LIST_FIND,
			nullptr, [this](const dwt::ScreenCoordinate&) { handleFindToggle(); });
		toolbar->setLayout(ids);

		rebar->add(toolbar, RBBS_NOGRIPPER);

		searchGrid->onEnabled([toolbar, findId](bool b) { toolbar->setButtonChecked(findId, b); });
	}

	initStatus();

	{
		auto showTree = addChild(WinUtil::Seeds::splitCheckBox);
		showTree->setChecked(true);
		showTree->onClicked([this, showTree] {
			auto checked = showTree->getChecked();
			dirs->setEnabled(checked);
			paned->maximize(checked ? nullptr : files);
		});
		status->setWidget(STATUS_SHOW_TREE, showTree);
	}

	setTimer([this]() -> bool { updateStatus(); return true; }, 500);

	dirs->setFocus();

	curDir = dl->getRoot();
	treeRoot = dirs->insert(new ItemInfo(true, curDir), nullptr);

	ClientManager::getInstance()->addListener(this);
	updateTitle();

	addAccel(FCONTROL, 'F', [this] { if(searchGrid->getEnabled()) searchBox->setFocus(); else handleFindToggle(); });
	addAccel(0, VK_F3, [this] { if(searchGrid->getEnabled()) handleFind(false); else handleFindToggle(); });
	initAccels();

	layout();

	lists.emplace(aUser, this);
}

DirectoryListingFrame::~DirectoryListingFrame() {
}

namespace {
	/* items cached by all open file lists. caching can take up a lot of memory so we use this
	counter to keep tabs on the caches and make sure they don't grow too big. */
	std::atomic<uint32_t> cacheCount(0);

	/* minimum amount of items to require a cache. this helps skip directories that don't have many
	files: file information for small directories is easy to build up on-the-fly without inducing
	any GUI freeze, so we don't cache these small directories. this value can be increased as long
	as displaying a directory with cacheLowerBound files doesn't freeze the GUI for too long. */
	const uint32_t cacheLowerBound = 1024;

	/* maximum amount of items all file list caches can contain. we aim for 500MB max on x86 and
	1GB max on x64, with the assumption that one cached item occupies around 1KB. */
#ifdef _WIN64
	const uint32_t maxCacheCount = 1024 * 1024;
#else
	const uint32_t maxCacheCount = 512 * 1024;
#endif

	template<typename T> bool canCache(T items) {
		return items > cacheLowerBound && cacheCount + static_cast<uint32_t>(items) < maxCacheCount;
	}
}

class FileListLoader : public Thread {
	typedef std::function<void ()> SuccessF;
	typedef std::function<void (tstring)> ErrorF;
	typedef std::function<void ()> EndF;

public:
	FileListLoader(DirectoryListingFrame& parent, SuccessF successF, ErrorF errorF, EndF endF) :
	parent(parent),
	successF(successF),
	errorF(errorF),
	endF(endF)
	{
	}

#ifdef _DEBUG
#define step(x) dcdebug("Loading file list <%s>: " x "\n", parent.path.c_str())
#else
#define step(x)
#endif

	int run() {
		/* load the file list; prepare the directory cache in order to have the directory tree
		ready by the time the file list is displayed. no need to lock the mutex at this point
		because it is guaranteed that the file list window won't try to read the directory cache
		until this process is over. */
		try {
			step("parsing XML & building structures");
			parent.dl->loadFile(parent.path);

			step("ADLS");
			ADLSearchManager::getInstance()->matchListing(*parent.dl);

			step("caching dirs");
			cacheDirs(parent.dl->getRoot());

			step("dir cache done; displaying");
			successF();

		} catch(const Exception& e) {
			step("error");
			errorF(Text::toT(e.getError()));
		}

		/* now that the file list is being displayed, prepare individual file items, hoping that
		they will have been processed by the time the user wants them to be displayed. */
		try {
			step("caching files");
			cacheFiles(parent.dl->getRoot());
		} catch(const Exception&) { }

		step("file cache done; destroying thread");
		endF();
		return 0;
	}

	/* if the loader is still running (it hasn't finished processing all the files in the list),
	this function can be used to request that the loader updates the cache of the specified
	directory. */
	bool updateCache(DirectoryListing::Directory* d) {
		step("getting a dir cache");
		Lock l(cs);
		auto i = cache.find(d);
		if(i != cache.end()) {
			step("dir cache found");
			parent.fileCache.emplace(d, move(i->second));
			cache.erase(i);
			return true;
		}
		step("dir cache not found");
		return false;
	}

	/* synchronize the parent window cache with the generated cache. no need to lock because the
	thread has already been destroyed at this point. */
	~FileListLoader() {
		step("updating parent cache");
		for(auto& i: cache) {
			if(parent.fileCache.find(i.first) == parent.fileCache.end()) {
				parent.fileCache.emplace(i.first, move(i.second));
			} else {
				cacheCount -= i.second.size();
			}
		}
	}

private:
	DirectoryListingFrame& parent;
	SuccessF successF;
	ErrorF errorF;
	EndF endF;

	unordered_map<DirectoryListing::Directory*, list<DirectoryListingFrame::ItemInfo>> cache;
	CriticalSection cs;

	void cacheDirs(DirectoryListing::Directory* d) {
		if(parent.dl->getAbort()) { throw Exception(); }

		for(auto i: d->directories) {
			++cacheCount;
			parent.dirCache.emplace(i, i);
			cacheDirs(i);
		}
	}

	void cacheFiles(DirectoryListing::Directory* d) {
		if(parent.dl->getAbort()) { throw Exception(); }

		const auto count = d->files.size();
		if(canCache(count)) {
			cacheCount += count;

			/* the directory may contain a lot of files, so we first create a cache for the whole
			directory then move it all later to the global cache. */
			list<DirectoryListingFrame::ItemInfo> files;
			for(auto i: d->files) {
				files.emplace_back(i);
			}
			{
				Lock l(cs);
				cache.emplace(d, move(files));
			}
		}

		// process sub-directories.
		for(auto i: d->directories) {
			cacheFiles(i);
		}
	}
};

void DirectoryListingFrame::loadFile(const tstring& dir) {
	setEnabled(false);
	status->setVisible(false);

	{
		// this control will cover the whole window.
		Label::Seed seed(WinUtil::createIcon(IDI_OPEN_FILE_LIST, 32));
		seed.style |= SS_CENTERIMAGE;
		loading = addChild(seed);
		WinUtil::setColor(loading);
		layout();
	}

	// add a "loading" message to the title bar.
	updateTitle();

	auto finishLoad = [this] {
		status->setVisible(true);
		setEnabled(true);

		loading->setVisible(false);
		loading->close(true);
		loading = 0;

		files->setFocus();

		layout();
		updateTitle();
		if(!error.empty()) {
			status->setText(STATUS_STATUS, error);
		}
		setDirty(SettingsManager::BOLD_FL);
	};

	loader = new FileListLoader(*this, [this, dir, finishLoad] { callAsync([=, this] {
		// success callback
		loaded = true;
		finishLoad();
		addRecent();
		refreshTree(dir);
		initStatusText();
	}); }, [this, finishLoad](tstring s) { callAsync([=, this] {
		// error callback
		error = std::move(s);
		finishLoad();
	}); }, [this] { callAsync([=, this] {
		// ending callback
		delete loader;
		loader = nullptr;
	}); });

	try {
		loader->start();

	} catch(const ThreadException& e) {
		error = Text::toT(e.getError());
		finishLoad();
		delete loader;
		loader = nullptr;
	}
}

void DirectoryListingFrame::loadXML(const string& txt) {
	try {
		auto sel = getSelectedDir();

		path = QueueManager::getInstance()->getListPath(dl->getUser()) + ".xml";

		if(!loaded && File::getSize(path) != -1) {
			// load the cached list.
			dl->updateXML(File(path, File::READ, File::OPEN).read());

			// mark all dirs as incomplete (the list we have loaded may be old).
			dl->setComplete(false);
		}

		auto base = dl->updateXML(txt);
		dl->save(path);

		// remove previous ADLS matches.
		for(auto dir = dirs->getChild(treeRoot); dir;) {
			auto d = dirs->getData(dir)->dir;
			if(d->getAdls()) {
				HTREEITEM child;
				while((child = dirs->getChild(dir))) {
					dirs->erase(child);
				}
				dirs->erase(dir);
				dir = dirs->getChild(treeRoot);
				d->getParent()->directories.erase(d);
				delete d;
			} else {
				dir = dirs->getNextSibling(dir);
			}
		}
		ADLSearchManager::getInstance()->matchListing(*dl);

		loaded = true;
		addRecent();

		HTREEITEM adlsPos = TVI_FIRST;
		for(auto d: dl->getRoot()->directories) {
			if(d->getAdls()) {
				adlsPos = addDir(d, treeRoot, adlsPos);
			}
		}
		refreshTree(Text::toT(Util::toNmdcFile(base)));

		if(!sel.empty()) {
			selectItem(Text::toT(sel));
		}

		status->setText(STATUS_STATUS, T_("Partial file list loaded"));

	} catch(const Exception& e) {
		error = Text::toT(e.getError());
		updateTitle();
	}

	initStatusText();
}

void DirectoryListingFrame::layout() {
	dwt::Rectangle r { getClientSize() };

	if(loading) {
		loading->setZOrder(HWND_TOPMOST);
		loading->resize(r);
		loading->redraw(true);
		redraw(true);
		return;
	}

	auto y = rebar->refresh();
	r.pos.y += y;
	r.size.y -= y;

	r.size.y -= status->refresh();

	grid->resize(r);
}

bool DirectoryListingFrame::preClosing() {
	ClientManager::getInstance()->removeListener(this);

	lists.erase(dl->getUser());

	if(loader) {
		dl->setAbort(true);
		loader->join();
	}

	return true;
}

void DirectoryListingFrame::postClosing() {
	clearFiles();

	delete dirs->getData(treeRoot);

	cacheCount -= dirCache.size();
	for(auto& i: fileCache) { cacheCount -= i.second.size(); }

	SettingsManager::getInstance()->set(SettingsManager::FILE_LIST_PANED_POS, paned->getSplitterPos(0));

	SettingsManager::getInstance()->set(SettingsManager::DIRECTORYLISTINGFRAME_ORDER, WinUtil::toString(files->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::DIRECTORYLISTINGFRAME_WIDTHS, WinUtil::toString(files->getColumnWidths()));
	SettingsManager::getInstance()->set(SettingsManager::DIRECTORYLISTINGFRAME_SORT, WinUtil::getTableSort(files));
}

void DirectoryListingFrame::handleSearchSelChanged() {
	auto p = reinterpret_cast<LastSearchPair*>(searchBox->getData(searchBox->getSelected()));
	if(p) {
		filterMethod->setSelected(p->second);
	}
}

void DirectoryListingFrame::handleFind(bool reverse) {
	searching = true;
	findFile(reverse);
	searching = false;
	updateStatus();
}

void DirectoryListingFrame::handleMatchQueue() {
	int matched = QueueManager::getInstance()->matchListing(*dl);
	status->setText(STATUS_STATUS, str(TFN_("Matched %1% file", "Matched %1% files", matched) % matched));
}

void DirectoryListingFrame::UserHolder::getList() {
	// imitate UserInfoBase::getList but keep the current dir and dl a full list.
	auto w = lists[user];
	try {
		QueueManager::getInstance()->addList(user, QueueItem::FLAG_CLIENT_VIEW, w->getSelectedDir());
	} catch(const Exception& e) {
		w->status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::UserHolder::matchQueue() {
	lists[user]->handleMatchQueue();
}

void DirectoryListingFrame::handleListDiff() {
	tstring file;
	if(WinUtil::browseFileList(this, file)) {
		DirectoryListing dirList(dl->getUser());
		try {
			dirList.loadFile(Text::fromT(file));
			dl->getRoot()->filterList(dirList);
			refreshTree(Util::emptyStringT);
			initStatusText();
			updateStatus();
		} catch(const Exception&) {
			/// @todo report to user?
		}
	}
}

void DirectoryListingFrame::handleFindToggle() {
	if(searchGrid->getEnabled()) {
		searchBox->clear();
		searchGrid->setEnabled(false);
		searchGrid->setVisible(false);
		grid->row(0).mode = GridInfo::STATIC;
	} else {
		for(auto& i: lastSearches | boost::adaptors::reversed) {
			auto p = i.get();
			searchBox->setData(searchBox->addValue(p->first), reinterpret_cast<LPARAM>(p));
		}
		searchGrid->setEnabled(true);
		searchGrid->setVisible(true);
		grid->row(0).mode = GridInfo::AUTO;
		searchBox->setFocus();
	}
	grid->layout();
}

void DirectoryListingFrame::refreshTree(const tstring& root) {
	HoldRedraw hold { dirs };
	auto ht = findItem(treeRoot, root);
	if(!ht) {
		ht = treeRoot;
	}

	auto d = dirs->getData(ht)->dir;
	HTREEITEM child;
	while((child = dirs->getChild(ht))) {
		dirs->erase(child);
	}
	updateDir(d, ht);

	dirs->setSelected(nullptr);
	selectItem(root);

	dirs->expand(treeRoot);
}

void DirectoryListingFrame::updateTitle() {
	tstring text = WinUtil::getNicks(dl->getUser());
	if(error.empty()) {
		text += _T(" - ") + WinUtil::getHubNames(dl->getUser()).first;
	} else {
		text = str(TF_("%1%: %2%") % text % error);
	}

	// bypass the recent item updater if the file list hasn't been loaded yet.
	if(loaded) {
		setText(text);
	} else {
		BaseType::setText(loading ? str(TF_("Loading file list: %1%") % text) : text);
	}

	dirs->getData(treeRoot)->setText(text);
	dirs->redraw();
}

ShellMenuPtr DirectoryListingFrame::makeSingleMenu(ItemInfo* ii) {
	auto menu = addChild(ShellMenu::Seed(WinUtil::Seeds::menu));

	menu->setTitle(escapeMenu(ii->getText(COLUMN_FILENAME)), WinUtil::fileImages->getIcon(ii->getImage(0)));

	menu->appendItem(T_("&Download"), [this] { handleDownload(); }, WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargets(menu.get(), ii);

	if(ii->type == ItemInfo::FILE) {
		menu->appendItem(T_("&View as text"), [this] { handleViewAsText(); });

		menu->appendSeparator();

		WinUtil::addHashItems(menu.get(), ii->file->getTTH(), Text::toT(ii->file->getName()), ii->file->getSize());
	}

	if((ii->type == ItemInfo::FILE && ii->file->getAdls()) ||
		(ii->type == ItemInfo::DIRECTORY && ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()) )	{
		menu->appendSeparator();
		menu->appendItem(T_("&Go to directory"), [this] { handleGoToDirectory(); });
	}

	addUserCommands(menu.get());
	return menu;
}

ShellMenuPtr DirectoryListingFrame::makeMultiMenu() {
	auto menu = addChild(ShellMenu::Seed(WinUtil::Seeds::menu));

	size_t sel = files->countSelected();
	menu->setTitle(str(TF_("%1% items") % sel), getParent()->getIcon(this));

	menu->appendItem(T_("&Download"), [this] { handleDownload(); }, WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargets(menu.get());
	addUserCommands(menu.get());

	return menu;
}

ShellMenuPtr DirectoryListingFrame::makeDirMenu(ItemInfo* ii) {
	auto menu = addChild(ShellMenu::Seed(WinUtil::Seeds::menu));

	menu->setTitle(escapeMenu(ii ? ii->getText(COLUMN_FILENAME) : getText()),
		ii ? WinUtil::fileImages->getIcon(ii->getImage(0)) : getParent()->getIcon(this));

	menu->appendItem(T_("&Download"), [this] { handleDownload(); }, WinUtil::menuIcon(IDI_DOWNLOAD), true, true);
	addTargets(menu.get());
	return menu;
}

void DirectoryListingFrame::addUserCommands(Menu* menu) {
	prepareMenu(menu, UserCommand::CONTEXT_FILELIST, ClientManager::getInstance()->getHubUrls(dl->getUser()));
}

void DirectoryListingFrame::addShellPaths(ShellMenu* menu, const vector<ItemInfo*>& sel) {
	StringList ShellMenuPaths;
	for(auto& ii: sel) {
		StringList paths;
		switch(ii->type) {
			case ItemInfo::FILE: paths = dl->getLocalPaths(ii->file); break;
			case ItemInfo::DIRECTORY: paths = dl->getLocalPaths(ii->dir); break;
			case ItemInfo::USER: break;
		}
		if(!paths.empty())
			ShellMenuPaths.insert(ShellMenuPaths.end(), paths.begin(), paths.end());
	}
	menu->appendShellMenu(ShellMenuPaths);
}

void DirectoryListingFrame::addUserMenu(Menu* menu) {
	appendUserItems(getParent(), menu->appendPopup(T_("User")));
}

void DirectoryListingFrame::addTargets(Menu* menu, ItemInfo* ii) {
	auto sub = menu->appendPopup(T_("Download &to..."));
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	size_t i = 0;
	for(; i < spl.size(); ++i) {
		sub->appendItem(Text::toT(spl[i].second), [this, i] { handleDownloadFavorite(i); });
	}

	if(i > 0) {
		sub->appendSeparator();
	}

	sub->appendItem(T_("&Browse..."), [this] { handleDownloadBrowse(); });

	targets.clear();

	if(ii && ii->type == ItemInfo::FILE) {
		targets = QueueManager::getInstance()->getTargets(ii->file->getTTH());
		if(!targets.empty()) {
			sub->appendSeparator();
			for(i = 0; i < targets.size(); ++i) {
				sub->appendItem(Text::toT(targets[i]), [this, i] { handleDownloadTarget(i); });
			}
		}
	}

	if(!WinUtil::lastDirs.empty()) {
		sub->appendSeparator();

		for(i = 0; i < WinUtil::lastDirs.size(); ++i) {
			sub->appendItem(WinUtil::lastDirs[i], [this, i] { handleDownloadLastDir(i); });
		}
	}
}

bool DirectoryListingFrame::handleFilesContextMenu(dwt::ScreenCoordinate pt) {
	std::vector<unsigned> selected = files->getSelection();
	if(!selected.empty()) {
		if(pt.x() == -1 && pt.y() == -1) {
			pt = files->getContextMenuPos();
		}

		ShellMenuPtr menu;

		if(selected.size() == 1) {
			menu = makeSingleMenu(files->getData(selected[0]));
		} else {
			menu = makeMultiMenu();
		}

		if(dl->getUser() == ClientManager::getInstance()->getMe()) {
			vector<ItemInfo*> sel;
			for(auto& i: selected)
				sel.push_back(files->getData(i));
			addShellPaths(menu.get(), sel);
		}

		addUserMenu(menu.get());

		WinUtil::addCopyMenu(menu.get(), files);

		usingDirMenu = false;
		menu->open(pt);
		return true;
	}
	return false;
}

bool DirectoryListingFrame::handleDirsContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = dirs->getContextMenuPos();
	} else {
		dirs->select(pt);
	}

	if(dirs->getSelected()) {
		ItemInfo* ii = dirs->getSelectedData();
		auto menu = makeDirMenu(ii);

		if(ii && dl->getUser() == ClientManager::getInstance()->getMe()) {
			addShellPaths(menu.get(), vector<ItemInfo*>(1, ii));
		}

		addUserMenu(menu.get());

		usingDirMenu = true;
		menu->open(pt);
		return true;
	}
	return false;
}

void DirectoryListingFrame::downloadFiles(const string& aTarget, bool view /* = false */) {
	int i=-1;
	while((i = files->getNext(i, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = files->getData(i);
		if(view && dl->getUser() == ClientManager::getInstance()->getMe()) {
			StringList paths = dl->getLocalPaths(ii->file);
			if(!paths.empty()) {
				TextFrame::openWindow(this->getParent(), paths[0]);
				continue;
			}
		}
		download(ii, aTarget, view);
	}
}

void DirectoryListingFrame::download(ItemInfo* ii, const string& dir, bool view) {
	try {
		if(ii->type == ItemInfo::FILE) {
			if(view) {
				File::deleteFile(dir + Util::validateFileName(ii->file->getName()));
			}
			dl->download(ii->file, dir + Text::fromT(ii->getText(COLUMN_FILENAME)), view, WinUtil::isShift() || view);
		} else if(!view) {
			dl->download(ii->dir, dir, WinUtil::isShift());
		}

	} catch(const Exception& e) {
		status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
}

void DirectoryListingFrame::handleDownload() {
	if(usingDirMenu) {
		ItemInfo* ii = dirs->getSelectedData();
		if(ii) {
			download(ii, SETTING(DOWNLOAD_DIRECTORY));
		}
	} else {
		downloadFiles(SETTING(DOWNLOAD_DIRECTORY));
	}
}

void DirectoryListingFrame::handleDownloadBrowse() {
	if(usingDirMenu) {
		ItemInfo* ii = dirs->getSelectedData();
		if(ii) {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(FolderDialog(this).open(target)) {
				WinUtil::addLastDir(target);
				download(ii, Text::fromT(target));
			}
		}
	} else {
		if(files->countSelected() == 1) {
			ItemInfo* ii = files->getSelectedData();
			try {
				if(ii->type == ItemInfo::FILE) {
					tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY)) + ii->getText(COLUMN_FILENAME);
					if(SaveDialog(this).open(target)) {
						WinUtil::addLastDir(Util::getFilePath(target));
						dl->download(ii->file, Text::fromT(target), false, WinUtil::isShift());
					}
				} else {
					tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
					if(FolderDialog(this).open(target)) {
						WinUtil::addLastDir(target);
						dl->download(ii->dir, Text::fromT(target), WinUtil::isShift());
					}
				}
			} catch(const Exception& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			tstring target = Text::toT(SETTING(DOWNLOAD_DIRECTORY));
			if(FolderDialog(this).open(target)) {
				WinUtil::addLastDir(target);
				downloadFiles(Text::fromT(target));
			}
		}
	}
}

void DirectoryListingFrame::handleDownloadLastDir(unsigned index) {
	if(index >= WinUtil::lastDirs.size()) {
		return;
	}
	download(Text::fromT(WinUtil::lastDirs[index]));
}

void DirectoryListingFrame::handleDownloadFavorite(unsigned index) {
	StringPairList spl = FavoriteManager::getInstance()->getFavoriteDirs();
	if(index >= spl.size()) {
		return;
	}
	download(spl[index].first);
}

void DirectoryListingFrame::handleDownloadTarget(unsigned index) {
	if(index >= targets.size()) {
		return;
	}

	if(files->countSelected() != 1) {
		return;
	}

	const string& target = targets[index];
	ItemInfo* ii = files->getSelectedData();
	try {
		dl->download(ii->file, target, false, WinUtil::isShift());
	} catch(const Exception& e) {
		status->setText(STATUS_STATUS, Text::toT(e.getError()));
	}
}


void DirectoryListingFrame::handleGoToDirectory() {
	if(files->countSelected() != 1)
		return;

	tstring fullPath;
	ItemInfo* ii = files->getSelectedData();

	if(ii->type == ItemInfo::FILE) {
		if(!ii->file->getAdls())
			return;

		DirectoryListing::Directory* pd = ii->file->getParent();
		while(pd != NULL && pd != dl->getRoot()) {
			fullPath = Text::toT(pd->getName()) + _T("\\") + fullPath;
			pd = pd->getParent();
		}
	} else if(ii->type == ItemInfo::DIRECTORY) {
		if(!(ii->dir->getAdls() && ii->dir->getParent() != dl->getRoot()))
			return;
		fullPath = Text::toT(((DirectoryListing::AdlDirectory*)ii->dir)->getFullPath());
	}

	selectItem(fullPath);
}

void DirectoryListingFrame::download(const string& target) {
	if(usingDirMenu) {
		ItemInfo* ii = dirs->getSelectedData();
		if(ii) {
			download(ii, target);
		}
	} else {
		if(files->countSelected() == 1) {
			ItemInfo* ii = files->getSelectedData();
			download(ii, target);
		} else {
			downloadFiles(target);
		}
	}
}

void DirectoryListingFrame::handleViewAsText() {
	downloadFiles(Util::getTempPath(), true);
}

HTREEITEM DirectoryListingFrame::findItem(HTREEITEM ht, const tstring& name) {
	string::size_type i = name.find(_T('\\'));
	if(i == string::npos)
		return ht;

	for(auto child = dirs->getChild(ht); child != NULL; child = dirs->getNextSibling(child)) {
		auto d = dirs->getData(child)->dir;
		if(Text::toT(d->getName()) == name.substr(0, i)) {
			return findItem(child, name.substr(i+1));
		}
	}
	return NULL;
}

void DirectoryListingFrame::selectItem(const tstring& name) {
	auto ht = findItem(treeRoot, name);
	if(ht) {
		dirs->setSelected(ht);
		dirs->ensureVisible(ht);
	}
}

string DirectoryListingFrame::getSelectedDir() const {
	auto sel = dirs->getSelectedData();
	if(sel && sel->type == ItemInfo::DIRECTORY) {
		return dl->getPath(sel->dir);
	}
	return Util::emptyString;
}

HTREEITEM DirectoryListingFrame::addDir(DirectoryListing::Directory* d, HTREEITEM parent, HTREEITEM insertAfter) {
	auto item = dirs->insert(getCachedDir(d), parent, insertAfter);
	if(d->getAdls())
		dirs->setItemState(item, TVIS_BOLD, TVIS_BOLD);
	updateDir(d, item);
	return item;
}

void DirectoryListingFrame::updateDir(DirectoryListing::Directory* d, HTREEITEM parent) {
	for(auto& i: d->directories) {
		addDir(i, parent);
	}
}

void DirectoryListingFrame::initStatusText() {
	status->setText(STATUS_TOTAL_FILES, str(TF_("Files (total): %1%") % dl->getTotalFileCount(true)));
	status->setText(STATUS_TOTAL_SIZE, str(TF_("Size (total): %1%") % Text::toT(Util::formatBytes(dl->getTotalSize(true)))));
	status->setText(STATUS_SPEED, str(TF_("Speed: %1%/s") % Text::toT(Util::formatBytes(speed))));
}

void DirectoryListingFrame::updateStatus() {
	if(!searching && !updating) {
		int cnt = files->countSelected();
		int64_t total = 0;
		if(cnt == 0) {
			cnt = files->size();
			total = files->forEachT(ItemInfo::TotalSize()).total;
		} else {
			total = files->forEachSelectedT(ItemInfo::TotalSize()).total;
		}

		status->setText(STATUS_SELECTED_FILES, str(TF_("Files: %1%") % cnt));

		status->setText(STATUS_SELECTED_SIZE, str(TF_("Size: %1%") % Text::toT(Util::formatBytes(total))));
	}
}

void DirectoryListingFrame::handleSelectionChanged() {
	if(!loaded)
		return;

	ItemInfo* ii = dirs->getSelectedData();
	if(!ii) {
		return;
	}

	DirectoryListing::Directory* d = ii->dir;
	if(!d) {
		return;
	}

	HoldRedraw hold { files };
	changeDir(d);

	if(!d->getComplete()) {
		dcdebug("Directory %s incomplete, downloading...\n", d->getName().c_str());
		if(dl->getUser().user->isOnline()) {
			try {
				QueueManager::getInstance()->addList(dl->getUser(), QueueItem::FLAG_PARTIAL_LIST, dl->getPath(d));
				status->setText(STATUS_STATUS, T_("Downloading list..."));
			} catch(const QueueException& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			status->setText(STATUS_STATUS, T_("User offline"));
		}
	}

	addHistory(dl->getPath(d));

	pathBox->clear();
	for(auto& i: history)
		pathBox->addValue(i.empty() ? getText() : Text::toT(i));
	pathBox->setSelected(historyIndex - 1);

	updateRecent();
}

DirectoryListingFrame::ItemInfo* DirectoryListingFrame::getCachedDir(DirectoryListing::Directory* d) {
	auto cache = dirCache.find(d);
	if(cache != dirCache.end()) {
		return &cache->second;
	}

	// the dir wasn't cached; add it now.
	++cacheCount;
	return &dirCache.emplace(d, d).first->second;
}

void DirectoryListingFrame::changeDir(DirectoryListing::Directory* d) {
	updating = true;
	clearFiles();
	curDir = d;

	for(auto& i: d->directories) {
		files->insert(files->size(), getCachedDir(i));
	}

	auto useCache = true;

	auto cache = fileCache.find(d);
	if(cache == fileCache.end()) {
		if(!loader || !loader->updateCache(d)) {
			/* dang, the file cache isn't ready for this directory. fill it on-the-fly; might
			freeze the interface (this is the operation the file cache is meant to prevent). */
			const auto count = d->files.size();
			if(canCache(count)) {
				cacheCount += count;
				list<ItemInfo> list;
				for(auto& i: d->files) {
					list.emplace_back(i);
				}
				fileCache.emplace(d, move(list));
			} else {
				useCache = false;
			}
		}
		if(useCache) {
			cache = fileCache.find(d);
		}
	}
	if(useCache) {
		dcdebug("loading file info from the cache\n");
		for(auto& i: cache->second) {
			files->insert(files->size(), &i);
		}
	} else {
		dcdebug("creating file info on-the-fly\n");
		for(auto& i: d->files) {
			files->insert(files->size(), new ItemInfo(i));
		}
	}

	files->resort();

	updating = false;
	updateStatus();
}

void DirectoryListingFrame::clearFiles() {
	if(curDir && fileCache.find(curDir) != fileCache.end()) {
		// files in this directory are cached; no need to delete them.
		files->clear();
		return;
	}

	files->forEachT([](ItemInfo* i) { if(i->type == ItemInfo::FILE) { delete i; } });
	files->clear();
}

void DirectoryListingFrame::addHistory(const string& name) {
	history.erase(history.begin() + historyIndex, history.end());
	while(history.size() > 25)
		history.pop_front();
	history.push_back(name);
	historyIndex = history.size();
}

void DirectoryListingFrame::up() {
	HTREEITEM t = dirs->getSelected();
	if(t == NULL)
		return;
	t = dirs->getParent(t);
	if(t == NULL)
		return;
	dirs->setSelected(t);
}

void DirectoryListingFrame::back() {
	if(history.size() > 1 && historyIndex > 1) {
		size_t n = min(historyIndex, history.size()) - 1;
		auto tmp = history;
		selectItem(Text::toT(history[n - 1]));
		historyIndex = n;
		history = tmp;
	}
}

void DirectoryListingFrame::forward() {
	if(history.size() > 1 && historyIndex < history.size()) {
		size_t n = min(historyIndex, history.size() - 1);
		auto tmp = history;
		selectItem(Text::toT(history[n]));
		historyIndex = n + 1;
		history = tmp;
	}
}

void DirectoryListingFrame::findFile(bool reverse) {
	const tstring findStr = searchBox->getText();
	if(findStr.empty())
		return;
	const auto method = static_cast<StringMatch::Method>(filterMethod->getSelected());

	{
		// make sure the new search string is at the top of the list
		auto p = make_pair(findStr, method);
		auto prev = std::find_if(lastSearches.begin(), lastSearches.end(),
			[p](const std::unique_ptr<LastSearchPair>& ptr) { return *ptr == p; });
		if(prev == lastSearches.end()) {
			size_t i = max(SETTING(SEARCH_HISTORY) - 1, 0);
			while(lastSearches.size() > i) {
				lastSearches.erase(lastSearches.begin());
			}
		} else {
			searchBox->erase(lastSearches.end() - 1 - prev); // the GUI list is in reverse order...
			searchBox->setText(findStr); // it erases the text-box too...
			lastSearches.erase(prev);
		}
		auto ptr = new LastSearchPair(p);
		lastSearches.emplace_back(ptr);
		searchBox->setData(searchBox->insertValue(0, findStr), reinterpret_cast<LPARAM>(ptr));
	}

	status->setText(STATUS_STATUS, str(TF_("Searching for: %1%") % findStr));

	// to make sure we set the status only after redrawing has been enabled back on the bar.
	tstring finalStatus;
	ScopedFunctor(([this, &finalStatus] { status->setText(STATUS_STATUS, finalStatus); }));

	HoldRedraw hold { files };
	HoldRedraw hold2 { dirs };
	HoldRedraw hold3 { status };

	auto start = dirs->getSelected();
	if(!start)
		start = treeRoot;

	auto prevHistory = history;
	auto prevHistoryIndex = historyIndex;

	auto selectDir = [this, start, &prevHistory, prevHistoryIndex](HTREEITEM dir) {
		// SelectItem won't update the list if SetRedraw was set to FALSE and then
		// to TRUE and the selected item is the same as the last one... workaround:
		if(dir == start)
			dirs->setSelected(nullptr);
		dirs->setSelected(dir);
		dirs->ensureVisible(dir);

		if(dir == start) {
			history = prevHistory;
			historyIndex = prevHistoryIndex;
		}
	};

	StringMatch matcher;
	matcher.pattern = Text::fromT(findStr);
	matcher.setMethod(method);
	matcher.prepare();

	vector<HTREEITEM> collapse;
	bool cycle = false;
	bool incomplete = false;
	const auto fileSel = files->getSelected();

	HTREEITEM item = start;
	auto pos = fileSel;

	while(true) {
		if(!incomplete && item) {
			auto data = dirs->getData(item);
			if(data && !data->dir->getComplete())
				incomplete = true;
		}

		// try to match the names currently in the list pane
		const int n = files->size();
		if(reverse && pos == -1)
			pos = n;
		bool found = false;
		for(reverse ? --pos : ++pos; reverse ? (pos >= 0) : (pos < n); reverse ? --pos : ++pos) {
			const ItemInfo& ii = *files->getData(pos);
			if(matcher.match((ii.type == ItemInfo::FILE) ? ii.file->getName() : ii.dir->getName())) {
				found = true;
				break;
			}
		}
		if(found)
			break;

		// flow to the next directory
		if(!reverse && dirs->getChild(item) && !dirs->isExpanded(item)) {
			dirs->expand(item);
			collapse.push_back(item);
		}
		HTREEITEM next = dirs->getNext(item, reverse ? TVGN_PREVIOUSVISIBLE : TVGN_NEXTVISIBLE);
		if(!next) {
			next = reverse ? dirs->getLast() : treeRoot;
			cycle = true;
		}
		while(reverse && dirs->getChild(next) && !dirs->isExpanded(next)) {
			dirs->expand(next);
			collapse.push_back(next);
			if(!(next = dirs->getNext(item, TVGN_PREVIOUSVISIBLE)))
				next = dirs->getLast();
		}
		if(next == start) {
			item = nullptr;
			break;
		}

		// refresh the list pane to respect sorting etc
		changeDir(dirs->getData(next)->dir);

		item = next;
		pos = -1;
	}

	for(auto& i: collapse)
		dirs->collapse(i);

	if(item) {
		selectDir(item);

		// Remove prev. selection from file list
		files->clearSelection();

		// Highlight the file
		files->setSelected(pos);
		files->ensureVisible(pos);

		if(cycle) {
			auto s_b(T_("beginning")), s_e(T_("end"));
			finalStatus = str(TF_("Reached the %1% of the file list, continuing from the %2%")
				% (reverse ? s_b : s_e) % (reverse ? s_e : s_b));
		}

	} else {
		// restore the previous view.
		selectDir(start);
		files->setSelected(fileSel);
		files->ensureVisible(fileSel);

		finalStatus = str(TF_("No matches found for: %1%") % findStr);
	}

	if(incomplete) {
		if(!finalStatus.empty())
			finalStatus += _T(" ");
		finalStatus += T_("[Ignored some incomplete directories]");
	}
}

void DirectoryListingFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;

	set<UserPtr> users;

	int sel = -1;
	while((sel = files->getNext(sel, LVNI_SELECTED)) != -1) {
		ItemInfo* ii = files->getData(sel);
		if(uc.once()) {
			if(users.find(dl->getUser()) != users.end())
				continue;
			users.insert(dl->getUser());
		}
		if(!dl->getUser().user->isOnline())
			return;
		ucParams["fileTR"] = [] { return _("NONE"); };
		if(ii->type == ItemInfo::FILE) {
			ucParams["type"] = [] { return _("File"); };
			ucParams["fileFN"] = [this, ii] { return dl->getPath(ii->file) + ii->file->getName(); };
			ucParams["fileSI"] = [ii] { return std::to_string(ii->file->getSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->file->getSize()); };
			ucParams["fileTR"] = [ii] { return ii->file->getTTH().toBase32(); };
			ucParams["fileMN"] = [ii] { return WinUtil::makeMagnet(ii->file->getTTH(), ii->file->getName(), ii->file->getSize()); };
		} else {
			ucParams["type"] = [] { return _("Directory"); };
			ucParams["fileFN"] = [this, ii] { return dl->getPath(ii->dir) + ii->dir->getName(); };
			ucParams["fileSI"] = [ii] { return std::to_string(ii->dir->getTotalSize()); };
			ucParams["fileSIshort"] = [ii] { return Util::formatBytes(ii->dir->getTotalSize()); };
		}

		// compatibility with 0.674 and earlier
		ucParams["file"] = ucParams["fileFN"];
		ucParams["filesize"] = ucParams["fileSI"];
		ucParams["filesizeshort"] = ucParams["fileSIshort"];
		ucParams["tth"] = ucParams["fileTR"];

		auto tmp = ucParams;
		ClientManager::getInstance()->userCommand(dl->getUser(), uc, tmp, true);
	}
}

void DirectoryListingFrame::handleDoubleClickFiles() {

	HTREEITEM t = dirs->getSelected();
	int i = files->getSelected();
	if(t != NULL && i != -1) {
		ItemInfo* ii = files->getData(i);

		if(ii->type == ItemInfo::FILE) {
			try {
				dl->download(ii->file, SETTING(DOWNLOAD_DIRECTORY) + Text::fromT(ii->getText(COLUMN_FILENAME)), false, WinUtil::isShift());
			} catch(const Exception& e) {
				status->setText(STATUS_STATUS, Text::toT(e.getError()));
			}
		} else {
			HTREEITEM ht = dirs->getChild(t);
			while(ht != NULL) {
				if(dirs->getData(ht)->dir == ii->dir) {
					dirs->setSelected(ht);
					break;
				}
				ht = dirs->getNextSibling(ht);
			}
		}
	}
}

bool DirectoryListingFrame::handleXMouseUp(const dwt::MouseEvent& mouseEvent) {
	switch(mouseEvent.ButtonPressed) {
	case dwt::MouseEvent::X1: back(); break;
	case dwt::MouseEvent::X2: forward(); break;
	default: break;
	}
	return true;
}

bool DirectoryListingFrame::handleKeyDownDirs(int c) {
	switch(c) {
	case VK_BACK: { back(); return true; }
	case VK_LEFT:
		{
			if(isControlPressed()) { back(); return true; }
			break;
		}
	case VK_RIGHT:
		{
			if(isControlPressed()) { forward(); return true; }
			break;
		}
	case VK_UP:
		{
			if(isControlPressed()) { up(); return true; }
			break;
		}
	}
	return false;
}

bool DirectoryListingFrame::handleKeyDownFiles(int c) {
	if(handleKeyDownDirs(c))
		return true;

	if(c == VK_RETURN) {
		if(files->countSelected() == 1) {
			ItemInfo* ii = files->getSelectedData();
			if(ii->type == ItemInfo::DIRECTORY) {
				HTREEITEM ht = dirs->getChild(dirs->getSelected());
				while(ht != NULL) {
					if(dirs->getData(ht)->dir == ii->dir) {
						dirs->setSelected(ht);
						break;
					}
					ht = dirs->getNextSibling(ht);
				}
			} else {
				downloadFiles(SETTING(DOWNLOAD_DIRECTORY));
			}
		} else {
			downloadFiles(SETTING(DOWNLOAD_DIRECTORY));
		}
		return true;
	}

	return false;
}

bool DirectoryListingFrame::handleSearchKeyDown(int c) {
	if(c == VK_RETURN && !(WinUtil::isShift() || WinUtil::isCtrl() || WinUtil::isAlt())) {
		handleFind(false);
		return true;
	}
	return false;
}

bool DirectoryListingFrame::handleSearchChar(int c) {
	// avoid the "beep" sound when enter is pressed
	return c == VK_RETURN;
}

void DirectoryListingFrame::tabMenuImpl(dwt::Menu* menu) {
	addUserMenu(menu);
	menu->appendSeparator();
}

DirectoryListingFrame::UserInfoList DirectoryListingFrame::selectedUsersImpl() {
	return UserInfoList(1, &user);
}

void DirectoryListingFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept {
	if(aUser.getUser() == dl->getUser().user)
		callAsync([this] { updateTitle(); });
}
void DirectoryListingFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept {
	if(aUser == dl->getUser().user)
		callAsync([this] { updateTitle(); });
}
void DirectoryListingFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
	if(aUser == dl->getUser().user)
		callAsync([this] { updateTitle(); });
}
