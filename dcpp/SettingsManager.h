/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_SETTINGS_MANAGER_H
#define DCPLUSPLUS_DCPP_SETTINGS_MANAGER_H

#include "Util.h"
#include "Speaker.h"
#include "Singleton.h"
#include "Exception.h"
#include "HubSettings.h"

namespace dcpp {

STANDARD_EXCEPTION(SearchTypeException);

class SimpleXML;

class SettingsManagerListener {
public:
	virtual ~SettingsManagerListener() { }
	template<int I>	struct X { enum { TYPE = I }; };

	typedef X<0> Load;
	typedef X<1> Save;
	typedef X<2> SearchTypesChanged;

	virtual void on(Load, SimpleXML&) noexcept { }
	virtual void on(Save, SimpleXML&) noexcept { }
	virtual void on(SearchTypesChanged) noexcept { }
};

class SettingsManager : public Singleton<SettingsManager>, public Speaker<SettingsManagerListener>
{
public:
	typedef std::unordered_map<string, StringList> SearchTypes;

	enum Types {
		TYPE_STRING,
		TYPE_INT,
		TYPE_BOOL,
		TYPE_INT64,
		TYPE_FLOAT
	};

	static StringList connectionSpeeds;

	enum StrSetting { STR_FIRST,
		NICK = STR_FIRST, UPLOAD_SPEED, DESCRIPTION, DOWNLOAD_DIRECTORY, EMAIL, EXTERNAL_IP, EXTERNAL_IP6,
		MAIN_FONT, TEXT_VIEWER_FONT, UPLOAD_FONT, DOWNLOAD_FONT,
		ADLSEARCHFRAME_ORDER, ADLSEARCHFRAME_WIDTHS,
		DIRECTORYLISTINGFRAME_ORDER, DIRECTORYLISTINGFRAME_WIDTHS,
		FAVHUBSFRAME_ORDER, FAVHUBSFRAME_WIDTHS,
		FINISHED_DL_FILES_ORDER, FINISHED_DL_FILES_WIDTHS,
		FINISHED_DL_USERS_ORDER, FINISHED_DL_USERS_WIDTHS,
		FINISHED_UL_FILES_ORDER, FINISHED_UL_FILES_WIDTHS,
		FINISHED_UL_USERS_ORDER, FINISHED_UL_USERS_WIDTHS,
		HUBFRAME_ORDER, HUBFRAME_WIDTHS,
		PUBLICHUBSFRAME_ORDER, PUBLICHUBSFRAME_WIDTHS,
		QUEUEFRAME_ORDER, QUEUEFRAME_WIDTHS,
		SEARCHFRAME_ORDER, SEARCHFRAME_WIDTHS,
		TRANSFERS_ORDER, TRANSFERS_WIDTHS,
		USERSFRAME_ORDER, USERSFRAME_WIDTHS,
		HUBLIST_SERVERS, HTTP_PROXY, LOG_DIRECTORY, LOG_FORMAT_POST_DOWNLOAD,
		LOG_FORMAT_POST_FINISHED_DOWNLOAD, LOG_FORMAT_POST_UPLOAD, LOG_FORMAT_MAIN_CHAT, LOG_FORMAT_PRIVATE_CHAT,
		TEMP_DOWNLOAD_DIRECTORY, BIND_ADDRESS, BIND_ADDRESS6, SOCKS_SERVER, SOCKS_USER, SOCKS_PASSWORD, CONFIG_VERSION,
		DEFAULT_AWAY_MESSAGE, TIME_STAMPS_FORMAT, COUNTRY_FORMAT,
		PRIVATE_ID, LOG_FILE_MAIN_CHAT,
		LOG_FILE_PRIVATE_CHAT, LOG_FILE_STATUS, LOG_FILE_UPLOAD, LOG_FILE_DOWNLOAD, LOG_FILE_FINISHED_DOWNLOAD,
		LOG_FILE_SYSTEM, LOG_FORMAT_SYSTEM, LOG_FORMAT_STATUS,
		TLS_PRIVATE_KEY_FILE, TLS_CERTIFICATE_FILE, TLS_TRUSTED_CERTIFICATES_PATH,
		LANGUAGE, TOOLBAR, LAST_SEARCH_TYPE, MAPPER, MAPPER6,
		SOUND_MAIN_CHAT, SOUND_PM, SOUND_PM_WINDOW, SOUND_FINISHED_DL, SOUND_FINISHED_FL, LAST_SHARED_FOLDER,
		SHARING_SKIPLIST_EXTENSIONS, SHARING_SKIPLIST_REGEX, SHARING_SKIPLIST_PATHS, WHITELIST_OPEN_URIS,
		ACFRAME_ORDER, ACFRAME_WIDTHS,
		STR_LAST };

	enum IntSetting { INT_FIRST = STR_LAST + 1,
		INCOMING_CONNECTIONS = INT_FIRST, INCOMING_CONNECTIONS6, OUTGOING_CONNECTIONS, TCP_PORT, TCP_PORT6,
		UDP_PORT, UDP_PORT6, TLS_PORT, TLS_PORT6,
		SOCKS_PORT, SOCKET_IN_BUFFER, SOCKET_OUT_BUFFER,

		TEXT_COLOR, BACKGROUND_COLOR, UPLOAD_TEXT_COLOR, UPLOAD_BG_COLOR, DOWNLOAD_TEXT_COLOR,
		DOWNLOAD_BG_COLOR, LINK_COLOR, LOG_COLOR,

		BANDWIDTH_LIMIT_START, BANDWIDTH_LIMIT_END, MAX_DOWNLOAD_SPEED_ALTERNATE,
		MAX_UPLOAD_SPEED_ALTERNATE, MAX_DOWNLOAD_SPEED_MAIN, MAX_UPLOAD_SPEED_MAIN,
		SLOTS_ALTERNATE_LIMITING, SLOTS_PRIMARY,

		MAIN_WINDOW_STATE, MAIN_WINDOW_SIZE_X, MAIN_WINDOW_SIZE_Y, MAIN_WINDOW_POS_X,
		MAIN_WINDOW_POS_Y, SETTINGS_WIDTH, SETTINGS_HEIGHT, SETTINGS_PAGE,

		PRIO_HIGHEST_SIZE, PRIO_HIGH_SIZE, PRIO_NORMAL_SIZE, PRIO_LOW_SIZE, AUTODROP_SPEED,
		AUTODROP_INTERVAL, AUTODROP_ELAPSED, AUTODROP_INACTIVITY, AUTODROP_MINSOURCES,
		AUTODROP_FILESIZE,

		BALLOON_MAIN_CHAT, BALLOON_PM, BALLOON_PM_WINDOW, BALLOON_FINISHED_DL, BALLOON_FINISHED_FL,

		ADLSEARCHFRAME_SORT, DIRECTORYLISTINGFRAME_SORT, FINISHED_DL_FILES_SORT,
		FINISHED_DL_USERS_SORT, FINISHED_UL_FILES_SORT, FINISHED_UL_USERS_SORT, HUBFRAME_SORT,
		PUBLICHUBSFRAME_SORT, QUEUEFRAME_SORT, SEARCHFRAME_SORT, TRANSFERS_SORT, USERSFRAME_SORT,

		// uncategorized
		AWAY_IDLE, AUTO_REFRESH_TIME, AUTO_SEARCH_LIMIT, BUFFER_SIZE, DOWNLOAD_SLOTS,
		HUB_LAST_LOG_LINES, MAGNET_ACTION, MAX_COMMAND_LENGTH, MAX_COMPRESSION, MAX_DOWNLOAD_SPEED,
		MAX_FILELIST_SIZE, MAX_HASH_SPEED, MAX_MESSAGE_LINES, MAX_PM_WINDOWS, MIN_MESSAGE_LINES,
		MIN_UPLOAD_SPEED, PM_LAST_LOG_LINES, SEARCH_HISTORY, SET_MINISLOT_SIZE,
		SETTINGS_SAVE_INTERVAL, SLOTS, TAB_STYLE, TAB_WIDTH, TOOLBAR_SIZE,
		AUTO_SEARCH_INTERVAL, MAX_EXTRA_SLOTS, TESTING_STATUS,

		INT_LAST };

	enum BoolSetting { BOOL_FIRST = INT_LAST + 1,
		ADD_FINISHED_INSTANTLY = BOOL_FIRST, ADLS_BREAK_ON_FIRST,
		ALLOW_UNTRUSTED_CLIENTS, ALLOW_UNTRUSTED_HUBS, ALWAYS_CCPM, ALWAYS_TRAY, AUTO_AWAY, LAST_AWAY,
		AUTO_DETECT_CONNECTION, AUTO_FOLLOW, AUTO_KICK, AUTO_KICK_NO_FAVS, AUTO_SEARCH,
		AUTO_SEARCH_AUTO_MATCH, AUTODROP_ALL, AUTODROP_DISCONNECT, AUTODROP_FILELISTS,
		AWAY_COMP_LOCK, AWAY_TIMESTAMP, BOLD_FINISHED_DOWNLOADS, BOLD_FINISHED_UPLOADS, BOLD_FL,
		BOLD_HUB, BOLD_PM, BOLD_QUEUE, BOLD_SEARCH, BOLD_SYSTEM_LOG, CLEAR_SEARCH,
		CLICKABLE_CHAT_LINKS,
		COMPRESS_TRANSFERS, CONFIRM_ADLS_REMOVAL, CONFIRM_EXIT, CONFIRM_HUB_CLOSING,
		CONFIRM_HUB_REMOVAL, CONFIRM_ITEM_REMOVAL, CONFIRM_USER_REMOVAL, DCEXT_REGISTER,
		DONT_DL_ALREADY_QUEUED, DONT_DL_ALREADY_SHARED, ENABLE_CCPM, FAV_SHOW_JOINS,
		FILTER_MESSAGES,
		FINISHED_DL_ONLY_FULL, FOLLOW_LINKS, GET_USER_COUNTRY, SHOW_USER_IP, SHOW_USER_COUNTRY, GET_USER_INFO,
		HUB_USER_COMMANDS, IGNORE_BOT_PMS, IGNORE_HUB_PMS, SKIP_TRAY_BOT_PMS, DISABLE_TASKBAR_MENU, JOIN_OPEN_NEW_WINDOW, KEEP_FINISHED_FILES,
		KEEP_LISTS, LIST_DUPES, LOG_DOWNLOADS, LOG_FILELIST_TRANSFERS, LOG_FINISHED_DOWNLOADS,
		LOG_MAIN_CHAT, LOG_PRIVATE_CHAT, LOG_STATUS_MESSAGES, LOG_SYSTEM, LOG_UPLOADS, MAGNET_ASK,
		MAGNET_REGISTER, MINIMIZE_TRAY, NO_AWAYMSG_TO_BOTS, NO_IP_OVERRIDE, NO_IP_OVERRIDE6, OPEN_USER_CMD_HELP,
		OWNER_DRAWN_MENUS, POPUP_BOT_PMS, POPUP_HUB_PMS, POPUP_PMS, POPUNDER_FILELIST, POPUNDER_PM,
		PRIO_LOWEST, PROMPT_PASSWORD, QUEUEFRAME_SHOW_TREE, REQUIRE_TLS, DISABLE_NMDC_TLS_CTM, SEARCH_FILTER_SHARED,
		SEARCH_ONLY_FREE_SLOTS, SEGMENTED_DL, SEND_BLOOM, SEND_UNKNOWN_COMMANDS,
		SFV_CHECK, SHARE_HIDDEN, SHOW_JOINS, SHOW_MENU_BAR, SHOW_STATUSBAR, SHOW_TOOLBAR,
		SHOW_TRANSFERVIEW, SKIP_ZERO_BYTE, SOCKS_RESOLVE, SORT_FAVUSERS_FIRST,
		STATUS_IN_CHAT, TIME_DEPENDENT_THROTTLE, TIME_STAMPS,
		TOGGLE_ACTIVE_WINDOW, URL_HANDLER, USE_CTRL_FOR_LINE_HISTORY, USE_SYSTEM_ICONS,
		USERS_FILTER_FAVORITE, USERS_FILTER_ONLINE, USERS_FILTER_QUEUE, USERS_FILTER_WAITING,
		REGISTER_SYSTEM_STARTUP, DONT_LOG_CCPM, AC_DISCLAIM,
		BOOL_LAST };

	enum Int64Setting { INT64_FIRST = BOOL_LAST + 1,
		TOTAL_UPLOAD = INT64_FIRST, TOTAL_DOWNLOAD,
		SHARING_SKIPLIST_MINSIZE, SHARING_SKIPLIST_MAXSIZE,
		INT64_LAST };

	enum FloatSetting { FLOAT_FIRST = INT64_LAST + 1,
		FILE_LIST_PANED_POS = FLOAT_FIRST, HUB_PANED_POS, QUEUE_PANED_POS, SEARCH_PANED_POS,
		TRANSFERS_PANED_POS, USERS_PANED_POS,
		FLOAT_LAST, SETTINGS_LAST = FLOAT_LAST };

	enum { 		
		INCOMING_DISABLED = -1, 
		INCOMING_ACTIVE, 
		INCOMING_ACTIVE_UPNP, 
		INCOMING_PASSIVE,
		INCOMING_LAST = 4
		};
		
	enum { OUTGOING_DIRECT, OUTGOING_SOCKS5 };

	enum {
		TAB_STYLE_BUTTONS = 1 << 1,
		TAB_STYLE_OD = 1 << 2,
		TAB_STYLE_BROWSER = 1 << 3
	};

	enum {	MAGNET_AUTO_SEARCH, MAGNET_AUTO_DOWNLOAD };

	enum { BALLOON_DISABLED, BALLOON_ALWAYS, BALLOON_BACKGROUND };

	enum { TESTING_ENABLED, TESTING_SEEN_ONCE, TESTING_DISABLED };

	const string& get(StrSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? strSettings[key - STR_FIRST] : strDefaults[key - STR_FIRST];
	}

	int get(IntSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? intSettings[key - INT_FIRST] : intDefaults[key - INT_FIRST];
	}
	bool get(BoolSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? boolSettings[key - BOOL_FIRST] : boolDefaults[key - BOOL_FIRST];
	}
	int64_t get(Int64Setting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? int64Settings[key - INT64_FIRST] : int64Defaults[key - INT64_FIRST];
	}
	float get(FloatSetting key, bool useDefault = true) const {
		return (isSet[key] || !useDefault) ? floatSettings[key - FLOAT_FIRST] : floatDefaults[key - FLOAT_FIRST];
	}

	void set(StrSetting key, string const& value) {
		strSettings[key - STR_FIRST] = value;
		isSet[key] = !value.empty();
	}

	void set(IntSetting key, int value) {
		if((key == SLOTS) && (value <= 0)) {
			value = 1;
		}
		intSettings[key - INT_FIRST] = value;
		isSet[key] = true;
	}

	void set(BoolSetting key, bool value) {
		boolSettings[key - BOOL_FIRST] = value;
		isSet[key] = true;
	}

	void set(IntSetting key, const string& value) {
		if(value.empty()) {
			intSettings[key - INT_FIRST] = 0;
			isSet[key] = false;
		} else {
			intSettings[key - INT_FIRST] = Util::toInt(value);
			isSet[key] = true;
		}
	}

	void set(Int64Setting key, int64_t value) {
		int64Settings[key - INT64_FIRST] = value;
		isSet[key] = true;
	}

	void set(Int64Setting key, const string& value) {
		if(value.empty()) {
			int64Settings[key - INT64_FIRST] = 0;
			isSet[key] = false;
		} else {
			int64Settings[key - INT64_FIRST] = Util::toInt64(value);
			isSet[key] = true;
		}
	}

	void set(FloatSetting key, float value) {
		floatSettings[key - FLOAT_FIRST] = value;
		isSet[key] = true;
	}
	void set(FloatSetting key, double value) {
		// yes, we loose precision here, but we're gonna loose even more when saving to the XML file...
		floatSettings[key - FLOAT_FIRST] = static_cast<float>(value);
		isSet[key] = true;
	}

	const string& getDefault(StrSetting key) const {
		return strDefaults[key - STR_FIRST];
	}

	int getDefault(IntSetting key) const {
		return intDefaults[key - INT_FIRST];
	}

	bool getDefault(BoolSetting key) const {
		return boolDefaults[key - BOOL_FIRST];
	}

	int64_t getDefault(Int64Setting key) const {
		return int64Defaults[key - INT64_FIRST];
	}

	float getDefault(FloatSetting key) const {
		return floatDefaults[key - FLOAT_FIRST];
	}

	void setDefault(StrSetting key, string const& value) {
		strDefaults[key - STR_FIRST] = value;
	}

	void setDefault(IntSetting key, int value) {
		intDefaults[key - INT_FIRST] = value;
	}

	void setDefault(BoolSetting key, int value) {
		boolDefaults[key - BOOL_FIRST] = value;
	}

	void setDefault(Int64Setting key, int64_t value) {
		int64Defaults[key - INT64_FIRST] = value;
	}

	void setDefault(FloatSetting key, float value) {
		floatDefaults[key - FLOAT_FIRST] = value;
	}

	template<typename KeyT> bool isDefault(KeyT key) {
		return !isSet[key] || get(key, false) == getDefault(key);
	}

	void unset(size_t key) { isSet[key] = false; }

	void load() {
		Util::migrate(getConfigFile());
		load(getConfigFile());
	}
	void save() {
		save(getConfigFile());
	}

	void load(const string& aFileName);
	void save(const string& aFileName);

	bool getType(const char* name, int& n, Types& type) const;
	bool getType(const int& n, Types& type) const;

	const string(&getSettingTags() const)[SETTINGS_LAST + 1]{
		return settingTags;
	}

	HubSettings getHubSettings() const;

	// Search types
	void validateSearchTypeName(const string& name) const;
	void setSearchTypeDefaults();
	void addSearchType(const string& name, const StringList& extensions, bool validated = false);
	void delSearchType(const string& name);
	void renameSearchType(const string& oldName, const string& newName);
	void modSearchType(const string& name, const StringList& extensions);

	const SearchTypes& getSearchTypes() const {
		return searchTypes;
	}
	const StringList& getExtensions(const string& name);

private:
	friend class Singleton<SettingsManager>;
	SettingsManager();
	virtual ~SettingsManager() { }

	static const string settingTags[SETTINGS_LAST+1];

	string strSettings[STR_LAST - STR_FIRST];
	int    intSettings[INT_LAST - INT_FIRST];
	bool boolSettings[BOOL_LAST - BOOL_FIRST];
	int64_t int64Settings[INT64_LAST - INT64_FIRST];
	float floatSettings[FLOAT_LAST - FLOAT_FIRST];

	string strDefaults[STR_LAST - STR_FIRST];
	int    intDefaults[INT_LAST - INT_FIRST];
	bool boolDefaults[BOOL_LAST - BOOL_FIRST];
	int64_t int64Defaults[INT64_LAST - INT64_FIRST];
	float floatDefaults[FLOAT_LAST - FLOAT_FIRST];

	bool isSet[SETTINGS_LAST];

	static string getConfigFile() { return Util::getPath(Util::PATH_USER_CONFIG) + "DCPlusPlus.xml"; }

	// Search types
	SearchTypes searchTypes; // name, extlist

	SearchTypes::iterator getSearchType(const string& name);
};

// Shorthand accessor macros
#define SETTING(k) SettingsManager::getInstance()->get(SettingsManager::k)

} // namespace dcpp

#endif // !defined(SETTINGS_MANAGER_H)
