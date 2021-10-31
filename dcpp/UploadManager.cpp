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
#include "UploadManager.h"

#include <cmath>

#include "ConnectionManager.h"
#include "LogManager.h"
#include "ShareManager.h"
#include "ClientManager.h"
#include "FilteredFile.h"
#include "ZUtils.h"
#include "HashManager.h"
#include "AdcCommand.h"
#include "FavoriteManager.h"
#include "CryptoManager.h"
#include "Upload.h"
#include "UserConnection.h"
#include "File.h"

namespace dcpp {

UploadManager::UploadManager() noexcept : running(0), extra(0), lastGrant(0), lastFreeSlots(-1) {
	ClientManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
}

UploadManager::~UploadManager() {
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	while(true) {
		{
			Lock l(cs);
			if(uploads.empty())
				break;
		}
		Thread::sleep(100);
	}
}

bool UploadManager::prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aStartPos, int64_t aBytes, bool listRecursive) {
	dcdebug("Preparing %s %s " I64_FMT " " I64_FMT " %d\n", aType.c_str(), aFile.c_str(), aStartPos, aBytes, listRecursive);

	if(aFile.empty() || aStartPos < 0 || aBytes < -1 || aBytes == 0) {
		aSource.fileNotAvail("Invalid request");
		return false;
	}

	/* do some preliminary checking of the transfer request that doesn't involve any filesystem
	access. we want to know the type of the upload to see if the user deserves a mini-slot. */

	bool miniSlot;

	string sourceFile;
	Transfer::Type type;

	try {
		if(aType == Transfer::names[Transfer::TYPE_FILE]) {
			auto info = ShareManager::getInstance()->toRealWithSize(aFile);
			sourceFile = move(info.first);
			type = (aFile == Transfer::USER_LIST_NAME_BZ || aFile == Transfer::USER_LIST_NAME) ?
				Transfer::TYPE_FULL_LIST : Transfer::TYPE_FILE;
			miniSlot = type == Transfer::TYPE_FULL_LIST || info.second <= static_cast<int64_t>(SETTING(SET_MINISLOT_SIZE) * 1024);

		} else if(aType == Transfer::names[Transfer::TYPE_TREE]) {
			sourceFile = ShareManager::getInstance()->toReal(aFile);
			type = Transfer::TYPE_TREE;
			miniSlot = true;

		} else if(aType == Transfer::names[Transfer::TYPE_PARTIAL_LIST]) {
			sourceFile = _("Partial file list");
			type = Transfer::TYPE_PARTIAL_LIST;
			miniSlot = true;

		} else {
			aSource.fileNotAvail("Unknown file type");
			return false;
		}
	} catch(const ShareException& e) {
		aSource.fileNotAvail(e.getError());
		return false;
	}

	/* let's see if a slot is available to serve the upload. */

	bool extraSlot = false;
	bool gotFullSlot = false;

	if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
		bool hasReserved = hasReservedSlot(aSource.getUser());
		bool isFavorite = FavoriteManager::getInstance()->hasSlot(aSource.getUser());
		bool hasFreeSlot = [&]() -> bool { Lock l(cs); return (getFreeSlots() > 0) && ((waitingFiles.empty() && connectingUsers.empty()) || isConnecting(aSource.getUser())); }();

		if(!(hasReserved || isFavorite || getAutoSlot() || hasFreeSlot)) {
			bool supportsMini = aSource.isSet(UserConnection::FLAG_SUPPORTS_MINISLOTS);
			bool allowedMini = aSource.isSet(UserConnection::FLAG_HASEXTRASLOT) || aSource.isSet(UserConnection::FLAG_OP) || getFreeExtraSlots() > 0;
			if(miniSlot && supportsMini && allowedMini) {
				extraSlot = true;
			} else {

				// Check for tth root identifier
				string tFile = aFile;
				if (tFile.compare(0, 4, "TTH/") == 0)
					tFile = ShareManager::getInstance()->toVirtual(TTHValue(aFile.substr(4)));

				aSource.maxedOut(addFailedUpload(aSource, tFile +
					" (" +  Util::formatBytes(aStartPos) + " - " + Util::formatBytes(aStartPos + aBytes) + ")"));
				aSource.disconnect();
				return false;
			}
		} else {
			gotFullSlot = true;
		}
	}

	/* cool! the request is correct, and the user can have a slot. let's prepare the file. */

	InputStream* is = 0;
	int64_t start = 0;
	int64_t size = 0;

	try {
		switch(type) {
		case Transfer::TYPE_FILE:
		case Transfer::TYPE_FULL_LIST:
			{
				if(aFile == Transfer::USER_LIST_NAME) {
					// Unpack before sending...
					string bz2 = File(sourceFile, File::READ, File::OPEN).read();
					string xml;
					CryptoManager::getInstance()->decodeBZ2(reinterpret_cast<const uint8_t*>(bz2.data()), bz2.size(), xml);
					is = new MemoryInputStream(xml);
					start = 0;
					size = xml.size();

				} else {
					File* f = new File(sourceFile, File::READ, File::OPEN);

					start = aStartPos;
					int64_t sz = f->getSize();
					size = (aBytes == -1) ? sz - start : aBytes;

					if((start + size) > sz) {
						aSource.fileNotAvail();
						delete f;
						return false;
					}

					f->setPos(start);
					is = f;
					if((start + size) < sz) {
						is = new LimitedInputStream<true>(is, size);
					}
				}
				break;
			}

		case Transfer::TYPE_TREE:
			{
				MemoryInputStream* mis = ShareManager::getInstance()->getTree(aFile);
				if(!mis) {
					aSource.fileNotAvail();
					return false;
				}

				start = 0;
				size = mis->getSize();
				is = mis;
				break;
			}

		case Transfer::TYPE_PARTIAL_LIST:
			{
				// Partial file list
				MemoryInputStream* mis = ShareManager::getInstance()->generatePartialList(aFile, listRecursive);
				if(!mis) {
					aSource.fileNotAvail();
					return false;
				}

				start = 0;
				size = mis->getSize();
				is = mis;
				break;
			}
		case Transfer::TYPE_LAST: break;
		}

	} catch(const ShareException& e) {
		aSource.fileNotAvail(e.getError());
		return false;
	} catch(const Exception& e) {
		LogManager::getInstance()->message(str(F_("Unable to send file %1%: %2%") % Util::addBrackets(sourceFile) % e.getError()));
		aSource.fileNotAvail();
		return false;
	}

	Lock l(cs);

	Upload* u = new Upload(aSource, sourceFile, TTHValue());
	u->setStream(is);
	u->setSegment(Segment(start, size));

	u->setType(type);

	uploads.push_back(u);

	/* the upload is all set. update slot counts if the user just gained a slot. */

	if(!aSource.isSet(UserConnection::FLAG_HASSLOT)) {
		setLastGrant(GET_TICK());

		if(extraSlot) {
			if(!aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource.setFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra++;
			}
		} else {
			if(aSource.isSet(UserConnection::FLAG_HASEXTRASLOT)) {
				aSource.unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
				extra--;
			}
			aSource.setFlag(UserConnection::FLAG_HASSLOT);
			running++;
		}

		reservedSlots.erase(aSource.getUser());

		if(gotFullSlot) {
			clearUserFiles(aSource.getUser());	// this user is using a full slot, nix them.

			// remove user from connecting list
			auto cu = connectingUsers.find(aSource.getUser());
			if(cu != connectingUsers.end()) {
				connectingUsers.erase(cu);
			}
		}
	}

	return true;
}

int64_t UploadManager::getRunningAverage() {
	Lock l(cs);
	int64_t avg = 0;
	for(auto u: uploads) {
		avg += u->getAverageSpeed();
	}
	return avg;
}

bool UploadManager::getAutoSlot() {
	/** A 0 in settings means disable */
	if(SETTING(MIN_UPLOAD_SPEED) == 0)
		return false;
	/** Only grant one slot per 30 sec */
	if(GET_TICK() < getLastGrant() + 30*1000)
		return false;
	/** Grant if upload speed is less than the threshold speed */
	return getRunningAverage() < (SETTING(MIN_UPLOAD_SPEED)*1024);
}

void UploadManager::removeUpload(Upload* aUpload) {
	Lock l(cs);
	dcassert(find(uploads.begin(), uploads.end(), aUpload) != uploads.end());
	uploads.erase(remove(uploads.begin(), uploads.end(), aUpload), uploads.end());
	delete aUpload;
}

void UploadManager::reserveSlot(const HintedUser& aUser) {
	{
		Lock l(cs);
		reservedSlots.insert(aUser);
	}

	if(aUser.user->isOnline()) {
		auto userToken = [&] () -> string
		{
			Lock l(cs);
			auto it = find_if(waitingUsers.cbegin(), waitingUsers.cend(), [&](const UserPtr& u) { return u == aUser.user; });
			return (it != waitingUsers.cend()) ? it->token : Util::emptyString;
		};
		
		string token;
		if((token = userToken()) != Util::emptyString)
			ClientManager::getInstance()->connect(aUser, token);
	}
}

bool UploadManager::hasReservedSlot(const UserPtr& user) const {
	Lock l(cs);
	return reservedSlots.find(user) != reservedSlots.end();
}

void UploadManager::on(UserConnectionListener::Get, UserConnection* aSource, const string& aFile, int64_t aResume) noexcept {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM::onGet Bad state, ignoring\n");
		return;
	}

	if(prepareFile(*aSource, Transfer::names[Transfer::TYPE_FILE], Util::toAdcFile(aFile), aResume, -1)) {
		aSource->setState(UserConnection::STATE_SEND);
		aSource->fileLength(Util::toString(aSource->getUpload()->getSize()));
	}
}

void UploadManager::on(UserConnectionListener::Send, UserConnection* aSource) noexcept {
	if(aSource->getState() != UserConnection::STATE_SEND) {
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}

	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	u->setStart(GET_TICK());
	u->tick();
	aSource->setState(UserConnection::STATE_RUNNING);
	aSource->transmitFile(u->getStream());
	fire(UploadManagerListener::Starting(), u);
}

void UploadManager::on(AdcCommand::GET, UserConnection* aSource, const AdcCommand& c) noexcept {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM::onGET Bad state, ignoring\n");
		return;
	}

	const string& type = c.getParam(0);
	const string& fname = c.getParam(1);
	int64_t aStartPos = Util::toInt64(c.getParam(2));
	int64_t aBytes = Util::toInt64(c.getParam(3));

	if(prepareFile(*aSource, type, fname, aStartPos, aBytes, c.hasFlag("RE", 4))) {
		Upload* u = aSource->getUpload();
		dcassert(u != NULL);

		AdcCommand cmd(AdcCommand::CMD_SND);
		cmd.addParam(type).addParam(fname)
			.addParam(Util::toString(u->getStartPos()))
			.addParam(Util::toString(u->getSize()));

		if(c.hasFlag("ZL", 4)) {
			u->setStream(new FilteredInputStream<ZFilter, true>(u->getStream()));
			u->setFlag(Upload::FLAG_ZUPLOAD);
			cmd.addParam("ZL1");
		}

		aSource->send(cmd);

		u->setStart(GET_TICK());
		u->tick();
		aSource->setState(UserConnection::STATE_RUNNING);
		aSource->transmitFile(u->getStream());
		fire(UploadManagerListener::Starting(), u);
	}
}

void UploadManager::on(UserConnectionListener::BytesSent, UserConnection* aSource, size_t aBytes, size_t aActual) noexcept {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);
	u->addPos(aBytes, aActual);
	u->tick();
}

void UploadManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) noexcept {
	Upload* u = aSource->getUpload();

	if(u) {
		fire(UploadManagerListener::Failed(), u, aError);

		dcdebug("UM::onFailed (%s): Removing upload\n", aError.c_str());
		removeUpload(u);
	}

	removeConnection(aSource);
}

void UploadManager::on(UserConnectionListener::TransmitDone, UserConnection* aSource) noexcept {
	dcassert(aSource->getState() == UserConnection::STATE_RUNNING);
	Upload* u = aSource->getUpload();
	dcassert(u != NULL);

	aSource->setState(UserConnection::STATE_GET);

	if(SETTING(LOG_UPLOADS) && u->getType() != Transfer::TYPE_TREE && (SETTING(LOG_FILELIST_TRANSFERS) || u->getType() != Transfer::TYPE_FULL_LIST)) {
		ParamMap params;
		u->getParams(*aSource, params);
		LOG(LogManager::UPLOAD, params);
	}

	fire(UploadManagerListener::Complete(), u);
	removeUpload(u);
}

size_t UploadManager::addFailedUpload(const UserConnection& source, string filename) {
	size_t queue_position = 0;
	{
		Lock l(cs);
		auto it = find_if(waitingUsers.begin(), waitingUsers.end(), [&](const UserPtr& u) -> bool { ++queue_position; return u == source.getUser(); });
		if (it==waitingUsers.end()) {
			waitingUsers.emplace_back(source.getHintedUser(), source.getToken());
		}
		waitingFiles[source.getUser()].insert(filename);		//files for which user's asked
	}
	fire(UploadManagerListener::WaitingAddFile(), source.getHintedUser(), filename);
	return queue_position;
}

void UploadManager::clearUserFiles(const UserPtr& source) {
	Lock l(cs);
	//run this when a user's got a slot or goes offline.
	auto sit = find_if(waitingUsers.begin(), waitingUsers.end(), [&source](const UserPtr& other) { return other == source; });
	if (sit == waitingUsers.end()) return;

	auto fit = waitingFiles.find(sit->user);
	if (fit != waitingFiles.end()) waitingFiles.erase(fit);
	fire(UploadManagerListener::WaitingRemoveUser(), sit->user);

	waitingUsers.erase(sit);
}

HintedUserList UploadManager::getWaitingUsers() const {
	Lock l(cs);
	HintedUserList u;
	for(auto i = waitingUsers.begin(), iend = waitingUsers.end(); i != iend; ++i) {
		u.push_back(i->user);
	}
	return u;
}

UploadManager::FileSet UploadManager::getWaitingUserFiles(const UserPtr &u) const {
	Lock l(cs);
	auto i = waitingFiles.find(u);
	if(i == waitingFiles.end()) {
		return FileSet();
	}

	return i->second;
}

bool UploadManager::isWaiting(const UserPtr &u) const {
	Lock l(cs);
	return waitingFiles.find(u) != waitingFiles.end();
}

void UploadManager::addConnection(UserConnectionPtr conn) {
	conn->addListener(this);
	conn->setState(UserConnection::STATE_GET);
}

void UploadManager::removeConnection(UserConnection* aSource) {
	dcassert(aSource->getUpload() == NULL);
	aSource->removeListener(this);
	if(aSource->isSet(UserConnection::FLAG_HASSLOT)) {
		running--;
		aSource->unsetFlag(UserConnection::FLAG_HASSLOT);
	}
	if(aSource->isSet(UserConnection::FLAG_HASEXTRASLOT)) {
		extra--;
		aSource->unsetFlag(UserConnection::FLAG_HASEXTRASLOT);
	}
}

void UploadManager::notifyQueuedUsers() {
	vector<WaitingUser> notifyList;
	{
		Lock l(cs);
		if (waitingUsers.empty()) return;		//no users to notify
		
		int freeslots = getFreeSlots();
		if(freeslots > 0)
		{
			freeslots -= connectingUsers.size();
			while(!waitingUsers.empty() && freeslots > 0) {
				// let's keep him in the connectingList until he asks for a file
				WaitingUser wu = waitingUsers.front();
				clearUserFiles(wu.user);
				if(wu.user.user->isOnline()) {
					connectingUsers[wu.user] = GET_TICK();
					notifyList.push_back(wu);
					freeslots--;
				}
			}
		}
	}
	for(auto it = notifyList.cbegin(); it != notifyList.cend(); ++it)
		ClientManager::getInstance()->connect(it->user, it->token);
}

void UploadManager::on(TimerManagerListener::Minute, uint64_t  aTick ) noexcept {
	UserList disconnects;
	{
		Lock l(cs);

		for(auto i = connectingUsers.begin(); i != connectingUsers.end();) {
			if((i->second + (90 * 1000)) < aTick) {
				clearUserFiles(i->first);
				connectingUsers.erase(i++);
			} else {
				++i;
			}
		}

		if( SETTING(AUTO_KICK) ) {
			for(auto u: uploads) {
				if(u->getUser()->isOnline()) {
					u->unsetFlag(Upload::FLAG_PENDING_KICK);
					continue;
				}

				if(u->isSet(Upload::FLAG_PENDING_KICK)) {
					disconnects.push_back(u->getUser());
					continue;
				}

				if(SETTING(AUTO_KICK_NO_FAVS) && FavoriteManager::getInstance()->isFavoriteUser(u->getUser())) {
					continue;
				}

				u->setFlag(Upload::FLAG_PENDING_KICK);
			}
		}
	}

	for(auto& i: disconnects) {
		LogManager::getInstance()->message(str(F_("Disconnected user leaving the hub: %1%") %
			Util::toString(ClientManager::getInstance()->getNicks(i->getCID()))));
		ConnectionManager::getInstance()->disconnect(i, CONNECTION_TYPE_UPLOAD);
	}

	int freeSlots = getFreeSlots();
	if(freeSlots != lastFreeSlots) {
		lastFreeSlots = freeSlots;
		ClientManager::getInstance()->infoUpdated();
	}
}

void UploadManager::on(GetListLength, UserConnection* conn) noexcept {
	conn->error("GetListLength not supported");
	conn->disconnect(false);
}

void UploadManager::on(AdcCommand::GFI, UserConnection* aSource, const AdcCommand& c) noexcept {
	if(aSource->getState() != UserConnection::STATE_GET) {
		dcdebug("UM::onSend Bad state, ignoring\n");
		return;
	}

	if(c.getParameters().size() < 2) {
		aSource->send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_PROTOCOL_GENERIC, "Missing parameters"));
		return;
	}

	const string& type = c.getParam(0);
	const string& ident = c.getParam(1);

	if(type == Transfer::names[Transfer::TYPE_FILE]) {
		try {
			aSource->send(ShareManager::getInstance()->getFileInfo(ident));
		} catch(const ShareException&) {
			aSource->fileNotAvail();
		}
	} else {
		aSource->fileNotAvail();
	}
}

// TimerManagerListener
void UploadManager::on(TimerManagerListener::Second, uint64_t) noexcept {
	{
		Lock l(cs);
		UploadList ticks;

		for(auto u: uploads) {
			if(u->getPos() > 0) {
				ticks.push_back(u);
				u->tick();
			}
		}

		if(!uploads.empty())
			fire(UploadManagerListener::Tick(), UploadList(uploads));
	}
		
	notifyQueuedUsers();
}

void UploadManager::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
	if(!aUser->isOnline()) {
		clearUserFiles(aUser);
	}
}

} // namespace dcpp
