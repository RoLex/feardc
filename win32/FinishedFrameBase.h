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

#ifndef DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H
#define DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H

#include <dcpp/File.h>
#include <dcpp/FinishedItem.h>
#include <dcpp/FinishedManager.h>
#include <dcpp/TimerManager.h>
#include <dcpp/ClientManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/DirectoryListing.h>
#include <dcpp/ShareManager.h>
#include <dcpp/ClientManager.h>

#include <dwt/util/StringUtils.h>

#include "StaticFrame.h"
#include "TypedTable.h"
#include "TextFrame.h"
#include "HoldRedraw.h"
#include "DirectoryListingFrame.h"
#include "ShellMenu.h"

using dwt::util::escapeMenu;

template<class T, bool in_UL>
class FinishedFrameBase :
	public StaticFrame<T>,
	private FinishedManagerListener
{
	typedef StaticFrame<T> BaseType;
	typedef FinishedFrameBase<T, in_UL> ThisType;

public:
	enum Status {
		STATUS_ONLY_FULL,
		STATUS_STATUS,
		STATUS_COUNT,
		STATUS_BYTES,
		STATUS_SPEED,
		STATUS_LAST
	};

	static void focusFile(TabViewPtr parent, const string& file) {
		BaseType::openWindow(parent, false);
		BaseType::frame->focusFile(file);
	}

protected:
	friend class StaticFrame<T>;
	friend class MDIChildFrame<T>;

	FinishedFrameBase(TabViewPtr parent, const tstring& title, unsigned helpId, unsigned resourceId) :
		BaseType(parent, title, helpId, resourceId),
		tabs(0),
		files(0),
		filesWindow(0),
		users(0),
		usersWindow(0),
		onlyFull(0),
		bOnlyFull(false)
	{
		{
			TabView::Seed seed = WinUtil::Seeds::tabs;
			seed.style &= ~TCS_OWNERDRAWFIXED;
			seed.location = dwt::Rectangle(this->getClientSize());
			seed.widthConfig = 0;
			tabs = this->addChild(seed);
		}

		{
			dwt::Container::Seed cs;
			cs.caption = T_("Grouped by files");
			cs.location = tabs->getClientSize();
			filesWindow = dwt::WidgetCreator<dwt::Container>::create(tabs, cs);
			filesWindow->setHelpId(in_UL ? IDH_FINISHED_UL : IDH_FINISHED_DL);
			filesWindow->onClosing(&noClose);
			tabs->add(filesWindow, WinUtil::tabIcon(IDI_GROUPED_BY_FILES));

			cs.style &= ~WS_VISIBLE;
			cs.caption = T_("Grouped by users");
			usersWindow = dwt::WidgetCreator<dwt::Container>::create(tabs, cs);
			usersWindow->setHelpId(in_UL ? IDH_FINISHED_UL : IDH_FINISHED_DL);
			usersWindow->onClosing(&noClose);
			tabs->add(usersWindow, WinUtil::tabIcon(IDI_GROUPED_BY_USERS));
		}

		{
			files = filesWindow->addChild(typename WidgetFiles::Seed(WinUtil::Seeds::table));
			files->setTableStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
			this->addWidget(files);

			WinUtil::makeColumns(files, filesColumns, FILES_COLUMN_LAST,
				SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_FILES_ORDER : SettingsManager::FINISHED_DL_FILES_ORDER),
				SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_FILES_WIDTHS : SettingsManager::FINISHED_DL_FILES_WIDTHS));
			WinUtil::setTableSort(files, FILES_COLUMN_LAST,
				in_UL ? SettingsManager::FINISHED_UL_FILES_SORT : SettingsManager::FINISHED_DL_FILES_SORT, FILES_COLUMN_TIME);

			files->setSmallImageList(WinUtil::fileImages);

			files->onDblClicked([this] { this->handleOpenFile(); });
			files->onKeyDown([this](int c) { return this->handleFilesKeyDown(c); });
			files->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return this->handleFilesContextMenu(sc); });
		}

		{
			users = usersWindow->addChild(typename WidgetUsers::Seed(WinUtil::Seeds::table));
			users->setTableStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);
			this->addWidget(users);

			WinUtil::makeColumns(users, usersColumns, USERS_COLUMN_LAST,
				SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_USERS_ORDER : SettingsManager::FINISHED_DL_USERS_ORDER),
				SettingsManager::getInstance()->get(in_UL ? SettingsManager::FINISHED_UL_USERS_WIDTHS : SettingsManager::FINISHED_DL_USERS_WIDTHS));
			WinUtil::setTableSort(users, USERS_COLUMN_LAST,
				in_UL ? SettingsManager::FINISHED_UL_USERS_SORT : SettingsManager::FINISHED_DL_USERS_SORT, USERS_COLUMN_TIME);

			users->setSmallImageList(WinUtil::userImages);

			users->onKeyDown([this](int c) { return this->handleUsersKeyDown(c); });
			users->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return this->handleUsersContextMenu(sc); });
		}

		this->initStatus();

		if(!in_UL) {
			bOnlyFull = SETTING(FINISHED_DL_ONLY_FULL);
			{
				CheckBox::Seed seed = WinUtil::Seeds::checkBox;
				seed.caption = T_("Only show fully downloaded files");
				onlyFull = this->addChild(seed);
				onlyFull->setHelpId(IDH_FINISHED_DL_ONLY_FULL);
				onlyFull->setChecked(bOnlyFull);
				onlyFull->onClicked([this] { this->handleOnlyFullClicked(); });
				this->status->setWidget(STATUS_ONLY_FULL, onlyFull);
			}

			filesWindow->onVisibilityChanged([this](bool b) { this->onlyFull->setVisible(b); });
		}

		this->status->onDblClicked(STATUS_STATUS, [] {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(
				in_UL ? LogManager::UPLOAD : LogManager::DOWNLOAD))));
		});

		this->status->setHelpId(STATUS_COUNT, in_UL ? IDH_FINISHED_UL_COUNT : IDH_FINISHED_DL_COUNT);
		this->status->setHelpId(STATUS_BYTES, in_UL ? IDH_FINISHED_UL_BYTES : IDH_FINISHED_DL_BYTES);
		this->status->setHelpId(STATUS_SPEED, in_UL ? IDH_FINISHED_UL_SPEED : IDH_FINISHED_DL_SPEED);

		layout();

		filesWindow->onVisibilityChanged([this](bool b) { if(b) this->updateStatus(); });
		usersWindow->onVisibilityChanged([this](bool b) { if(b) this->updateStatus(); });

		FinishedManager::getInstance()->addListener(this);

		updateLists();
	}

	virtual ~FinishedFrameBase() { }

	void layout() {
		dwt::Rectangle r(this->getClientSize());

		r.size.y -= this->status->refresh();

		tabs->resize(r);
	}

	bool preClosing() {
		FinishedManager::getInstance()->removeListener(this);
		return true;
	}

	void postClosing() {
		saveColumns(files,
			in_UL ? SettingsManager::FINISHED_UL_FILES_ORDER : SettingsManager::FINISHED_DL_FILES_ORDER,
			in_UL ? SettingsManager::FINISHED_UL_FILES_WIDTHS : SettingsManager::FINISHED_DL_FILES_WIDTHS,
			in_UL ? SettingsManager::FINISHED_UL_FILES_SORT : SettingsManager::FINISHED_DL_FILES_SORT);
		saveColumns(users,
			in_UL ? SettingsManager::FINISHED_UL_USERS_ORDER : SettingsManager::FINISHED_DL_USERS_ORDER,
			in_UL ? SettingsManager::FINISHED_UL_USERS_WIDTHS : SettingsManager::FINISHED_DL_USERS_WIDTHS,
			in_UL ? SettingsManager::FINISHED_UL_USERS_SORT : SettingsManager::FINISHED_DL_USERS_SORT);

		if(!in_UL)
			SettingsManager::getInstance()->set(SettingsManager::FINISHED_DL_ONLY_FULL, bOnlyFull);
	}

private:
	enum {
		FILES_COLUMN_FIRST,
		FILES_COLUMN_FILE = FILES_COLUMN_FIRST,
		FILES_COLUMN_PATH,
		FILES_COLUMN_NICKS,
		FILES_COLUMN_TRANSFERRED,
		FILES_COLUMN_FILESIZE,
		FILES_COLUMN_PERCENTAGE,
		FILES_COLUMN_SPEED,
		FILES_COLUMN_CRC32,
		FILES_COLUMN_TIME,
		FILES_COLUMN_ELAPSED,
		FILES_COLUMN_LAST
	};

	enum {
		USERS_COLUMN_FIRST,
		USERS_COLUMN_NICK = USERS_COLUMN_FIRST,
		USERS_COLUMN_HUB,
		USERS_COLUMN_TRANSFERRED,
		USERS_COLUMN_SPEED,
		USERS_COLUMN_FILES,
		USERS_COLUMN_TIME,
		USERS_COLUMN_ELAPSED,
		USERS_COLUMN_LAST
	};

	static const ColumnInfo filesColumns[FILES_COLUMN_LAST];

	static const ColumnInfo usersColumns[USERS_COLUMN_LAST];

	class FileInfo : public FastAlloc<FileInfo> {
	public:
		FileInfo(const string& file_, const FinishedFileItemPtr& entry_) : file(file_), entry(entry_) {
			columns[FILES_COLUMN_FILE] = Text::toT(Util::getFileName(file));
			columns[FILES_COLUMN_PATH] = Text::toT(Util::getFilePath(file));
			columns[FILES_COLUMN_FILESIZE] = Text::toT(Util::formatBytes(entry->getFileSize()));

			update();
		}

		/// @return whether the list needs to be re-sorted.
		bool update(int sortCol = -1) {
			tstring old;
			if(sortCol != -1)
				old = columns[sortCol];

			auto lock = FinishedManager::getInstance()->lock();
			{
				StringList nicks;
				for(auto& i: entry->getUsers())
					nicks.push_back(Util::toString(ClientManager::getInstance()->getNicks(i)));
				columns[FILES_COLUMN_NICKS] = Text::toT(Util::toString(nicks));
			}
			columns[FILES_COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(entry->getTransferred()));
			columns[FILES_COLUMN_PERCENTAGE] = Text::toT(Util::toString(entry->getTransferredPercentage()) + '%');
			columns[FILES_COLUMN_SPEED] = Text::toT(Util::formatBytes(entry->getAverageSpeed()) + "/s");
			columns[FILES_COLUMN_CRC32] = entry->getCrc32Checked() ? T_("Yes") : T_("No");
			columns[FILES_COLUMN_TIME] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));
			columns[FILES_COLUMN_ELAPSED] = Text::toT(Util::formatSeconds(entry->getMilliSeconds() / 1000));

			return (sortCol != -1) ? (old != columns[sortCol]) : false;
		}

		const tstring& getText(int col) const {
			return columns[col];
		}
		int getImage(int col) const {
			return col == 0 ? WinUtil::getFileIcon(file) : -1;
		}

		static int compareItems(const FileInfo* a, const FileInfo* b, int col) {
			switch(col) {
				case FILES_COLUMN_TRANSFERRED: return compare(a->entry->getTransferred(), b->entry->getTransferred());
				case FILES_COLUMN_FILESIZE: return compare(a->entry->getFileSize(), b->entry->getFileSize());
				case FILES_COLUMN_PERCENTAGE: return compare(a->entry->getTransferredPercentage(), b->entry->getTransferredPercentage());
				case FILES_COLUMN_SPEED: return compare(a->entry->getAverageSpeed(), b->entry->getAverageSpeed());
				case FILES_COLUMN_ELAPSED: return compare(a->entry->getMilliSeconds(), b->entry->getMilliSeconds());
				default: return compare(a->columns[col], b->columns[col]);
			}
		}

		void viewAsText(TabViewPtr parent) {
			TextFrame::openWindow(parent, file);
		}

		void open(TabViewPtr parent, const string& ownList) {
			// see if we are opening our own file list.
			if(in_UL && file == ownList) {
				DirectoryListingFrame::openOwnList(parent);
				return;
			}

			// see if we are opening a file list.
			UserPtr u = DirectoryListing::getUserFromFilename(file);
			if(u) {
				const auto& users = entry->getUsers();
				auto hu = find(users.cbegin(), users.cend(), u);
				if(hu != users.cend()) {
					DirectoryListingFrame::openWindow(parent, Text::toT(file), Util::emptyStringT,
						*hu, entry->getAverageSpeed(), DirectoryListingFrame::FORCE_ACTIVE);
					return;
				}
			}

			WinUtil::openFile(Text::toT(file));
		}

		void openFolder() {
			WinUtil::openFolder(Text::toT(file));
		}

		void remove() {
			FinishedManager::getInstance()->remove(in_UL, file);
		}

		string file;
		FinishedFileItemPtr entry;

	private:
		tstring columns[FILES_COLUMN_LAST];
	};

	class UserInfo : public FastAlloc<UserInfo> {
	public:
		UserInfo(const HintedUser& user_, const FinishedUserItemPtr& entry_) : user(user_), entry(entry_) {
			update();
		}

		/// @return whether the list needs to be re-sorted.
		bool update(int sortCol = -1) {
			tstring old;
			if(sortCol != -1)
				old = columns[sortCol];

			auto lock = FinishedManager::getInstance()->lock();
			columns[USERS_COLUMN_NICK] = WinUtil::getNicks(user);
			columns[USERS_COLUMN_HUB] = Text::toT(Util::toString(ClientManager::getInstance()->getHubNames(user)));
			columns[USERS_COLUMN_TRANSFERRED] = Text::toT(Util::formatBytes(entry->getTransferred()));
			columns[USERS_COLUMN_SPEED] = Text::toT(Util::formatBytes(entry->getAverageSpeed()) + "/s");
			columns[USERS_COLUMN_FILES] = Text::toT(Util::toString(entry->getFiles()));
			columns[USERS_COLUMN_TIME] = Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime()));
			columns[USERS_COLUMN_ELAPSED] = Text::toT(Util::formatSeconds(entry->getMilliSeconds() / 1000));

			return (sortCol != -1) ? (old != columns[sortCol]) : false;
		}

		const tstring& getText(int col) const {
			return columns[col];
		}
		int getImage(int) const {
			return 0;
		}

		static int compareItems(const UserInfo* a, const UserInfo* b, int col) {
			switch(col) {
				case USERS_COLUMN_TRANSFERRED: return compare(a->entry->getTransferred(), b->entry->getTransferred());
				case USERS_COLUMN_SPEED: return compare(a->entry->getAverageSpeed(), b->entry->getAverageSpeed());
				case USERS_COLUMN_ELAPSED: return compare(a->entry->getMilliSeconds(), b->entry->getMilliSeconds());
				default: return compare(a->columns[col], b->columns[col]);
			}
		}

		void remove() {
			FinishedManager::getInstance()->remove(in_UL, user);
		}

		HintedUser user;
		FinishedUserItemPtr entry;

	private:
		tstring columns[USERS_COLUMN_LAST];
	};

	TabViewPtr tabs;

	typedef TypedTable<FileInfo> WidgetFiles;
	typedef WidgetFiles* WidgetFilesPtr;
	WidgetFilesPtr files;
	dwt::ContainerPtr filesWindow;

	typedef TypedTable<UserInfo> WidgetUsers;
	typedef WidgetUsers* WidgetUsersPtr;
	WidgetUsersPtr users;
	dwt::ContainerPtr usersWindow;

	dwt::CheckBoxPtr onlyFull;
	bool bOnlyFull;

	void focusFile(const string& file) {
		tabs->setActive(filesWindow);
		files->forEachT([this, &file](FileInfo* fi) {
			if(fi->file == file) {
				auto pos = this->files->find(fi);
				this->files->setSelected(pos);
				this->files->ensureVisible(pos);
			}
		});
	}

	static bool noClose() {
		return false;
	}

	bool handleFilesKeyDown(int c) {
		switch(c) {
			case VK_RETURN: handleOpenFile(); return true;
			case VK_DELETE: handleRemoveFiles(); return true;
			default: return false;
		}
	}

	bool handleUsersKeyDown(int c) {
		switch(c) {
			case VK_DELETE: handleRemoveUsers(); return true;
			default: return false;
		}
	}

	struct UserCollector {
		void operator()(FileInfo* data) {
			const HintedUserList& users_ = data->entry->getUsers();
			users.insert(users.end(), users_.begin(), users_.end());
		}
		void operator()(UserInfo* data) {
			users.push_back(data->user);
		}
		HintedUserList users;
	};

	struct FileChecker {
		FileChecker() : allFilesExist(true), isBz2(false) { }
		void operator()(FileInfo* data) {
			if(File::getSize(data->file) != -1)
				ShellMenuPaths.push_back(data->file);
			else
				allFilesExist = false;
 			isBz2 |= Util::getFileExt(data->file) == ".bz2";
 		}
		bool allFilesExist;
		StringList ShellMenuPaths;
		bool isBz2;
	};

	bool handleFilesContextMenu(dwt::ScreenCoordinate pt) {
		size_t sel = files->countSelected();
		if(sel) {
			if(pt.x() == -1 && pt.y() == -1) {
				pt = files->getContextMenuPos();
			}

			FileChecker checker = files->forEachSelectedT(FileChecker());
			auto selData = (sel == 1) ? files->getSelectedData() : 0;

			auto menu = filesWindow->addChild(ShellMenu::Seed(WinUtil::Seeds::menu));
			menu->setTitle(selData ? escapeMenu(selData->getText(FILES_COLUMN_FILE)) : str(TF_("%1% files") % sel),
				selData ? WinUtil::fileImages->getIcon(selData->getImage(0)) : tabs->getIcon(filesWindow));
			menu->appendItem(T_("&View as text"), [this] { this->handleViewAsText(); }, dwt::IconPtr(), checker.allFilesExist && !checker.isBz2);
			menu->appendItem(T_("&Open"), [this] { this->handleOpenFile(); }, dwt::IconPtr(), checker.allFilesExist, true);
			menu->appendItem(T_("Open &folder"), [this] { this->handleOpenFolder(); });
			menu->appendSeparator();
			menu->appendItem(T_("&Remove"), [this] { this->handleRemoveFiles(); });
			menu->appendItem(T_("Remove &all"), [this] { this->handleRemoveAll(); });
			menu->appendSeparator();
			WinUtil::addUserItems(menu.get(), files->forEachSelectedT(UserCollector()).users, this->getParent());
			menu->appendShellMenu(checker.ShellMenuPaths);
			WinUtil::addCopyMenu(menu.get(), files);

			menu->open(pt);
			return true;
		}
		return false;
	}

	bool handleUsersContextMenu(dwt::ScreenCoordinate pt) {
		size_t sel = users->countSelected();
		if(sel) {
			if(pt.x() == -1 && pt.y() == -1) {
				pt = users->getContextMenuPos();
			}

			auto selData = (sel == 1) ? users->getSelectedData() : 0;

			auto menu = usersWindow->addChild(WinUtil::Seeds::menu);
			menu->setTitle(selData ? escapeMenu(selData->getText(USERS_COLUMN_NICK)) : str(TF_("%1% users") % sel),
				selData ? WinUtil::userImages->getIcon(selData->getImage(0)) : tabs->getIcon(usersWindow));
			menu->appendItem(T_("&Remove"), [this] { this->handleRemoveUsers(); });
			menu->appendItem(T_("Remove &all"), [this] { this->handleRemoveAll(); });
			menu->appendSeparator();
			WinUtil::addUserItems(menu.get(), users->forEachSelectedT(UserCollector()).users, this->getParent());
			WinUtil::addCopyMenu(menu.get(), users);

			menu->open(pt);
			return true;
		}
		return false;
	}

	void handleViewAsText() {
		files->forEachSelectedT([this](FileInfo* fi) { fi->viewAsText(this->getParent()); });
	}

	void handleOpenFile() {
		files->forEachSelectedT([this](FileInfo* fi) { fi->open(this->getParent(),
			in_UL ? ShareManager::getInstance()->getOwnListFile() : Util::emptyString); });
	}

	void handleOpenFolder() {
		files->forEachSelected(&FileInfo::openFolder);
	}

	void handleRemoveFiles() {
		files->forEachSelected(&FileInfo::remove);
	}

	void handleRemoveUsers() {
		users->forEachSelected(&UserInfo::remove);
	}

	void handleRemoveAll() {
		FinishedManager::getInstance()->removeAll(in_UL);
	}

	void handleOnlyFullClicked() {
		bOnlyFull = onlyFull->getChecked();

		clearTables();
		updateLists();
	}

	void updateLists() {
		auto lock = FinishedManager::getInstance()->lock();
		{
			HoldRedraw hold { files };
			for(auto& i: FinishedManager::getInstance()->getMapByFile(in_UL))
				addFile(i.first, i.second);
		}
		{
			HoldRedraw hold { users };
			for(auto& i: FinishedManager::getInstance()->getMapByUser(in_UL))
				addUser(i.first, i.second);
		}

		updateStatus();
	}

	void updateStatus() {
		size_t count = 0;
		int64_t bytes = 0;
		int64_t time = 0;
		if(users->getVisible()) {
			count = users->size();
			for(size_t i = 0; i < count; ++i) {
				const FinishedUserItemPtr& entry = users->getData(i)->entry;
				bytes += entry->getTransferred();
				time += entry->getMilliSeconds();
			}
		} else {
			count = files->size();
			for(size_t i = 0; i < count; ++i) {
				const FinishedFileItemPtr& entry = files->getData(i)->entry;
				bytes += entry->getTransferred();
				time += entry->getMilliSeconds();
			}
		}

		this->status->setText(STATUS_COUNT, str(TFN_("%1% item", "%1% items", count) % count));
		this->status->setText(STATUS_BYTES, Text::toT(Util::formatBytes(bytes)));
		this->status->setText(STATUS_SPEED, str(TF_("%1%/s") % Text::toT(Util::formatBytes((time > 0) ? bytes * ((int64_t)1000) / time : 0))));
	}

	bool addFile(const string& file, const FinishedFileItemPtr& entry) {
		if(bOnlyFull && !entry->isFull())
			return false;

		int loc = files->insert(new FileInfo(file, entry));
		if(files->getVisible())
			files->ensureVisible(loc);
		return true;
	}

	void addUser(const HintedUser& user, const FinishedUserItemPtr& entry) {
		int loc = users->insert(new UserInfo(user, entry));
		if(users->getVisible())
			users->ensureVisible(loc);
	}

	FileInfo* findFileInfo(const string& file) {
		for(size_t i = 0; i < files->size(); ++i) {
			FileInfo* data = files->getData(i);
			if(data->file == file)
				return data;
		}
		return 0;
	}

	UserInfo* findUserInfo(const HintedUser& user) {
		for(size_t i = 0; i < users->size(); ++i) {
			UserInfo* data = users->getData(i);
			if(data->user == user)
				return data;
		}
		return 0;
	}

	template<typename TableType>
	void saveColumns(TableType table, SettingsManager::StrSetting order, SettingsManager::StrSetting widths, SettingsManager::IntSetting sort) {
		SettingsManager::getInstance()->set(order, WinUtil::toString(table->getColumnOrder()));
		SettingsManager::getInstance()->set(widths, WinUtil::toString(table->getColumnWidths()));
		SettingsManager::getInstance()->set(sort, WinUtil::getTableSort(table));
	}

	template<typename TableType>
	void clearTable(TableType table) {
		table->clear();
	}
	inline void clearTables() {
		clearTable(files);
		clearTable(users);
	}

	void onAddedFile(const string& file, const FinishedFileItemPtr& entry) {
		FileInfo* data = findFileInfo(file);
		if(data) {
			// this file already exists; simply update it
			onUpdatedFile(file);
		} else if(addFile(file, entry)) {
			updateStatus();
			this->setDirty(in_UL ? SettingsManager::BOLD_FINISHED_UPLOADS : SettingsManager::BOLD_FINISHED_DOWNLOADS);
		}
	}

	void onAddedUser(const HintedUser& user, const FinishedUserItemPtr& entry) {
		addUser(user, entry);
		updateStatus();
	}

	void onUpdatedFile(const string& file) {
		FileInfo* data = findFileInfo(file);
		if(data) {
			bool resort = data->update(files->getSortColumn());
			files->update(data);
			if(resort)
				files->resort();
			updateStatus();
		}
	}

	void onUpdatedUser(const HintedUser& user) {
		UserInfo* data = findUserInfo(user);
		if(data) {
			bool resort = data->update(users->getSortColumn());
			users->update(data);
			if(resort)
				users->resort();
			updateStatus();
		}
	}

	void onRemovedFile(const string& file) {
		FileInfo* data = findFileInfo(file);
		if(data) {
			files->erase(data);
			updateStatus();
		}
	}

	void onRemovedUser(const HintedUser& user) {
		UserInfo* data = findUserInfo(user);
		if(data) {
			users->erase(data);
			updateStatus();
		}
	}

	void onRemovedAll() {
		clearTables();
		updateStatus();
	}

	virtual void on(AddedFile, bool upload, const string& file, const FinishedFileItemPtr& entry) noexcept {
		if(upload == in_UL)
			this->callAsync([=, this] { this->onAddedFile(file, entry); });
	}

	virtual void on(AddedUser, bool upload, const HintedUser& user, const FinishedUserItemPtr& entry) noexcept {
		if(upload == in_UL)
			this->callAsync([=, this] { this->onAddedUser(user, entry); });
	}

	virtual void on(UpdatedFile, bool upload, const string& file, const FinishedFileItemPtr& entry) noexcept {
		if(upload == in_UL) {
			if(bOnlyFull && entry->isFull())
				this->callAsync([=, this] { this->onAddedFile(file, entry); });
			else
				this->callAsync([=, this] { this->onUpdatedFile(file); });
		}
	}

	virtual void on(UpdatedUser, bool upload, const HintedUser& user) noexcept {
		if(upload == in_UL)
			this->callAsync([=, this] { this->onUpdatedUser(user); });
	}

	virtual void on(RemovedFile, bool upload, const string& file) noexcept {
		if(upload == in_UL)
			this->callAsync([=, this] { this->onRemovedFile(file); });
	}

	virtual void on(RemovedUser, bool upload, const HintedUser& user) noexcept {
		if(upload == in_UL)
			this->callAsync([=, this] { this->onRemovedUser(user); });
	}

	virtual void on(RemovedAll, bool upload) noexcept {
		if(upload == in_UL)
			this->callAsync([=, this] { this->onRemovedAll(); });
	}
};

template<class T, bool in_UL>
const ColumnInfo FinishedFrameBase<T, in_UL>::filesColumns[] = {
	{ N_("Filename"), 125, false },
	{ N_("Path"), 250, false },
	{ N_("Nicks"), 250, false },
	{ N_("Transferred"), 80, true },
	{ N_("File size"), 80, true },
	{ N_("% transferred"), 80, true },
	{ N_("Speed"), 100, true },
	{ N_("CRC Checked"), 80, false },
	{ N_("Time"), 125, false },
	{ N_("Elapsed"), 125, false }
};

template<class T, bool in_UL>
const ColumnInfo FinishedFrameBase<T, in_UL>::usersColumns[] = {
	{ N_("Nick"), 125, false },
	{ N_("Hub"), 125, false },
	{ N_("Transferred"), 80, true },
	{ N_("Speed"), 100, true },
	{ N_("Files"), 300, false },
	{ N_("Time"), 125, false },
	{ N_("Elapsed"), 125, false }
};

#endif // !defined(DCPLUSPLUS_WIN32_FINISHED_FRAME_BASE_H)
