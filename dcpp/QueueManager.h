/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_QUEUE_MANAGER_H
#define DCPLUSPLUS_DCPP_QUEUE_MANAGER_H

#include <functional>
#include <unordered_map>

#include <boost/lockfree/queue.hpp>

#include "atomic.h"
#include "BundleItem.h"
#include "ClientManagerListener.h"
#include "CriticalSection.h"
#include "DirectoryListing.h"
#include "Exception.h"
#include "File.h"
#include "MerkleTree.h"
#include "QueueItem.h"
#include "QueueManagerListener.h"
#include "SearchManagerListener.h"
#include "Singleton.h"
#include "TimerManager.h"
#include "User.h"

namespace dcpp {

using std::function;
using std::list;
using std::pair;
using std::unordered_multimap;
using std::unordered_map;

STANDARD_EXCEPTION(QueueException);
EXTEND_EXCEPTION(QueueSelfException, QueueException);

class UserConnection;

class DirectoryItem {
public:
	typedef DirectoryItem* Ptr;
	typedef unordered_multimap<UserPtr, Ptr, User::Hash> DirectoryMap;
	typedef DirectoryMap::iterator DirectoryIter;
	typedef pair<DirectoryIter, DirectoryIter> DirectoryPair;

	typedef vector<Ptr> List;
	typedef List::iterator Iter;

	DirectoryItem() : priority(QueueItem::DEFAULT) { }
	DirectoryItem(const UserPtr& aUser, const string& aName, const string& aTarget,
		QueueItem::Priority p) : name(aName), target(aTarget), priority(p), user(aUser) { }
	~DirectoryItem() { }

	UserPtr& getUser() { return user; }
	void setUser(const UserPtr& aUser) { user = aUser; }

	GETSET(string, name, Name);
	GETSET(string, target, Target);
	GETSET(QueueItem::Priority, priority, Priority);
private:
	UserPtr user;
};

class ConnectionQueueItem;
class QueueLoader;

class QueueManager : public Singleton<QueueManager>, public Speaker<QueueManagerListener>, private TimerManagerListener,
	private SearchManagerListener, private ClientManagerListener
{
public:
	typedef list<QueueItemPtr> QueueItemList;

	/** Add a file to the queue. */
	void add(const string& aTarget, int64_t aSize, const TTHValue& root, const HintedUser& aUser,
		int aFlags = 0, bool addBad = true, const BundlePtr &bundle = BundlePtr());
	void add(const string& aRoot, const BundlePtr& bundle, const HintedUser& aUser, int aFlags = 0);

	/** Add a user's filelist to the queue. */
	void addList(const HintedUser& HintedUser, int aFlags, const string& aInitialDir = Util::emptyString);
	/** Readd a source that was removed */
	void readd(const string& target, const HintedUser& aUser);
	/** Add a directory to the queue (downloads filelist and matches the directory). */
	void addDirectory(const string& aDir, const HintedUser& aUser, const string& aTarget,
		QueueItem::Priority p = QueueItem::DEFAULT) noexcept;

	int matchListing(const DirectoryListing& dl) noexcept;

	bool getTTH(const string& name, TTHValue& tth) noexcept;

	int64_t getSize(const string& target) noexcept;
	int64_t getPos(const string& target) noexcept;
	void getSizeInfo(int64_t& size, int64_t& pos, const string& target) noexcept;

	/** Move the target location of a queued item. Running items are silently ignored */
	void move(const string& aSource, const string& aTarget) noexcept;

	void remove(const string& aTarget) noexcept;
	void removeSource(const string& aTarget, const UserPtr& aUser, int reason, bool removeConn = true) noexcept;
	void removeSource(const UserPtr& aUser, int reason) noexcept;

	void recheck(const string& aTarget);

	void setPriority(const string& aTarget, QueueItem::Priority p) noexcept;

	StringList getTargets(const TTHValue& tth);

	void lockedOperation(const function<void (const QueueItem::StringMap&)>& currentQueue);

	Download* getDownload(UserConnection& aSource) noexcept;
	void putDownload(Download* aDownload, bool finished) noexcept;

	pair<size_t, int64_t> getQueued(const UserPtr& aUser) const;

	/** @return The highest priority download the user has, PAUSED may also mean no downloads */
	QueueItem::Priority hasDownload(const UserPtr& aUser) noexcept;

	int countOnlineSources(const string& aTarget);

	void loadQueue(function<void (float)> progressF) noexcept;
	void saveQueue(bool force = false) noexcept;

	string getListPath(const HintedUser& user);
	void noDeleteFileList(const string& path);

	GETSET(uint64_t, lastSave, LastSave);
	GETSET(string, queueFile, QueueFile);

private:
	static const int64_t MOVER_LIMIT = 10*1024*1024;

	class FileMover : public Thread {
	public:
		FileMover() : files(8) { }
		virtual ~FileMover() { join(); }

		void moveFile(const string& source, const string& target);
		virtual int run();

	private:
		static atomic_flag active;
		typedef pair<string, string> FilePair;
		boost::lockfree::queue<FilePair*> files;
	} mover;

	class Rechecker : public Thread {
		struct DummyOutputStream : OutputStream {
			virtual size_t write(const void*, size_t n) { return n; }
			virtual size_t flush() { return 0; }
		};

	public:
		explicit Rechecker(QueueManager* qm_) : qm(qm_), files(8) { }
		virtual ~Rechecker() { join(); }

		void add(const string& file);
		virtual int run();

	private:
		QueueManager* qm;
		static atomic_flag active;
		boost::lockfree::queue<string*> files;
	} rechecker;

	/** All queue items by target */
	class FileQueue {
	public:
		FileQueue() : lastInsert(queue.end()) { }
		~FileQueue();
		void add(QueueItem* qi);
		QueueItem* add(const string& aTarget, int64_t aSize, int aFlags, QueueItem::Priority p,
			const string& aTempTarget, time_t aAdded, const TTHValue& root);

		QueueItem* find(const string& target);
		QueueItemList find(const TTHValue& tth);

		QueueItem* findAutoSearch(StringList& recent);
		size_t getSize() { return queue.size(); }
		QueueItem::StringMap& getQueue() { return queue; }
		void move(QueueItem* qi, const string& aTarget);
		void remove(QueueItem* qi);
	private:
		QueueItem::StringMap queue;
		/** A hint where to insert an item... */
		QueueItem::StringMap::iterator lastInsert;
	};

	/** All queue items indexed by user (this is a cache for the FileQueue really...) */
	class UserQueue {
	public:
		void add(QueueItem* qi);
		void add(QueueItem* qi, const UserPtr& aUser);
		QueueItem* getNext(const UserPtr& aUser, QueueItem::Priority minPrio = QueueItem::LOWEST, int64_t wantedSize = 0);
		QueueItem* getRunning(const UserPtr& aUser);
		void addDownload(QueueItem* qi, Download* d);
		void removeDownload(QueueItem* qi, const UserPtr& d);
		void remove(QueueItem* qi, bool removeRunning = true);
		void remove(QueueItem* qi, const UserPtr& aUser, bool removeRunning = true);
		void setPriority(QueueItem* qi, QueueItem::Priority p);

		unordered_map<UserPtr, QueueItemList, User::Hash>& getList(size_t i) { return userQueue[i]; }
		bool isRunning(const UserPtr& aUser) const {
			return (running.find(aUser) != running.end());
		}

		pair<size_t, int64_t> getQueued(const UserPtr& aUser) const;
	private:
		/** QueueItems by priority and user (this is where the download order is determined) */
		unordered_map<UserPtr, QueueItemList, User::Hash> userQueue[QueueItem::LAST];
		/** Currently running downloads, a QueueItem is always either here or in the userQueue */
		unordered_map<UserPtr, QueueItemPtr, User::Hash> running;
	};

	friend class QueueLoader;
	friend class Singleton<QueueManager>;

	QueueManager();
	virtual ~QueueManager();

	mutable CriticalSection cs;

	/** Bundles queued for download */
	map<TTHValue, BundleItem> bundles;
	/** QueueItems by target */
	FileQueue fileQueue;
	/** QueueItems by user */
	UserQueue userQueue;
	/** Directories queued for downloading */
	DirectoryItem::DirectoryMap directories;
	/** Recent searches list, to avoid searching for the same thing too often */
	StringList recent;
	/** The queue needs to be saved */
	bool dirty;
	/** Next search */
	uint64_t nextSearch;
	/** File lists not to delete */
	StringList protectedFileLists;
	/** Sanity check for the target filename */
	static string checkTarget(const string& aTarget, bool checkExsistence);
	/** Add a source to an existing queue item */
	bool addSource(QueueItem* qi, const HintedUser& aUser, Flags::MaskType addBad);

	void processList(const string& name, const HintedUser& user, int flags);

	void load(const SimpleXML& aXml);
	void moveFile(const string& source, const string& target);
	static void moveFile_(const string& source, const string& target);
	void moveStuckFile(QueueItem* qi);
	void rechecked(QueueItem* qi);

	void setDirty();

	bool checkSfv(QueueItem* qi, Download* d);
	uint32_t calcCrc32(const string& file);

	void logFinishedDownload(QueueItem* qi, Download* d, bool crcChecked);

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, uint64_t aTick) noexcept;
	virtual void on(TimerManagerListener::Minute, uint64_t aTick) noexcept;

	// SearchManagerListener
	virtual void on(SearchManagerListener::SR, const SearchResultPtr&) noexcept;

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept;
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;
};

} // namespace dcpp

#endif // !defined(QUEUE_MANAGER_H)
