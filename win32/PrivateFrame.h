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

#ifndef DCPLUSPLUS_WIN32_PRIVATE_FRAME_H
#define DCPLUSPLUS_WIN32_PRIVATE_FRAME_H

#include <dcpp/ClientManagerListener.h>
#include <dcpp/ConnectionManagerListener.h>
#include <dcpp/CriticalSection.h>
#include <dcpp/User.h>
#include <dcpp/UserConnectionListener.h>

#include "MDIChildFrame.h"
#include "IRecent.h"
#include "AspectChat.h"
#include "UserInfoBase.h"
#include "AspectUserCommand.h"

class PrivateFrame :
	public MDIChildFrame<PrivateFrame>,
	public IRecent<PrivateFrame>,
	private ClientManagerListener,
	private ConnectionManagerListener,
	private UserConnectionListener,
	public AspectChat<PrivateFrame>,
	public AspectUserInfo<PrivateFrame>,
	public AspectUserCommand<PrivateFrame>
{
	typedef MDIChildFrame<PrivateFrame> BaseType;
	typedef AspectChat<PrivateFrame> ChatType;

	friend class MDIChildFrame<PrivateFrame>;
	friend class AspectChat<PrivateFrame>;
	friend class AspectUserInfo<PrivateFrame>;
	friend class AspectUserCommand<PrivateFrame>;

	using IRecent<PrivateFrame>::setText;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_CHANNEL,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

	/// @return whether a new window can be opened (wrt the "Max PM windows" setting).
	static bool gotMessage(TabViewPtr parent, const ChatMessage& message, const string& hubHint, bool fromBot);
	static void openWindow(TabViewPtr parent, const HintedUser& replyTo, const tstring& msg = Util::emptyStringT,
		const string& logPath = Util::emptyString, bool activate = true);
	static void activateWindow(const UserPtr& u);
	static bool isOpen(const UserPtr& u) { return frames.find(u) != frames.end(); }
	static void closeAll(bool offline);

	WindowParams getWindowParams() const;
	static void parseWindowParams(TabViewPtr parent, const WindowParams& params);
	static bool isFavorite(const WindowParams& params);

	void sendMessage(const tstring& msg, bool thirdPerson = false);

private:
	UserInfoBase replyTo;
	bool online;

	mutable CriticalSection mutex;
	UserConnection* conn;

	time_t lastMessageTime;

	ParamMap ucLineParams;

	typedef unordered_map<UserPtr, PrivateFrame*, User::Hash> FrameMap;
	static FrameMap frames;

	PrivateFrame(TabViewPtr parent, const HintedUser& replyTo_, const string& logPath = Util::emptyString);
	virtual ~PrivateFrame();

	void layout();
	bool preClosing();

	string getLogPath() const;
	void openLog();
	void fillLogParams(ParamMap& params) const;
	void addedChat(const tstring& message);
	void addedChat(const ChatMessage& message);
	void addStatus(const tstring& text);
	void updateOnlineStatus(bool newChannel = false);
	void updateChannel();
	void startCC(bool silent = false);
	void closeCC(bool silent = false);
	bool ccReady() const;

	bool handleChatContextMenu(dwt::ScreenCoordinate pt);
	void handleChannelMenu();

	void runUserCommand(const UserCommand& uc);

	// MDIChildFrame
	void tabMenuImpl(dwt::Menu* menu);

	// AspectChat
	void enterImpl(const tstring& s);

	// AspectUserInfo
	UserInfoList selectedUsersImpl();

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept;
	virtual void on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept;
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept;

	// ConnectionManagerListener
	virtual void on(ConnectionManagerListener::Connected, ConnectionQueueItem* cqi, UserConnection* uc) noexcept;
	virtual void on(ConnectionManagerListener::Removed, ConnectionQueueItem* cqi) noexcept;

	// UserConnectionListener
	virtual void on(UserConnectionListener::PrivateMessage, UserConnection* uc, const ChatMessage& message) noexcept;
};

#endif // !defined(PRIVATE_FRAME_H)
