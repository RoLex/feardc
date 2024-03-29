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

#include "stdafx.h"

#include "MainWindow.h"

#include <dcpp/Client.h>
#include <dcpp/ClientManager.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/ConnectivityManager.h>
#include <dcpp/Download.h>
#include <dcpp/DownloadManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/GeoManager.h>
#include <dcpp/HttpManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/QueueManager.h>
#include <dcpp/ResourceManager.h>
#include <dcpp/ScopedFunctor.h>
#include <dcpp/SearchManager.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/ThrottleManager.h>
#include <dcpp/TimerManager.h>
#include <dcpp/UploadManager.h>
#include <dcpp/PluginManager.h>
#include <dcpp/version.h>
#include <dcpp/WindowInfo.h>

#include <dwt/Application.h>
#include <dwt/DWTException.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/Notification.h>
#include <dwt/widgets/Rebar.h>
#include <dwt/widgets/Spinner.h>
#include <dwt/widgets/SplitterContainer.h>
#include <dwt/widgets/ToolBar.h>

#include "ACFrame.h"
#include "AboutDlg.h"
#include "ADLSearchFrame.h"
#include "CrashLogger.h"
#include "DirectoryListingFrame.h"
#include "FavHubsFrame.h"
#include "FinishedDLFrame.h"
#include "FinishedULFrame.h"
#include "HashProgressDlg.h"
#include "HubFrame.h"
#include "NotepadFrame.h"
#include "ParamDlg.h"
#include "PluginApiWin.h"
#include "PluginInfoDlg.h"
#include "PrivateFrame.h"
#include "PublicHubsFrame.h"
#include "QueueFrame.h"
#include "resource.h"
#include "SearchFrame.h"
#include "SettingsDialog.h"
#include "SingleInstance.h"
#include "StatsFrame.h"
#include "SystemFrame.h"
#include "TextFrame.h"
#include "TransferView.h"
#include "UsersFrame.h"

#ifdef HAVE_HTMLHELP_H
#include <htmlhelp.h>
#endif
#include <wtsapi32.h>

using dwt::Container;
using dwt::Rebar;
using dwt::SplitterContainer;
using dwt::Spinner;
using dwt::ToolBar;

decltype(MainWindow::pluginCommands) MainWindow::pluginCommands;

static dwt::IconPtr mainIcon(WinUtil::createIcon(IDI_DCPP, 32));
static dwt::IconPtr mainSmallIcon(WinUtil::createIcon(IDI_DCPP, 16));

MainWindow::MainWindow() :
dwt::Window(0, dwt::NormalDispatcher::newClass<MainWindow>(mainIcon, mainSmallIcon)),
rebar(0),
paned(0),
transfers(0),
toolbar(0),
tabs(0),
tray_pm(false),
geoStaticServe(false),
stopperThread(NULL),
lastUp(0),
lastDown(0),
lastTick(GET_TICK()),
away(false),
awayIdle(false),
fullSlots(false)
{
	// Don't forget to update version.xml when changing these links!
	links.homepage = _T("https://client.feardc.net/");
	links.downloads = links.homepage + _T("download.html");
	links.geoip_city = _T("https://client.feardc.net/geoip/GeoLite2-City.mmdb.gz");
	links.faq = links.homepage + _T("faq.html");
	links.help = links.homepage + _T("help.html");
	links.discuss = _T("https://github.com/rolex/feardc/issues/");
	links.features = _T("https://github.com/rolex/feardc/issues/");
	links.bugs = _T("https://github.com/rolex/feardc/issues/");
	links.donate = _T("https://www.paypal.com/paypalme/feardc/");
	links.blog = _T("https://dcpp.wordpress.com/");
	links.community = _T("https://www.dcbase.org/");
	links.pluginrepo = links.homepage + _T("plugins.html");
	links.contribute = _T("https://www.dcbase.org/contribute/");

	initWindow();
	initTabs();
	initMenu();
	initToolbar();
	initStatusBar();
	initTransfers();
	initTray();

	addAccel(FCONTROL, '0', [this] { switchMenuBar(); });
	addAccel(FCONTROL, '1', [this] { switchToolbar(); });
	addAccel(FCONTROL, '2', [this] { switchTransfers(); });
	addAccel(FCONTROL, '3', [this] { switchStatus(); });
	addAccel(FCONTROL, 'D', [this] { QueueFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'E', [this] { handleRefreshFileList(); });
	addAccel(FCONTROL, 'G', [this] { handleConnectFavHubGroup(); });
	addAccel(FCONTROL, 'H', [this] { FavHubsFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'L', [this] { handleOpenFileList(); });
	addAccel(FCONTROL, 'N', [this] { NotepadFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'P', [this] { PublicHubsFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'Q', [this] { handleQuickConnect(); });
	addAccel(FCONTROL, 'S', [this] { SearchFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, 'U', [this] { UsersFrame::openWindow(getTabView()); });
	addAccel(FCONTROL, VK_F3, [this] { handleSettings(); });
	addAccel(0, VK_F5, [this] { handleRefreshFileList(); });
	initAccels();

	onActivate([this](bool active) { handleActivate(active); });
	onSized([this](const dwt::SizedEvent &se) { handleSized(se); });
	onHelp(&WinUtil::help);

	updateStatus();
	setTimer([this]() -> bool { updateStatus(); return true; }, 1000, TIMER_STATUS);

	layout();

	for(auto& conn: conns) { conn = nullptr; }

	ConnectionManager::getInstance()->addListener(this);
	HttpManager::getInstance()->addListener(this);
	LogManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);

	onClosing([this] { return handleClosing(); });

	onRaw([this](WPARAM w, LPARAM) { return handleActivateApp(w); }, dwt::Message(WM_ACTIVATEAPP));
	onRaw([this](WPARAM, LPARAM) { return handleEndSession(); }, dwt::Message(WM_ENDSESSION));
	onRaw([this](WPARAM, LPARAM l) { return handleCopyData(l); }, dwt::Message(WM_COPYDATA));
	onRaw([this](WPARAM, LPARAM) { return handleWhereAreYou(); }, dwt::Message(SingleInstance::WMU_WHERE_ARE_YOU));

	if (!SETTING(DISABLE_TASKBAR_MENU)) {
		//In the event that explorer.exe crashes lets make sure the overlay icon is reset
		UINT tbcMsg = ::RegisterWindowMessage(L"TaskbarButtonCreated");
		if(tbcMsg != WM_NULL) {
			::ChangeWindowMessageFilterEx(this->handle(), tbcMsg, 1, 0);
			onRaw([this, tbcMsg](WPARAM, LPARAM) { handleTaskbarOverlay(); return 0; }, dwt::Message(tbcMsg));
		}
	}

	filterIter = dwt::Application::instance().addFilter([this](MSG &msg) { return filter(msg); });

	File::ensureDirectory(SETTING(LOG_DIRECTORY));

	TimerManager::getInstance()->start();

	conns[CONN_VERSION] = HttpManager::getInstance()->download("https://client.feardc.net/version.xml");

	try {
		ConnectivityManager::getInstance()->setup(true, true);
	} catch (const Exception& e) {
		showPortsError(e.getError());
	}

	// track when the computer is locked / unlocked.
	::WTSRegisterSessionNotification(handle(), NOTIFY_FOR_THIS_SESSION);
	onRaw([](WPARAM wParam, LPARAM) -> LRESULT {
		switch(wParam) {
		case WTS_SESSION_LOCK: if(SETTING(AWAY_COMP_LOCK)) Util::incAway(); break;
		case WTS_SESSION_UNLOCK: if(SETTING(AWAY_COMP_LOCK)) Util::decAway(); break;
		}
		return 0;
	}, dwt::Message(WM_WTSSESSION_CHANGE));
	onDestroy([this] { ::WTSUnRegisterSessionNotification(handle()); });

	{
		bool skipHubCon = WinUtil::isShift();

		const auto& list = WindowManager::getInstance()->getList();

		for(auto& i: list) {
			auto id = i.getId();
			auto params = i.getParams();

			if(skipHubCon && id == WindowManager::hub())
				params["NoConnect"] = WindowParam(Util::emptyString);

			callAsync([this, id, params] { openWindow(id, params); });
		}
	}

	callAsync([this] { parseCommandLine(tstring(::GetCommandLine())); });

	callAsync([this] {
		int cmdShow = dwt::Application::instance().getCmdShow();
		if(cmdShow == SW_SHOWDEFAULT || cmdShow == SW_SHOWNORMAL)
			cmdShow = SETTING(MAIN_WINDOW_STATE);

		int pos_x = SETTING(MAIN_WINDOW_POS_X);
		int pos_y = SETTING(MAIN_WINDOW_POS_Y);
		int size_x = SETTING(MAIN_WINDOW_SIZE_X);
		int size_y = SETTING(MAIN_WINDOW_SIZE_Y);
		if(pos_x != static_cast<int>(CW_USEDEFAULT) && pos_y != static_cast<int>(CW_USEDEFAULT) &&
			size_x != static_cast<int>(CW_USEDEFAULT) && size_y != static_cast<int>(CW_USEDEFAULT) &&
			size_x > 10 && size_y > 10)
		{
			WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
			wp.showCmd = cmdShow;
			wp.rcNormalPosition = dwt::Rectangle(pos_x, pos_y, size_x, size_y);
			::SetWindowPlacement(handle(), &wp);
		} else {
			/* invalid pos / size values; just show the window (Windows will have decided a
			position for the window by itself). */
			::ShowWindow(handle(), cmdShow);
		}

		handleActivate(true); //Now as it is already visible, set the proper focus for the active restored window.

		if(cmdShow == SW_MINIMIZE || cmdShow == SW_SHOWMINIMIZED || cmdShow == SW_SHOWMINNOACTIVE)
			handleMinimized();
	});

	Util::setAway(SETTING(LAST_AWAY));

	if(SETTING(NICK).empty()) {
		callAsync([this] {
			SystemFrame::openWindow(getTabView(), false, false);

			WinUtil::helpId(this, IDH_GET_STARTED);
			handleSettings();
		});
	}

	PluginManager::getInstance()->runHook(HOOK_UI_CREATED, handle(), NULL);

	if(SETTING(SETTINGS_SAVE_INTERVAL) > 0)
		setSaveTimer();

	if (!SETTING(DISABLE_TASKBAR_MENU))
		handleTaskbarOverlay();
}

void MainWindow::initWindow() {
	// Create main window
	dcdebug("initWindow\n");

	Seed cs(_T(APPNAME) _T(" ") _T(VERSIONSTRING));

	cs.style &= ~WS_VISIBLE;
	cs.exStyle |= WS_EX_APPWINDOW;
	if (ResourceManager::getInstance()->isRTL())
		cs.exStyle |= WS_EX_RTLREADING;

	create(cs);

	setHelpId(IDH_MAIN);

	rebar = addChild(Rebar::Seed());

	paned = addChild(SplitterContainer::Seed(SETTING(TRANSFERS_PANED_POS), true));
}

void MainWindow::initMenu() {
	dcdebug("initMenu\n");

	{
		Menu::Seed cs = WinUtil::Seeds::menu;
		cs.popup = false;
		mainMenu = addChild(cs);
	}

	{
		auto file = mainMenu->appendPopup(T_("&File"));

		file->appendItem(T_("&Quick connect...\tCtrl+Q"), [this] { handleQuickConnect(); }, WinUtil::menuIcon(IDI_HUB));
		file->appendItem(T_("Connect to a favorite hub &group...\tCtrl+G"), [this] { handleConnectFavHubGroup(); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		file->appendSeparator();

		file->appendItem(T_("&Reconnect\tCtrl+R"), [this] { handleReconnect(); }, WinUtil::menuIcon(IDI_RECONNECT));
		file->appendSeparator();

		file->appendItem(T_("Open file list...\tCtrl+L"), [this] { handleOpenFileList(); }, WinUtil::menuIcon(IDI_OPEN_FILE_LIST));
		file->appendItem(T_("Open own list"), [this] { DirectoryListingFrame::openOwnList(getTabView()); }, WinUtil::menuIcon(IDI_OPEN_OWN_FILE_LIST));
		file->appendItem(T_("Match downloaded lists"), [this] { handleMatchAll(); });
		file->appendItem(T_("Refresh file list\tF5"), [this] { handleRefreshFileList(); }, WinUtil::menuIcon(IDI_REFRESH));
		file->appendSeparator();

		file->appendItem(T_("Open downloads directory"), [this] { handleOpenDownloadsDir(); }, WinUtil::menuIcon(IDI_OPEN_DL_DIR));
		file->appendItem(T_("Open crash log"), [this] { TextFrame::openWindow(getTabView(), Text::fromT(CrashLogger::getPath())); });
		file->appendSeparator();

		file->appendItem(T_("Settings\tCtrl+F3"), [this] { handleSettings(); }, WinUtil::menuIcon(IDI_SETTINGS));
		file->appendSeparator();

		file->appendItem(T_("GeoIP database update"), [this] { updateGeo(); });
		file->appendSeparator();

		pluginMenu = file->appendPopup(T_("Plugins"), WinUtil::menuIcon(IDI_PLUGINS));
		addPluginCommands(pluginMenu);
		file->appendSeparator();

		file->appendItem(T_("E&xit\tAlt+F4"), [this] { close(true); }, WinUtil::menuIcon(IDI_EXIT));
	}

	{
		viewMenu = mainMenu->appendPopup(T_("&View"));

		viewIndexes[PublicHubsFrame::id] = viewMenu->appendItem(T_("&Public Hubs\tCtrl+P"),
			[this] { PublicHubsFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_PUBLICHUBS));
		viewIndexes[FavHubsFrame::id] = viewMenu->appendItem(T_("&Favorite Hubs\tCtrl+H"),
			[this] { FavHubsFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		viewIndexes[UsersFrame::id] = viewMenu->appendItem(T_("&Users\tCtrl+U"),
			[this] { UsersFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_USERS));
		viewMenu->appendSeparator();
		viewIndexes[QueueFrame::id] = viewMenu->appendItem(T_("&Download Queue\tCtrl+D"),
			[this] { QueueFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_QUEUE));
		viewIndexes[FinishedDLFrame::id] = viewMenu->appendItem(T_("Finished Downloads"),
			[this] { FinishedDLFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FINISHED_DL));
		viewIndexes[FinishedULFrame::id] = viewMenu->appendItem(T_("Finished Uploads"),
			[this] { FinishedULFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_FINISHED_UL));
		viewMenu->appendSeparator();
		viewMenu->appendItem(T_("&Search\tCtrl+S"), [this] { SearchFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_SEARCH));
		viewIndexes[ADLSearchFrame::id] = viewMenu->appendItem(T_("ADL Search"),
			[this] { ADLSearchFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_ADLSEARCH));
		viewMenu->appendSeparator();
		viewIndexes[NotepadFrame::id] = viewMenu->appendItem(T_("&Notepad\tCtrl+N"),
			[this] { NotepadFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_NOTEPAD));
		viewIndexes[SystemFrame::id] = viewMenu->appendItem(T_("System Log"),
			[this] { SystemFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_LOGS));
		viewIndexes[StatsFrame::id] = viewMenu->appendItem(T_("Network Statistics"),
			[this] { StatsFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_NET_STATS));
#ifdef DEBUG
		viewIndexes[ACFrame::id] = viewMenu->appendItem(T_("About:config"),
			[this] { ACFrame::openWindow(getTabView()); }, WinUtil::menuIcon(IDI_DCPP));
#endif

		viewMenu->appendItem(T_("Indexing progress"), [this] { handleHashProgress(); }, WinUtil::menuIcon(IDI_INDEXING));
		viewMenu->appendSeparator();
		viewIndexes["Menu"] = viewMenu->appendItem(T_("Menu bar\tCtrl+0"), [this] { switchMenuBar(); });
		viewIndexes["Toolbar"] = viewMenu->appendItem(T_("Toolbar\tCtrl+1"), [this] { switchToolbar(); });
		viewIndexes["Transfers"] = viewMenu->appendItem(T_("Transfer view\tCtrl+2"), [this] { switchTransfers(); });
		viewIndexes["Status"] = viewMenu->appendItem(T_("Status bar\tCtrl+3"), [this] { switchStatus(); });
	}

	{
		auto window = mainMenu->appendPopup(T_("&Window"));

		window->appendItem(T_("Reconnect disconnected hubs"), &HubFrame::reconnectDisconnected, WinUtil::menuIcon(IDI_RECONNECT));
		window->appendSeparator();
		
		window->appendItem(T_("Close all hubs"), [] { HubFrame::closeAll(false); }, WinUtil::menuIcon(IDI_HUB));
		window->appendItem(T_("Close disconnected hubs"), [] { HubFrame::closeAll(true); }, WinUtil::menuIcon(IDI_HUB_OFF));
		window->appendItem(T_("Close all hubs of a favorite group"), [this] { handleCloseFavGroup(false); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		window->appendItem(T_("Close hubs not in a favorite group"), [this] { handleCloseFavGroup(true); }, WinUtil::menuIcon(IDI_FAVORITE_HUBS));
		window->appendSeparator();

		window->appendItem(T_("Close all PM windows"), [] { PrivateFrame::closeAll(false); }, WinUtil::menuIcon(IDI_PRIVATE));
		window->appendItem(T_("Close all offline PM windows"), [] { PrivateFrame::closeAll(true); }, WinUtil::menuIcon(IDI_PRIVATE_OFF));
		window->appendSeparator();

		window->appendItem(T_("Close all file list windows"), &DirectoryListingFrame::closeAll, WinUtil::menuIcon(IDI_DIRECTORY));
		window->appendSeparator();

		window->appendItem(T_("Close all search windows"), &SearchFrame::closeAll, WinUtil::menuIcon(IDI_SEARCH));
	}

	{
		auto help = mainMenu->appendPopup(T_("&Help"));

		help->appendItem(T_("Help &Contents\tF1"), [this] { WinUtil::helpId(this, IDH_INDEX); }, WinUtil::menuIcon(IDI_HELP));
		help->appendItem(T_("Get started"), [this] { WinUtil::helpId(this, IDH_GET_STARTED); }, WinUtil::menuIcon(IDI_GET_STARTED));
		help->appendSeparator();
		help->appendItem(T_("Change Log"), [this] { WinUtil::helpId(this, IDH_CHANGELOG); }, WinUtil::menuIcon(IDI_CHANGELOG));
		help->appendItem(T_("About FearDC"), [this] { handleAbout(); }, WinUtil::menuIcon(IDI_DCPP));
		help->appendSeparator();

		help = help->appendPopup(T_("Links"), WinUtil::menuIcon(IDI_LINKS));
		help->appendItem(T_("Homepage"), [this] { WinUtil::openLink(links.homepage); });
		help->appendItem(T_("Downloads"), [this] { WinUtil::openLink(links.downloads); });
		help->appendItem(T_("Frequently asked questions"), [this] { WinUtil::openLink(links.faq); });
		help->appendItem(T_("Help center"), [this] { WinUtil::openLink(links.help); });
		help->appendItem(T_("Discussion forum"), [this] { WinUtil::openLink(links.discuss); });
		help->appendItem(T_("Request a feature"), [this] { WinUtil::openLink(links.features); });
		help->appendItem(T_("Report a bug"), [this] { WinUtil::openLink(links.bugs); });
		help->appendItem(T_("Donate (paypal)"), [this] { WinUtil::openLink(links.donate); }, WinUtil::menuIcon(IDI_DONATE));
		help->appendItem(T_("Contribute"), [this] { WinUtil::openLink(links.contribute); });
		help->appendItem(T_("Blog"), [this] { WinUtil::openLink(links.blog); });
		help->appendItem(T_("DC Network Foundation community"), [this] { WinUtil::openLink(links.community); });
		help->appendItem(T_("Plugin repository"), [this] { WinUtil::openLink(links.pluginrepo); });
	}

	mainMenu->setMenu();

	if(SETTING(SHOW_MENU_BAR)) {
		viewMenu->checkItem(viewIndexes["Menu"], true);
	} else {
		::SetMenu(handle(), nullptr);
	}

	// hide the temporary menu bar on WM_EXITMENULOOP
	onRaw([this](WPARAM wParam, LPARAM) -> LRESULT {
		if(!wParam && !SETTING(SHOW_MENU_BAR) && ::GetMenu(handle())) {
			::SetMenu(handle(), nullptr);
		}
		return 0;
	}, dwt::Message(WM_EXITMENULOOP));
}

void MainWindow::initToolbar() {
	if(!SETTING(SHOW_TOOLBAR))
		return;

	dcdebug("initToolbar\n");
	toolbar = addChild(ToolBar::Seed());

	toolbar->addButton(PublicHubsFrame::id, WinUtil::toolbarIcon(IDI_PUBLICHUBS), 0, T_("Public Hubs"), false,
		IDH_TOOLBAR_PUBLIC_HUBS, [this] { PublicHubsFrame::openWindow(getTabView()); });
	toolbar->addButton("Reconnect", WinUtil::toolbarIcon(IDI_RECONNECT), 0, T_("Reconnect"), false,
		IDH_TOOLBAR_RECONNECT, [this] { handleReconnect(); });
	toolbar->addButton(FavHubsFrame::id, WinUtil::toolbarIcon(IDI_FAVORITE_HUBS), 0, T_("Favorite Hubs"), false,
		IDH_TOOLBAR_FAVORITE_HUBS, [this] { FavHubsFrame::openWindow(getTabView()); },
		[this](const dwt::ScreenCoordinate& pt) { handleFavHubsDropDown(pt); });
	toolbar->addButton(UsersFrame::id, WinUtil::toolbarIcon(IDI_USERS), 0, T_("Users"), false,
		IDH_TOOLBAR_USERS, [this] { UsersFrame::openWindow(getTabView()); });
	toolbar->addButton(QueueFrame::id, WinUtil::toolbarIcon(IDI_QUEUE), 0, T_("Download Queue"), false,
		IDH_TOOLBAR_QUEUE, [this] { QueueFrame::openWindow(getTabView()); });
	toolbar->addButton(FinishedDLFrame::id, WinUtil::toolbarIcon(IDI_FINISHED_DL), 0, T_("Finished Downloads"), false,
		IDH_TOOLBAR_FINISHED_DL, [this] { FinishedDLFrame::openWindow(getTabView()); });
	toolbar->addButton(FinishedULFrame::id, WinUtil::toolbarIcon(IDI_FINISHED_UL), 0, T_("Finished Uploads"), false,
		IDH_TOOLBAR_FINISHED_UL, [this] { FinishedULFrame::openWindow(getTabView()); });
	toolbar->addButton(SearchFrame::id, WinUtil::toolbarIcon(IDI_SEARCH), 0, T_("Search"), false,
		IDH_TOOLBAR_SEARCH, [this] { SearchFrame::openWindow(getTabView()); });
	toolbar->addButton(ADLSearchFrame::id, WinUtil::toolbarIcon(IDI_ADLSEARCH), 0, T_("ADL Search"), false,
		IDH_TOOLBAR_ADL_SEARCH, [this] { ADLSearchFrame::openWindow(getTabView()); });
	toolbar->addButton(StatsFrame::id, WinUtil::toolbarIcon(IDI_NET_STATS), 0, T_("Network Statistics"), false,
		IDH_TOOLBAR_NET_STATS, [this] { StatsFrame::openWindow(getTabView()); });
	toolbar->addButton("OpenDlDir", WinUtil::toolbarIcon(IDI_OPEN_DL_DIR), 0, T_("Open downloads directory"), false,
		IDH_TOOLBAR_DOWNLOADS_DIR, [this] { handleOpenDownloadsDir(); });
	toolbar->addButton("OpenFL", WinUtil::toolbarIcon(IDI_OPEN_FILE_LIST), 0, T_("Open file list..."), false,
		IDH_TOOLBAR_FILE_LIST, [this] { handleOpenFileList(); });
	toolbar->addButton("OpenOwnFL", WinUtil::toolbarIcon(IDI_OPEN_OWN_FILE_LIST), 0, T_("Open own list"), false,
		IDH_TOOLBAR_OWN_FILE_LIST, [this] { DirectoryListingFrame::openOwnList(getTabView()); });
	toolbar->addButton("Recents", WinUtil::toolbarIcon(IDI_RECENTS), 0, T_("Recent windows"), false,
		IDH_TOOLBAR_RECENT, nullptr, [this](const dwt::ScreenCoordinate& pt) { handleRecent(pt); });
	toolbar->addButton("Settings", WinUtil::toolbarIcon(IDI_SETTINGS), 0, T_("Settings"), false,
		IDH_TOOLBAR_SETTINGS, [this] { handleSettings(); });
	toolbar->addButton(NotepadFrame::id, WinUtil::toolbarIcon(IDI_NOTEPAD), 0, T_("Notepad"), false,
		IDH_TOOLBAR_NOTEPAD, [this] { NotepadFrame::openWindow(getTabView()); });
	toolbar->addButton("Refresh", WinUtil::toolbarIcon(IDI_REFRESH), 0, T_("Refresh file list"), false,
		IDH_TOOLBAR_REFRESH, [this] { handleRefreshFileList(); });
	toolbar->addButton("Plugins", WinUtil::toolbarIcon(IDI_PLUGINS), 0, T_("Plugins"), false,
		IDH_TOOLBAR_PLUGINS, [this] { handlePluginSettings(); },
		[this](const dwt::ScreenCoordinate& pt) { handlePlugins(pt); });
	toolbar->addButton("CSHelp", WinUtil::toolbarIcon(IDI_WHATS_THIS), 0, T_("What's This?"), false,
		IDH_TOOLBAR_WHATS_THIS, [this] { handleWhatsThis(); });

	if(SettingsManager::getInstance()->isDefault(SettingsManager::TOOLBAR)) {
		// gotta create a default layout for the toolbar
		const string comma(",");
		SettingsManager::getInstance()->setDefault(SettingsManager::TOOLBAR,
			PublicHubsFrame::id + comma +
			comma +
			"Reconnect" + comma +
			FavHubsFrame::id + comma +
			UsersFrame::id + comma +
			comma +
			QueueFrame::id + comma +
			FinishedDLFrame::id + comma +
			FinishedULFrame::id + comma +
			comma +
			SearchFrame::id + comma +
			ADLSearchFrame::id + comma +
			comma +
			StatsFrame::id + comma +
			comma +
			"OpenDlDir" + comma +
			"OpenFL" + comma +
			"OpenOwnFL" + comma +
			"Refresh" + comma +
			comma +
			"Recents" + comma +
			comma +
			"Settings" + comma +
			NotepadFrame::id + comma +
			comma +
			"Plugins" + comma +
			comma +
			"CSHelp");
	}
	toolbar->setLayout(StringTokenizer<string>(SETTING(TOOLBAR), ',').getTokens());
	toolbar->onCustomized([this] { handleToolbarCustomized(); });
	toolbar->onCustomizeHelp([this] { WinUtil::helpId(toolbar, IDH_CUSTOMIZE_TB); });
	toolbar->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleToolbarContextMenu(sc); });

	toolbar->onHelp(&WinUtil::help);

	rebar->add(toolbar);

	viewMenu->checkItem(viewIndexes["Toolbar"], true);
}

void MainWindow::initStatusBar() {
	if(!SETTING(SHOW_STATUSBAR))
		return;

	dcdebug("initStatusBar\n");
	initStatus(true);

	updateAwayStatus();

	/// @todo set to resizedrag width really
	status->setSize(STATUS_DUMMY, 32);

	status->setIcon(STATUS_COUNTS, WinUtil::statusIcon(IDI_HUB));
	status->setIcon(STATUS_SLOTS, WinUtil::statusIcon(IDI_SLOTS));
	{
		dwt::IconPtr icon_DL(WinUtil::statusIcon(IDI_DOWNLOAD));
		dwt::IconPtr icon_UL(WinUtil::statusIcon(IDI_UPLOAD));
		status->setIcon(STATUS_DOWN_TOTAL, icon_DL);
		status->setIcon(STATUS_UP_TOTAL, icon_UL);
		status->setIcon(STATUS_DOWN_DIFF, icon_DL);
		status->setIcon(STATUS_UP_DIFF, icon_UL);
	}
	status->setIcon(STATUS_DOWN_LIMIT, WinUtil::statusIcon(IDI_DLIMIT));
	status->setIcon(STATUS_UP_LIMIT, WinUtil::statusIcon(IDI_ULIMIT));

	{
		auto f = [this] { handleSlotsMenu(); };
		status->onClicked(STATUS_SLOTS, f);
		status->onRightClicked(STATUS_SLOTS, f);
	}

	{
		auto f = [this] { handleLimiterMenu(false); };
		status->onClicked(STATUS_DOWN_LIMIT, f);
		status->onRightClicked(STATUS_DOWN_LIMIT, f);
	}
	{
		auto f = [this] { handleLimiterMenu(true); };
		status->onClicked(STATUS_UP_LIMIT, f);
		status->onRightClicked(STATUS_UP_LIMIT, f);
	}

	status->onDblClicked(STATUS_STATUS, [] {
		WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM))));
	});

	status->onDblClicked(STATUS_AWAY, &Util::switchAway);

	{
		auto f = [this] { StatsFrame::openWindow(getTabView(), false); };
		status->onDblClicked(STATUS_DOWN_TOTAL, f);
		status->onDblClicked(STATUS_UP_TOTAL, f);
		status->onDblClicked(STATUS_DOWN_DIFF, f);
		status->onDblClicked(STATUS_UP_DIFF, f);
	}

	status->setHelpId(STATUS_STATUS, IDH_MAIN_STATUS);
	status->setHelpId(STATUS_AWAY, IDH_MAIN_AWAY);
	status->setHelpId(STATUS_COUNTS, IDH_MAIN_HUBS);
	status->setHelpId(STATUS_SLOTS, IDH_MAIN_SLOTS);
	status->setHelpId(STATUS_DOWN_TOTAL, IDH_MAIN_DOWN_TOTAL);
	status->setHelpId(STATUS_UP_TOTAL, IDH_MAIN_UP_TOTAL);
	status->setHelpId(STATUS_DOWN_DIFF, IDH_MAIN_DOWN_DIFF);
	status->setHelpId(STATUS_UP_DIFF, IDH_MAIN_UP_DIFF);
	status->setHelpId(STATUS_DOWN_LIMIT, IDH_MAIN_DOWN_LIMIT);
	status->setHelpId(STATUS_UP_LIMIT, IDH_MAIN_UP_LIMIT);

	viewMenu->checkItem(viewIndexes["Status"], true);
}

void MainWindow::initTabs() {
	dcdebug("initTabs\n");
	TabView::Seed seed = WinUtil::Seeds::tabs;
	seed.widthConfig = SETTING(TAB_WIDTH);
	if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_OD) {
		if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BROWSER)
			seed.tabStyle = TabView::Seed::WinBrowser;
	} else {
		seed.style &= ~TCS_OWNERDRAWFIXED;
		seed.widthConfig = (seed.widthConfig - 100) / 9; // max width to max chars
	}
	if(SETTING(TAB_STYLE) & SettingsManager::TAB_STYLE_BUTTONS)
		seed.style |= TCS_BUTTONS;
	seed.closeIcon = WinUtil::tabIcon(IDI_EXIT);
	seed.toggleActive = SETTING(TOGGLE_ACTIVE_WINDOW);
	seed.ctrlTab = true;
	tabs = paned->addChild(seed);

	if (!SETTING(DISABLE_TASKBAR_MENU))
		tabs->initTaskbar(this);

	tabs->onTitleChanged([this](const tstring &title) { handleTabsTitleChanged(title); });
	tabs->onHelp(&WinUtil::help);
}

void MainWindow::initTransfers() {
	if(!SETTING(SHOW_TRANSFERVIEW))
		return;

	dcdebug("initTransfers\n");
	transfers = new TransferView(paned, getTabView());

	viewMenu->checkItem(viewIndexes["Transfers"], true);
}

void MainWindow::initTray() {
	dcdebug("initTray\n");
	notifier = addChild(dwt::Notification::Seed(mainSmallIcon));
	notifier->onContextMenu([this] { handleTrayContextMenu(); });
	notifier->onIconClicked([this] { handleTrayClicked(); });
	notifier->onIconDbClicked([this] { handleRestore(); });
	notifier->onUpdateTip([this] { handleTrayUpdate(); });
	if(SETTING(ALWAYS_TRAY)) {
		notifier->setVisible(true);
	}
}

bool MainWindow::filter(MSG& msg) {
	if(msg.message == WM_SYSKEYDOWN && (msg.wParam == VK_MENU || msg.wParam == VK_F10) &&
		!SETTING(SHOW_MENU_BAR) && !::GetMenu(handle()) && !isShiftPressed())
	{
		// show the temporary menu bar when pressing Alt or F10
		::SetMenu(handle(), mainMenu->handle());

	} else if((msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP) && (msg.wParam == VK_MENU || msg.wParam == VK_F10)  &&
		!SETTING(SHOW_MENU_BAR) && ::GetMenu(handle()))
	{
		// hide the temporary menu bar if when releasing Alt or F10, the menu bar isn't focused
		callAsync([this] {
			MENUBARINFO info = { sizeof(MENUBARINFO) };
			if(!::GetMenuBarInfo(handle(), OBJID_MENU, 0, &info) || !info.fBarFocused) {
				::SetMenu(handle(), nullptr);
			}
		});
	}

	if(tabs && tabs->filter(msg)) {
		return true;
	}

#ifdef HAVE_HTMLHELP_H
	if(::HtmlHelp(NULL, NULL, HH_PRETRANSLATEMESSAGE, reinterpret_cast<DWORD_PTR>(&msg))) {
		return true;
	}
#endif

	Container* active = getTabView()->getActive();
	if(active) {
		if(::IsDialogMessage( active->handle(), & msg )) {
			return true;
		}
	}

	return false;
}

void MainWindow::addPluginCommand(const string& guid, const tstring& text, function<void ()> command, const tstring& icon) {
	pluginCommands[guid][text] = make_pair(command, icon);

	if(WinUtil::mainWindow && !WinUtil::mainWindow->closing()) {
		WinUtil::mainWindow->refreshPluginMenu();
	}
}

void MainWindow::removePluginCommand(const string& guid, const tstring& text) {
	auto commandIt = pluginCommands.find(guid);
	if(commandIt == pluginCommands.end()) { return; }
	auto& commands = commandIt->second;

	auto i = commands.find(text);
	if(i == commands.end()) { return; }
	commands.erase(i);

	if(commands.empty()) {
		pluginCommands.erase(commandIt);
	}

	if(WinUtil::mainWindow && !WinUtil::mainWindow->closing()) {
		WinUtil::mainWindow->refreshPluginMenu();
	}
}

void MainWindow::refreshPluginMenu() {
	pluginMenu->clear();
	addPluginCommands(pluginMenu);
}

void MainWindow::addPluginCommands(Menu* menu) {
	menu->appendItem(T_("Add"), [this] { PluginUtils::addPlugin(this); });
	menu->appendItem(T_("Configure"), [this] { handlePluginSettings(); });

	const auto plugins = PluginManager::getInstance()->getPluginList();
	if(!plugins.empty()) {
		menu->appendSeparator();
	}

	for(auto& guid: plugins) {
		const auto enabled = PluginManager::getInstance()->isLoaded(guid);
		const auto name = Text::toT(PluginManager::getInstance()->getPlugin(guid).name);

		auto sub = menu->appendPopup(name, WinUtil::menuIcon(enabled ? IDI_GREEN_BALL : IDI_RED_BALL), false);
		sub->setTitle(name); // don't want the enabled/disabled icon repeated there

		sub->appendItem(T_("Enable"), [this, guid] { PluginUtils::enablePlugin(guid, this); }, WinUtil::menuIcon(IDI_GREEN_BALL), !enabled);
		sub->appendItem(T_("Disable"), [this, guid] { PluginUtils::disablePlugin(guid, this); }, WinUtil::menuIcon(IDI_RED_BALL), enabled);

		sub->appendSeparator();
		sub->appendItem(T_("Configure"), [this, guid] { PluginUtils::configPlugin(guid, this); }, nullptr, enabled);

		auto commandIt = pluginCommands.find(guid);
		if(commandIt != pluginCommands.end()) {
			sub->appendSeparator();

			for(auto& i: commandIt->second) {
				dwt::IconPtr icon;
				if(!i.second.second.empty()) {
					try { icon = new dwt::Icon(i.second.second, dwt::Point(16, 16)); }
					catch(const dwt::DWTException&) { }
				}
				sub->appendItem(i.first, i.second.first, icon);
			}
		}
	}
}

void MainWindow::notify(const tstring& title, const tstring& message, function<void ()> callback, const dwt::IconPtr& balloonIcon) {
	notifier->addMessage(str(TF_("%1% - %2%") % APPNAME % title), message, [this, callback] { handleRestore(); if(callback) callback(); }, balloonIcon);
}

void MainWindow::setStaticWindowState(const string& id, bool open) {
	if(toolbar)
		toolbar->setButtonChecked(id, open);

	auto i = viewIndexes.find(id);
	if(i != viewIndexes.end())
		viewMenu->checkItem(i->second, open);
}

void MainWindow::TrayPM() {
	if(!tray_pm && notifier->isVisible() && !onForeground()) {
		//notifier->setIcon(WinUtil::createIcon(IDI_TRAY_PM, 16));
		notifier->setIcon(WinUtil::mergeIcons({IDI_DCPP, IDI_TRAY_PM}));
		tray_pm = true;
	}
}

UserConnection* MainWindow::getPMConn(const UserPtr& user, UserConnectionListener* listener) {
	Lock l(ccpmMutex);
	auto i = ccpms.find(user);
	if(i != ccpms.end()) {
		auto uc = i->second;
		ccpms.erase(i);
		uc->addListener(listener);
		uc->removeListener(this);
		return uc;
	}
	return nullptr;
}

void MainWindow::handleTabsTitleChanged(const tstring& title) {
	setText(title.empty() ? _T(APPNAME) _T(" ") _T(VERSIONSTRING) : _T(APPNAME) _T(" ") _T(VERSIONSTRING) _T(" - [") + title + _T("]"));
}

static void multiConnect(const string& group, TabViewPtr parent) {
	for(auto& i: FavoriteManager::getInstance()->getFavoriteHubs(group))
		HubFrame::openWindow(parent, i->getServer());
}

void MainWindow::handleFavHubsDropDown(const dwt::ScreenCoordinate& pt) {
	auto menu = addChild(WinUtil::Seeds::menu);

	typedef map<string, Menu*, noCaseStringLess> GroupMenus;
	GroupMenus groupMenus;

	for(auto& i: FavoriteManager::getInstance()->getFavHubGroups())
		groupMenus[i.first] = nullptr;

	for(auto& i: groupMenus) {
		i.second = menu->appendPopup(escapeMenu(Text::toT(i.first)));
		auto group = i.first;
		i.second->appendItem(T_("Connect to all hubs in this group"), [=, this] { multiConnect(group, getTabView()); });
		i.second->appendSeparator();
	}

	for(auto entry: FavoriteManager::getInstance()->getFavoriteHubs()) {
		auto groupMenu = groupMenus.find(entry->getGroup());
		((groupMenu == groupMenus.end()) ? menu.get() : groupMenu->second)->appendItem(
			escapeMenu(Text::toT(entry->getName())),
			[this, entry] { HubFrame::openWindow(getTabView(), entry->getServer()); });
	}

	menu->open(pt);
}

void addActiveParam(WindowParams& params) {
	params["Active"] = WindowParam("1");
}

template<typename T, typename configureF>
void addRecentMenu(Menu* menu, MainWindow* mainWindow, const tstring& text, unsigned iconId, unsigned favIconId, configureF f) {
	auto popup = menu->appendPopup(text, WinUtil::menuIcon(iconId));
	popup->appendItem(T_("&Configure..."), std::bind(f, mainWindow, T::id, text),
		WinUtil::menuIcon(IDI_SETTINGS), true, true);
	popup->appendSeparator();

	const auto& recent = WindowManager::getInstance()->getRecent();
	auto it = recent.find(T::id);
	if(it == recent.end()) {
		popup->appendItem(T_("(No recent item found)"), nullptr, nullptr, false);
	} else {

		dwt::IconPtr favIcon = WinUtil::menuIcon(favIconId);

		const auto& list = it->second;
		for(auto& i: list) {
			auto params = i.getParams();

			auto title = params.find("Title");
			if(title == params.end() || title->second.empty())
				continue;

			addActiveParam(params);

			popup->appendItem(escapeMenu(Text::toT(title->second)),
				std::bind(&T::parseWindowParams, mainWindow->getTabView(), params),
				T::isFavorite(params) ? favIcon : nullptr);
		}
	}
}

void MainWindow::handleRecent(const dwt::ScreenCoordinate& pt) {
	auto menu = addChild(WinUtil::Seeds::menu);

	auto f = &MainWindow::handleConfigureRecent;
	addRecentMenu<HubFrame>(menu.get(), this, T_("Recent hubs"), IDI_HUB, IDI_FAVORITE_HUBS, f);
	addRecentMenu<PrivateFrame>(menu.get(), this, T_("Recent PMs"), IDI_PRIVATE, IDI_FAVORITE_USER_ON, f);
	addRecentMenu<DirectoryListingFrame>(menu.get(), this, T_("Recent file lists"), IDI_DIRECTORY, IDI_FAVORITE_USER_ON, f);

	menu->open(pt);
}

void MainWindow::handleConfigureRecent(const string& id, const tstring& title) {
	ParamDlg dlg(this, title);
	dlg.addIntTextBox(T_("Maximum number of recent items to save"),
		Text::toT(std::to_string(WindowManager::getInstance()->getMaxRecentItems(id))), 0);
	if(dlg.run() == IDOK) {
		const unsigned max = Util::toUInt(Text::fromT(dlg.getValue()));
		dwt::MessageBox(this).show(str(TF_("%1% recent items will be saved from now on.") % max), title,
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONINFORMATION);
		WindowManager::getInstance()->setMaxRecentItems(id, max);
	}
}

void MainWindow::handlePlugins(const dwt::ScreenCoordinate& pt) {
	auto menu = addChild(WinUtil::Seeds::menu);

	menu->setTitle(T_("Plugins"), WinUtil::menuIcon(IDI_PLUGINS));
	addPluginCommands(menu.get());

	menu->open(pt);
}

void MainWindow::handlePluginSettings() {
	SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PAGE, SettingsDialog::pluginPagePos);
	handleSettings();
}

void MainWindow::fillLimiterMenu(Menu* menu, bool upload) {
	const auto title = upload ? T_("Upload limit") : T_("Download limit");
	const auto menuIcon = WinUtil::menuIcon(upload ? IDI_ULIMIT : IDI_DLIMIT);
	menu->setTitle(title, menuIcon);

	const auto setting = ThrottleManager::getCurSetting(
		upload ? SettingsManager::MAX_UPLOAD_SPEED_MAIN : SettingsManager::MAX_DOWNLOAD_SPEED_MAIN);
	const auto x = SettingsManager::getInstance()->get(setting);

	int arr[] = { 0 /* disabled */, x /* current value */,
		x + 1, x + 2, x + 5, x + 10, x + 20, x + 50, x + 100,
		x - 1, x - 2, x - 5, x - 10, x - 20, x - 50, x - 100,
		x * 11 / 10, x * 6 / 5, x * 3 / 2, x * 2, x * 3, x * 4, x * 5, x * 10, x * 100,
		x * 10 / 11, x * 5 / 6, x * 2 / 3, x / 2, x / 3, x / 4, x / 5, x / 10, x / 100 };

	// set ensures ordered unique members; remove_if performs range and relevancy checking.
	auto minDelta = (x >= 1024) ? (20 * pow(1024, floor(log(static_cast<float>(x)) / log(1024.)) - 1)) : 0;
	set<int> values(arr, std::remove_if(arr, arr + sizeof(arr) / sizeof(int), [x, minDelta](int i) {
		return i < 0 || i > ThrottleManager::MAX_LIMIT ||
			(minDelta && i != x && abs(i - x) < minDelta); // not too close to the original value
	}));

	for(auto value: values) {
		auto same = value == x;
		auto formatted = Text::toT(Util::formatBytes(value * 1024));
		auto pos = menu->appendItem(value ? escapeMenu(str(TF_("%1%/s") % formatted)) : T_("Disabled"),
			[setting, value] { ThrottleManager::setSetting(setting, value); }, 0, !same);
		if(same)
			menu->checkItem(pos);
		if(!value)
			menu->appendSeparator();
	}

	menu->appendSeparator();
	menu->appendItem(T_("Custom..."), [this, title, setting, x] {
		ParamDlg dlg(this, title);
		dlg.addIntTextBox(T_("New limit (KiB/s) (0 = infinite)"), Text::toT(std::to_string(x)), 0, ThrottleManager::MAX_LIMIT);
		if(dlg.run() == IDOK) {
			ThrottleManager::setSetting(setting, Util::toUInt(Text::fromT(dlg.getValue())));
		}
	});
}

void MainWindow::handleLimiterMenu(bool upload) {
	auto menu = addChild(WinUtil::Seeds::menu);
	fillLimiterMenu(menu.get(), upload);
	menu->open();
}

void MainWindow::handleReconnect() {
	forwardHub(&HubFrame::reconnect);
}

void MainWindow::forwardHub(void (HubFrame::*f)()) {
	HubFrame* active = dynamic_cast<HubFrame*>(getTabView()->getActive());
	if(active) {
		(active->*f)();
	}
}

void MainWindow::handleTaskbarOverlay() {
	tabs->setOverlayIcon(tabs->getActive(), WinUtil::createIcon(away ? IDI_RED_BALL : IDI_GREEN_BALL, 16), away ? _T("Away") : _T("Available"));
}

void MainWindow::handleQuickConnect() {
	if(!WinUtil::checkNick())
		return;

	ParamDlg dlg(this, T_("Quick Connect"), T_("Hub address or Magnet link"));
	if(dlg.run() == IDOK && !WinUtil::parseLink(dlg.getValue(), false)) {
		HubFrame::openWindow(getTabView(), Text::fromT(dlg.getValue()));
	}
}

void MainWindow::handleConnectFavHubGroup() {
	if(!WinUtil::checkNick())
		return;

	tstring group;
	if(chooseFavHubGroup(T_("Connect to a favorite hub group"), group)) {
		const auto& hubs = FavoriteManager::getInstance()->getFavoriteHubs(Text::fromT(group));
		for(auto& hub: hubs) {
			HubFrame::openWindow(getTabView(), hub->getServer());
		}
	}
}

void MainWindow::handleSized(const dwt::SizedEvent& sz) {
	if(sz.isMinimized) {
		handleMinimized();
	} else if(sz.isMaximized || sz.isRestored) {
		if(SETTING(AUTO_AWAY)) {
			Util::decAway();
		}
		if(!SETTING(ALWAYS_TRAY)) {
			notifier->setVisible(false);
		}
		layout();
	}
}

void MainWindow::handleMinimized() {
	if(SETTING(AUTO_AWAY)) {
		Util::incAway();
	}
	if(SETTING(MINIMIZE_TRAY) != WinUtil::isShift()) {
		if(!SETTING(ALWAYS_TRAY)) {
			notifier->setVisible(true);
		}
		setVisible(false);
	}
}

LRESULT MainWindow::handleActivateApp(WPARAM wParam) {
	if(wParam == TRUE && tray_pm) {
		notifier->setIcon(mainSmallIcon);
		tray_pm = false;
	}
	return 0;
}

void MainWindow::statusMessage(time_t t, const string& m) {
	if(!status)
		return;

	string message(m);
	WinUtil::reducePaths(message);
	status->setText(STATUS_STATUS, Text::toT("[" + Util::getShortTimeString(t) + "] " + message));
}

bool MainWindow::chooseFavHubGroup(const tstring& title, tstring& group) {
	set<tstring, noCaseStringLess> groups;

	const FavHubGroups& favHubGroups = FavoriteManager::getInstance()->getFavHubGroups();
	if(favHubGroups.empty()) {
		dwt::MessageBox(this).show(T_("No favorite hub group found"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return false;
	}

	for(auto& i: favHubGroups)
		groups.insert(Text::toT(i.first));

	ParamDlg dlg(this, title, T_("Select a favorite hub group"), TStringList(groups.begin(), groups.end()));
	if(dlg.run() == IDOK) {
		group = dlg.getValue();
		return true;
	}
	return false;
}

void MainWindow::setSaveTimer() {
	setTimer([this]() -> bool { saveSettings(); return true; }, SETTING(SETTINGS_SAVE_INTERVAL) * 60 * 1000, TIMER_SAVE);
}

void MainWindow::saveSettings() {
	saveWindowSettings();
	SettingsManager::getInstance()->save();
}

void MainWindow::saveWindowSettings() {
	{
		const auto& views = tabs->getChildren();
		auto active = tabs->getActive();

		auto wm = WindowManager::getInstance();
		wm->clear();

		for(auto& i: views) {
			auto child = static_cast<MDIChildFrame<dwt::Container>*>(i);
			auto params = child->getWindowParams();
			if(child == active)
				addActiveParam(params);
			wm->add(child->getId(), params);
		}
	}

	SettingsManager::getInstance()->set(SettingsManager::TRANSFERS_PANED_POS, paned->getSplitterPos(0));

	WINDOWPLACEMENT wp { sizeof(wp) };
	if(::GetWindowPlacement(this->handle(), &wp)) {

		dwt::Rectangle rect { wp.rcNormalPosition };
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_X, static_cast<int>(rect.left()));
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_POS_Y, static_cast<int>(rect.top()));
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_X, static_cast<int>(rect.width()));
		SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_SIZE_Y, static_cast<int>(rect.height()));

		if(wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE) {
			SettingsManager::getInstance()->set(SettingsManager::MAIN_WINDOW_STATE, static_cast<int>(wp.showCmd));
		}
	}
}

bool MainWindow::handleClosing() {
	if(!closing()) {
		if(!SETTING(CONFIRM_EXIT) || (dwt::MessageBox(this).show(T_("Really exit?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES))
		{

			saveWindowSettings();

			setVisible(false);
			if(transfers)
				transfers->prepareClose();

			ConnectionManager::getInstance()->removeListener(this);
			HttpManager::getInstance()->removeListener(this);
			LogManager::getInstance()->removeListener(this);
			QueueManager::getInstance()->removeListener(this);

			{
				Lock l(ccpmMutex);
				ccpms.clear();
			}

			SearchManager::getInstance()->disconnect();
			ConnectionManager::getInstance()->disconnect();

			DWORD id;
			dcdebug("Starting stopper\n");
			stopperThread = CreateThread(NULL, 0, &stopper, this, 0, &id);
		}
		return false;
	}

	dcdebug("Waiting for stopper\n");
	// This should end immediately, as it only should be the stopper that sends another WM_CLOSE
	WaitForSingleObject(stopperThread, 60*1000);
	CloseHandle(stopperThread);
	::PostQuitMessage(0);
	dcdebug("Quit message posted\n");

	WinUtil::mainWindow = nullptr;
	return true;
}

void MainWindow::layout() {
	dwt::Rectangle r { getClientSize() };

	if(!rebar->empty()) {
		auto y = rebar->refresh();
		r.pos.y += y;
		r.size.y -= y;
	}

	if(status) {
		r.size.y -= status->refresh();
	}

	paned->resize(r);
}

LRESULT MainWindow::handleWhereAreYou() {
	return SingleInstance::WMU_WHERE_ARE_YOU;
}

void MainWindow::updateStatus() {
	uint64_t now= GET_TICK();
	uint64_t tdiff = now - lastTick;
	if (tdiff < 100) {
		tdiff = 1;
	}

	uint64_t up = Socket::getTotalUp();
	uint64_t down = Socket::getTotalDown();
	uint64_t updiff = up - lastUp;
	uint64_t downdiff = down - lastDown;

	lastTick = now;
	lastUp = up;
	lastDown = down;

	/*
		todo: move this to client
		<TotalUpload type="int64">-1654786742</TotalUpload>
		int64_t lastUp > uint64_t lastUp
	*/
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + static_cast<int64_t>(updiff));
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + static_cast<int64_t>(downdiff));

	if(SETTING(AWAY_IDLE)) {
		LASTINPUTINFO info = { sizeof(LASTINPUTINFO) };
		if((::GetLastInputInfo(&info) && static_cast<int64_t>(::GetTickCount() - info.dwTime) > SETTING(AWAY_IDLE) * 60 * 1000) ^ awayIdle) {
			awayIdle = !awayIdle;
			awayIdle ? Util::incAway() : Util::decAway();
		}
	}

	if(!status)
		return;

	if(Util::getAway() != away) {
		away = !away;
		updateAwayStatus();
	}

	tstring s = Text::toT(Client::getCounts());
	status->setText(STATUS_COUNTS, s);
	status->setToolTip(STATUS_COUNTS, str(TF_("Hubs: %1%") % s));

	auto freeSlots = UploadManager::getInstance()->getFreeSlots();
	auto totalSlots = SETTING(SLOTS);
	status->setText(STATUS_SLOTS, str(TF_("%1%/%2%") % freeSlots % totalSlots), true);
	status->setToolTip(STATUS_SLOTS, str(TF_("%1% free slots of %2% total upload slots - Click to adjust") % freeSlots % totalSlots));
	if(!freeSlots ^ fullSlots) {
		fullSlots = !freeSlots;
		status->setIcon(STATUS_SLOTS, WinUtil::statusIcon(fullSlots ? IDI_SLOTS_FULL : IDI_SLOTS));
	}

	s = Text::toT(Util::formatBytes(down));
	status->setText(STATUS_DOWN_TOTAL, s);
	status->setToolTip(STATUS_DOWN_TOTAL, str(TF_("D: %1%") % s));

	s = Text::toT(Util::formatBytes(up));
	status->setText(STATUS_UP_TOTAL, s);
	status->setToolTip(STATUS_UP_TOTAL, str(TF_("U: %1%") % s));

	s = Text::toT(Util::formatBytes((downdiff*1000)/tdiff));
	status->setText(STATUS_DOWN_DIFF, str(TF_("%1%/s") % s));
	status->setToolTip(STATUS_DOWN_DIFF, str(TF_("D: %1%/s (%2%)") % s % DownloadManager::getInstance()->getDownloadCount()));

	s = Text::toT(Util::formatBytes((updiff*1000)/tdiff));
	status->setText(STATUS_UP_DIFF, str(TF_("%1%/s") % s));
	status->setToolTip(STATUS_UP_DIFF, str(TF_("U: %1%/s (%2%)") % s % UploadManager::getInstance()->getUploadCount()));

	s = Text::toT(Util::formatBytes(ThrottleManager::getDownLimit() * 1024));
	status->setText(STATUS_DOWN_LIMIT, str(TF_("%1%/s") % s));
	status->setToolTip(STATUS_DOWN_LIMIT, str(TF_("Download limit: %1%/s - Click to adjust") % s));

	s = Text::toT(Util::formatBytes(ThrottleManager::getUpLimit() * 1024));
	status->setText(STATUS_UP_LIMIT, str(TF_("%1%/s") % s));
	status->setToolTip(STATUS_UP_LIMIT, str(TF_("Upload limit: %1%/s - Click to adjust") % s));
}

void MainWindow::updateAwayStatus() {
	status->setIcon(STATUS_AWAY, WinUtil::statusIcon(away ? IDI_USER_AWAY : IDI_USER));
	status->setToolTip(STATUS_AWAY, away ? (T_("Status: Away - Double-click to switch to Available")) :
		(T_("Status: Available - Double-click to switch to Away")));

	if (!SETTING(DISABLE_TASKBAR_MENU))
		handleTaskbarOverlay();
}

MainWindow::~MainWindow() {
	dwt::Application::instance().removeFilter(filterIter);
}

namespace {

BOOL CALLBACK updateFont(HWND hwnd, LPARAM prevFont) {
	dwt::Control* widget = dwt::hwnd_cast<dwt::Control*>(hwnd);
	if(widget && widget->getFont()->handle() == reinterpret_cast<HFONT>(prevFont)) {
		widget->setFont(WinUtil::font);
		widget->layout();
		if(dynamic_cast<dwt::Grid*>(widget->getParent())) {
			widget->getParent()->layout();
		}
	}
	return TRUE;
}

BOOL CALLBACK updateColors(HWND hwnd, LPARAM) {
	dwt::Control* widget = dwt::hwnd_cast<dwt::Control*>(hwnd);
	if(widget) {
		// not every widget is custom colored; those that are also catch ID_UPDATECOLOR (see WinUtil::setColor).
		widget->sendCommand(ID_UPDATECOLOR);
	}
	return TRUE;
}

} // unnamed namespace

void MainWindow::handleSettings() {
	// this may be called directly from the tray icon when the main window is still hidden
	if(isIconic())
		handleRestore();

	auto prevTCP = SETTING(TCP_PORT);
	auto prevUDP = SETTING(UDP_PORT);
	auto prevTLS = SETTING(TLS_PORT);

	auto prevConn = SETTING(INCOMING_CONNECTIONS);
	auto prevConn6 = SETTING(INCOMING_CONNECTIONS6);
	auto prevMapper = SETTING(MAPPER);
	auto prevMapper6 = SETTING(MAPPER6);
	auto prevBind = SETTING(BIND_ADDRESS);
	auto prevBind6 = SETTING(BIND_ADDRESS6);
	auto prevProxy = CONNSETTING(OUTGOING_CONNECTIONS);

	auto prevGeo = SETTING(GET_USER_COUNTRY);

	auto prevFont = SETTING(MAIN_FONT);
	auto prevUploadFont = SETTING(UPLOAD_FONT);
	auto prevDownloadFont = SETTING(DOWNLOAD_FONT);

	auto prevTray = SETTING(ALWAYS_TRAY);
	auto prevSortFavUsersFirst = SETTING(SORT_FAVUSERS_FIRST);
	auto prevURLReg = SETTING(URL_HANDLER);
	auto prevMagnetReg = SETTING(MAGNET_REGISTER);
	auto prevDcextReg = SETTING(DCEXT_REGISTER);
	auto prevSystemStartupReg = SETTING(REGISTER_SYSTEM_STARTUP);
	auto prevSettingsSave = SETTING(SETTINGS_SAVE_INTERVAL);

	if(SettingsDialog(this).run() == IDOK) {
		SettingsManager::getInstance()->save();

		try {
			ConnectivityManager::getInstance()->setup((SETTING(INCOMING_CONNECTIONS) != prevConn ||
				SETTING(TCP_PORT) != prevTCP || SETTING(UDP_PORT) != prevUDP || SETTING(TLS_PORT) != prevTLS ||
				SETTING(MAPPER) != prevMapper || SETTING(BIND_ADDRESS) != prevBind), 
				(SETTING(INCOMING_CONNECTIONS6) != prevConn6 || 
				SETTING(TCP_PORT) != prevTCP || SETTING(UDP_PORT) != prevUDP || SETTING(TLS_PORT) != prevTLS ||
				SETTING(MAPPER6) != prevMapper6 || SETTING(BIND_ADDRESS6) != prevBind6));
		} catch (const Exception& e) {
			showPortsError(e.getError());
		}

		auto outConns = CONNSETTING(OUTGOING_CONNECTIONS);
		if(outConns != prevProxy || outConns == SettingsManager::OUTGOING_SOCKS5) {
			Socket::socksUpdated();
		}

		ClientManager::getInstance()->infoUpdated();

		if(SETTING(GET_USER_COUNTRY) != prevGeo) {
			if(SETTING(GET_USER_COUNTRY)) {
				GeoManager::getInstance()->init();
				checkGeoUpdate();
			} else {
				GeoManager::getInstance()->close();
			}
		}

		if(SETTING(MAIN_FONT) != prevFont) {
			auto prev = WinUtil::font;
			WinUtil::initFont();
			::EnumChildWindows(handle(), updateFont, reinterpret_cast<LPARAM>(prev->handle()));
			mainMenu->setFont(WinUtil::font);
			::DrawMenuBar(handle());
		}
		if(SETTING(UPLOAD_FONT) != prevUploadFont) {
			WinUtil::updateUploadFont();
		}
		if(SETTING(DOWNLOAD_FONT) != prevDownloadFont) {
			WinUtil::updateDownloadFont();
		}

		bool newColors = false;
		if(static_cast<COLORREF>(SETTING(TEXT_COLOR)) != WinUtil::textColor) {
			WinUtil::textColor = SETTING(TEXT_COLOR);
			newColors = true;
		}
		if(static_cast<COLORREF>(SETTING(BACKGROUND_COLOR)) != WinUtil::bgColor) {
			WinUtil::bgColor = SETTING(BACKGROUND_COLOR);
			newColors = true;
		}
		if(newColors) {
			::EnumChildWindows(handle(), updateColors, 0);
		}

		if(SETTING(ALWAYS_TRAY) != prevTray)
			notifier->setVisible(SETTING(ALWAYS_TRAY));

		if(SETTING(SORT_FAVUSERS_FIRST) != prevSortFavUsersFirst)
			HubFrame::resortUsers();

		if(SETTING(URL_HANDLER) != prevURLReg)
			WinUtil::registerHubHandlers();
		if(SETTING(MAGNET_REGISTER) != prevMagnetReg)
			WinUtil::registerMagnetHandler();
		if(SETTING(DCEXT_REGISTER) != prevDcextReg)
			WinUtil::registerDcextHandler();
		
		if(SETTING(REGISTER_SYSTEM_STARTUP) != prevSystemStartupReg)
			WinUtil::setApplicationStartup();

		if(SETTING(SETTINGS_SAVE_INTERVAL) != prevSettingsSave)
			setSaveTimer();

		refreshPluginMenu();
	}
}

void MainWindow::showPortsError(const string& port) {
	dwt::MessageBox(this).show(Text::toT(str(F_(
		"Unable to open %1% port. Searching or file transfers will not work correctly until you "
		"change settings or turn off any application that might be using that port."
		) % port)), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
}

void MainWindow::handleOpenFileList() {
	tstring file;
	if(WinUtil::browseFileList(this, file)) {
		auto u = DirectoryListing::getUserFromFilename(Text::fromT(file));
		if(u) {
			DirectoryListingFrame::openWindow(getTabView(), file, Util::emptyStringT,
				HintedUser(u, Util::emptyString), 0, DirectoryListingFrame::FORCE_ACTIVE);
		} else {
			dwt::MessageBox(this).show(T_("Invalid file list name"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
				dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		}
	}
}

DWORD WINAPI MainWindow::stopper(void* p) {
	MainWindow* mf = reinterpret_cast<MainWindow*>(p);
	HWND wnd, wnd2 = NULL;

	while( (wnd=::GetWindow(mf->getTabView()->handle(), GW_CHILD)) != NULL) {
		if(wnd == wnd2) {
			::Sleep(100);
		} else {
			::PostMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	::PostMessage(mf->handle(), WM_CLOSE, 0, 0);
	return 0;
}

class ListMatcher : public Thread {
public:
	ListMatcher(StringList&& files) : files(move(files)) { }

	int run() {
		for(auto& i: files) {
			UserPtr u = DirectoryListing::getUserFromFilename(i);
			if (!u)
				continue;
			HintedUser user(u, Util::emptyString);
			DirectoryListing dl(user);
			try {
				dl.loadFile(i);
				int matched = QueueManager::getInstance()->matchListing(dl);
				LogManager::getInstance()->message(str(FN_("%1%: matched %2% file", "%1%: matched %2% files", matched)
				% Util::toString(ClientManager::getInstance()->getNicks(user))
				% matched));
			} catch(const Exception&) {
			}
		}
		delete this;
		return 0;
	}

private:
	StringList files;
};

void MainWindow::handleMatchAll() {
	auto matcher = new ListMatcher(File::findFiles(Util::getListPath(), "*.xml*"));
	try {
		matcher->start();
	} catch(const ThreadException& e) {
		LogManager::getInstance()->message(e.getError());
		delete matcher;
	}
}

void MainWindow::handleActivate(bool active) {
	// hide the temporary menu bar when moving out of the main window
	if(!active && !SETTING(SHOW_MENU_BAR) && ::GetMenu(handle())) {
		::SetMenu(handle(), nullptr);
	}

	// focus the active tab window
	Container* w = getTabView()->getActive();
	if(w) {
		w->setFocus();
	}
}

void MainWindow::completeVersionUpdate(bool success, const string& result) {
	if(success) { try {
		SimpleXML xml;
		xml.fromXML(result);
		xml.stepIn();

		string url = Text::fromT(links.homepage);

		if(xml.findChild("Version")) {
			auto remoteVersion = Util::toDouble(xml.getChildData());
			auto newStable = remoteVersion > VERSIONFLOAT;

			bool newTesting = false;
			if ((SETTING(TESTING_STATUS) != SettingsManager::TESTING_DISABLED) && !newStable && (GET_TIME() % (60 * 60) < 5 * 60)) {
				// when available, we offer a testing release to a random subset of users; 
				// for those who is using the latest stable version and did not opt-out of displaying testing related info
				// plus has some luck so the current time is in the first 5 minutes of the current hour.
				xml.resetCurrentChild();
				if(xml.findChild("TestingVersion")) {
					auto remoteTestingVersion = Util::toDouble(xml.getChildData());
					newTesting = remoteTestingVersion > VERSIONFLOAT;
				}
			}

			if(newStable || newTesting) {
				xml.resetCurrentChild();
				if(xml.findChild(newTesting ? "TestingURL" : "URL")) {
					url = xml.getChildData();
				}
				xml.resetCurrentChild();
				if(xml.findChild(newTesting ? "TestingTitle" : "Title")) {
					const string& title = xml.getChildData();
					xml.resetCurrentChild();
					if(xml.findChild(newTesting ? "TestingMessage" : "Message")) {
						if(url.empty()) {
							const string& msg = xml.getChildData();
							dwt::MessageBox(this).show(Text::toT(msg), Text::toT(title));
						} else {
							if(dwt::MessageBox(this).show(
								str(TF_("%1%\nOpen download page?") % Text::toT(xml.getChildData())), Text::toT(title),
								dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)
							{
								WinUtil::openLink(Text::toT(url));
							}
						}
					}
				}

			} else if(remoteVersion < VERSIONFLOAT) {
				// how awesome, the user is using a testing version!

				if(SETTING(TESTING_STATUS) == SettingsManager::TESTING_ENABLED) {
					notify(T_("Testing version of FearDC"),
						T_("Thank you for using a testing version of FearDC!") + _T("\n\n") +
						T_("Feel free to report any bug via Help > Links > Report a bug.") + _T("\n\n") +
						T_("This message will show up in the status bar in the future.") + _T("\n") +
						T_("Testing nags can be fully disabled via Settings > Advanced."),
					nullptr, nullptr),
					SettingsManager::getInstance()->set(SettingsManager::TESTING_STATUS,
						SettingsManager::TESTING_SEEN_ONCE);

				} else if(SETTING(TESTING_STATUS) == SettingsManager::TESTING_SEEN_ONCE) {
					if(status) {
						status->setText(STATUS_STATUS,
							T_("Thank you for using a testing version of FearDC!") + _T(" ") +
							T_("Feel free to report any bug via Help > Links > Report a bug."));
					}
				}
			}
		}

		xml.resetCurrentChild();
		if(xml.findChild("Links")) {
			xml.stepIn();
			if(xml.findChild("Homepage")) {
				links.homepage = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Downloads")) {
				links.downloads = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("GeoIP_City")) {
				links.geoip_city = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Faq")) {
				links.faq = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Bugs")) {
				links.bugs = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Features")) {
				links.features = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Help")) {
				links.help = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Forum")) {
				links.discuss = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Blog")) {
				links.blog = Text::toT(xml.getChildData());
			}
			xml.resetCurrentChild();
			if(xml.findChild("Community")) {
				links.community = Text::toT(xml.getChildData());
			}
			xml.stepOut();
		}

		xml.resetCurrentChild();
		if(xml.findChild("Blacklist")) {
			xml.stepIn();
			while(xml.findChild("Blacklisted")) {
				const string& domain = xml.getChildAttrib("Domain");
				if(domain.empty())
					continue;
				const string& reason = xml.getChildAttrib("Reason");
				if(reason.empty())
					continue;
				FavoriteManager::getInstance()->addBlacklist(domain, reason);
			}
			xml.stepOut();
		}

		xml.resetCurrentChild();
		if(xml.findChild("GeoIPStaticServe")) {
			geoStaticServe = true;
		}
		
		xml.stepOut();
	} catch (const Exception&) { } }

	// check after the version.xml download in case it contains updated GeoIP2 links.
	if(SETTING(GET_USER_COUNTRY)) {
		checkGeoUpdate();
	}
}

void MainWindow::checkGeoUpdate() {
	checkGeoUpdateDB();
}

void MainWindow::checkGeoUpdateDB() {
	// update when the database is non-existent, older than 16 days and it is frequently updated (GeoIP2 updates every 14 days)

	try {
		File f(GeoManager::getDbPath() + ".gz", File::READ, File::OPEN);

		if (f.getSize() > 0 && (geoStaticServe || static_cast<time_t>(f.getLastModified()) > GET_TIME() - 3600 * 24 * 16))
			return;

	} catch (const FileException&) { }

	updateGeoDB();
}

void MainWindow::updateGeo() {
	if (SETTING(GET_USER_COUNTRY)) {
		updateGeoDB();

	} else {
		dwt::MessageBox(this).show(T_("IP -> country mappings are disabled. Turn them back on via Settings > Appearance."),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
	}
}

void MainWindow::updateGeoDB() {
	auto& conn = conns[CONN_GEOIP];

	if (static_cast<HttpConnection*>(conn))
		return;

	auto& file = geoFile;

	try {
		file.reset(new File(GeoManager::getDbPath() + ".gz.tmp", File::WRITE, File::CREATE | File::TRUNCATE));
	} catch(const FileException&) {
		LogManager::getInstance()->message("The GeoIP2 database could not be updated");
		return;
	}

	LogManager::getInstance()->message("Updating the GeoIP2 database...");
	conn = HttpManager::getInstance()->download(Text::fromT(links.geoip_city), file.get());
}

void MainWindow::completeGeoUpdate(bool success) {
	if(success) {
		try {
			File::renameFile(GeoManager::getDbPath() + ".gz.tmp", GeoManager::getDbPath() + ".gz");
		} catch(const FileException&) { }

		GeoManager::getInstance()->update();

		LogManager::getInstance()->message("The GeoIP2 database has been successfully updated");

	} else {
		File::deleteFile(GeoManager::getDbPath() + ".gz.tmp");
		LogManager::getInstance()->message("The GeoIP2 database could not be updated");
	}
}

void MainWindow::parseCommandLine(const tstring& cmdLine) {
	// this string may or may not contain the executable's path at the beginning.

	tstring::size_type i;

	if (
		(i = cmdLine.find(_T("dchub://"))) != tstring::npos ||
		(i = cmdLine.find(_T("nmdcs://"))) != tstring::npos ||
		(i = cmdLine.find(_T("adc://"))) != tstring::npos ||
		(i = cmdLine.find(_T("adcs://"))) != tstring::npos ||
		(i = cmdLine.find(_T("magnet:?"))) != tstring::npos
	) {
		WinUtil::parseLink(cmdLine.substr(i), false);

	} else if ((i = cmdLine.find(_T("dcext:"))) != tstring::npos) {
		auto path = Text::fromT(cmdLine.substr(i + 6));
		Util::sanitizeUrl(path);
		PluginInfoDlg(this, path).run();
	}
}

LRESULT MainWindow::handleCopyData(LPARAM lParam) {
	parseCommandLine(reinterpret_cast<LPCTSTR>(reinterpret_cast<COPYDATASTRUCT*>(lParam)->lpData));
	return TRUE;
}

void MainWindow::handleHashProgress() {
	HashProgressDlg(this, false).run();
}

void MainWindow::handleCloseFavGroup(bool reversed) {
	tstring group;
	if(chooseFavHubGroup(reversed ? T_("Close hubs not in a favorite group") : T_("Close all hubs of a favorite group"), group))
		HubFrame::closeFavGroup(Text::fromT(group), reversed);
}

void MainWindow::handleAbout() {
	AboutDlg(this).run();
}

void MainWindow::handleOpenDownloadsDir() {
	WinUtil::openFile(Text::toT(SETTING(DOWNLOAD_DIRECTORY)));
}

LRESULT MainWindow::handleEndSession() {
	saveSettings();
	QueueManager::getInstance()->saveQueue();

	return 0;
}

void MainWindow::handleToolbarCustomized() {
	string setting;
	StringList list = toolbar->getLayout();
	for(auto i = list.begin(), iend = list.end(); i != iend; ++i) {
		setting += *i;
		if(i != iend - 1)
			setting += ',';
	}
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR, setting);
}

bool MainWindow::handleToolbarContextMenu(const dwt::ScreenCoordinate& pt) {
	auto menu = addChild(WinUtil::Seeds::menu);
	menu->setTitle(T_("Toolbar"));

	menu->appendItem(T_("&Customize\tDouble-click"), [this] { toolbar->customize(); },
		WinUtil::menuIcon(IDI_SETTINGS), true, true);

	{
		auto size = menu->appendPopup(T_("Size"));
		int sizes[] = { 16, 20, 22, 24, 32 };
		int setting = SETTING(TOOLBAR_SIZE);
		for(size_t i = 0, iend = sizeof(sizes) / sizeof(int); i < iend; ++i) {
			int n = sizes[i];
			unsigned pos = size->appendItem(Text::toT(std::to_string(n)), [=, this] { handleToolbarSize(n); });
			if(n == setting)
				size->checkItem(pos);
		}
	}

	menu->appendSeparator();
	menu->appendItem(T_("&Hide\tCtrl+1"), [this] { switchToolbar(); });

	menu->open(pt);
	return true;
}

void MainWindow::handleToolbarSize(int size) {
	SettingsManager::getInstance()->set(SettingsManager::TOOLBAR_SIZE, size);
	switchToolbar();
	switchToolbar();
}

void MainWindow::switchMenuBar() {
	auto show = !SETTING(SHOW_MENU_BAR);
	SettingsManager::getInstance()->set(SettingsManager::SHOW_MENU_BAR, show);
	::SetMenu(handle(), show ? mainMenu->handle() : nullptr);
	viewMenu->checkItem(viewIndexes["Menu"], show);

	if(!show) {
		dwt::MessageBox(this).show(T_("The menu bar is now hidden. Press Alt or F10 to temporarily bring it back into view."),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONINFORMATION);
	}
}

void MainWindow::switchToolbar() {
	if(toolbar) {
		rebar->remove(toolbar);
		toolbar->close();
		toolbar = 0;

		SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, false);
		viewMenu->checkItem(viewIndexes["Toolbar"], false);

	} else {
		SettingsManager::getInstance()->set(SettingsManager::SHOW_TOOLBAR, true);
		initToolbar();

		// re-check currently opened static windows
		const auto& views = tabs->getChildren();
		for(auto& i: views) {
			toolbar->setButtonChecked(static_cast<MDIChildFrame<dwt::Container>*>(i)->getId(), true);
		}
	}

	layout();
}

void MainWindow::switchTransfers() {
	if(transfers) {
		transfers->prepareClose();
		paned->removeChild(transfers);
		transfers = 0;

		SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, false);
		viewMenu->checkItem(viewIndexes["Transfers"], false);
	} else {
		SettingsManager::getInstance()->set(SettingsManager::SHOW_TRANSFERVIEW, true);
		initTransfers();
	}

	paned->layout();
}

void MainWindow::switchStatus() {
	if(status) {
		status->close();
		status = 0;

		SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, false);
		viewMenu->checkItem(viewIndexes["Status"], false);

	} else {
		SettingsManager::getInstance()->set(SettingsManager::SHOW_STATUSBAR, true);
		initStatusBar();
	}

	layout();
}

void MainWindow::handleSlotsMenu() {
	auto changeSlots = [this](int slots) -> function<void ()> { return [=, this] {
		SettingsManager::getInstance()->set(SettingsManager::SLOTS, slots);
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::SLOTS), slots);
		updateStatus();
	}; };

	auto menu = addChild(WinUtil::Seeds::menu);

	menu->setTitle(T_("Upload slots"), WinUtil::menuIcon(IDI_SLOTS));

	auto setting = SETTING(SLOTS);
	for(decltype(setting) i = 1; i < setting + 10; ++i) {
		auto pos = menu->appendItem(Text::toT(std::to_string(i)), changeSlots(i), nullptr, true, i == setting);
		if(i == setting)
			menu->checkItem(pos);
	}

	menu->open();
}

void MainWindow::handleRefreshFileList() {
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true);
}

void MainWindow::handleRestore() {
	::SetForegroundWindow(handle());
	if(isIconic())
		restore();
	else
		setVisible(true);
}

bool MainWindow::handleMessage(const MSG& msg, LRESULT& retVal) {
	bool handled = dwt::Window::handleMessage(msg, retVal);
	if(!handled && msg.message == WM_COMMAND && tabs) {
		handled = tabs->handleMessage(msg, retVal);
	}
	return handled;
}

void MainWindow::handleTrayContextMenu() {
	auto menu = addChild(WinUtil::Seeds::menu);

	menu->appendItem(T_("Show"), [this] { handleRestore(); }, WinUtil::menuIcon(IDI_DCPP), true, true);
	menu->appendItem(T_("Open downloads directory"), [this] { handleOpenDownloadsDir(); }, WinUtil::menuIcon(IDI_OPEN_DL_DIR));
	menu->appendSeparator();
	menu->appendItem(T_("Settings"), [this] { handleSettings(); }, WinUtil::menuIcon(IDI_SETTINGS));
	fillLimiterMenu(menu->appendPopup(T_("Download limit"), WinUtil::menuIcon(IDI_DLIMIT), false), false);
	fillLimiterMenu(menu->appendPopup(T_("Upload limit"), WinUtil::menuIcon(IDI_ULIMIT), false), true);
	menu->appendSeparator();
	menu->appendItem(T_("Exit"), [this] { close(true); }, WinUtil::menuIcon(IDI_EXIT));

	POINT pt;
	::GetCursorPos(&pt);
	menu->open(dwt::ScreenCoordinate(dwt::Point(pt)), TPM_BOTTOMALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON);
}

void MainWindow::handleTrayClicked() {
	if(isIconic()) {
		handleRestore();
	} else {
		minimize();
	}
}

void MainWindow::handleTrayUpdate() {
	notifier->setTooltip(Text::toT(str(F_(APPNAME " " VERSIONSTRING "\r\nD: %1%/s (%2%)\r\nU: %3%/s (%4%)") %
		Util::formatBytes(DownloadManager::getInstance()->getRunningAverage()) %
		DownloadManager::getInstance()->getDownloadCount() %
		Util::formatBytes(UploadManager::getInstance()->getRunningAverage()) %
		UploadManager::getInstance()->getUploadCount())));
}

void MainWindow::handleWhatsThis() {
	sendMessage(WM_SYSCOMMAND, SC_CONTEXTHELP);
}

void MainWindow::on(ConnectionManagerListener::Connected, ConnectionQueueItem* cqi, UserConnection* uc) noexcept {
	if(cqi->getType() == CONNECTION_TYPE_PM) {

		// C-C PMs are not supported outside of PM windows.
		if(!SETTING(POPUP_PMS)) {
			uc->disconnect(true);
			return;
		}

		// until a message is received, no need to open a PM window.
		Lock l(ccpmMutex);
		ccpms[cqi->getUser()] = uc;
		uc->addListener(this);
	}
}

void MainWindow::on(ConnectionManagerListener::Removed, ConnectionQueueItem* cqi) noexcept {
	if(cqi->getType() == CONNECTION_TYPE_PM) {
		Lock l(ccpmMutex);
		ccpms.erase(cqi->getUser());
	}
}

void MainWindow::on(UserConnectionListener::PrivateMessage, UserConnection* uc, const ChatMessage& message) noexcept {
	auto user = uc->getHintedUser();
	callAsync([this, message, user] {
		auto opened = PrivateFrame::isOpen(user) || PrivateFrame::gotMessage(getTabView(), message, user.hint, false);

		// remove our listener as the PM window now handles the conn.
		{
			Lock l(ccpmMutex);
			auto i = ccpms.find(user);
			if(i != ccpms.end()) {
				auto uc = i->second;
				uc->removeListener(this);
				if(!opened) {
					uc->disconnect(true);
				}
				ccpms.erase(i);
			}
		}

		if(opened) {
			TrayPM();
		}
	});
}

void MainWindow::on(HttpManagerListener::Failed, HttpConnection* c, const string&) noexcept {
	if(c == conns[CONN_VERSION]) {
		conns[CONN_VERSION] = nullptr;
		callAsync([this] { completeVersionUpdate(false, Util::emptyString); });

	} else if(c == conns[CONN_GEOIP]) {
		conns[CONN_GEOIP] = nullptr;
		geoFile.reset();
		completeGeoUpdate(false);
	}
}

void MainWindow::on(HttpManagerListener::Complete, HttpConnection* c, OutputStream* stream) noexcept {
	if(c == conns[CONN_VERSION]) {
		conns[CONN_VERSION] = nullptr;

		auto str = static_cast<StringOutputStream*>(stream)->getString();
		callAsync([str, this] { completeVersionUpdate(true, str); });

	} else if(c == conns[CONN_GEOIP]) {
		conns[CONN_GEOIP] = nullptr;
		geoFile.reset();
		completeGeoUpdate(true);
	}
}

void MainWindow::on(HttpManagerListener::ResetStream, HttpConnection* c) noexcept {
	if(c == conns[CONN_GEOIP]) {
		geoFile->setPos(0);
		geoFile->setEOF();
	}
}

void MainWindow::on(LogManagerListener::Message, time_t t, const string& m) noexcept {
	callAsync([=, this] { statusMessage(t, m); });
}

void MainWindow::on(QueueManagerListener::Finished, QueueItem* qi, const string& dir, int64_t speed) noexcept {
	if(qi->isSet(QueueItem::FLAG_CLIENT_VIEW)) {
		if(qi->isSet(QueueItem::FLAG_USER_LIST)) {
			auto file = qi->getListName();
			auto user = qi->getDownloads()[0]->getHintedUser();
			callAsync([this, file, dir, user, speed] {
				DirectoryListingFrame::openWindow(getTabView(), Text::toT(file), Text::toT(dir), user, speed);
				WinUtil::notify(WinUtil::NOTIFICATION_FINISHED_FL, Text::toT(Util::getFileName(file)), [=, this] {
					DirectoryListingFrame::activateWindow(user);
				});
			});

		} else if(qi->isSet(QueueItem::FLAG_TEXT)) {
			auto file = qi->getTarget();
			callAsync([this, file] {
				TextFrame::openWindow(getTabView(), file, true, true);
			});
		}
	}

	if(!qi->isSet(QueueItem::FLAG_USER_LIST)) {
		auto file = qi->getTarget();
		callAsync([this, file] {
			WinUtil::notify(WinUtil::NOTIFICATION_FINISHED_DL, Text::toT(file), [=, this] {
				FinishedDLFrame::focusFile(getTabView(), file);
			});
		});
	}
}

void MainWindow::on(QueueManagerListener::PartialList, const HintedUser& aUser, const string& text) noexcept {
	callAsync([this, aUser, text] { DirectoryListingFrame::openWindow(getTabView(), aUser, text, 0); });
}

void MainWindow::openWindow(const string& id, const WindowParams& params) {
	if(0);
#define compare_id(frame) else if(frame::id == id) frame::parseWindowParams(getTabView(), params)
	compare_id(HubFrame);
	compare_id(PrivateFrame);
	compare_id(DirectoryListingFrame);
	compare_id(PublicHubsFrame);
	compare_id(FavHubsFrame);
	compare_id(UsersFrame);
	compare_id(QueueFrame);
	compare_id(FinishedDLFrame);
	compare_id(FinishedULFrame);
	compare_id(ADLSearchFrame);
	compare_id(NotepadFrame);
	compare_id(SystemFrame);
	compare_id(StatsFrame);
	compare_id(TextFrame);
	compare_id(ACFrame);
#undef compare_id
}
