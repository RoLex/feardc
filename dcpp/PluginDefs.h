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

#ifndef PLUGINSDK_PLUGINDEFS_H
#define PLUGINSDK_PLUGINDEFS_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Version of the plugin api (must change if old plugins simply can't be seen as viably working) */
#define DCAPI_CORE_VER				8

#ifdef _WIN32
# define DCAPI __stdcall
# define DCEXP __declspec(dllexport)
# ifdef DCAPI_HOST
#  define DCIMP __declspec(dllexport)
# else
#  define DCIMP __declspec(dllimport)
# endif
#else
# ifdef STDCALL
#  define DCAPI STDCALL
# else
#  define DCAPI
# endif
# define DCEXP __attribute__ ((visibility("default")))
# define DCIMP __attribute__ ((visibility("default")))
#endif

/* Hooks (events) system - required interface! */
#define DCINTF_HOOKS				"generic.plugins.DCHooks"
#define DCINTF_HOOKS_VER			DCAPI_CORE_VER

/* Recommended interfaces */
#define DCINTF_CONFIG				"generic.plugins.DCConfig"	/* Config management */
#define DCINTF_CONFIG_VER			1

#define DCINTF_LOGGING				"generic.plugins.DCLog"		/* Logging functions */
#define DCINTF_LOGGING_VER			1

/* Optional interfaces */
#define DCINTF_DCPP_CONNECTIONS		"dcpp.network.DCConnection"	/* Peer connections */
#define DCINTF_DCPP_CONNECTIONS_VER	1

#define DCINTF_DCPP_HUBS			"dcpp.network.DCHub"		/* Hubs */
#define DCINTF_DCPP_HUBS_VER		1

#define DCINTF_DCPP_QUEUE			"dcpp.queue.DCQueue"		/* Download Queue */
#define DCINTF_DCPP_QUEUE_VER		1

#define DCINTF_DCPP_UTILS			"dcpp.utils.DCUtils"		/* Utility and convenience functions */
#define DCINTF_DCPP_UTILS_VER		1

#define DCINTF_DCPP_TAGGER			"dcpp.xml.DCTagger"			/* Manipulation of an XML tagger */
#define DCINTF_DCPP_TAGGER_VER		1

#define DCINTF_DCPP_UI				"dcpp.ui.DCUI"				/* User interface */
#define DCINTF_DCPP_UI_VER			1

#define DCINTF_DCPP_DATAACCESSOR				"dcpp.dataaccessor.DCDataAccess"				/* Data access */
#define DCINTF_DCPP_DATAACCESSOR_VER			1

/* Hook GUID's for Hooks (events) system */
#define HOOK_CHAT_IN				"dcpp.chat.onIncomingChat"		/* Incoming chat from hub (obj: HubData) */
#define HOOK_CHAT_OUT				"dcpp.chat.onOutgoingChat"		/* Outgoing chat (obj: HubData) */
#define HOOK_CHAT_PM_IN				"dcpp.chat.onIncomingPM"		/* Incoming private message (obj: UserData) */
#define HOOK_CHAT_PM_OUT			"dcpp.chat.onOutgoingPM"		/* Outgoing private message (obj: UserData) */

#define HOOK_TIMER_SECOND			"dcpp.timer.onSecond"			/* Timer event fired once per second (data: tick value) */
#define HOOK_TIMER_MINUTE			"dcpp.timer.onMinute"			/* Timer event fired once per minute (data: tick value) */

#define HOOK_HUB_ONLINE				"dcpp.hubs.onOnline"			/* (New) hub has just gone online (obj: HubData) */
#define HOOK_HUB_OFFLINE			"dcpp.hubs.onOffline"			/* Hub has just gone offline (obj: HubData) */
#define HOOK_USER_ONLINE			"dcpp.users.onOnline"			/* User is online (obj: UserData) */
#define HOOK_USER_OFFLINE			"dcpp.users.onOffline"			/* User is offline (obj: UserData) */

#define HOOK_NETWORK_HUB_IN			"dcpp.network.onHubDataIn"		/* Incoming protocol messages from hub (obj: HubData) */
#define HOOK_NETWORK_HUB_OUT		"dcpp.network.onHubDataOut"		/* Outgoing protocol message to hub (obj: HubData) */
#define HOOK_NETWORK_CONN_IN		"dcpp.network.onClientDataIn"	/* Incoming client<->client protocol message (obj: ConnectionData) */
#define HOOK_NETWORK_CONN_OUT		"dcpp.network.onClientDataOut"	/* Outgoing client<->client protocol message (obj: ConnectionData) */
#define HOOK_NETWORK_UDP_IN			"dcpp.network.onUDPDataIn"		/* Incoming UDP data (obj: UDPData) */
#define HOOK_NETWORK_UDP_OUT		"dcpp.network.onUDPDataOut"		/* Outgoing UDP data (obj: UDPData) */

#define HOOK_QUEUE_ADDED			"dcpp.queue.onAdded"			/* (New) item has been added to download queue (obj: QueueData) */
#define HOOK_QUEUE_MOVED			"dcpp.queue.onMoved"			/* Download queue item has been moved to new location (obj: QueueData) */
#define HOOK_QUEUE_REMOVED			"dcpp.queue.onRemoved"			/* Item has just been removed from download queue (obj: QueueData) */
#define HOOK_QUEUE_FINISHED			"dcpp.queue.onFinished"			/* Item has just finished downloading (obj: QueueData) */

#define HOOK_UI_CREATED				"dcpp.ui.onCreated"				/* Host application UI has been created (obj: if any, impl. dependant) */
#define HOOK_UI_CHAT_TAGS			"dcpp.ui.onChatTags"			/* Chat message tags before tag merging (obj: UserData; data: TagData) */
#define HOOK_UI_CHAT_DISPLAY		"dcpp.ui.onChatDisplay"			/* Chat messages before they are displayed in chat (obj: UserData; data: StringData) */
#define HOOK_UI_CHAT_COMMAND		"dcpp.ui.onChatCommand"			/* Client side commands in hub chat (obj: HubData; data: CommandData) */
#define HOOK_UI_CHAT_COMMAND_PM		"dcpp.ui.onChatCommandPM"		/* Client side commands in private chat (obj: UserData; data: CommandData) */

#define HOOK_DATAACESSOR_HTTP_RESOURCE_NOTIFICATION		"dcpp.dataaccessor.onHTTPResourceNotification"		/* Notification that transfer has completed */
#define HOOK_DATAACESSOR_HTTP_RESOURCE_NOTIFICATION_FAILED		"dcpp.dataaccessor.onHTTPResourceNotificationFailed"		/* Notification that transfer has failed */
#define HOOK_DATAACESSOR_HTTP_RESOURCE_STREAM		"dcpp.dataaccessor.onHTTPResourceStream"		/* Notification that transfer has updated */

/* Main hook events (returned by pluginInit) */
typedef enum tagPluginState {
	ON_INSTALL = 0,												/* Replaces ON_LOAD for the very first loading of the plugin */
	ON_LOAD,													/* Loading the plugin at program start; sent after the pluginInit call (obj: DCCore) */
	ON_LOAD_RUNTIME,											/* Replaces ON_LOAD when loading the plugin at runtime */
	ON_CONFIGURE,												/* The user wants to configure the plugin (obj: DCCore, data: impl. dependant) */
	ON_UNLOAD,													/* The plugin is going to be unloaded (no params) */
	ON_UNINSTALL												/* Replaces ON_UNLOAD when the plugin is being uninstalled */
} PluginState;

/* Argument types */
typedef enum tagConfigType {
	CFG_TYPE_UNKNOWN = -2,										/* Can be used when querying core settings with magic guid: "CoreSetup" */
	CFG_TYPE_REMOVE,											/* Config value will be removed */
	CFG_TYPE_STRING,											/* Config value is a string */
	CFG_TYPE_INT,												/* Config value is a 32bit integer */
	CFG_TYPE_BOOL,												/* Config value is a bool */
	CFG_TYPE_INT64												/* Config value is a 64bit integer */
} ConfigType;

typedef enum tagProtocolType {
	PROTOCOL_ADC = 0,											/* Protocol used is ADC */
	PROTOCOL_NMDC,												/* Protocol used is NMDC */
	PROTOCOL_DHT												/* DHT node (not used, reserved) */
} ProtocolType;

typedef enum tagPathType {
	PATH_GLOBAL_CONFIG = 0,										/* Global configuration */
	PATH_USER_CONFIG,											/* Per-user configuration (queue, favorites, ...) */
	PATH_USER_LOCAL,											/* Per-user local data (cache, temp files, ...)	*/					
	PATH_RESOURCES,												/* Various resources (help files etc) */
	PATH_LOCALE													/* Translations */
} PathType;

typedef enum tagMsgType {
	MSG_CLIENT = 0,												/* General text style */
	MSG_STATUS,													/* Message in status bar */
	MSG_SYSTEM													/* Message with system message format */
} MsgType;

typedef enum tagQueuePrio {
	PRIO_DEFAULT = -1,
	PRIO_LOWEST = 1,
	PRIO_LOW,
	PRIO_NORMAL,
	PRIO_HIGH,
	PRIO_HIGHEST
} QueuePrio;

/* Data types */
typedef void *hookHandle, *subsHandle, *intfHandle, *dcptr_t;
typedef enum tagDCBool { dcFalse = 0, dcTrue } dcBool;
typedef uint64_t dctime_t;

/* Workaround for other bool defs */
#define Bool dcBool
#define True dcTrue
#define False dcFalse

/* Config Value */
typedef struct tagConfigValue {
	ConfigType type;											/* Indicates which type of value this is */
} ConfigValue, *ConfigValuePtr;

/* Config Value: string */
typedef struct tagConfigStr {
	ConfigType type;											/* Indicates which type of value this is */
	const char* value;
} ConfigStr, *ConfigStrPtr;

/* Config Value: integer */
typedef struct tagConfigInt {
	ConfigType type;											/* Indicates which type of value this is */
	int32_t value;
} ConfigInt, *ConfigIntPtr;

/* Config Value: boolean */
typedef struct tagConfigBool {
	ConfigType type;											/* Indicates which type of value this is */
	Bool value;
} ConfigBool, *ConfigBoolPtr;

/* Config Value: integer (64bit) */
typedef struct tagConfigInt64 {
	ConfigType type;											/* Indicates which type the value holds */
	int64_t value;
} ConfigInt64, *ConfigInt64Ptr;

/* String Data (for substitutions) */
typedef struct tagStringData {
	const char* in;												/* Incoming string */
	char* out;													/* Resulting new string (take care to free!) */
} StringData, *StringDataPtr;

/* Client side chat commands */
typedef struct tagCommandData {
	const char* command;										/* Command name */
	const char* params;											/* Command parameters passed */
} CommandData, *CommandDataPtr;

/* Users */
typedef struct tagUserData {
	const char* nick;											/* Users nick (only quaranteed to not be empty in NMDC environment) */
	const char* hubHint;										/* Contains hub url to find the user from */
	const char* cid;											/* User CID */
	dcptr_t object;												/* The source/destination for the data (may be null) */
	uint32_t sid;												/* Users SID in hubHint hub (only valid in ADC environment) */
	ProtocolType protocol;										/* The protocol used */
	Bool isOp;													/* Whether user has a key or not */
	Bool isManaged;												/* False if plugin has to call release_user(...) for this object */
} UserData, *UserDataPtr;

/* Hubs */
typedef struct tagHubData {
	const char* url;											/* Hub url address */
	const char* ip;												/* Hub ip address */
	dcptr_t object;												/* The source/destination for the data  (may be null) */
	uint16_t port;												/* Hub port */
	ProtocolType protocol;										/* The protocol used */
	Bool isOp;													/* Whether we have a key on this hub or not */
	Bool isSecure;												/* True for TLS encrypted connections */
	Bool isManaged;												/* False if plugin has to call release(...) for this object */
} HubData, *HubDataPtr;

/* Peer connections */
typedef struct tagConnectionData {
	const char* ip;												/* The ip address (remote) for this connection */
	dcptr_t object;												/* The source/destination for the data (may be null) */
	uint16_t port;												/* The port for this connection */
	ProtocolType protocol;										/* The protocol used */
	Bool isOp;													/* Whether user has a key or not */
	Bool isSecure;												/* True for TLS encrypted connections */
	Bool isManaged;												/* Always True (Plugins can not lookup, or track the scope of, a specific instance) */
} ConnectionData, *ConnectionDataPtr;

/* UDP */
typedef struct tagUDPData {
	const char* ip;												/* The ip address (remote) for this connection */
	uint16_t port;												/* The port for this connection */
} UDPData, *UDPDataPtr;

/* Queue items and files */
typedef struct tagQueueData {
	const char* target;											/* The *final* location for the file */
	const char* location;										/* The *current* location for the file (may be same as target) */
	const char* hash;											/* TTH hash of the file */
	dcptr_t object;												/* The source/destination for the data (may be null) */
	uint64_t size;												/* File size (bytes) */
	Bool isFileList;											/* FileList download */
	Bool isManaged;												/* False if plugin has to call release(...) for this object */
} QueueData, *QueueDataPtr;

/* Tagging intentions */
typedef struct tagTagData {
	dcptr_t object;												/* Internal */
	Bool isManaged;												/* Always True for now */
} TagData, *TagDataPtr;

/* Plugin meta data */
typedef struct tagMetaData { 
	const char* name;											/* Name of the plugin */
	const char* author;											/* Name/Nick of the plugin author */
	const char* description;									/* *Short* description of plugin functionality (may be multiple lines) */
	const char* web;											/* Authors website if any */
	const char* guid;											/* Plugins unique GUID */
	const char** dependencies;									/* Array of plugin dependencies */
	uint32_t numDependencies;									/* Number of plugin GUIDs in dependencies array */
	uint32_t apiVersion;										/* Base (DCCore) API version the plugin was compiled against */
	double version;												/* Plugin version */
} MetaData, *MetaDataPtr;

/* Generic interface dummy */
typedef struct tagDCInterface {
	/* The version of the interface */
	uint32_t apiVersion;
} DCInterface, *DCInterfacePtr;

typedef struct {
		const void* pData;
		uint64_t size;
} DataArray, *DataArrayPtr;

/* Core plugin system */
typedef struct tagDCCore {
	/* Core API version */
	uint32_t apiVersion;

	/* Interface registry */
	intfHandle			(DCAPI *register_interface)	(const char* guid, dcptr_t pInterface);
	DCInterfacePtr		(DCAPI *query_interface)	(const char* guid, uint32_t version);
	Bool				(DCAPI *release_interface)	(intfHandle hInterface);

	/* Check if another plugin is loaded (for soft dependencies) */
	Bool				(DCAPI *has_plugin)			(const char* guid);				

	/* Retrieve host application name */
	const char*			(DCAPI *host_name)			();
} DCCore, *DCCorePtr;

/* Plugin main function */
typedef Bool (DCAPI* DCMAIN)		(PluginState pluginState, DCCorePtr core, dcptr_t pData);

/* Hooks (events) system - required interface! */

/* Hook function prototypes */
typedef Bool (DCAPI* DCHOOK)		(dcptr_t pObject, dcptr_t pData, dcptr_t pCommon, Bool* bBreak);

/* Hook system functions */
typedef struct tagDCHooks {
	/* Hooks API version */
	uint32_t apiVersion;

	/* Hook creation */
	hookHandle			(DCAPI *create_hook)		(const char* guid, DCHOOK defProc);
	Bool				(DCAPI *destroy_hook)		(hookHandle hHook);

	/* Hook interaction */
	subsHandle			(DCAPI *bind_hook)			(const char* guid, DCHOOK hookProc, void* pCommon);
	Bool				(DCAPI *run_hook)			(hookHandle hHook, dcptr_t pObject, dcptr_t pData);
	size_t				(DCAPI *release_hook)		(subsHandle hHook);
} DCHooks, *DCHooksPtr;

/* Recommended interfaces */

/* Config management */
typedef struct tagDCConfig {
	/* Config API version */
	uint32_t apiVersion;

	ConfigStrPtr		(DCAPI *get_path)			(PathType type);
	ConfigStrPtr		(DCAPI *get_install_path)	(const char* guid);

	void				(DCAPI *set_cfg)			(const char* guid, const char* setting, ConfigValuePtr val);
	ConfigValuePtr		(DCAPI *get_cfg)			(const char* guid, const char* setting, ConfigType type);

	/* Return the language used by the host as an IETF language tag. */
	ConfigStrPtr		(DCAPI *get_language)		();

	ConfigValuePtr		(DCAPI *copy)				(const ConfigValuePtr val);
	void				(DCAPI *release)			(ConfigValuePtr val);
} DCConfig, *DCConfigPtr;

/* Logging functions */
typedef struct tagDCLog {
	/* Logging API version */
	uint32_t apiVersion;

	void		(DCAPI *log)						(const char* msg);
} DCLog, *DCLogPtr;

/* Optional interfaces */

/* Peer connection */
typedef struct tagDCConnection {
	/* Connection API version */
	uint32_t apiVersion;

	void			(DCAPI *send_udp_data)			(const char* ip, uint32_t port, dcptr_t data, size_t n);
	void			(DCAPI *send_protocol_cmd)		(ConnectionDataPtr hConn, const char* cmd);
	void			(DCAPI *terminate_conn)			(ConnectionDataPtr hConn, Bool graceless);

	UserDataPtr		(DCAPI *get_user)				(ConnectionDataPtr hConn);
} DCConnection, *DCConnectionPtr;

/* Hubs */
typedef struct tagDCHub {
	/* Hubs API version */
	uint32_t apiVersion;

	HubDataPtr		(DCAPI *add_hub)				(const char* url, const char* nick, const char* password);
	HubDataPtr		(DCAPI *find_hub)				(const char* url);
	void			(DCAPI *remove_hub)				(HubDataPtr hHub);

	void			(DCAPI *emulate_protocol_cmd)	(HubDataPtr hHub, const char* cmd);
	void			(DCAPI *send_protocol_cmd)		(HubDataPtr hHub, const char* cmd);

	void			(DCAPI *send_message)			(HubDataPtr hHub, const char* msg, Bool thirdPerson);
	void			(DCAPI *local_message)			(HubDataPtr hHub, const char* msg, MsgType type);
	Bool			(DCAPI *send_private_message)	(UserDataPtr hUser, const char* msg, Bool thirdPerson);

	UserDataPtr		(DCAPI *find_user)				(const char* cid, const char* hubUrl);
	UserDataPtr		(DCAPI *copy_user)				(const UserDataPtr hUser);
	void			(DCAPI *release_user)			(UserDataPtr hCopy);

	HubDataPtr		(DCAPI *copy)					(const HubDataPtr hHub);
	void			(DCAPI *release)				(HubDataPtr hCopy);
} DCHub, *DCHubPtr;

/* Download Queue */
typedef struct tagDCQueue {
	/* Queue API version */
	uint32_t apiVersion;

	QueueDataPtr	(DCAPI *add_list)				(UserDataPtr hUser, Bool silent);
	QueueDataPtr	(DCAPI *add_download)			(const char* hash, uint64_t size, const char* target);
	QueueDataPtr	(DCAPI *find_download)			(const char* target);
	void			(DCAPI *remove_download)		(QueueDataPtr hItem);

	void			(DCAPI *set_priority)			(QueueDataPtr hItem, QueuePrio priority);
	Bool			(DCAPI *pause)					(QueueDataPtr hItem);

	QueueDataPtr	(DCAPI *copy)					(const QueueDataPtr hItem);
	void			(DCAPI *release)				(QueueDataPtr hCopy);
} DCQueue, *DCQueuePtr;

/* Utility and convenience functions */
typedef struct tagDCUtils {
	/* Utility API version */
	uint32_t apiVersion;

	/* These functions attempt a conversion; they return the required buffer size. */

	size_t		(DCAPI *to_utf8)					(char* dst, const char* src, size_t n);
	size_t		(DCAPI *from_utf8)					(char* dst, const char* src, size_t n);

	size_t		(DCAPI *utf8_to_wcs)				(wchar_t* dst, const char* src, size_t n);
	size_t		(DCAPI *wcs_to_utf8)				(char* dst, const wchar_t* src, size_t n);

	size_t		(DCAPI *to_base32)					(char* dst, const uint8_t* src, size_t n);
	size_t		(DCAPI *from_base32)				(uint8_t* dst, const char* src, size_t n);
} DCUtils, *DCUtilsPtr;

/* Manipulation of an XML tagger */
typedef struct tagDCTagger {
	/* Tagger API version */
	uint32_t apiVersion;

	const char*	(DCAPI *get_text)					(TagDataPtr hTags);

	void		(DCAPI *add_tag)					(TagDataPtr hTags, size_t start, size_t end, const char* id, const char* attributes);
	void		(DCAPI *replace_text)				(TagDataPtr hTags, size_t start, size_t end, const char* replacement);
} DCTagger, *DCTaggerPtr;

/* User interface */
typedef void (DCAPI* DCCommandFunc)		(const char* name);

typedef struct DCUI {
	/* User interface API version */
	uint32_t apiVersion;

	/* Add a command identified by "name".
	"icon" is optional; it is the path to an icon file used to illustrate the command. */
	void		(DCAPI *add_command)				(const char* guid, const char* name, DCCommandFunc command, const char* icon);
	/* Remove a command previously added with add_command. */
	void		(DCAPI *remove_command)				(const char* guid, const char* name);

	void		(DCAPI *play_sound)					(const char* path);
	void		(DCAPI *notify)						(const char* title, const char* message);
} DCUI, *DCUIPtr;

/* Data access */
typedef struct DCDataAccess {
	uint32_t apiVersion;

	/* "uri" is the HTTP file path to grab.
	"localPath" is the local file path to download to.
	"localPath" may be NULL or empty to not download to a specific file. */
	void		(DCAPI *get_http_resource)	(const char* uri, const char* localPath);

	DataArrayPtr	(DCAPI *copy)					(const DataArrayPtr hItem);
	void			(DCAPI *release)				(DataArrayPtr hCopy);
} DCDataAcess, *DCDataAccessPtr;

#ifdef __cplusplus
}
#endif

#endif /* !defined(DCPLUSPLUS_DCPP_PLUGIN_DEFS_H) */
