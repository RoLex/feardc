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

#ifndef DCPLUSPLUS_DCPP_QUEUE_ITEM_H
#define DCPLUSPLUS_DCPP_QUEUE_ITEM_H

#include <list>
#include <set>

#include "User.h"
#include "FastAlloc.h"
#include "MerkleTree.h"
#include "Flags.h"
#include "forward.h"
#include "Segment.h"
#include "Util.h"
#include "GetSet.h"
#include "HintedUser.h"
#include "PluginEntity.h"

namespace dcpp {

using std::set;

class QueueManager;

class QueueItem : public Flags, public FastAlloc<QueueItem>, public PluginEntity<QueueData> {
public:
	typedef unordered_map<string*, QueueItemPtr, noCaseStringHash, noCaseStringEq> StringMap;

	enum Priority {
		DEFAULT = -1,
		PAUSED = 0,
		LOWEST,
		LOW,
		NORMAL,
		HIGH,
		HIGHEST,
		LAST
	};

	enum FileFlags {
		/** Normal download, no flags set */
		FLAG_NORMAL = 0x00,

		/** This is a user file listing download */
		FLAG_USER_LIST = 0x02,

		/** The file list is downloaded to use for directory download (used with USER_LIST) */
		FLAG_DIRECTORY_DOWNLOAD = 0x04,

		/** The file is downloaded to be viewed in the gui */
		FLAG_CLIENT_VIEW = 0x08,

		/** Flag to indicate that file should be viewed as a text file */
		FLAG_TEXT = 0x20,

		/** Match the queue against this list */
		FLAG_MATCH_QUEUE = 0x80,

		/** The file list downloaded was actually an .xml.bz2 list */
		FLAG_XML_BZLIST = 0x100,

		/** Only download a part of the file list */
		FLAG_PARTIAL_LIST = 0x200
	};

	class Source : public Flags {
	public:
		enum {
			FLAG_NONE = 0x00,
			FLAG_FILE_NOT_AVAILABLE = 0x01,
			FLAG_PASSIVE = 0x02,
			FLAG_REMOVED = 0x04,
			FLAG_CRC_FAILED = 0x08,
			FLAG_CRC_WARN = 0x10,
			FLAG_NO_TTHF = 0x20,
			FLAG_BAD_TREE = 0x40,
			FLAG_NO_TREE = 0x80,
			FLAG_SLOW_SOURCE = 0x100,
			FLAG_UNTRUSTED = 0x200,
			FLAG_UNENCRYPTED = 0x400,
			FLAG_MASK = FLAG_FILE_NOT_AVAILABLE
				| FLAG_PASSIVE | FLAG_REMOVED | FLAG_CRC_FAILED | FLAG_CRC_WARN
				| FLAG_BAD_TREE | FLAG_NO_TREE | FLAG_SLOW_SOURCE | FLAG_UNTRUSTED
				| FLAG_UNENCRYPTED
		};

		Source(const HintedUser& aUser) : user(aUser) { }
		Source(const Source& aSource) : Flags(aSource), user(aSource.user) { }

		bool operator==(const UserPtr& aUser) const { return user == aUser; }

		GETSET(HintedUser, user, User);
	};

	typedef std::vector<Source> SourceList;
	typedef SourceList::iterator SourceIter;
	typedef SourceList::const_iterator SourceConstIter;

	typedef set<Segment> SegmentSet;
	typedef SegmentSet::iterator SegmentIter;
	typedef SegmentSet::const_iterator SegmentConstIter;

	QueueItem(const string& aTarget, int64_t aSize, Priority aPriority, int aFlag,
		time_t aAdded, const TTHValue& tth) :
		Flags(aFlag), target(aTarget), size(aSize),
		priority(aPriority), added(aAdded),	tthRoot(tth)
	{ }

	QueueItem(const QueueItem& rhs) :
		Flags(rhs), done(rhs.done), downloads(rhs.downloads), target(rhs.target),
		size(rhs.size), priority(rhs.priority), added(rhs.added), tthRoot(rhs.tthRoot),
		sources(rhs.sources), badSources(rhs.badSources), tempTarget(rhs.tempTarget)
	{ }

	virtual ~QueueItem() { }

	int countOnlineUsers() const;
	bool hasOnlineUsers() const { return countOnlineUsers() > 0; }
	void getOnlineUsers(HintedUserList& l) const;

	SourceList& getSources() { return sources; }
	const SourceList& getSources() const { return sources; }
	SourceList& getBadSources() { return badSources; }
	const SourceList& getBadSources() const { return badSources; }

	string getTargetFileName() const { return Util::getFileName(getTarget()); }

	SourceIter getSource(const UserPtr& aUser) { return find(sources.begin(), sources.end(), aUser); }
	SourceIter getBadSource(const UserPtr& aUser) { return find(badSources.begin(), badSources.end(), aUser); }
	SourceConstIter getSource(const UserPtr& aUser) const { return find(sources.begin(), sources.end(), aUser); }
	SourceConstIter getBadSource(const UserPtr& aUser) const { return find(badSources.begin(), badSources.end(), aUser); }

	bool isSource(const UserPtr& aUser) const { return getSource(aUser) != sources.end(); }
	bool isBadSource(const UserPtr& aUser) const { return getBadSource(aUser) != badSources.end(); }
	bool isBadSourceExcept(const UserPtr& aUser, Flags::MaskType exceptions) const {
		auto i = getBadSource(aUser);
		if(i != badSources.end())
			return i->isAnySet(exceptions^Source::FLAG_MASK);
		return false;
	}

	int64_t getDownloadedBytes() const;
	double getDownloadedFraction() const { return static_cast<double>(getDownloadedBytes()) / getSize(); }

	DownloadList& getDownloads() { return downloads; }

	/** Next segment that is not done and not being downloaded, zero-sized segment returned if there is none is found */
	Segment getNextSegment(int64_t blockSize, int64_t wantedSize) const;

	void addSegment(const Segment& segment);
	void resetDownloaded() { done.clear(); }

	bool isFinished() const {
		return done.size() == 1 && *done.begin() == Segment(0, getSize());
	}

	bool isRunning() const {
		return !isWaiting();
	}
	bool isWaiting() const {
		return downloads.empty();
	}

	string getListName() const;

	const string& getTempTarget();
	void setTempTarget(const string& aTempTarget) { tempTarget = aTempTarget; }

	QueueData* getPluginObject() noexcept;

	GETSET(SegmentSet, done, Done);
	GETSET(DownloadList, downloads, Downloads);
	GETSET(string, target, Target);
	GETSET(int64_t, size, Size);
	GETSET(Priority, priority, Priority);
	GETSET(time_t, added, Added);
	GETSET(TTHValue, tthRoot, TTH);
private:
	QueueItem& operator=(const QueueItem&);

	friend class QueueManager;
	SourceList sources;
	SourceList badSources;
	string tempTarget;

	void addSource(const HintedUser& aUser);
	void removeSource(const UserPtr& aUser, int reason);
};

} // namespace dcpp

#endif // !defined(QUEUE_ITEM_H)
