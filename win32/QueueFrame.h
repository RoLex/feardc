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

#ifndef DCPLUSPLUS_WIN32_QUEUE_FRAME_H
#define DCPLUSPLUS_WIN32_QUEUE_FRAME_H

#include <memory>
#include <unordered_map>

#include <boost/core/noncopyable.hpp>

#include <dcpp/FastAlloc.h>
#include <dcpp/QueueManagerListener.h>
#include <dcpp/QueueItem.h>

#include "StaticFrame.h"

using std::unique_ptr;
using std::unordered_multimap;

class QueueFrame :
	public StaticFrame<QueueFrame>,
	private QueueManagerListener
{
	typedef StaticFrame<QueueFrame> BaseType;
public:
	enum Status {
		STATUS_SHOW_TREE,
		STATUS_STATUS,
		STATUS_PARTIAL_COUNT,
		STATUS_PARTIAL_BYTES,
		STATUS_TOTAL_COUNT,
		STATUS_TOTAL_BYTES,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

private:
	friend class StaticFrame<QueueFrame>;
	friend class MDIChildFrame<QueueFrame>;

	enum {
		COLUMN_FIRST,
		COLUMN_TARGET = COLUMN_FIRST,
		COLUMN_STATUS,
		COLUMN_SIZE,
		COLUMN_DOWNLOADED,
		COLUMN_PRIORITY,
		COLUMN_USERS,
		COLUMN_PATH,
		COLUMN_EXACT_SIZE,
		COLUMN_ERRORS,
		COLUMN_ADDED,
		COLUMN_TTH,
		COLUMN_TYPE,
		COLUMN_LAST
	};

	class DirItemInfo : public FastAlloc<DirItemInfo> {
	public:
		DirItemInfo(const string& dir);
		DirItemInfo(const string& dir_, const tstring& text_) : dir(dir_), text(text_) { }
		const tstring& getText() const { return text; }
		int getImage(int col = 0);
		int getSelectedImage();
		const string& getDir() const { return dir; }
	private:
		string dir;
		tstring text;
	};

	class QueueItemInfo;
	friend class QueueItemInfo;

	class QueueItemInfo : public Flags, public FastAlloc<QueueItemInfo>, boost::noncopyable {
	public:

		struct Display : public FastAlloc<Display> {
			tstring columns[COLUMN_LAST];
		};

		enum {
			MASK_TARGET = 1 << COLUMN_TARGET,
			MASK_STATUS = 1 << COLUMN_STATUS,
			MASK_SIZE = 1 << COLUMN_SIZE,
			MASK_DOWNLOADED = 1 << COLUMN_DOWNLOADED,
			MASK_PRIORITY = 1 << COLUMN_PRIORITY,
			MASK_USERS = 1 << COLUMN_USERS,
			MASK_PATH = 1 << COLUMN_PATH,
			MASK_ERRORS = 1 << COLUMN_ERRORS,
			MASK_ADDED = 1 << COLUMN_ADDED,
			MASK_TTH = 1 << COLUMN_TTH,
			MASK_TYPE = 1 << COLUMN_TYPE
		};

		enum Status {
			STATUS_WAITING,
			STATUS_RUNNING,
			STATUS_FINISHED
		};

		QueueItemInfo(const QueueItem& aQI) : Flags(aQI), target(aQI.getTarget()),
			path(Util::getFilePath(aQI.getTarget())),
			size(aQI.getSize()), downloadedBytes(aQI.getDownloadedBytes()),
			added(aQI.getAdded()), priority(aQI.getPriority()), status(getStatus(aQI)), tth(aQI.getTTH()),
			sources(aQI.getSources()), badSources(aQI.getBadSources()), updateMask((uint32_t)-1)
		{
		}

		void update();
		void recheck();
		void remove();

		// TypedTable functions
		const tstring& getText(int col) {
			return getDisplay()->columns[col];
		}

		int getImage(int col) const {
			return col == 0 ? WinUtil::getFileIcon(getTarget()) : -1;
		}

		static int compareItems(QueueItemInfo* a, QueueItemInfo* b, int col) {
			switch(col) {
				case COLUMN_SIZE: case COLUMN_EXACT_SIZE: return compare(a->getSize(), b->getSize());
				case COLUMN_PRIORITY: return compare((int)a->getPriority(), (int)b->getPriority());
				case COLUMN_DOWNLOADED: return compare(a->getDownloadedBytes(), b->getDownloadedBytes());
				case COLUMN_ADDED: return compare(a->getAdded(), b->getAdded());
				default: return compare(a->getDisplay()->columns[col], b->getDisplay()->columns[col]);
			}
		}

		static Status getStatus(const QueueItem& qi) {
			return qi.isRunning() ? STATUS_RUNNING : qi.isFinished() ? STATUS_FINISHED : STATUS_WAITING;
		}

		QueueItem::SourceList& getSources() { return sources; }
		QueueItem::SourceList& getBadSources() { return badSources; }

		Display* getDisplay() {
			if(!display.get()) {
				display.reset(new Display);
				update();
			}

			return display.get();
		}

		bool isSource(const UserPtr& u) {
			return find(sources.begin(), sources.end(), u) != sources.end();
		}
		bool isBadSource(const UserPtr& u) {
			return find(badSources.begin(), badSources.end(), u) != badSources.end();
		}

		GETSET(string, target, Target);
		GETSET(string, path, Path);
		GETSET(int64_t, size, Size);
		GETSET(int64_t, downloadedBytes, DownloadedBytes);
		GETSET(time_t, added, Added);
		GETSET(QueueItem::Priority, priority, Priority);
		GETSET(Status, status, Status);
		GETSET(TTHValue, tth, TTH);
		GETSET(QueueItem::SourceList, sources, Sources);
		GETSET(QueueItem::SourceList, badSources, BadSources);
		uint32_t updateMask;

	private:
		unique_ptr<Display> display;
	};

	typedef unique_ptr<QueueItemInfo> QueueItemPtr;

	ToolBarPtr toolbar;
	RebarPtr rebar;
	SplitterContainerPtr paned;

	StringList toolbarIds;

	typedef TypedTree<DirItemInfo> WidgetDirs;
	typedef WidgetDirs* WidgetDirsPtr;
	WidgetDirsPtr dirs;

	typedef TypedTable<QueueItemInfo, false> WidgetFiles;
	typedef WidgetFiles* WidgetFilesPtr;
	WidgetFilesPtr files;

	unordered_multimap<string, QueueItemPtr, noCaseStringHash, noCaseStringEq> directories;

	std::string curDir;

	bool dirty;
	bool filesDirty;

	bool toolbarItemsStatus;

	bool usingDirMenu;

	int64_t queueSize;
	int queueItems;

	HTREEITEM fileLists;

	QueueFrame(TabViewPtr parent);
	virtual ~QueueFrame();

	void updateStatus();
	void updateFiles();

	void addQueueItem(QueueItemPtr&& ii, bool single);
	void addQueueList(const QueueItem::StringMap& l);

	HTREEITEM addDirectory(const string& dir, bool isFileList = false, HTREEITEM startAt = NULL);
	void removeDirectories(HTREEITEM ht);
	void removeDirectory(const string& dir, bool isFileList = false);

	bool isCurDir(const string& aDir) const;

	QueueItemInfo* getItemInfo(const string& target);

	void moveSelected();
	void moveSelectedDir();
	void moveDir(HTREEITEM ht, const string& target);

	void moveNode(HTREEITEM item, HTREEITEM parent);

	void removeSelected();
	void removeSelectedDir();

	const string& getSelectedDir();
	const string& getDir(HTREEITEM ht);

	void removeDir(HTREEITEM ht);
	void setPriority(HTREEITEM ht, const QueueItem::Priority& p);

	void changePriority(bool inc);
	void changePriority(HTREEITEM ht, bool inc);
	void changePriority(QueueItemInfo* ii, bool inc);

	MenuPtr makeSingleMenu(QueueItemInfo* qii);
	MenuPtr makeMultiMenu();
	MenuPtr makeDirMenu();

	void reEvaluateToolbar();

	void addBrowseMenu(Menu* menu, QueueItemInfo* qii);
	void addRemoveMenu(Menu* menu, QueueItemInfo* qii);
	void addRemoveSourcesMenu(Menu* menu, QueueItemInfo* qii);
	void addPMMenu(Menu* menu, QueueItemInfo* qii);
	void addPriorityMenu(Menu* menu);
	void addReaddMenu(Menu* menu, QueueItemInfo* qii);
	bool addUsers(Menu* menu, void (QueueFrame::*handler)(const HintedUser&), const QueueItem::SourceList& sources, bool offline);

	void layout();

	bool preClosing();
	void postClosing();

	void handleMove();
	void handleRemove();
	void handleRecheck();
	void handlePriority(QueueItem::Priority p);
	void handlePM(const HintedUser& user);
	void handleRemoveSource(const HintedUser& user);
	void handleRemoveSources(const HintedUser& user);
	void handleBrowseList(const HintedUser& user);
	void handleReadd(const HintedUser& user);
	bool handleKeyDownFiles(int c);
	bool handleKeyDownDirs(int c);
	bool handleFilesContextMenu(dwt::ScreenCoordinate pt);
	bool handleDirsContextMenu(dwt::ScreenCoordinate pt);

	void onAdded(QueueItemInfo* ii);
	void onRemoved(const string& s);
	void onUpdated(QueueItem* qi);
	void onRechecked(const string& target, const tstring& message);

	virtual void on(QueueManagerListener::Added, QueueItem* aQI) noexcept;
	virtual void on(QueueManagerListener::Moved, QueueItem* aQI, const string& oldTarget) noexcept;
	virtual void on(QueueManagerListener::Removed, QueueItem* aQI) noexcept;
	virtual void on(QueueManagerListener::SourcesUpdated, QueueItem* aQI) noexcept;
	virtual void on(QueueManagerListener::StatusUpdated, QueueItem* aQI) noexcept { on(QueueManagerListener::SourcesUpdated(), aQI); }
	virtual void on(QueueManagerListener::Finished, QueueItem* aQI, const string&, int64_t) noexcept { on(QueueManagerListener::SourcesUpdated(), aQI); }

	virtual void on(QueueManagerListener::RecheckStarted, const string& target) noexcept;
	virtual void on(QueueManagerListener::RecheckNoFile, const string& target) noexcept;
	virtual void on(QueueManagerListener::RecheckFileTooSmall, const string& target) noexcept;
	virtual void on(QueueManagerListener::RecheckDownloadsRunning, const string& target) noexcept;
	virtual void on(QueueManagerListener::RecheckNoTree, const string& target) noexcept;
	virtual void on(QueueManagerListener::RecheckAlreadyFinished, const string& target) noexcept;
	virtual void on(QueueManagerListener::RecheckDone, const string& target) noexcept;
};

#endif // !defined(QUEUE_FRAME_H)
