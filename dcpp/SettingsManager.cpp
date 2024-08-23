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

#include "stdinc.h"
#include "SettingsManager.h"

#include "SimpleXML.h"
#include "Util.h"
#include "File.h"
#include "version.h"
#include "AdcHub.h"
#include "CID.h"
#include "ConnectivityManager.h"
#include "SearchManager.h"
#include "StringTokenizer.h"

namespace dcpp {

StringList SettingsManager::connectionSpeeds;

const string SettingsManager::settingTags[] =
{
	// Strings
	"Nick", "UploadSpeed", "Description", "DownloadDirectory", "EMail", "ExternalIp", "ExternalIp6",
	"MainFont", "TextViewerFont", "UploadFont", "DownloadFont",
	"ADLSearchFrameOrder", "ADLSearchFrameWidths",
	"DirectoryListingFrameOrder", "DirectoryListingFrameWidths",
	"FavHubsFrameOrder", "FavHubsFrameWidths",
	"FinishedDLFilesOrder", "FinishedDLFilesWidths",
	"FinishedDLUsersOrder", "FinishedDLUsersWidths",
	"FinishedULFilesOrder", "FinishedULFilesWidths",
	"FinishedULUsersOrder", "FinishedULUsersWidths",
	"HubFrameOrder", "HubFrameWidths",
	"PublicHubsFrameOrder", "PublicHubsFrameWidths",
	"QueueFrameOrder", "QueueFrameWidths",
	"SearchFrameOrder", "SearchFrameWidths",
	"TransfersOrder", "TransfersWidths",
	"UsersFrameOrder", "UsersFrameWidths",
	"HublistServers", "HttpProxy", "LogDirectory", "LogFormatPostDownload",
	"LogFormatPostFinishedDownload", "LogFormatPostUpload", "LogFormatMainChat", "LogFormatPrivateChat",
	"TempDownloadDirectory", "BindAddress", "BindAddress6", "SocksServer", "SocksUser", "SocksPassword", "ConfigVersion",
	"DefaultAwayMessage", "TimeStampsFormat", "CountryFormat",
	"CID", "LogFileMainChat", "LogFilePrivateChat",
	"LogFileStatus", "LogFileUpload", "LogFileDownload", "LogFileFinishedDownload", "LogFileSystem",
	"LogFormatSystem", "LogFormatStatus",
	"TLSPrivateKeyFile", "TLSCertificateFile", "TLSTrustedCertificatesPath",
	"Language", "Toolbar", "LastSearchType", "Mapper", "Mapper6",
	"SoundMainChat", "SoundPM", "SoundPMWindow", "SoundFinishedDL", "SoundFinishedFL", "LastSharedFolder",
	"SharingSkiplistExtensions", "SharingSkiplistRegEx", "SharingSkiplistPaths", "WhitelistOpenURIs",
	"ACFrameOrder", "ACFrameWidths",
	"SENTRY",
	// Ints
	"IncomingConnections", "IncomingConnections6", "OutgoingConnections", "InPort", "InPort6", 
	"UDPPort", "UDPPort6", "TLSPort", "TLSPort6",
	"SocksPort", "SocketInBuffer", "SocketOutBuffer",
	"TextColor", "BackgroundColor", "UploadTextColor", "UploadBgColor", "DownloadTextColor",
	"DownloadBgColor", "LinkColor", "LogColor",
	"BandwidthLimitStart", "BandwidthLimitEnd", "MaxDownloadSpeedRealTime",
	"MaxUploadSpeedTime", "MaxDownloadSpeedPrimary", "MaxUploadSpeedPrimary",
	"SlotsAlternateLimiting", "SlotsPrimaryLimiting",
	"MainWindowState", "MainWindowSizeX", "MainWindowSizeY", "MainWindowPosX",
	"MainWindowPosY", "SettingsWidth", "SettingsHeight", "SettingsPage",
	"HighestPrioSize", "HighPrioSize", "NormalPrioSize", "LowPrioSize", "AutoDropSpeed",
	"AutoDropInterval", "AutoDropElapsed", "AutoDropInactivity", "AutoDropMinSources",
	"AutoDropFilesize",
	"BalloonMainChat", "BalloonPM", "BalloonPMWindow", "BalloonFinishedDL", "BalloonFinishedFL",
	"ADLSearchFrameSort", "DirectoryListingFrameSort", "FinishedDLFilesSort",
	"FinishedDLUsersSort", "FinishedULFilesSort", "FinishedULUsersSort", "HubFrameSort",
	"PublicHubsFrameSort", "QueueFrameSort", "SearchFrameSort", "TransfersSort", "UsersFrameSort",
	"AwayIdle", "AutoRefreshTime", "AutoSearchLimit", "BufferSize", "DownloadSlots",
	"HubLastLogLines", "MagnetAction", "MaxCommandLength", "MaxCompression", "MaxDownloadSpeed",
	"MaxFilelistSize", "MaxHashSpeed", "MaxMessageLines", "MaxPMWindows", "MinMessageLines",
	"MinUploadSpeed", "PMLastLogLines", "SearchHistory", "SetMinislotSize",
	"SettingsSaveInterval", "Slots", "TabStyle", "TabWidth", "ToolbarSize", "AutoSearchInterval",
	"MaxExtraSlots", "TestingStatus",
	"SENTRY",
	// Bools
	"AddFinishedInstantly", "AdlsBreakOnFirst",
	"AllowUntrustedClients", "AllowUntrustedHubs", "AlwaysCCPM", "AlwaysTray", "AutoAway", "LastAway",
	"AutoDetectIncomingConnection", "AutoFollow", "AutoKick", "AutoKickNoFavs", "AutoSearch",
	"AutoSearchAutoMatch", "AutoDropAll", "AutoDropDisconnect", "AutoDropFilelists",
	"AwayCompLock", "AwayTimeStamp", "BoldFinishedDownloads", "BoldFinishedUploads", "BoldFL",
	"BoldHub", "BoldPm", "BoldQueue", "BoldSearch", "BoldSystemLog", "ClearSearch",
	"ClickableChatLinks",
	"CompressTransfers", "ConfirmADLSRemoval", "ConfirmExit", "ConfirmHubClosing",
	"ConfirmHubRemoval", "ConfirmItemRemoval", "ConfirmUserRemoval", "DcextRegister",
	"DontDlAlreadyQueued", "DontDLAlreadyShared", "EnableCCPM", "FavShowJoins",
	"FilterMessages",
	"FinishedDLOnlyFull", "FollowLinks", "GetUserCountry", "ShowUserIp", "ShowUserCountry", "GetUserInfo",
	"HubUserCommands", "IgnoreBotPms", "IgnoreHubPms", "SkipTrayBotPms", "OpenNewWindow", "KeepFinishedFiles",
	"KeepLists", "AllowNATTraversal", "ListDuplicates", "LogDownloads", "LogFilelistTransfers", "LogFinishedDownloads",
	"LogMainChat", "LogPrivateChat", "LogStatusMessages", "LogSystem", "LogUploads", "MagnetAsk",
	"MagnetRegister", "MinimizeToTray", "NoAwayMsgToBots", "NoIpOverride", "NoIpOverride6", "OpenUserCmdHelp",
	"OwnerDrawnMenus", "PopupBotPms", "PopupHubPms", "PopupPMs", "PopunderFilelist", "PopunderPm",
	"LowestPrio", "PromptPassword", "QueueFrameShowTree", "RequireTLS", "DisableNmdcCtmTLS", "SearchFilterShared",
	"SearchOnlyFreeSlots", "SegmentedDL", "SendBloom", "SendUnknownCommands",
	"SFVCheck", "ShareHidden", "ShowJoins", "ShowMenuBar", "ShowStatusbar", "ShowToolbar",
	"ShowTransferview", "SkipZeroByte", "SocksResolve", "SortFavUsersFirst",
	"StatusInChat", "TimeDependentThrottle", "TimeStamps",
	"ToggleActiveTab", "UrlHandler", "UseCTRLForLineHistory", "UseSystemIcons",
	"UsersFilterFavorite", "UsersFilterOnline", "UsersFilterQueue", "UsersFilterWaiting",
	"RegisterSystemStartup", "DontLogCCPMChat", "AboutCfgDisclaimer", "EnableTaskbarPreview",
	"EnableSUDP",
	"SENTRY",
	// Int64
	"TotalUpload", "TotalDownload", "SharingSkiplistMinSize", "SharingSkiplistMaxSize",
	"SENTRY",
	// Floats
	"FileListPanedPos", "HubPanedPos", "QueuePanedPos", "SearchPanedPos",
	"TransfersPanedPos", "UsersPanedPos",
	"SENTRY"
};

SettingsManager::SettingsManager() {
	connectionSpeeds = {
		"0.005",
		"0.01",
		"0.02",
		"0.05",
		"0.1",
		"0.2",
		"0.5",
		"1",
		"2",
		"5",
		"10",
		"20",
		"50",
		"100",
		"1000"
	};

	for(int i=0; i<SETTINGS_LAST; i++)
		isSet[i] = false;

	for(int i=0; i<INT_LAST-INT_FIRST; i++) {
		intDefaults[i] = 0;
		intSettings[i] = 0;
	}
	for(int i=0; i<BOOL_LAST-BOOL_FIRST; i++) {
		boolDefaults[i] = false;
		boolSettings[i] = false;
	}
	for(int i=0; i<INT64_LAST-INT64_FIRST; i++) {
		int64Defaults[i] = 0;
		int64Settings[i] = 0;
	}
	for(int i=0; i<FLOAT_LAST-FLOAT_FIRST; i++) {
		floatDefaults[i] = 0;
		floatSettings[i] = 0;
	}

	setDefault(DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_DOWNLOADS));
	setDefault(TEMP_DOWNLOAD_DIRECTORY, Util::getPath(Util::PATH_USER_LOCAL) + "Incomplete" PATH_SEPARATOR_STR);
	setDefault(BIND_ADDRESS, "0.0.0.0");
	setDefault(BIND_ADDRESS6, "::");
	setDefault(SLOTS, 1);
	setDefault(TCP_PORT, 0);
	setDefault(UDP_PORT, 0);
	setDefault(TLS_PORT, 0);
	setDefault(TCP_PORT6, 0);
	setDefault(UDP_PORT6, 0);
	setDefault(TLS_PORT6, 0);
	setDefault(INCOMING_CONNECTIONS, INCOMING_ACTIVE);
	setDefault(INCOMING_CONNECTIONS6, INCOMING_DISABLED);
	setDefault(OUTGOING_CONNECTIONS, OUTGOING_DIRECT);
	setDefault(AUTO_DETECT_CONNECTION, true);
	setDefault(AUTO_FOLLOW, true);
	setDefault(CLEAR_SEARCH, true);
	setDefault(CLICKABLE_CHAT_LINKS, true);
	setDefault(SHARE_HIDDEN, false);
	setDefault(FILTER_MESSAGES, true);
	setDefault(MINIMIZE_TRAY, true);
	setDefault(ALWAYS_TRAY, true);
	setDefault(AUTO_SEARCH, false);
	setDefault(AUTO_SEARCH_INTERVAL, 120);
	setDefault(TIME_STAMPS, true);
	setDefault(POPUP_HUB_PMS, true);
	setDefault(POPUP_BOT_PMS, true);
	setDefault(IGNORE_HUB_PMS, false);
	setDefault(IGNORE_BOT_PMS, false);
	setDefault(SKIP_TRAY_BOT_PMS, false);
	setDefault(LIST_DUPES, true);
	setDefault(BUFFER_SIZE, 64);
	setDefault(HUBLIST_SERVERS, "https://te-home.net/?do=hublist&get=hublist.xml.bz2;https://hublist.pwiam.com/hublist.xml.bz2;https://dchublist.biz/?do=hublist&get=hublist.xml.bz2;https://dchublist.ru/hublist.xml.bz2;https://dcnf.github.io/Hublist/hublist.xml.bz2;");
	setDefault(DOWNLOAD_SLOTS, 6);
	setDefault(MAX_DOWNLOAD_SPEED, 0);
	setDefault(LOG_DIRECTORY, Util::getPath(Util::PATH_USER_LOCAL) + "Logs" PATH_SEPARATOR_STR);
	setDefault(LOG_UPLOADS, false);
	setDefault(LOG_DOWNLOADS, false);
	setDefault(LOG_FINISHED_DOWNLOADS, false);
	setDefault(LOG_PRIVATE_CHAT, false);
	setDefault(LOG_MAIN_CHAT, false);
	setDefault(STATUS_IN_CHAT, true);
	setDefault(SHOW_JOINS, false);
	setDefault(UPLOAD_SPEED, connectionSpeeds[0]);
	setDefault(USE_SYSTEM_ICONS, true);
	setDefault(POPUP_PMS, true);
	setDefault(MIN_UPLOAD_SPEED, 0);
	setDefault(LOG_FORMAT_POST_DOWNLOAD, "%Y-%m-%d %H:%M: %[target] " + string(_("downloaded from")) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIactual]), %[speed], %[time], %[fileTR]");
	setDefault(LOG_FORMAT_POST_FINISHED_DOWNLOAD, "%Y-%m-%d %H:%M: %[target] " + string(_("downloaded from")) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIsession]), %[speed], %[time], %[fileTR]");
	setDefault(LOG_FORMAT_POST_UPLOAD, "%Y-%m-%d %H:%M: %[source] " + string(_("uploaded to")) + " %[userNI] (%[userCID]), %[fileSI] (%[fileSIactual]), %[speed], %[time], %[fileTR]");
	setDefault(LOG_FORMAT_MAIN_CHAT, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_PRIVATE_CHAT, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_STATUS, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FORMAT_SYSTEM, "[%Y-%m-%d %H:%M] %[message]");
	setDefault(LOG_FILE_MAIN_CHAT, "%[hubURL].log");
	setDefault(LOG_FILE_STATUS, "%[hubURL]_status.log");
	setDefault(LOG_FILE_PRIVATE_CHAT, "%[userNI].%[userCID].log");
	setDefault(LOG_FILE_UPLOAD, "Uploads.log");
	setDefault(LOG_FILE_DOWNLOAD, "Downloads.log");
	setDefault(LOG_FILE_FINISHED_DOWNLOAD, "Finished_downloads.log");
	setDefault(LOG_FILE_SYSTEM, "system.log");
	setDefault(GET_USER_INFO, true);
	setDefault(URL_HANDLER, false);
	setDefault(SETTINGS_WIDTH, 700);
	setDefault(SETTINGS_HEIGHT, 600);
	setDefault(SOCKS_PORT, 1080);
	setDefault(SOCKS_RESOLVE, 1);
	setDefault(CONFIG_VERSION, "0.181");		// 0.181 is the last version missing configversion
	setDefault(KEEP_LISTS, false);
	setDefault(ALLOW_NAT_TRAVERSAL, true);
	setDefault(AUTO_KICK, false);
	setDefault(QUEUEFRAME_SHOW_TREE, true);
	setDefault(COMPRESS_TRANSFERS, true);
	setDefault(SFV_CHECK, true);
	setDefault(AUTO_AWAY, false);
	setDefault(LAST_AWAY, false);
	setDefault(AWAY_COMP_LOCK, true);
	setDefault(AWAY_IDLE, 10);
	setDefault(AWAY_TIMESTAMP, false);
	setDefault(TIME_STAMPS_FORMAT, "%H:%M");
	setDefault(COUNTRY_FORMAT, "%[2code] - %[name]");
	setDefault(MAX_COMPRESSION, 6);
	setDefault(NO_AWAYMSG_TO_BOTS, true);
	setDefault(SKIP_ZERO_BYTE, false);
	setDefault(ADLS_BREAK_ON_FIRST, false);
	setDefault(HUB_USER_COMMANDS, true);
	setDefault(AUTO_SEARCH_AUTO_MATCH, true);
	setDefault(LOG_FILELIST_TRANSFERS, false);
	setDefault(LOG_SYSTEM, false);
	setDefault(SEND_UNKNOWN_COMMANDS, true);
	setDefault(MAX_HASH_SPEED, 0);
	setDefault(OPEN_USER_CMD_HELP, true);
	setDefault(GET_USER_COUNTRY, true);
	setDefault(SHOW_USER_IP, false);
	setDefault(SHOW_USER_COUNTRY, false);
	setDefault(FAV_SHOW_JOINS, false);
	setDefault(LOG_STATUS_MESSAGES, false);
	setDefault(SHOW_MENU_BAR, true);
	setDefault(SHOW_TRANSFERVIEW, true);
	setDefault(SHOW_STATUSBAR, true);
	setDefault(SHOW_TOOLBAR, true);
	setDefault(POPUNDER_PM, false);
	setDefault(POPUNDER_FILELIST, false);
	setDefault(MAGNET_REGISTER, true);
	setDefault(DCEXT_REGISTER, true);
	setDefault(MAGNET_ASK, true);
	setDefault(MAGNET_ACTION, MAGNET_AUTO_SEARCH);
	setDefault(ADD_FINISHED_INSTANTLY, false);
	setDefault(DONT_DL_ALREADY_SHARED, false);
	setDefault(USE_CTRL_FOR_LINE_HISTORY, true);
	setDefault(JOIN_OPEN_NEW_WINDOW, false);
	setDefault(HUB_LAST_LOG_LINES, 10);
	setDefault(PM_LAST_LOG_LINES, 10);
	setDefault(TOGGLE_ACTIVE_WINDOW, false);
	setDefault(SEARCH_HISTORY, 10);
	setDefault(SET_MINISLOT_SIZE, 512);
	setDefault(MAX_FILELIST_SIZE, 512);
	setDefault(PRIO_HIGHEST_SIZE, 64);
	setDefault(PRIO_HIGH_SIZE, 0);
	setDefault(PRIO_NORMAL_SIZE, 0);
	setDefault(PRIO_LOW_SIZE, 0);
	setDefault(PRIO_LOWEST, false);
	setDefault(AUTODROP_SPEED, 1024);
	setDefault(AUTODROP_INTERVAL, 10);
	setDefault(AUTODROP_ELAPSED, 15);
	setDefault(AUTODROP_INACTIVITY, 10);
	setDefault(AUTODROP_MINSOURCES, 2);
	setDefault(AUTODROP_FILESIZE, 0);
	setDefault(AUTODROP_ALL, false);
	setDefault(AUTODROP_FILELISTS, false);
	setDefault(AUTODROP_DISCONNECT, false);
	setDefault(NO_IP_OVERRIDE, false);
	setDefault(SEARCH_ONLY_FREE_SLOTS, false);
	setDefault(SEARCH_FILTER_SHARED, true);
	setDefault(SOCKET_IN_BUFFER, 0); // OS default
	setDefault(SOCKET_OUT_BUFFER, 0); // OS default
	setDefault(TLS_TRUSTED_CERTIFICATES_PATH, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR);
	setDefault(TLS_PRIVATE_KEY_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.key");
	setDefault(TLS_CERTIFICATE_FILE, Util::getPath(Util::PATH_USER_CONFIG) + "Certificates" PATH_SEPARATOR_STR "client.crt");
	setDefault(BOLD_FINISHED_DOWNLOADS, true);
	setDefault(BOLD_FINISHED_UPLOADS, true);
	setDefault(BOLD_QUEUE, true);
	setDefault(BOLD_HUB, true);
	setDefault(BOLD_PM, true);
	setDefault(BOLD_FL, true);
	setDefault(BOLD_SEARCH, true);
	setDefault(BOLD_SYSTEM_LOG, true);
	setDefault(AUTO_REFRESH_TIME, 60);
	setDefault(AUTO_SEARCH_LIMIT, 5);
	setDefault(AUTO_KICK_NO_FAVS, false);
	setDefault(PROMPT_PASSWORD, false);
	setDefault(DONT_DL_ALREADY_QUEUED, false);
	setDefault(MAX_COMMAND_LENGTH, 512*1024);
	setDefault(ALLOW_UNTRUSTED_HUBS, true);
	setDefault(ALLOW_UNTRUSTED_CLIENTS, true);
	setDefault(SORT_FAVUSERS_FIRST, false);
	setDefault(SEGMENTED_DL, true);
	setDefault(FOLLOW_LINKS, false);
	setDefault(SEND_BLOOM, true);
	setDefault(OWNER_DRAWN_MENUS, true);
	setDefault(FINISHED_DL_ONLY_FULL, true);
	setDefault(CONFIRM_EXIT, true);
	setDefault(CONFIRM_HUB_CLOSING, true);
	setDefault(CONFIRM_HUB_REMOVAL, true);
	setDefault(CONFIRM_USER_REMOVAL, true);
	setDefault(CONFIRM_ITEM_REMOVAL, true);
	setDefault(CONFIRM_ADLS_REMOVAL, true);
	setDefault(TOOLBAR_SIZE, 20);
	setDefault(TAB_WIDTH, 150);
	setDefault(TAB_STYLE, TAB_STYLE_OD | TAB_STYLE_BROWSER);
	setDefault(FILE_LIST_PANED_POS, .3);
	setDefault(HUB_PANED_POS, .7);
	setDefault(QUEUE_PANED_POS, .3);
	setDefault(SEARCH_PANED_POS, .2);
	setDefault(TRANSFERS_PANED_POS, .7);
	setDefault(USERS_PANED_POS, .7);
	setDefault(KEEP_FINISHED_FILES, false);
	setDefault(MIN_MESSAGE_LINES, 1);
	setDefault(MAX_MESSAGE_LINES, 10);
	setDefault(MAX_UPLOAD_SPEED_MAIN, 0);
	setDefault(MAX_DOWNLOAD_SPEED_MAIN, 0);
	setDefault(TIME_DEPENDENT_THROTTLE, false);
	setDefault(MAX_DOWNLOAD_SPEED_ALTERNATE, 0);
	setDefault(MAX_UPLOAD_SPEED_ALTERNATE, 0);
	setDefault(BANDWIDTH_LIMIT_START, 1);
	setDefault(BANDWIDTH_LIMIT_END, 1);
	setDefault(SLOTS_ALTERNATE_LIMITING, 1);
	setDefault(SLOTS_PRIMARY, 3);
	setDefault(SETTINGS_SAVE_INTERVAL, 10);
	setDefault(BALLOON_MAIN_CHAT, BALLOON_DISABLED);
	setDefault(BALLOON_PM, BALLOON_DISABLED);
	setDefault(BALLOON_PM_WINDOW, BALLOON_DISABLED);
	setDefault(BALLOON_FINISHED_DL, BALLOON_ALWAYS);
	setDefault(BALLOON_FINISHED_FL, BALLOON_DISABLED);
	setDefault(USERS_FILTER_ONLINE, false);
	setDefault(USERS_FILTER_FAVORITE, true);
	setDefault(USERS_FILTER_QUEUE, false);
	setDefault(USERS_FILTER_WAITING, false);
	setDefault(MAX_PM_WINDOWS, 50);
	setDefault(REQUIRE_TLS, true); // True by default: We assume TLS is commonplace enough among ADC clients.
	setDefault(DISABLE_NMDC_TLS_CTM, false);
	setDefault(ENABLE_CCPM, true);
	setDefault(ALWAYS_CCPM, false);
	setDefault(DONT_LOG_CCPM, false);
	setDefault(LAST_SHARED_FOLDER, Util::emptyString);
	setDefault(SHARING_SKIPLIST_EXTENSIONS, Util::emptyString);
	setDefault(SHARING_SKIPLIST_REGEX, Util::emptyString);
	setDefault(SHARING_SKIPLIST_PATHS, Util::emptyString);
	setDefault(SHARING_SKIPLIST_MINSIZE, 0);
	setDefault(SHARING_SKIPLIST_MAXSIZE, 0);
	setDefault(REGISTER_SYSTEM_STARTUP, false);
	setDefault(MAX_EXTRA_SLOTS, 3);
	setDefault(TESTING_STATUS, TESTING_ENABLED);
	setDefault(WHITELIST_OPEN_URIS, "http:;https:;www;mailto:");
	setDefault(ENABLE_SUDP, true);
	setDefault(AC_DISCLAIM, true);

	setSearchTypeDefaults();

#ifdef _WIN32
	setDefault(MAIN_WINDOW_STATE, SW_SHOWNORMAL);
	setDefault(MAIN_WINDOW_SIZE_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_SIZE_Y, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_X, CW_USEDEFAULT);
	setDefault(MAIN_WINDOW_POS_Y, CW_USEDEFAULT);
	setDefault(UPLOAD_TEXT_COLOR, RGB(255, 255, 255));
	setDefault(UPLOAD_BG_COLOR, RGB(205, 60, 55));
	setDefault(DOWNLOAD_TEXT_COLOR, RGB(255, 255, 255));
	setDefault(DOWNLOAD_BG_COLOR, RGB(55, 170, 85));

	setDefault(ENABLE_TASKBAR_PREVIEW, true);
#endif
}

void SettingsManager::load(string const& aFileName)
{
	try {
		SimpleXML xml;

		xml.fromXML(File(aFileName, File::READ, File::OPEN).read());

		xml.resetCurrentChild();

		xml.stepIn();

		if(xml.findChild("Settings"))
		{
			xml.stepIn();

			int i;

			for(i=STR_FIRST; i<STR_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(StrSetting(i), xml.getChildData());
				xml.resetCurrentChild();
			}
			for(i=INT_FIRST; i<INT_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(IntSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(i=BOOL_FIRST; i<BOOL_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(BoolSetting(i), Util::toInt(xml.getChildData()));
				xml.resetCurrentChild();
			}
			for(i=FLOAT_FIRST; i<FLOAT_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(FloatSetting(i), Util::toInt(xml.getChildData()) / 1000.);
				xml.resetCurrentChild();
			}
			for(i=INT64_FIRST; i<INT64_LAST; i++)
			{
				const string& attr = settingTags[i];
				dcassert(attr.find("SENTRY") == string::npos);

				if(xml.findChild(attr))
					set(Int64Setting(i), Util::toInt64(xml.getChildData()));
				xml.resetCurrentChild();
			}

			xml.stepOut();
		}

		xml.resetCurrentChild();
		if(xml.findChild("SearchTypes")) {
			searchTypes.clear();
			xml.stepIn();
			while(xml.findChild("SearchType")) {
				const string& extensions = xml.getChildData();
				if(extensions.empty()) {
					continue;
				}
				const string& name = xml.getChildAttrib("Id");
				if(name.empty()) {
					continue;
				}
				searchTypes[name] = StringTokenizer<string>(extensions, ';').getTokens();
			}
			xml.stepOut();
		}

		if(SETTING(PRIVATE_ID).length() != 39 || !CID(SETTING(PRIVATE_ID))) {
			set(PRIVATE_ID, CID::generate().toBase32());
		}

		double v = Util::toDouble(SETTING(CONFIG_VERSION));

		/* Handle setting migrations here with the following pattern:
		 *
		 * if(v < 0.x) { // Fix old settings here } */

		if(v < DCVERSIONFLOAT && SETTING(TESTING_STATUS) != TESTING_DISABLED) {
			// moving up to a new version; reset preferences when not disabled.
			unset(TESTING_STATUS);
		}

		if(v <= 0.674) {
			// Formats changed, might as well remove these...
			unset(LOG_FORMAT_POST_DOWNLOAD);
			unset(LOG_FORMAT_POST_UPLOAD);
			unset(LOG_FORMAT_MAIN_CHAT);
			unset(LOG_FORMAT_PRIVATE_CHAT);
			unset(LOG_FORMAT_STATUS);
			unset(LOG_FORMAT_SYSTEM);
			unset(LOG_FILE_MAIN_CHAT);
			unset(LOG_FILE_STATUS);
			unset(LOG_FILE_PRIVATE_CHAT);
			unset(LOG_FILE_UPLOAD);
			unset(LOG_FILE_DOWNLOAD);
			unset(LOG_FILE_SYSTEM);
		}

		// previous incoming connection modes, kept here for back-compat.
		enum { OLD_INCOMING_DIRECT, OLD_INCOMING_UPNP, OLD_INCOMING_NAT, OLD_INCOMING_PASSIVE };

		if(v <= 0.770 && SETTING(INCOMING_CONNECTIONS) != OLD_INCOMING_PASSIVE) {
			set(AUTO_DETECT_CONNECTION, false); //Don't touch if it works
		}

		if(v <= 0.799) {
			// port previous conn settings
			switch(SETTING(INCOMING_CONNECTIONS)) {
			case OLD_INCOMING_UPNP: set(INCOMING_CONNECTIONS, INCOMING_ACTIVE_UPNP); break;
			case OLD_INCOMING_PASSIVE: set(INCOMING_CONNECTIONS, INCOMING_PASSIVE); break;
			default: set(INCOMING_CONNECTIONS, INCOMING_ACTIVE); break;
			}
		}

		if(v <= 0.782) {
			// These were remade completely...
			unset(USERSFRAME_ORDER);
			unset(USERSFRAME_WIDTHS);
		}

		if(v <= 0.791) {
			// the meaning of a blank default away message has changed: it now means "no away message".
			if(SETTING(DEFAULT_AWAY_MESSAGE).empty()) {
				set(DEFAULT_AWAY_MESSAGE, "I'm away. State your business and I might answer later if you're lucky.");
			}
		}

		if(v < 0.820) {
			// reset search columns
			unset(SEARCHFRAME_ORDER);
			unset(SEARCHFRAME_WIDTHS);
			unset(SEARCHFRAME_SORT);

			// reset the toolbar (new buttons).
			unset(TOOLBAR);
		}

		if(v <= 0.830) {
			unset(MAX_COMMAND_LENGTH);
		}

		if(v < 0.841) {
			// reset as a column has been added to show status icons.
			unset(FAVHUBSFRAME_ORDER);
			unset(FAVHUBSFRAME_WIDTHS);
		}

		if(v <= 0.851) {
			// reset as a column has been added to show status icons.
			unset(PUBLICHUBSFRAME_ORDER);
			unset(PUBLICHUBSFRAME_WIDTHS);
		}

		if(v <= 0.868) {
			// Move back to default as this is now true by default:
			// We assume TLS is commonplace enough among ADC clients.
			unset(REQUIRE_TLS);
		}

		if(v <= 0.871) {
			// add all the newly introduced default hublist servers automatically. 
			// change this to the version number of the previous release each time a new default hublist server entry added.
			string lists = get(HUBLIST_SERVERS);
			StringTokenizer<string> t(getDefault(HUBLIST_SERVERS), ';');
			
			for(auto& i: t.getTokens()) {
				if(lists.find(i) == string::npos)
					lists += ";" + i;
			}
			set(HUBLIST_SERVERS, lists);
		}

		if(SETTING(SET_MINISLOT_SIZE) < 512)
			set(SET_MINISLOT_SIZE, 512);
		if(SETTING(AUTODROP_INTERVAL) < 1)
			set(AUTODROP_INTERVAL, 1);
		if(SETTING(AUTODROP_ELAPSED) < 1)
			set(AUTODROP_ELAPSED, 1);
		if(SETTING(AUTO_SEARCH_LIMIT) > 5)
			set(AUTO_SEARCH_LIMIT, 5);
		else if(SETTING(AUTO_SEARCH_LIMIT) < 1)
			set(AUTO_SEARCH_LIMIT, 1);

		if(SETTING(AUTO_SEARCH_INTERVAL) < 120)
			set(AUTO_SEARCH_INTERVAL, 120);

#ifdef DCPP_REGEN_CID
		set(PRIVATE_ID, CID::generate().toBase32());
#endif

		File::ensureDirectory(SETTING(TLS_TRUSTED_CERTIFICATES_PATH));

		fire(SettingsManagerListener::Load(), xml);

		xml.stepOut();

	} catch(const Exception&) {
		if(!CID(SETTING(PRIVATE_ID)))
			set(PRIVATE_ID, CID::generate().toBase32());
	}
}

void SettingsManager::save(string const& aFileName) {

	SimpleXML xml;
	xml.addTag("DCPlusPlus");
	xml.stepIn();
	xml.addTag("Settings");
	xml.stepIn();

	int i;
	string type("type"), curType("string");

	for(i=STR_FIRST; i<STR_LAST; i++)
	{
		if(i == CONFIG_VERSION) {
			xml.addTag(settingTags[i], DCVERSIONSTRING);
			xml.addChildAttrib(type, curType);
		} else if(isSet[i]) {
			xml.addTag(settingTags[i], get(StrSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}

	curType = "int";
	for(i=INT_FIRST; i<INT_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], get(IntSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	for(i=BOOL_FIRST; i<BOOL_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], get(BoolSetting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	for(i=FLOAT_FIRST; i<FLOAT_LAST; i++)
	{
		if(isSet[i]) {
			xml.addTag(settingTags[i], static_cast<int>(get(FloatSetting(i), false) * 1000.));
			xml.addChildAttrib(type, curType);
		}
	}
	curType = "int64";
	for(i=INT64_FIRST; i<INT64_LAST; i++)
	{
		if(isSet[i])
		{
			xml.addTag(settingTags[i], get(Int64Setting(i), false));
			xml.addChildAttrib(type, curType);
		}
	}
	xml.stepOut();
	
	xml.addTag("SearchTypes");
	xml.stepIn();
	for(auto& i: searchTypes) {
		xml.addTag("SearchType", Util::toString(";", i.second));
		xml.addChildAttrib("Id", i.first);
	}
	xml.stepOut();

	fire(SettingsManagerListener::Save(), xml);

	try {
		File out(aFileName + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		BufferedOutputStream<false> f(&out);
		f.write(SimpleXML::utf8Header);
		xml.toXML(&f);
		f.flush();
		out.close();
		File::deleteFile(aFileName);
		File::renameFile(aFileName + ".tmp", aFileName);
	} catch(const FileException&) {
		// ...
	}
}

HubSettings SettingsManager::getHubSettings() const {
	HubSettings ret;
	ret.get(HubSettings::Nick) = get(NICK);
	ret.get(HubSettings::Description) = get(DESCRIPTION);
	ret.get(HubSettings::Email) = get(EMAIL);
	ret.get(HubSettings::ShowJoins) = get(SHOW_JOINS);
	ret.get(HubSettings::FavShowJoins) = get(FAV_SHOW_JOINS);
	ret.get(HubSettings::LogMainChat) = get(LOG_MAIN_CHAT);
	ret.get(HubSettings::DisableCtmTLS) = get(DISABLE_NMDC_TLS_CTM);

	ret.get(HubSettings::Connection) = CONNSETTING(INCOMING_CONNECTIONS);
	ret.get(HubSettings::Connection6) = CONNSETTING(INCOMING_CONNECTIONS6);
	
	return ret;
}

void SettingsManager::validateSearchTypeName(const string& name) const {
	if(name.empty() || (name.size() == 1 && name[0] >= '1' && name[0] <= '6')) {
		throw SearchTypeException(_("Invalid search type name"));
	}
	for(int type = SearchManager::TYPE_ANY; type != SearchManager::TYPE_LAST; ++type) {
		if(SearchManager::getTypeStr(type) == name) {
			throw SearchTypeException(_("This search type already exists"));
		}
	}
}

void SettingsManager::setSearchTypeDefaults() {
	searchTypes.clear();

	// for conveniency, the default search exts will be the same as the ones defined by SEGA.
	const auto& searchExts = AdcHub::getSearchExts();
	for(size_t i = 0, n = searchExts.size(); i < n; ++i)
		searchTypes[string(1, '1' + i)] = searchExts[i];

	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::addSearchType(const string& name, const StringList& extensions, bool validated) {
	if(!validated) {
		validateSearchTypeName(name);
	}

	if(searchTypes.find(name) != searchTypes.end()) {
		throw SearchTypeException(_("This search type already exists"));
	}

	searchTypes[name] = extensions;
	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::delSearchType(const string& name) {
	validateSearchTypeName(name);
	searchTypes.erase(name);
	fire(SettingsManagerListener::SearchTypesChanged());
}

void SettingsManager::renameSearchType(const string& oldName, const string& newName) {
	validateSearchTypeName(newName);
	StringList exts = getSearchType(oldName)->second;
	addSearchType(newName, exts, true);
	searchTypes.erase(oldName);
}

void SettingsManager::modSearchType(const string& name, const StringList& extensions) {
	getSearchType(name)->second = extensions;
	fire(SettingsManagerListener::SearchTypesChanged());
}

const StringList& SettingsManager::getExtensions(const string& name) {
	return getSearchType(name)->second;
}

SettingsManager::SearchTypes::iterator SettingsManager::getSearchType(const string& name) {
	auto ret = searchTypes.find(name);
	if(ret == searchTypes.end()) {
		throw SearchTypeException(_("No such search type"));
	}
	return ret;
}

bool SettingsManager::getType(const char* name, int& n, Types& type) const {
	for(n = 0; n < FLOAT_LAST; ++n) {
		if(strcmp(settingTags[n].c_str(), name) == 0) {
			return getType(n, type);
		}
	}
	return false;
}

bool SettingsManager::getType(const int& n, Types& type) const {
	if(n < STR_FIRST || n >= SETTINGS_LAST)
		return false;

	if(n < STR_LAST) {
		type = TYPE_STRING;
	} else if(n < INT_LAST) {
		type = TYPE_INT;
	} else if(n < BOOL_LAST) {
		type = TYPE_BOOL;
	} else if(n < INT64_LAST) {
		type = TYPE_INT64;
	} else {
		type = TYPE_FLOAT;
	}
	return true;
}

} // namespace dcpp
