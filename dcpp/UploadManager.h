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

#ifndef DCPLUSPLUS_DCPP_UPLOAD_MANAGER_H
#define DCPLUSPLUS_DCPP_UPLOAD_MANAGER_H

#include <algorithm>
#include <list>
#include <set>
#include <unordered_map>

#include "forward.h"
#include "UserConnectionListener.h"
#include "Singleton.h"
#include "UploadManagerListener.h"
#include "ClientManagerListener.h"
#include "User.h"
#include "TimerManager.h"
#include "Speaker.h"
#include "SettingsManager.h"
#include "HintedUser.h"
#include "UserConnection.h"
#include "GetSet.h"

namespace dcpp {

using std::max;
using std::list;
using std::set;
using std::unordered_map;

struct WaitingUser {
	HintedUser user;
	string token;
	
	WaitingUser(const HintedUser& _user, const std::string& _token) : user(_user), token(_token) { }
	
	operator const UserPtr&() const { return user.user; }
};

class UploadManager : private ClientManagerListener, private UserConnectionListener, public Speaker<UploadManagerListener>, private TimerManagerListener, public Singleton<UploadManager>
{
public:
	/** @return Number of uploads. */
	size_t getUploadCount() { Lock l(cs); return uploads.size(); }

	/**
	 * @remarks This is only used in the tray icons. Could be used in
	 * MainFrame too.
	 *
	 * @return Running average download speed in Bytes/s
	 */
	int64_t getRunningAverage();

	/** @return Number of free slots. */
	int getFreeSlots() { return max((SETTING(SLOTS) - running), 0); }

	/** @internal */
	int getFreeExtraSlots() { return max(max(SETTING(MAX_EXTRA_SLOTS), 1) - getExtra(), 0); }

	/** @param aUser Reserve an upload slot for this user and connect. */
	void reserveSlot(const HintedUser& aUser);
	bool isConnecting(const UserPtr& aUser) const { return connectingUsers.find(aUser) != connectingUsers.end(); }

	typedef set<string> FileSet;
	typedef unordered_map<UserPtr, FileSet, User::Hash> FilesMap;
	void clearUserFiles(const UserPtr&);
	HintedUserList getWaitingUsers() const;
	bool isWaiting(const UserPtr &) const;
	FileSet getWaitingUserFiles(const UserPtr&) const;

	/** @internal */
	void addConnection(UserConnectionPtr conn);

	GETSET(int, running, Running);
	GETSET(int, extra, Extra);
	GETSET(uint64_t, lastGrant, LastGrant);
	
private:
	UploadList uploads;
	mutable CriticalSection cs;

	typedef unordered_set<UserPtr, User::Hash> SlotSet;
	typedef SlotSet::iterator SlotIter;
	SlotSet reservedSlots;
	
	typedef unordered_map<UserPtr, uint64_t, User::Hash> SlotMap;
	typedef SlotMap::iterator SlotMapIter;
	SlotMap connectingUsers;

	int lastFreeSlots; /// amount of free slots at the previous minute

	typedef list<WaitingUser> WaitingUserList;

	//functions for manipulating waitingFiles and waitingUsers
	WaitingUserList waitingUsers;		//this one merely lists the users waiting for slots
	FilesMap waitingFiles;		//set of files which this user has asked for
	size_t addFailedUpload(const UserConnection& source, string filename);
	void notifyQueuedUsers();

	friend class Singleton<UploadManager>;
	UploadManager() noexcept;
	virtual ~UploadManager();

	bool hasReservedSlot(const UserPtr& user) const;
	bool getAutoSlot();
	void removeConnection(UserConnection* aConn);
	void removeUpload(Upload* aUpload);

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;

	// TimerManagerListener
	virtual void on(Second, uint64_t aTick) noexcept;
	virtual void on(Minute, uint64_t aTick) noexcept;

	// UserConnectionListener
	virtual void on(BytesSent, UserConnection*, size_t, size_t) noexcept;
	virtual void on(Failed, UserConnection*, const string&) noexcept;
	virtual void on(Get, UserConnection*, const string&, int64_t) noexcept;
	virtual void on(Send, UserConnection*) noexcept;
	virtual void on(GetListLength, UserConnection* conn) noexcept;
	virtual void on(TransmitDone, UserConnection*) noexcept;

	virtual void on(AdcCommand::GET, UserConnection*, const AdcCommand&) noexcept;
	virtual void on(AdcCommand::GFI, UserConnection*, const AdcCommand&) noexcept;

	bool prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aResume, int64_t aBytes, bool listRecursive = false);
};

} // namespace dcpp

#endif // !defined(UPLOAD_MANAGER_H)
