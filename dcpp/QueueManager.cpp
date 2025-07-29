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

#include "stdinc.h"
#include "QueueManager.h"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm_ext/for_each.hpp>

#include "ClientManager.h"
#include "ConnectionManager.h"
#include "Download.h"
#include "FileReader.h"
#include "FinishedManager.h"
#include "HashManager.h"
#include "LogManager.h"
#include "ScopedFunctor.h"
#include "SearchManager.h"
#include "SearchResult.h"
#include "SFVReader.h"
#include "ShareManager.h"
#include "SimpleXML.h"
#include "UserConnection.h"
#include "version.h"
#include "ZUtils.h"

#ifdef ff
#undef ff
#endif

namespace dcpp {

using boost::adaptors::map_values;
using boost::range::for_each;

std::atomic_flag QueueManager::Rechecker::active = ATOMIC_FLAG_INIT;
std::atomic_flag QueueManager::FileMover::active = ATOMIC_FLAG_INIT;

QueueManager::FileQueue::~FileQueue() {
	for_each(queue | map_values, DeleteFunction());
}

QueueItem* QueueManager::FileQueue::add(const string& aTarget, int64_t aSize,
						  int aFlags, QueueItem::Priority p, const string& aTempTarget,
						  time_t aAdded, const TTHValue& root)
{
	if(p == QueueItem::DEFAULT) {
		p = QueueItem::NORMAL;
		if(aSize <= SETTING(PRIO_HIGHEST_SIZE)*1024) {
			p = QueueItem::HIGHEST;
		} else if(aSize <= SETTING(PRIO_HIGH_SIZE)*1024) {
			p = QueueItem::HIGH;
		} else if(aSize <= SETTING(PRIO_NORMAL_SIZE)*1024) {
			p = QueueItem::NORMAL;
		} else if(aSize <= SETTING(PRIO_LOW_SIZE)*1024) {
			p = QueueItem::LOW;
		} else if(SETTING(PRIO_LOWEST)) {
			p = QueueItem::LOWEST;
		}
	}

	QueueItem* qi = new QueueItem(aTarget, aSize, p, aFlags, aAdded, root);

	if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
		qi->setPriority(QueueItem::HIGHEST);
	}

	qi->setTempTarget(aTempTarget);

	dcassert(find(aTarget) == NULL);
	add(qi);
	return qi;
}

void QueueManager::FileQueue::add(QueueItem* qi) {
	if(lastInsert == queue.end())
		lastInsert = queue.emplace(const_cast<string*>(&qi->getTarget()), qi).first;
	else
		lastInsert = queue.insert(lastInsert, make_pair(const_cast<string*>(&qi->getTarget()), qi));
}

void QueueManager::FileQueue::remove(QueueItem* qi) {
	if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
		++lastInsert;
	queue.erase(const_cast<string*>(&qi->getTarget()));
	delete qi;
}

QueueItem* QueueManager::FileQueue::find(const string& target) {
	auto i = queue.find(const_cast<string*>(&target));
	return (i == queue.end()) ? NULL : i->second;
}

QueueManager::QueueItemList QueueManager::FileQueue::find(const TTHValue& tth) {
	QueueItemList ql;
	for(auto& i: queue) {
		auto qi = i.second;
		if(qi->getTTH() == tth) {
			ql.push_back(qi);
		}
	}
	return ql;
}

static QueueItem* findCandidate(QueueItem* cand, QueueItem::StringMap::iterator start, QueueItem::StringMap::iterator end, const StringList& recent) {
	for(auto i = start; i != end; ++i) {
		QueueItem* q = i->second;

		// We prefer to search for things that are not running...
		if((cand != NULL) && q->isRunning())
			continue;
		// No finished files
		if(q->isFinished())
			continue;
		// No user lists
		if(q->isSet(QueueItem::FLAG_USER_LIST))
			continue;
		// No paused downloads
		if(q->getPriority() == QueueItem::PAUSED)
			continue;
		// No files that already have more than AUTO_SEARCH_LIMIT online sources
		if(q->countOnlineUsers() >= SETTING(AUTO_SEARCH_LIMIT))
			continue;
		// Did we search for it recently?
		if(find(recent.begin(), recent.end(), q->getTarget()) != recent.end())
			continue;

		cand = q;

		if(cand->isWaiting())
			break;
	}
	return cand;
}

QueueItem* QueueManager::FileQueue::findAutoSearch(StringList& recent) {
	// We pick a start position at random, hoping that we will find something to search for...
	QueueItem::StringMap::size_type start = (QueueItem::StringMap::size_type)Util::rand(0, (uint32_t)queue.size());

	auto i = queue.begin();
	advance(i, start);

	QueueItem* cand = findCandidate(NULL, i, queue.end(), recent);
	if(cand == NULL || cand->isRunning()) {
		cand = findCandidate(cand, queue.begin(), i, recent);
	}
	return cand;
}

void QueueManager::FileQueue::move(QueueItem* qi, const string& aTarget) {
	if(lastInsert != queue.end() && Util::stricmp(*lastInsert->first, qi->getTarget()) == 0)
		lastInsert = queue.end();
	queue.erase(const_cast<string*>(&qi->getTarget()));
	qi->setTarget(aTarget);
	add(qi);
}

void QueueManager::UserQueue::add(QueueItem* qi) {
	for(auto& i: qi->getSources()) {
		add(qi, i.getUser());
	}
}

void QueueManager::UserQueue::add(QueueItem* qi, const UserPtr& aUser) {
	auto& l = userQueue[qi->getPriority()][aUser];

	if(qi->getDownloadedBytes() > 0) {
		l.push_front(qi);
	} else {
		l.push_back(qi);
	}
}

QueueItem* QueueManager::UserQueue::getNext(const UserPtr& aUser, QueueItem::Priority minPrio, int64_t wantedSize) {
	int p = QueueItem::LAST - 1;

	do {
		auto i = userQueue[p].find(aUser);
		if(i != userQueue[p].end()) {
			dcassert(!i->second.empty());
			for(auto qi: i->second) {
				if(qi->isWaiting()) {
					return qi;
				}

				// No segmented downloading when getting the tree
				if(qi->getDownloads()[0]->getType() == Transfer::TYPE_TREE) {
					continue;
				}
				if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
					auto blockSize = HashManager::getInstance()->getBlockSize(qi->getTTH());
					if(blockSize == 0)
						blockSize = qi->getSize();
					if(qi->getNextSegment(blockSize, wantedSize).getSize() == 0) {
						dcdebug("No segment for %s in %s, block " I64_FMT "\n", aUser->getCID().toBase32().c_str(), qi->getTarget().c_str(), blockSize);
						continue;
					}
				}
				return qi;
			}
		}
		p--;
	} while(p >= minPrio);

	return NULL;
}

void QueueManager::UserQueue::addDownload(QueueItem* qi, Download* d) {
	qi->getDownloads().push_back(d);

	// Only one download per user...
	dcassert(running.find(d->getUser()) == running.end());
	running[d->getUser()] = qi;
}

void QueueManager::UserQueue::removeDownload(QueueItem* qi, const UserPtr& user) {
	running.erase(user);

	for(auto i = qi->getDownloads().begin(); i != qi->getDownloads().end(); ++i) {
		if((*i)->getUser() == user) {
			qi->getDownloads().erase(i);
			break;
		}
	}
}

void QueueManager::UserQueue::setPriority(QueueItem* qi, QueueItem::Priority p) {
	remove(qi, false);
	qi->setPriority(p);
	add(qi);
}

pair<size_t, int64_t> QueueManager::UserQueue::getQueued(const UserPtr& aUser) const {
	pair<size_t, int64_t> ret(0, 0);
	for(size_t i = QueueItem::LOWEST; i < QueueItem::LAST; ++i) {
		auto& ulm = userQueue[i];
		auto iulm = ulm.find(aUser);
		if(iulm == ulm.end()) {
			continue;
		}

		for(auto& qi: iulm->second) {
			++ret.first;
			if(qi->getSize() != -1) {
				ret.second += qi->getSize() - qi->getDownloadedBytes();
			}
		}
	}
	return ret;
}

QueueItem* QueueManager::UserQueue::getRunning(const UserPtr& aUser) {
	auto i = running.find(aUser);
	return (i == running.end()) ? 0 : i->second;
}

void QueueManager::UserQueue::remove(QueueItem* qi, bool removeRunning) {
	for(auto& i: qi->getSources()) {
		remove(qi, i.getUser(), removeRunning);
	}
}

void QueueManager::UserQueue::remove(QueueItem* qi, const UserPtr& aUser, bool removeRunning) {
	if(removeRunning && qi == getRunning(aUser)) {
		removeDownload(qi, aUser);
	}

	dcassert(qi->isSource(aUser));
	auto& ulm = userQueue[qi->getPriority()];
	auto j = ulm.find(aUser);
	dcassert(j != ulm.end());
	auto& l = j->second;
	auto i = find(l.begin(), l.end(), qi);
	dcassert(i != l.end());
	l.erase(i);

	if(l.empty()) {
		ulm.erase(j);
	}
}

void QueueManager::FileMover::moveFile(const string& source, const string& target) {
	files.push(new FilePair(source, target));
	if(!active.test_and_set()) {
		start();
	}
}

int QueueManager::FileMover::run() {
	ScopedFunctor([this] { active.clear(); });

	while(true) {
		unique_ptr<FilePair> next;
		if(!files.pop(next)) {
			return 0;
		}

		moveFile_(next->first, next->second);
	}
	return 0;
}

void QueueManager::Rechecker::add(const string& file) {
	files.push(new string(file));
	if(!active.test_and_set()) {
		start();
	}
}

int QueueManager::Rechecker::run() {
	ScopedFunctor([this] { active.clear(); });

	while(true) {
		unique_ptr<string> file;
		if(!files.pop(file)) {
			return 0;
		}

		QueueItem* q;
		int64_t tempSize;
		TTHValue tth;

		{
			Lock l(qm->cs);

			q = qm->fileQueue.find(*file);
			if(!q || q->isSet(QueueItem::FLAG_USER_LIST))
				continue;

			qm->fire(QueueManagerListener::RecheckStarted(), q->getTarget());
			dcdebug("Rechecking %s\n", file->c_str());

			tempSize = File::getSize(q->getTempTarget());

			if(tempSize == -1) {
				qm->fire(QueueManagerListener::RecheckNoFile(), q->getTarget());
				continue;
			}

			if(tempSize < 64*1024) {
				qm->fire(QueueManagerListener::RecheckFileTooSmall(), q->getTarget());
				continue;
			}

			if(tempSize != q->getSize()) {
				File(q->getTempTarget(), File::WRITE, File::OPEN).setSize(q->getSize());
			}

			if(q->isRunning()) {
				qm->fire(QueueManagerListener::RecheckDownloadsRunning(), q->getTarget());
				continue;
			}

			tth = q->getTTH();
		}

		TigerTree tt;
		bool gotTree = HashManager::getInstance()->getTree(tth, tt);

		string tempTarget;

		{
			Lock l(qm->cs);

			// get q again in case it has been (re)moved
			q = qm->fileQueue.find(*file);
			if(!q)
				continue;

			if(!gotTree) {
				qm->fire(QueueManagerListener::RecheckNoTree(), q->getTarget());
				continue;
			}

			//Clear segments
			q->resetDownloaded();

			tempTarget = q->getTempTarget();
		}

		TigerTree ttFile(tt.getBlockSize());

		try {
			FileReader(true).read(tempTarget, [&](const void* x, size_t n) {
				return ttFile.update(x, n), true;
			});
		} catch(const FileException & e) {
			dcdebug("Error while reading file: %s\n", e.what());
		}

		Lock l(qm->cs);

		// get q again in case it has been (re)moved
		q = qm->fileQueue.find(*file);
		if(!q)
			continue;

		ttFile.finalize();

		if(ttFile.getRoot() == tth) {
			//If no bad blocks then the file probably got stuck in the temp folder for some reason
			qm->moveStuckFile(q);
			continue;
		}

		size_t pos = 0;
		boost::for_each(tt.getLeaves(), ttFile.getLeaves(), [&](const TTHValue& our, const TTHValue& file) {
			if(our == file) {
				q->addSegment(Segment(pos, tt.getBlockSize()));
			}

			pos += tt.getBlockSize();
		});

		qm->rechecked(q);
	}

	return 0;
}

QueueManager::QueueManager() :
lastSave(0),
queueFile(Util::getPath(Util::PATH_USER_CONFIG) + "Queue.xml"),
rechecker(this),
dirty(true),
nextSearch(0)
{
	TimerManager::getInstance()->addListener(this);
	SearchManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);

	File::ensureDirectory(Util::getListPath());
}

QueueManager::~QueueManager() {
	SearchManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);

	if(!SETTING(KEEP_LISTS)) {
		string path = Util::getListPath();

		std::sort(protectedFileLists.begin(), protectedFileLists.end());

		auto filelists = File::findFiles(path, "*.xml*");
		std::sort(filelists.begin(), filelists.end());
		std::for_each(filelists.begin(), std::set_difference(filelists.begin(), filelists.end(),
			protectedFileLists.begin(), protectedFileLists.end(), filelists.begin()),
			[](const string& file) { File::deleteFile(file); });
	}
}

bool QueueManager::getTTH(const string& name, TTHValue& tth) noexcept {
	Lock l(cs);
	QueueItem* qi = fileQueue.find(name);
	if(qi) {
		tth = qi->getTTH();
		return true;
	}
	return false;
}

void QueueManager::on(TimerManagerListener::Minute, uint64_t aTick) noexcept {
	string fn;
	string searchString;
	bool online = false;

	{
		Lock l(cs);

		if(SETTING(AUTO_SEARCH) && (aTick >= nextSearch) && (fileQueue.getSize() > 0)) {
			// We keep 30 recent searches to avoid duplicate searches
			while((recent.size() >= fileQueue.getSize()) || (recent.size() > 30)) {
				recent.erase(recent.begin());
			}

			QueueItem* qi = fileQueue.findAutoSearch(recent);
			if(qi) {
				searchString = qi->getTTH().toBase32();
				online = qi->hasOnlineUsers();
				recent.push_back(qi->getTarget());

				// Previously, this code used a static difference between the online and offline interval.
				// This difference was 180 seconds (online = 120, offline = 300).
				uint64_t intervalDiff = 180;

				uint64_t onlineInterval = (SETTING(AUTO_SEARCH_INTERVAL) * 1000);
				uint64_t offlineInterval = ((SETTING(AUTO_SEARCH_INTERVAL) + intervalDiff)  * 1000);

				nextSearch = aTick + (online ? onlineInterval : offlineInterval );
			}
		}
	}

	if(!searchString.empty()) {
		SearchManager::getInstance()->search(searchString, 0, SearchManager::TYPE_TTH, SearchManager::SIZE_DONTCARE, "auto");
	}
}

void QueueManager::addList(const HintedUser& aUser, int aFlags, const string& aInitialDir /* = Util::emptyString */) {
	add(aInitialDir, -1, TTHValue(), aUser, QueueItem::FLAG_USER_LIST | aFlags);
}

string QueueManager::getListPath(const HintedUser& user) {
	StringList nicks = ClientManager::getInstance()->getNicks(user);
	string nick = nicks.empty() ? Util::emptyString : Util::cleanPathChars(nicks[0]) + ".";
	return checkTarget(Util::getListPath() + nick + user.user->getCID().toBase32(), /*checkExistence*/ false);
}

void QueueManager::add(const string& aTarget, int64_t aSize, const TTHValue& root, const HintedUser& aUser,
	int aFlags /* = 0 */, bool addBad /* = true */)
{
	bool wantConnection = true;

	// Check that we're not downloading from ourselves...
	if(aUser == ClientManager::getInstance()->getMe()) {
		throw QueueSelfException(_("You're trying to download from yourself!"));
	}

	// Check if we're not downloading something already in our share
	if(SETTING(DONT_DL_ALREADY_SHARED)){
		if (ShareManager::getInstance()->isTTHShared(root)){
			throw QueueException(_("A file with the same hash already exists in your share"));
		}
	}

	string target;
	string tempTarget;
	if((aFlags & QueueItem::FLAG_USER_LIST) == QueueItem::FLAG_USER_LIST) {
		target = getListPath(aUser);
		tempTarget = aTarget;
	} else {
		target = checkTarget(aTarget, /*checkExistence*/ true);
	}

	// Check if it's a zero-byte file, if so, create and return...
	if(aSize == 0) {
		if(!SETTING(SKIP_ZERO_BYTE)) {
			File::ensureDirectory(target);
			File f(target, File::WRITE, File::CREATE);
		}
		return;
	}

	{
		Lock l(cs);

		// This will be pretty slow on large queues...
		if(SETTING(DONT_DL_ALREADY_QUEUED) && !(aFlags & QueueItem::FLAG_USER_LIST || aFlags & QueueItem::FLAG_CLIENT_VIEW)) {
			auto ql = fileQueue.find(root);
			if (!ql.empty()) {
				// Found one or more existing queue items, lets see if we can add the source to them
				// Check if any of the existing queue items are for permanent downloads; if so then no addition
				// If all existing queue items are for pending temporary downloads then add a new queue item or the source
				bool sourceAdded = false, permanentExists = false;
				for(auto& i: ql) {
					if(!i->isSource(aUser)) {
						try {
							wantConnection = addSource(i, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0);
							sourceAdded = true;
						} catch(...) { }
					}
					if (!i->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
						permanentExists = true;
					}
				}

				if(!sourceAdded && permanentExists) {
					throw QueueException(_("This file is already queued"));
				}
				if (permanentExists) goto connect;
			}
		}

		QueueItem* q = fileQueue.find(target);
		if(q == NULL) {
			q = fileQueue.add(target, aSize, aFlags, QueueItem::DEFAULT, tempTarget, GET_TIME(), root);
			fire(QueueManagerListener::Added(), q);
		} else {
			if(q->getSize() != aSize) {
				throw QueueException(_("A file with a different size already exists in the queue"));
			}
			if(!(root == q->getTTH())) {
				throw QueueException(_("A file with a different TTH root already exists in the queue"));
			}

			if(q->isFinished()) {
				throw QueueException(_("This file has already finished downloading"));
			}

			q->setFlag(aFlags);
		}

		wantConnection = addSource(q, aUser, addBad ? QueueItem::Source::FLAG_MASK : 0);
	}

connect:
	if(wantConnection && aUser.user->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::readd(const string& target, const HintedUser& aUser) {
	bool wantConnection = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(target);
		if(q && q->isBadSource(aUser)) {
			wantConnection = addSource(q, aUser, QueueItem::Source::FLAG_MASK);
		}
	}
	if(wantConnection && aUser.user->isOnline())
		ConnectionManager::getInstance()->getDownloadConnection(aUser);
}

void QueueManager::setDirty() {
	if(!dirty) {
		dirty = true;
		lastSave = GET_TICK();
	}
}

string QueueManager::checkTarget(const string& aTarget, bool checkExistence) {
	if(aTarget.length() > PATH_MAX) {
		throw QueueException(_("Target filename too long"));
	}

#ifdef _WIN32
	// Check that target starts with a drive or is an UNC path
	if( (aTarget[1] != ':' || aTarget[2] != '\\') &&
		(aTarget[0] != '\\' && aTarget[1] != '\\') ) {
		throw QueueException(_("Invalid target file (missing directory, check default download directory setting)"));
	}
#else
	// Check that target contains at least one directory...we don't want headless files...
	if(aTarget[0] != '/') {
		throw QueueException(_("Invalid target file (missing directory, check default download directory setting)"));
	}
#endif

	string target = Util::validateFileName(aTarget);

	// Check that the file doesn't already exist...
	if(checkExistence && File::getSize(target) != -1) {
		throw FileException(_("File already exists at the target location"));
	}
	return target;
}

/** Add a source to an existing queue item */
bool QueueManager::addSource(QueueItem* qi, const HintedUser& aUser, Flags::MaskType addBad) {
	bool wantConnection = (qi->getPriority() != QueueItem::PAUSED) && !userQueue.getRunning(aUser);

	if(qi->isSource(aUser)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			return wantConnection;
		}
		throw QueueException(str(F_("Duplicate source: %1%") % Util::getFileName(qi->getTarget())));
	}

	if(qi->isBadSourceExcept(aUser, addBad)) {
		throw QueueException(str(F_("Duplicate source: %1%") % Util::getFileName(qi->getTarget())));
	}

	qi->addSource(aUser);

	if(aUser.user->isSet(User::PASSIVE) && !ClientManager::getInstance()->isActive() ) {
		qi->removeSource(aUser, QueueItem::Source::FLAG_PASSIVE);
		wantConnection = false;
	} else if(qi->isFinished()) {
		wantConnection = false;
	} else {
		userQueue.add(qi, aUser);
	}

	fire(QueueManagerListener::SourcesUpdated(), qi);
	setDirty();

	return wantConnection;
}

void QueueManager::addDirectory(const string& aDir, const HintedUser& aUser, const string& aTarget, QueueItem::Priority p /* = QueueItem::DEFAULT */) noexcept {
	bool needList;
	{
		Lock l(cs);

		auto dp = directories.equal_range(aUser);

		for(auto i = dp.first; i != dp.second; ++i) {
			if(Util::stricmp(aDir.c_str(), i->second->getName().c_str()) == 0)
				return;
		}

		// Unique directory, fine...
		directories.emplace(aUser, new DirectoryItem(aUser, aDir, aTarget, p));
		needList = (dp.first == dp.second);
		setDirty();
	}

	if(needList) {
		try {
			addList(aUser, QueueItem::FLAG_DIRECTORY_DOWNLOAD);
		} catch(const Exception&) {
			// Ignore, we don't really care...
		}
	}
}

QueueItem::Priority QueueManager::hasDownload(const UserPtr& aUser) noexcept {
	Lock l(cs);
	QueueItem* qi = userQueue.getNext(aUser, QueueItem::LOWEST);
	if(!qi) {
		return QueueItem::PAUSED;
	}
	return qi->getPriority();
}

typedef unordered_map<TTHValue, const DirectoryListing::File*> TTHMap;

namespace {
void buildMap(const DirectoryListing::Directory* dir, TTHMap& tthMap) noexcept {
	std::for_each(dir->directories.cbegin(), dir->directories.cend(), [&](DirectoryListing::Directory* d) {
		if(!d->getAdls())
			buildMap(d, tthMap);
	});

	std::for_each(dir->files.cbegin(), dir->files.cend(), [&](DirectoryListing::File* f) {
		tthMap.emplace(f->getTTH(), f);
	});
}
}

int QueueManager::matchListing(const DirectoryListing& dl) noexcept {
	int matches = 0;

	{
		Lock l(cs);

		TTHMap tthMap;
		buildMap(dl.getRoot(), tthMap);

		for(auto& i: fileQueue.getQueue()) {
			auto qi = i.second;
			if(qi->isFinished())
				continue;
			if(qi->isSet(QueueItem::FLAG_USER_LIST))
				continue;
			auto j = tthMap.find(qi->getTTH());
			if(j != tthMap.end() && j->second->getSize() == qi->getSize()) {
				try {
					addSource(qi, dl.getUser(), QueueItem::Source::FLAG_FILE_NOT_AVAILABLE);
				} catch(...) {
					// Ignore...
				}
				matches++;
			}
		}
	}
	if(matches > 0)
		ConnectionManager::getInstance()->getDownloadConnection(dl.getUser());
	return matches;
}

int64_t QueueManager::getPos(const string& target) noexcept {
	Lock l(cs);
	QueueItem* qi = fileQueue.find(target);
	if(qi) {
		return qi->getDownloadedBytes();
	}
	return -1;
}

int64_t QueueManager::getSize(const string& target) noexcept {
	Lock l(cs);
	QueueItem* qi = fileQueue.find(target);
	if(qi) {
		return qi->getSize();
	}
	return -1;
}

void QueueManager::getSizeInfo(int64_t& size, int64_t& pos, const string& target) noexcept {
	Lock l(cs);
	QueueItem* qi = fileQueue.find(target);
	if(qi) {
		size = qi->getSize();
		pos = qi->getDownloadedBytes();
	} else {
		size = -1;
	}
}


void QueueManager::move(const string& aSource, const string& aTarget) noexcept {
	string target = Util::validateFileName(aTarget);
	if(aSource == target)
		return;

	bool delSource = false;

	Lock l(cs);
	QueueItem* qs = fileQueue.find(aSource);
	if(qs) {
		// Don't move running downloads
		if(qs->isRunning()) {
			return;
		}
		// Don't move file lists
		if(qs->isSet(QueueItem::FLAG_USER_LIST))
			return;

		// Let's see if the target exists...then things get complicated...
		QueueItem* qt = fileQueue.find(target);
		if(qt == NULL || Util::stricmp(aSource, target) == 0) {
			// Good, update the target and move in the queue...
			fileQueue.move(qs, target);
			fire(QueueManagerListener::Moved(), qs, aSource);
			setDirty();
		} else {
			// Don't move to target of different size
			if(qs->getSize() != qt->getSize() || qs->getTTH() != qt->getTTH())
				return;

			for(auto& i: qs->getSources()) {
				try {
					addSource(qt, i.getUser(), QueueItem::Source::FLAG_MASK);
				} catch(const Exception&) {
				}
			}
			delSource = true;
		}
	}

	if(delSource) {
		remove(aSource);
	}
}

StringList QueueManager::getTargets(const TTHValue& tth) {
	Lock l(cs);
	auto ql = fileQueue.find(tth);
	StringList sl;
	for(auto& i: ql) {
		sl.push_back(i->getTarget());
	}
	return sl;
}

void QueueManager::lockedOperation(const function<void (const QueueItem::StringMap&)>& currentQueue) {
	Lock l(cs);
	if(currentQueue) currentQueue(fileQueue.getQueue());
}

Download* QueueManager::getDownload(UserConnection& aSource) noexcept {
	Lock l(cs);

	UserPtr& u = aSource.getUser();
	dcdebug("Getting download for %s...", u->getCID().toBase32().c_str());

	QueueItem* q = userQueue.getNext(u, QueueItem::LOWEST, aSource.getChunkSize());

	if(!q) {
		dcdebug("none\n");
		return 0;
	}

	// Check that the file we will be downloading to exists
	if(q->getDownloadedBytes() > 0) {
		int64_t tempSize = File::getSize(q->getTempTarget());
		if(tempSize != q->getSize()) {
			// <= 0.706 added ".antifrag" to temporary download files if antifrag was enabled...
			// 0.705 added ".antifrag" even if antifrag was disabled
			std::string antifrag = q->getTempTarget() + ".antifrag";
			if(File::getSize(antifrag) > 0) {
				File::renameFile(antifrag, q->getTempTarget());
				tempSize = File::getSize(q->getTempTarget());
			}
			if(tempSize != q->getSize()) {
				if(tempSize > 0 && tempSize < q->getSize()) {
					// Probably started with <=0.699 or with 0.705 without antifrag enabled...
					try {
						File(q->getTempTarget(), File::WRITE, File::OPEN).setSize(q->getSize());
					} catch(const FileException&) { }
				} else {
					// Temp target gone?
					q->resetDownloaded();
				}
			}
		}
	}

	Download* d = new Download(aSource, *q);

	userQueue.addDownload(q, d);

	fire(QueueManagerListener::StatusUpdated(), q);
	dcdebug("found %s\n", q->getTarget().c_str());
	return d;
}

void QueueManager::moveFile(const string& source, const string& target) {
	File::ensureDirectory(target);
	if(File::getSize(source) > MOVER_LIMIT) {
		mover.moveFile(source, target);
	} else {
		moveFile_(source, target);
	}
}

void QueueManager::moveFile_(const string& source, const string& target) {
	try {
		File::renameFile(source, target);
		getInstance()->fire(QueueManagerListener::FileMoved(), target);
	} catch(const FileException& e1) {
		// Try to just rename it to the correct name at least
		string newTarget = Util::getFilePath(source) + Util::getFileName(target);
		try {
			File::renameFile(source, newTarget);
			LogManager::getInstance()->message(str(F_("Unable to move %1% to %2% (%3%); renamed to %4%") %
				Util::addBrackets(source) % Util::addBrackets(target) % e1.getError() % Util::addBrackets(newTarget)));
		} catch(const FileException& e2) {
			LogManager::getInstance()->message(str(F_("Unable to move %1% to %2% (%3%) nor to rename to %4% (%5%)") %
				Util::addBrackets(source) % Util::addBrackets(target) % e1.getError() % Util::addBrackets(newTarget) % e2.getError()));
		}
	}
}

void QueueManager::moveStuckFile(QueueItem* qi) {
	moveFile(qi->getTempTarget(), qi->getTarget());

	if(qi->isFinished()) {
		userQueue.remove(qi);
	}

	string target = qi->getTarget();

	if(!SETTING(KEEP_FINISHED_FILES)) {
		fire(QueueManagerListener::Removed(), qi);
		fileQueue.remove(qi);
	 } else {
		qi->addSegment(Segment(0, qi->getSize()));
		fire(QueueManagerListener::StatusUpdated(), qi);
	}

	fire(QueueManagerListener::RecheckAlreadyFinished(), target);
}

void QueueManager::rechecked(QueueItem* qi) {
	fire(QueueManagerListener::RecheckDone(), qi->getTarget());
	fire(QueueManagerListener::StatusUpdated(), qi);

	setDirty();
}

void QueueManager::putDownload(Download* aDownload, bool finished) noexcept {
	HintedUserList getConn;
 	string fl_fname;
	int fl_flag = 0;

	// Make sure the download gets killed
	unique_ptr<Download> d(aDownload);
	aDownload = nullptr;

	d->close();

	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(d->getPath());
		if(!q) {
			// Target has been removed, clean up the mess
			auto hasTempTarget = !d->getTempTarget().empty();
			auto isFullList = d->getType() == Transfer::TYPE_FULL_LIST;
			auto isFile = d->getType() == Transfer::TYPE_FILE && d->getTempTarget() != d->getPath();

			if(hasTempTarget && (isFullList || isFile)) {
				File::deleteFile(d->getTempTarget());
			}

			return;
		}

		if(!finished) {
			if(d->getType() == Transfer::TYPE_FULL_LIST && !d->getTempTarget().empty()) {
				// No use keeping an unfinished file list...
				File::deleteFile(d->getTempTarget());
			}

			if(d->getType() != Transfer::TYPE_TREE && q->getDownloadedBytes() == 0) {
				q->setTempTarget(Util::emptyString);
			}

			if(d->getType() == Transfer::TYPE_FILE) {
				// mark partially downloaded chunk, but align it to block size
				int64_t downloaded = d->getPos();
				downloaded -= downloaded % d->getTigerTree().getBlockSize();

				if(downloaded > 0) {
					q->addSegment(Segment(d->getStartPos(), downloaded));
					setDirty();
				}
			}

			if(q->getPriority() != QueueItem::PAUSED) {
				q->getOnlineUsers(getConn);
			}

			userQueue.removeDownload(q, d->getUser());
			fire(QueueManagerListener::StatusUpdated(), q);
		} else { // Finished
			if(d->getType() == Transfer::TYPE_PARTIAL_LIST) {
				userQueue.remove(q);
				fire(QueueManagerListener::PartialList(), d->getHintedUser(), d->getPFS());
				fire(QueueManagerListener::Removed(), q);
				fileQueue.remove(q);
			} else if(d->getType() == Transfer::TYPE_TREE) {
				// Got a full tree, now add it to the HashManager
				dcassert(d->getTreeValid());

				if(!HashManager::getInstance()->addTree(d->getTigerTree())) {
					// Huh? Hash data corruption or access problems. Avoid flooding with infinite retries if the problem persists...
					q->getSource(d->getUser())->setFlag(QueueItem::Source::FLAG_NO_TREE);
				}

				userQueue.removeDownload(q, d->getUser());
				fire(QueueManagerListener::StatusUpdated(), q);
			} else if(d->getType() == Transfer::TYPE_FULL_LIST) {
				if(d->isSet(Download::FLAG_XML_BZ_LIST)) {
					q->setFlag(QueueItem::FLAG_XML_BZLIST);
				} else {
					q->unsetFlag(QueueItem::FLAG_XML_BZLIST);
				}

				auto dir = q->getTempTarget(); // We cheated and stored the initial display directory here (when opening lists from search)
				q->addSegment(Segment(0, q->getSize()));

				// Now, let's see if this was a directory download filelist...
				if( (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) && directories.find(d->getUser()) != directories.end()) ||
					(q->isSet(QueueItem::FLAG_MATCH_QUEUE)) )
				{
					fl_fname = q->getListName();
					fl_flag = (q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD) ? QueueItem::FLAG_DIRECTORY_DOWNLOAD : 0)
						| (q->isSet(QueueItem::FLAG_MATCH_QUEUE) ? QueueItem::FLAG_MATCH_QUEUE : 0);
				}

				fire(QueueManagerListener::Finished(), q, dir, d->getAverageSpeed());
				userQueue.remove(q);

				fire(QueueManagerListener::Removed(), q);
				fileQueue.remove(q);
			} else if(d->getType() == Transfer::TYPE_FILE) {
				q->addSegment(d->getSegment());

				auto crcChecked = q->isFinished() && SETTING(SFV_CHECK) && checkSfv(q, d.get());

				// In case of a failed crc check segments are reset
				if(q->isFinished()) {
					// Check if we need to move the file
					if(!d->getTempTarget().empty() && (Util::stricmp(d->getPath().c_str(), d->getTempTarget().c_str()) != 0) ) {
						moveFile(d->getTempTarget(), d->getPath());
					}

					if (SETTING(LOG_FINISHED_DOWNLOADS)) {
						logFinishedDownload(q, d.get(), crcChecked);
					}

					userQueue.remove(q);
					fire(QueueManagerListener::Finished(), q, Util::emptyString, d->getAverageSpeed());

					if(SETTING(KEEP_FINISHED_FILES)) {
						fire(QueueManagerListener::StatusUpdated(), q);
					} else {
						fire(QueueManagerListener::Removed(), q);
						fileQueue.remove(q);
					}
				} else {
					userQueue.removeDownload(q, d->getUser());
					fire(QueueManagerListener::StatusUpdated(), q);
				}
			} else {
				dcassert(0);
			}

			setDirty();
		}
	}

	for(auto& i: getConn) {
		ConnectionManager::getInstance()->getDownloadConnection(i);
	}

	if(!fl_fname.empty()) {
		processList(fl_fname, d->getHintedUser(), fl_flag);
	}
}

void QueueManager::processList(const string& name, const HintedUser& user, int flags) {
	DirectoryListing dirList(user);
	try {
		dirList.loadFile(name);
	} catch(const Exception&) {
		LogManager::getInstance()->message(str(F_("Unable to open filelist: %1%") % Util::addBrackets(name)));
		return;
	}

	if(flags & QueueItem::FLAG_DIRECTORY_DOWNLOAD) {
		DirectoryItem::List dl;
		{
			Lock l(cs);
			auto dp = directories.equal_range(user) | map_values;
			dl.assign(boost::begin(dp), boost::end(dp));
			directories.erase(user);
		}

		for(auto di: dl) {
			dirList.download(di->getName(), di->getTarget(), false);
			delete di;
		}
	}
	if(flags & QueueItem::FLAG_MATCH_QUEUE) {
		size_t files = matchListing(dirList);
		LogManager::getInstance()->message(str(FN_("%1%: Matched %2% file", "%1%: Matched %2% files", files) %
			Util::toString(ClientManager::getInstance()->getNicks(user)) % files));
	}
}

void QueueManager::recheck(const string& aTarget) {
	rechecker.add(aTarget);
}

void QueueManager::remove(const string& aTarget) noexcept {
	UserList x;
	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(aTarget);
		if(!q)
			return;

		if(q->isSet(QueueItem::FLAG_DIRECTORY_DOWNLOAD)) {
			dcassert(q->getSources().size() == 1);
			auto dp = directories.equal_range(q->getSources()[0].getUser());
			for(auto i = dp.first; i != dp.second; ++i) {
				delete i->second;
			}
			directories.erase(q->getSources()[0].getUser());
		}

		if(q->isRunning()) {
			for(auto& i: q->getDownloads()) {
				x.push_back(i->getUser());
			}
		} else if(!q->getTempTarget().empty() && q->getTempTarget() != q->getTarget()) {
			File::deleteFile(q->getTempTarget());
		}

		fire(QueueManagerListener::Removed(), q);

		if(!q->isFinished()) {
			userQueue.remove(q);
		}
		fileQueue.remove(q);

		setDirty();
	}

	for(auto& i: x) {
		ConnectionManager::getInstance()->disconnect(i, CONNECTION_TYPE_DOWNLOAD);
	}
}

#define MAX_SIZE_WO_TREE 20*1024*1024

void QueueManager::removeSource(const string& aTarget, const UserPtr& aUser, int reason, bool removeConn /* = true */) noexcept {
	bool isRunning = false;
	bool removeCompletely = false;
	{
		Lock l(cs);
		QueueItem* q = fileQueue.find(aTarget);
		if(!q)
			return;

		if(!q->isSource(aUser))
			return;

		if(q->isSet(QueueItem::FLAG_USER_LIST)) {
			removeCompletely = true;
			goto endCheck;
		}

		if(reason == QueueItem::Source::FLAG_NO_TREE) {
			q->getSource(aUser)->setFlag(reason);
			if (q->getSize() < MAX_SIZE_WO_TREE) {
				return;
			}
		}

		if(q->isRunning() && userQueue.getRunning(aUser) == q) {
			isRunning = true;
			userQueue.removeDownload(q, aUser);
			fire(QueueManagerListener::StatusUpdated(), q);
		}

		if(!q->isFinished()) {
			userQueue.remove(q, aUser);
		}
		q->removeSource(aUser, reason);

		fire(QueueManagerListener::SourcesUpdated(), q);
		setDirty();
	}
endCheck:
	if(isRunning && removeConn) {
		ConnectionManager::getInstance()->disconnect(aUser, CONNECTION_TYPE_DOWNLOAD);
	}
	if(removeCompletely) {
		remove(aTarget);
	}
}

pair<size_t, int64_t> QueueManager::getQueued(const UserPtr& aUser) const {
	Lock l(cs);
	return userQueue.getQueued(aUser);
}

void QueueManager::removeSource(const UserPtr& aUser, int reason) noexcept {
	// @todo remove from finished items
	bool isRunning = false;
	string removeRunning;
	{
		Lock l(cs);
		QueueItem* qi = NULL;
		while( (qi = userQueue.getNext(aUser, QueueItem::PAUSED)) != NULL) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				remove(qi->getTarget());
			} else {
				userQueue.remove(qi, aUser);
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}

		qi = userQueue.getRunning(aUser);
		if(qi) {
			if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
				removeRunning = qi->getTarget();
			} else {
				userQueue.removeDownload(qi, aUser);
				userQueue.remove(qi, aUser);
				isRunning = true;
				qi->removeSource(aUser, reason);
				fire(QueueManagerListener::StatusUpdated(), qi);
				fire(QueueManagerListener::SourcesUpdated(), qi);
				setDirty();
			}
		}
	}

	if(isRunning) {
		ConnectionManager::getInstance()->disconnect(aUser, CONNECTION_TYPE_DOWNLOAD);
	}
	if(!removeRunning.empty()) {
		remove(removeRunning);
	}
}

void QueueManager::setPriority(const string& aTarget, QueueItem::Priority p) noexcept {
	HintedUserList getConn;
	UserList fileConnections;

	{
		Lock l(cs);

		QueueItem* q = fileQueue.find(aTarget);
		if( (q != NULL) && (q->getPriority() != p) && !q->isFinished() ) {
			if(q->getPriority() == QueueItem::PAUSED || p == QueueItem::HIGHEST) {
				// Problem, we have to request connections to all these users...
				q->getOnlineUsers(getConn);
			}

			// Cancel current connections if the intention is to pause the item
			if(p == QueueItem::PAUSED) {
				if(q->isRunning()) {
					for(auto& i: q->getDownloads()) {
						fileConnections.push_back(i->getUser());
					}
				}
			}

			userQueue.setPriority(q, p);
			setDirty();
			fire(QueueManagerListener::StatusUpdated(), q);
		}
	}

	for(auto& i: getConn) {
		ConnectionManager::getInstance()->getDownloadConnection(i);
	}

	for(auto& i: fileConnections) {
		ConnectionManager::getInstance()->disconnect(i, CONNECTION_TYPE_DOWNLOAD);
	}
}

int QueueManager::countOnlineSources(const string& aTarget) {
	Lock l(cs);

	QueueItem* qi = fileQueue.find(aTarget);
	if(!qi)
		return 0;
	int onlineSources = 0;
	for(auto& i: qi->getSources()) {
		if(i.getUser().user->isOnline())
			onlineSources++;
	}
	return onlineSources;
}

void QueueManager::saveQueue(bool force) noexcept {
	if(!dirty && !force)
		return;

	std::vector<CID> cids;

	try {
		Lock l(cs);

		File ff(getQueueFile() + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&ff);

		f.write(SimpleXML::utf8Header);
		f.write(LIT("<Downloads Version=\"" DCVERSIONSTRING "\">\r\n"));
		string tmp;
		string b32tmp;
		for(auto& i: fileQueue.getQueue()) {
			QueueItem* qi = i.second;
			if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
				f.write(LIT("\t<Download Target=\""));
				f.write(SimpleXML::escape(qi->getTarget(), tmp, true));
				f.write(LIT("\" Size=\""));
				f.write(Util::toString(qi->getSize()));
				f.write(LIT("\" Priority=\""));
				f.write(Util::toString((int)qi->getPriority()));
				f.write(LIT("\" Added=\""));
				f.write(Util::toString(qi->getAdded()));
				b32tmp.clear();
				f.write(LIT("\" TTH=\""));
				f.write(qi->getTTH().toBase32(b32tmp));
				if(!qi->getDone().empty()) {
					f.write(LIT("\" TempTarget=\""));
					f.write(SimpleXML::escape(qi->getTempTarget(), tmp, true));
				}
				f.write(LIT("\">\r\n"));

				for(auto& j: qi->getDone()) {
					f.write(LIT("\t\t<Segment Start=\""));
					f.write(Util::toString(j.getStart()));
					f.write(LIT("\" Size=\""));
					f.write(Util::toString(j.getSize()));
					f.write(LIT("\"/>\r\n"));
				}

				for(auto& j: qi->sources) {
					const CID& cid = j.getUser().user->getCID();
					const string& hint = j.getUser().hint;

					f.write(LIT("\t\t<Source CID=\""));
					f.write(cid.toBase32());
					if(!hint.empty()) {
						f.write(LIT("\" Hub=\""));
						f.write(hint);
					}
					f.write(LIT("\"/>\r\n"));

					cids.push_back(cid);
				}

				f.write(LIT("\t</Download>\r\n"));
			}
		}

		f.write(LIT("</Downloads>\r\n"));
		f.flush();
		ff.close();
		File::deleteFile(getQueueFile());
		File::renameFile(getQueueFile() + ".tmp", getQueueFile());

		dirty = false;
	} catch(const FileException&) {
		// ...
	}
	// Put this here to avoid very many saves tries when disk is full...
	lastSave = GET_TICK();

	for_each(cids, [](const CID& cid) { ClientManager::getInstance()->saveUser(cid); });
}

class QueueLoader : public SimpleXMLReader::CallBack {
public:
	QueueLoader(const CountedInputStream<false>& countedStream, uint64_t fileSize, function<void (float)> progressF) :
		countedStream(countedStream),
		streamPos(0),
		fileSize(fileSize),
		progressF(progressF),
		cur(nullptr),
		inDownloads(false)
	{ }
	void startTag(const string& name, StringPairList& attribs, bool simple);
	void endTag(const string& name);

private:
	const CountedInputStream<false>& countedStream;
	uint64_t streamPos;
	uint64_t fileSize;
	function<void (float)> progressF;

	string target;

	QueueItem* cur;
	bool inDownloads;
};

void QueueManager::loadQueue(function<void (float)> progressF) noexcept {
	try {
		Util::migrate(getQueueFile());

		File f(getQueueFile(), File::READ, File::OPEN);
		CountedInputStream<false> countedStream(&f);
		QueueLoader l(countedStream, f.getSize(), progressF);
		SimpleXMLReader(&l).parse(countedStream);
		dirty = false;
	} catch(const Exception&) {
		// ...
	}
}

static const string sDownloads = "Downloads";
static const string sDownload = "Download";
static const string sTempTarget = "TempTarget";
static const string sTarget = "Target";
static const string sSize = "Size";
static const string sDownloaded = "Downloaded";
static const string sPriority = "Priority";
static const string sSource = "Source";
static const string sAdded = "Added";
static const string sTTH = "TTH";
static const string sCID = "CID";
static const string sHubHint = "Hub";
static const string sSegment = "Segment";
static const string sStart = "Start";

void QueueLoader::startTag(const string& name, StringPairList& attribs, bool simple) {
	ScopedFunctor([this] {
		auto readBytes = countedStream.getReadBytes();
		if(readBytes != streamPos) {
			streamPos = readBytes;
			progressF(static_cast<float>(readBytes) / static_cast<float>(fileSize));
		}
	});

	QueueManager* qm = QueueManager::getInstance();
	if(!inDownloads && name == sDownloads) {
		inDownloads = true;
	} else if(inDownloads) {
		if(cur == nullptr && name == sDownload) {
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));
			if(size == 0)
				return;
			try {
				const string& tgt = getAttrib(attribs, sTarget, 0);
				// @todo do something better about existing files
				target = QueueManager::checkTarget(tgt,  /*checkExistence*/ false);
				if(target.empty())
					return;
			} catch(const Exception&) {
				return;
			}
			QueueItem::Priority p = (QueueItem::Priority)Util::toInt(getAttrib(attribs, sPriority, 3));
			time_t added = static_cast<time_t>(Util::toInt(getAttrib(attribs, sAdded, 4)));
			const string& tthRoot = getAttrib(attribs, sTTH, 5);
			if(tthRoot.empty())
				return;

			string tempTarget = getAttrib(attribs, sTempTarget, 5);
			int64_t downloaded = Util::toInt64(getAttrib(attribs, sDownloaded, 5));
			if (downloaded > size || downloaded < 0)
				downloaded = 0;

			if(SETTING(DONT_DL_ALREADY_SHARED)){
				if (ShareManager::getInstance()->isTTHShared(TTHValue(tthRoot))){
					LogManager::getInstance()->message(str(F_("The queued file %1% already exists in your share, removing from the queue") 
						% Util::addBrackets(target)));
					return;
				}
			}

			if(added == 0)
				added = GET_TIME();

			QueueItem* qi = qm->fileQueue.find(target);

			if(qi == nullptr) {
				qi = qm->fileQueue.add(target, size, 0, p, tempTarget, added, TTHValue(tthRoot));
				if(downloaded > 0) {
					qi->addSegment(Segment(0, downloaded));
				}
				qm->fire(QueueManagerListener::Added(), qi);
			}
			if(!simple)
				cur = qi;
		} else if(cur && name == sSegment) {
			int64_t start = Util::toInt64(getAttrib(attribs, sStart, 0));
			int64_t size = Util::toInt64(getAttrib(attribs, sSize, 1));

			if(size > 0 && start >= 0 && (start + size) <= cur->getSize()) {
				cur->addSegment(Segment(start, size));
			}
		} else if(cur && name == sSource) {
			const string& cid = getAttrib(attribs, sCID, 0);
			if(cid.length() != 39) {
				// Skip loading this source - sorry old users
				return;
			}
			UserPtr user = ClientManager::getInstance()->getUser(CID(cid));

			try {
				const string& hubHint = getAttrib(attribs, sHubHint, 1);
				HintedUser hintedUser(user, hubHint);
				if(qm->addSource(cur, hintedUser, 0) && user->isOnline())
					ConnectionManager::getInstance()->getDownloadConnection(hintedUser);
			} catch(const Exception&) {
				return;
			}
		}
	}
}

void QueueLoader::endTag(const string& name) {
	if(inDownloads) {
		if(name == sDownload) {
			cur = nullptr;
		} else if(name == sDownloads) {
			inDownloads = false;
		}
	}
}

void QueueManager::noDeleteFileList(const string& path) {
	if(!SETTING(KEEP_LISTS)) {
		protectedFileLists.push_back(path);
	}
}

// SearchManagerListener
void QueueManager::on(SearchManagerListener::SR, const SearchResultPtr& sr) noexcept {
	bool added = false;
	bool wantConnection = false;

	{
		Lock l(cs);
		auto matches = fileQueue.find(sr->getTTH());

		for(auto qi: matches) {
			// Size compare to avoid popular spoof
			if(qi->getSize() == sr->getSize() && !qi->isSource(sr->getUser()) && !qi->isBadSource(sr->getUser())) {
				try {
					if(!SETTING(AUTO_SEARCH_AUTO_MATCH))
						wantConnection = addSource(qi, sr->getUser(), 0);
					added = true;
				} catch(const Exception&) {
					// ...
				}
				break;
			}
		}
	}

	if(added && SETTING(AUTO_SEARCH_AUTO_MATCH)) {
		try {
			addList(sr->getUser(), QueueItem::FLAG_MATCH_QUEUE);
		} catch(const Exception&) {
			// ...
		}
	}
	if(added && sr->getUser().user->isOnline() && wantConnection) {
		ConnectionManager::getInstance()->getDownloadConnection(sr->getUser());
	}
}

// ClientManagerListener
void QueueManager::on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept {
	bool hasDown = false;
	{
		Lock l(cs);
		for(int i = 0; i < QueueItem::LAST; ++i) {
			auto j = userQueue.getList(i).find(aUser);
			if(j != userQueue.getList(i).end()) {
				for(auto& m: j->second)
					fire(QueueManagerListener::StatusUpdated(), m);
				if(i != QueueItem::PAUSED)
					hasDown = true;
			}
		}
	}

	if(hasDown) {
		// the user just came on, so there's only 1 possible hub, no need for a hint
		ConnectionManager::getInstance()->getDownloadConnection(HintedUser(aUser, Util::emptyString));
	}
}

void QueueManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
	Lock l(cs);
	for(int i = 0; i < QueueItem::LAST; ++i) {
		auto j = userQueue.getList(i).find(aUser);
		if(j != userQueue.getList(i).end()) {
			for(auto& m: j->second)
				fire(QueueManagerListener::StatusUpdated(), m);
		}
	}
}

void QueueManager::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
	if(dirty && ((lastSave + 10000) < aTick)) {
		saveQueue();
	}
}

bool QueueManager::checkSfv(QueueItem* qi, Download* d) {
	SFVReader sfv(qi->getTarget());

	if(sfv.hasCRC()) {
		bool crcMatch = false;
		try {
			crcMatch = (calcCrc32(qi->getTempTarget()) == sfv.getCRC());
		} catch(const FileException& ) {
			// Couldn't read the file to get the CRC(!!!)
		}

		if(!crcMatch) {
			/// @todo There is a slight chance that something happens with a file while it's being saved to disk 
			/// maybe calculate tth along with crc and if tth is ok and crc is not flag the file as bad at once
			/// if tth mismatches (possible disk error) then repair / redownload the file

			File::deleteFile(qi->getTempTarget());
			qi->resetDownloaded();
			dcdebug("QueueManager: CRC32 mismatch for %s\n", qi->getTarget().c_str());
			LogManager::getInstance()->message(str(F_("CRC32 inconsistency (SFV-Check): %1%") % Util::addBrackets(qi->getTarget()))); 

			setPriority(qi->getTarget(), QueueItem::PAUSED);

			QueueItem::SourceList sources = qi->getSources();
			for(auto& i: sources) {
				removeSource(qi->getTarget(), i.getUser(), QueueItem::Source::FLAG_CRC_FAILED, false);
			}

			fire(QueueManagerListener::CRCFailed(), d, _("CRC32 inconsistency (SFV-Check)"));
		} else {
			dcdebug("QueueManager: CRC32 match for %s\n", qi->getTarget().c_str());
			fire(QueueManagerListener::CRCChecked(), d);
		}
		return true;
	}
	return false;
}

uint32_t QueueManager::calcCrc32(const string& file) {
	CRC32Filter crc32;
	FileReader(true).read(file, [&](const void* x, size_t n) {
		return crc32(x, n), true;
	});
	return crc32.getValue();
}

void QueueManager::logFinishedDownload(QueueItem* qi, Download* d, bool crcChecked)
{
	ParamMap params;
	params["target"] = qi->getTarget();
	params["fileSI"] = Util::toString(qi->getSize());
	params["fileSIshort"] = Util::formatBytes(qi->getSize());
	params["fileTR"] = qi->getTTH().toBase32();
	params["sfv"] = Util::toString(crcChecked ? 1 : 0);

	FinishedManager::getInstance()->getParams(qi->getTarget(), params);

	LOG(LogManager::FINISHED_DOWNLOAD, params);
}
} // namespace dcpp
