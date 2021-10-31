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

#include "stdinc.h"
#include "QueueItem.h"

#include "HashManager.h"
#include "Download.h"
#include "File.h"
#include "Util.h"

namespace dcpp {

namespace {
	const string TEMP_EXTENSION = ".dctmp";

	string getTempName(const string& aFileName, const TTHValue& aRoot) {
		string tmp(aFileName);
		tmp += "." + aRoot.toBase32();
		tmp += TEMP_EXTENSION;
		return tmp;
	}
}

int QueueItem::countOnlineUsers() const {
	int n = 0;
	for(auto& i: sources) {
		if(i.getUser().user->isOnline())
			n++;
	}
	return n;
}

void QueueItem::getOnlineUsers(HintedUserList& l) const {
	for(auto& i: sources)
		if(i.getUser().user->isOnline())
			l.push_back(i.getUser());
}

void QueueItem::addSource(const HintedUser& aUser) {
	dcassert(!isSource(aUser.user));
	auto i = getBadSource(aUser);
	if(i != badSources.end()) {
		sources.push_back(*i);
		badSources.erase(i);
	} else {
		sources.emplace_back(aUser);
	}
}

void QueueItem::removeSource(const UserPtr& aUser, int reason) {
	auto i = getSource(aUser);
	dcassert(i != sources.end());
	i->setFlag(reason);
	badSources.push_back(*i);
	sources.erase(i);
}

const string& QueueItem::getTempTarget() {
	if(!isSet(QueueItem::FLAG_USER_LIST) && tempTarget.empty()) {
		if(!SETTING(TEMP_DOWNLOAD_DIRECTORY).empty() && (File::getSize(getTarget()) == -1)) {
#ifdef _WIN32
			ParamMap params;
			if(target.length() >= 3 && target[1] == ':' && target[2] == '\\')
				params["targetdrive"] = target.substr(0, 3);
			else
				params["targetdrive"] = Util::getPath(Util::PATH_USER_LOCAL).substr(0, 3);
			setTempTarget(Util::formatParams(SETTING(TEMP_DOWNLOAD_DIRECTORY), params) + getTempName(getTargetFileName(), getTTH()));
#else //_WIN32
			setTempTarget(SETTING(TEMP_DOWNLOAD_DIRECTORY) + getTempName(getTargetFileName(), getTTH()));
#endif //_WIN32
		}
	}
	return tempTarget;
}

QueueData* QueueItem::getPluginObject() noexcept {
	resetEntity();

	pod.target = pluginString(getTarget());
	pod.location = isFinished() ? pod.target : pluginString(getTempTarget());
	pod.hash = pluginString(tthRoot.toBase32());
	pod.object = this;
	pod.size = size;
	pod.isFileList = isSet(QueueItem::FLAG_USER_LIST) ? True : False;

	return &pod;
}

Segment QueueItem::getNextSegment(int64_t blockSize, int64_t wantedSize) const {
	if(getSize() == -1 || blockSize == 0) {
		return Segment(0, -1);
	}

	if(!SETTING(SEGMENTED_DL)) {
		if(!downloads.empty()) {
			return Segment(0, 0);
		}

		int64_t start = 0;
		int64_t end = getSize();

		if(!done.empty()) {
			const Segment& first = *done.begin();

			if(first.getStart() > 0) {
				end = Util::roundUp(first.getStart(), blockSize);
			} else {
				start = Util::roundDown(first.getEnd(), blockSize);

				if(done.size() > 1) {
					const Segment& second = *(++done.begin());
					end = Util::roundUp(second.getStart(), blockSize);
				}
			}
		}

		return Segment(start, std::min(getSize(), end) - start);
	}

	double donePart = static_cast<double>(getDownloadedBytes()) / getSize();

	// We want smaller blocks at the end of the transfer, squaring gives a nice curve...
	int64_t targetSize = wantedSize * std::max(0.25, (1. - (donePart * donePart)));

	if(targetSize > blockSize) {
		// Round off to nearest block size
		targetSize = Util::roundDown(targetSize, blockSize);
	} else {
		targetSize = blockSize;
	}

	int64_t start = 0;
	int64_t curSize = targetSize;

	while(start < getSize()) {
		int64_t end = std::min(getSize(), start + curSize);
		Segment block(start, end - start);
		bool overlaps = false;
		for(auto i = done.begin(); !overlaps && i != done.end(); ++i) {
			if(curSize <= blockSize) {
				int64_t dstart = i->getStart();
				int64_t dend = i->getEnd();
				// We accept partial overlaps, only consider the block done if it is fully consumed by the done block
				if(dstart <= start && dend >= end) {
					overlaps = true;
				}
			} else {
				overlaps = block.overlaps(*i);
			}
		}

		for(auto i = downloads.begin(); !overlaps && i !=downloads.end(); ++i) {
			overlaps = block.overlaps((*i)->getSegment());
		}

		if(!overlaps) {
			return block;
		}

		if(curSize > blockSize) {
			curSize -= blockSize;
		} else {
			start = end;
			curSize = targetSize;
		}
	}

	return Segment(0, 0);
}

int64_t QueueItem::getDownloadedBytes() const {
	int64_t total = 0;
	for(auto& i: done) {
		total += i.getSize();
	}
	return total;
}

void QueueItem::addSegment(const Segment& segment) {
	done.insert(segment);

	// Consolidate segments
	if(done.size() == 1)
		return;

	for(auto i = ++done.begin() ; i != done.end(); ) {
		auto prev = i;
		prev--;
		if(prev->getEnd() >= i->getStart()) {
			Segment big(prev->getStart(), i->getEnd() - prev->getStart());
			done.erase(prev);
			done.erase(i++);
			done.insert(big);
		} else {
			++i;
		}
	}
}

string QueueItem::getListName() const {
	dcassert(isSet(QueueItem::FLAG_USER_LIST));
	if(isSet(QueueItem::FLAG_XML_BZLIST)) {
		return getTarget() + ".xml.bz2";
	} else {
		return getTarget() + ".xml";
	}
}

}
