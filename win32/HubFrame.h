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

#ifndef DCPLUSPLUS_WIN32_HUB_FRAME_H
#define DCPLUSPLUS_WIN32_HUB_FRAME_H

#include <functional>

#include <dcpp/forward.h>
#include <dcpp/OnlineUser.h>
#include <dcpp/ClientListener.h>
#include <dcpp/User.h>
#include <dcpp/FavoriteManagerListener.h>

#include "AspectChat.h"
#include "AspectUserCommand.h"
#include "ListFilter.h"
#include "IRecent.h"
#include "MDIChildFrame.h"
#include "UserInfoBase.h"

using std::function;

class HubFrame :
	public MDIChildFrame<HubFrame>,
	public IRecent<HubFrame>,
	private ClientListener,
	private FavoriteManagerListener,
	public AspectChat<HubFrame>,
	public AspectUserInfo<HubFrame>,
	public AspectUserCommand<HubFrame>
{
	typedef MDIChildFrame<HubFrame> BaseType;
	typedef AspectChat<HubFrame> ChatType;

	friend class MDIChildFrame<HubFrame>;
	friend class AspectChat<HubFrame>;
	friend class AspectUserInfo<HubFrame>;
	friend class AspectUserCommand<HubFrame>;

	using IRecent<HubFrame>::setText;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_SECURE,
		STATUS_USERS,
		STATUS_SHARED,
		STATUS_AVERAGE_SHARED,
		STATUS_SHOW_USERS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

	static void openWindow(TabViewPtr parent, string url, bool activate = true, bool connect = true);
	static void activateWindow(const string& url);

private:
	typedef std::function<bool (HubFrame*)> ClosePred;
	static void closeAll(ClosePred f = 0);
public:
	static void closeAll(bool disconnected);
	static void closeFavGroup(const string& group, bool reversed);
	
	static void reconnectDisconnected();

	static void resortUsers();

	WindowParams getWindowParams() const;
	static void parseWindowParams(TabViewPtr parent, const WindowParams& params);
	static bool isFavorite(const WindowParams& params);

private:
	friend class MainWindow;

	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST,
		COLUMN_SHARED,
		COLUMN_DESCRIPTION,
		COLUMN_TAG,
		COLUMN_CONNECTION,
		COLUMN_IP,
		COLUMN_COUNTRY,
		COLUMN_EMAIL,
		COLUMN_CID,
		COLUMN_LAST
	};

	struct UserTask {
		UserTask(const OnlineUser& ou);

		HintedUser user;
		Identity identity;
	};

	class UserInfo : public UserInfoBase, public FastAlloc<UserInfo> {
	public:
		UserInfo(const UserTask& u) : UserInfoBase(u.user) {
			update(u.identity, -1);
		}

		const tstring& getText(int col) const {
			return columns[col];
		}
		int getImage(int col) const;
		int getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const;

		static int compareItems(const UserInfo* a, const UserInfo* b, int col);
		bool update(const Identity& identity, int sortCol);

		string getNick() const { return identity.getNick(); }
		bool isHidden() const { return identity.isHidden(); }

		GETSET(Identity, identity, Identity);

	private:
		tstring columns[COLUMN_LAST];
	};

	typedef unordered_map<UserPtr, UserInfo*, User::Hash> UserMap;

	struct CountAvailable {
		CountAvailable() : available(0) { }
		int64_t available;
		void operator()(UserInfo *ui) {
			available += ui->getIdentity().getBytesShared();
		}
		void operator()(UserMap::const_reference ui) {
			available += ui.second->getIdentity().getBytesShared();
		}
	};

	SplitterContainerPtr paned;

	GridPtr userGrid;

	typedef TypedTable<UserInfo, false> WidgetUsers;
	typedef WidgetUsers* WidgetUsersPtr;
	WidgetUsersPtr users;

	ListFilter filter;
	GridPtr filterOpts;

	CheckBoxPtr showUsers;

	UserMap userMap;

	Client* client;
	string url;
	size_t selCount;
	bool statusDirty;
	bool waitingForPW;
	bool resort;
	bool confirmClose;

	vector<pair<function<void (const UserTask&)>, unique_ptr<UserTask>>> tasks;
	bool updateUsers;

	UserInfo* currentUser; /// only for situations when the user list is hidden

	ParamMap ucLineParams;
	bool hubMenu;

	tstring complete;
	StringList tabCompleteNicks;
	bool inTabComplete;

	static vector<HubFrame*> frames;

	HubFrame(TabViewPtr parent, string&& url, bool connect);
	virtual ~HubFrame();

	void layout();
	bool preClosing();
	void postClosing();

	bool tab();

	void addedChat(const tstring& message);
	void addedChat(const ChatMessage& message);
	void addStatus(const tstring& text, bool legitimate = true);

	size_t getUserCount() const;
	pair<size_t, tstring> getStatusUsers() const;
	pair<tstring, tstring> getStatusShared() const;
	void updateStatus();
	void updateSecureStatus();

	void initTimer();
	bool runTimer();

	void execTasks();

	UserInfo* findUser(const tstring& nick);
	bool updateUser(const UserTask& u);
	void removeUser(const UserPtr& aUser);
	const tstring& getNick(const UserPtr& u);

	void updateUserList(UserInfo* ui = NULL);

	void clearUserList();
	void clearTaskList();

	void addAsFavorite();
	void removeFavoriteHub();

	bool userClick(tstring& txt, const dwt::ScreenCoordinate& pt);

	void runUserCommand(const UserCommand& uc);

	bool handleMessageChar(int c);
	bool handleMessageKeyDown(int c);
	bool handleUsersKeyDown(int c);
	bool handleChatLink(const tstring& link);
	bool handleChatContextMenu(dwt::ScreenCoordinate pt);
	bool handleUsersContextMenu(dwt::ScreenCoordinate pt);
	void handleShowUsersClicked();
	void handleDoubleClickUsers();
	void handleCopyHub(bool keyprinted);
	void handleSearchHub();
	void handleAddAsFavorite();

	void showFilterOpts();
	void hideFilterOpts(dwt::Widget* w);

	string getLogPath(bool status = false) const;
	void openLog(bool status = false);

	string stripNick(const string& nick) const;
	tstring scanNickPrefix(const tstring& prefix);

	void reconnect();
	void disconnect(bool allowReco);
	void redirect(string&& target);

	// MDIChildFrame
	void tabMenuImpl(dwt::Menu* menu);

	// AspectChat
	void enterImpl(const tstring& s);

	// AspectUserInfo
	UserInfoList selectedUsersImpl() const;

	void onConnected();
	void onDisconnected();
	void onGetPassword();
	void onPrivateMessage(const ChatMessage& message);

	// FavoriteManagerListener
	virtual void on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) noexcept;
	virtual void on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) noexcept;
	void resortForFavsFirst(bool justDoIt = false);

	// ClientListener
	virtual void on(Connecting, Client*) noexcept;
	virtual void on(Connected, Client*) noexcept;
	virtual void on(ClientListener::UserUpdated, Client*, const OnlineUser&) noexcept;
	virtual void on(UsersUpdated, Client*, const OnlineUserList&) noexcept;
	virtual void on(ClientListener::UserRemoved, Client*, const OnlineUser&) noexcept;
	virtual void on(Redirect, Client*, const string&) noexcept;
	virtual void on(Failed, Client*, const string&) noexcept;
	virtual void on(GetPassword, Client*) noexcept;
	virtual void on(HubUpdated, Client*) noexcept;
	virtual void on(Message, Client*, const ChatMessage&) noexcept;
	virtual void on(StatusMessage, Client*, const string&, int = ClientListener::FLAG_NORMAL) noexcept;
	virtual void on(NickTaken, Client*) noexcept;
	virtual void on(LoginTimeout, Client*) noexcept;
	virtual void on(SearchFlood, Client*, const string&) noexcept;
	virtual void on(ClientLine, Client*, const string& line, int type) noexcept;
	virtual void on(HubMCTo, Client*, const string&, const string&) noexcept;
	void onStatusMessage(const string& line, int flags);

	// Icon management
	void setTabIcon();

	int tabIcon;

};

#endif // !defined(HUB_FRAME_H)
