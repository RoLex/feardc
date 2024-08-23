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

#ifndef DCPLUSPLUS_WIN32_MAIN_WINDOW_H
#define DCPLUSPLUS_WIN32_MAIN_WINDOW_H

#include <atomic>

#include <dcpp/forward.h>

#include <dcpp/ConnectionManagerListener.h>
#include <dcpp/CriticalSection.h>
#include <dcpp/HttpManagerListener.h>
#include <dcpp/LogManagerListener.h>
#include <dcpp/QueueManagerListener.h>
#include <dcpp/User.h>
#include <dcpp/UserConnectionListener.h>
#include <dcpp/WindowInfo.h>

#include <dwt/widgets/Window.h>

#include "forward.h"
#include "AspectStatus.h"

using std::function;
using std::unique_ptr;

class MainWindow :
	public dwt::Window,
	public AspectStatus<MainWindow>,
	private ConnectionManagerListener,
	private UserConnectionListener,
	private HttpManagerListener,
	private LogManagerListener,
	private QueueManagerListener
{
public:
	enum Status {
		STATUS_STATUS,
		STATUS_AWAY,
		STATUS_COUNTS,
		STATUS_SLOTS,
		STATUS_SHARE_SIZE,
		STATUS_FILES_SHARED,
		STATUS_DOWN_TOTAL,
		STATUS_UP_TOTAL,
		STATUS_DOWN_DIFF,
		STATUS_UP_DIFF,
		STATUS_DOWN_LIMIT,
		STATUS_UP_LIMIT,
		STATUS_DUMMY,
		STATUS_LAST
	};

	TabViewPtr getTabView() { return tabs; }

	virtual bool handleMessage( const MSG & msg, LRESULT & retVal );

	MainWindow();

	virtual ~MainWindow();

	void handleSettings();

	static void addPluginCommand(const string& guid, const tstring& text, function<void ()> command, const tstring& icon);
	static void removePluginCommand(const string& guid, const tstring& text);
	void refreshPluginMenu();

	/** show a balloon popup. refer to the dwt::Notification::addMessage doc for info about parameters. */
	void notify(const tstring& title, const tstring& message, function<void ()> callback = nullptr, const dwt::IconPtr& balloonIcon = nullptr);
	void setStaticWindowState(const string& id, bool open);
	void TrayPM();

	UserConnection* getPMConn(const UserPtr& user, UserConnectionListener* listener);

	bool closing() const { return stopperThread != 0; }

private:
	class DirectoryListInfo {
	public:
		DirectoryListInfo(const UserPtr& aUser, const tstring& aFile, const tstring& aDir, int64_t aSpeed) : user(aUser), file(aFile), dir(aDir), speed(aSpeed) { }
		UserPtr user;
		tstring file;
		tstring dir;
		int64_t speed;
	};
	class DirectoryBrowseInfo {
	public:
		DirectoryBrowseInfo(const UserPtr& ptr, string aText) : user(ptr), text(aText) { }
		UserPtr user;
		string text;
	};

	struct {
		tstring homepage;
		tstring downloads;
		tstring geoip_city;
		tstring faq;
		tstring help;
		tstring discuss;
		tstring features;
		tstring bugs;
		tstring donate;
		tstring blog;
		tstring community;
		tstring pluginrepo;
		tstring contribute;
	} links;

	enum {
		TIMER_STATUS,
		TIMER_SAVE
	};

	enum {
		CONN_VERSION,
		CONN_GEOIP,

		CONN_LAST
	};

	RebarPtr rebar;
	SplitterContainerPtr paned;
	MenuPtr mainMenu;
	Menu* pluginMenu;
	Menu* viewMenu;
	TransferView* transfers;
	ToolBarPtr toolbar;
	TabViewPtr tabs;

	typedef unordered_map<string, unsigned> ViewIndexes;
	ViewIndexes viewIndexes; /// indexes of menu commands of the "View" menu that open static windows

	bool tray_pm;
	unordered_map<UserPtr, UserConnection*, User::Hash> ccpms;
	CriticalSection ccpmMutex;

	/* sorted list of plugin commands. static because they may be added before the window has
	actually been created.
	plugin GUID -> { command name -> pair<callback, icon path> } */
	static unordered_map<string, map<tstring, pair<function<void ()>, tstring>, noCaseStringLess>> pluginCommands;

	std::atomic<HttpConnection*> conns[CONN_LAST];
	unique_ptr<File> geoFile;
	bool geoStaticServe; /// signals when GeoIP2 databases are not updated frequently for some reason

	HANDLE stopperThread;

	uint64_t lastUp;
	uint64_t lastDown;
	uint64_t lastTick;
	uint64_t lastGetShare;
	bool away;
	bool awayIdle;
	bool fullSlots;

	dwt::Application::FilterIter filterIter;
	dwt::NotificationPtr notifier;

	void initWindow();
	void initMenu();
	void initToolbar();
	void initStatusBar();
	void initTabs();
	void initTransfers();
	void initTray();

	// User actions
	void handleFavHubsDropDown(const dwt::ScreenCoordinate& pt);
	void handleRecent(const dwt::ScreenCoordinate& pt);
	void handleConfigureRecent(const string& id, const tstring& title);
	void handlePlugins(const dwt::ScreenCoordinate& pt);
	void handlePluginSettings();
	void handleLimiterMenu(bool upload);
	void handleQuickConnect();
	void handleConnectFavHubGroup();
	void handleOpenFileList();
	void handleRefreshFileList();
	void handleMatchAll();
	void handleOpenDownloadsDir();
	void handleCloseFavGroup(bool reversed);
	void handleAbout();
	void handleHashProgress();
	void handleWhatsThis();
	void handleSize();
	void handleActivate(bool active);
	LRESULT handleEndSession();
	void handleToolbarCustomized();
	bool handleToolbarContextMenu(const dwt::ScreenCoordinate& pt);
	void handleToolbarSize(int size);
	void switchMenuBar();
	void switchToolbar();
	void switchTransfers();
	void switchStatus();
	void handleSlotsMenu();
	void handleReconnect();
	void forwardHub(void (HubFrame::*f_t)());
	void handleTaskbarOverlay();

	// Other events
	void handleSized(const dwt::SizedEvent& sz);
	void handleMinimized();
	LRESULT handleActivateApp(WPARAM wParam);
	LRESULT handleCopyData(LPARAM lParam);
	LRESULT handleWhereAreYou();

	void handleTabsTitleChanged(const tstring& title);

	void handleTrayContextMenu();
	void handleTrayClicked();
	void handleTrayUpdate();

	void openWindow(const string& id, const WindowParams& params);
	void layout();
	void updateStatus();
	void updateAwayStatus();
	void addPluginCommands(Menu* menu);
	void showPortsError(const string& port);
	void setSaveTimer();
	void saveSettings();
	void saveWindowSettings();
	void parseCommandLine(const tstring& line);
	bool chooseFavHubGroup(const tstring& title, tstring& group);
	void fillLimiterMenu(Menu* menu, bool upload);
	void statusMessage(time_t t, const string& m);

	void completeVersionUpdate(bool success, const string& result);
	void checkGeoUpdate();
	void checkGeoUpdateDB();
	void updateGeo();
	void updateGeoDB();
	void completeGeoUpdate(bool success);

	bool filter(MSG& msg);

	bool handleClosing();
	void handleRestore();

	static DWORD WINAPI stopper(void* p);

	// ConnectionManagerListener
	virtual void on(ConnectionManagerListener::Connected, ConnectionQueueItem* cqi, UserConnection* uc) noexcept;
	virtual void on(ConnectionManagerListener::Removed, ConnectionQueueItem* cqi) noexcept;

	// UserConnectionListener
	virtual void on(UserConnectionListener::PrivateMessage, UserConnection* uc, const ChatMessage& message) noexcept;

	// HttpManagerListener
	void on(HttpManagerListener::Failed, HttpConnection*, const string&) noexcept;
	void on(HttpManagerListener::Complete, HttpConnection*, OutputStream*) noexcept;
	void on(HttpManagerListener::ResetStream, HttpConnection*) noexcept;

	// LogManagerListener
	void on(LogManagerListener::Message, time_t t, const string& m) noexcept;

	// QueueManagerListener
	void on(QueueManagerListener::Finished, QueueItem* qi, const string& dir, int64_t speed) noexcept;
	void on(QueueManagerListener::PartialList, const HintedUser&, const string& text) noexcept;
};

#endif // !defined(MAIN_FRM_H)
