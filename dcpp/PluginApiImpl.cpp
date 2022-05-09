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

/**
 * The PluginApiImpl class contains implementations of certain callback functions,
 * they are simply separated here.
 *
 * Notes:
 *	- Current implementation does not correctly run HOOK_CHAT_PM_OUT for PM's sent as a result of Client::sendUserCmd
 *	- dcpp may run HOOK_HUB_OFFLINE for hubs that never ran HOOK_HUB_ONLINE before (if socket creation fails in Client::connect)
 */

#include "stdinc.h"
#include "PluginApiImpl.h"

#include "Client.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "FavoriteManager.h"
#include "File.h"
#include "LogManager.h"
#include "PluginManager.h"
#include "QueueManager.h"
#include "Tagger.h"
#include "UserConnection.h"
#include "version.h"
#include "HttpManager.h"
#include "HttpConnection.h"

namespace dcpp {

#define IMPL_HOOKS_COUNT 26

static const char* hookGuids[IMPL_HOOKS_COUNT] = {
	HOOK_CHAT_IN,
	HOOK_CHAT_OUT,
	HOOK_CHAT_PM_IN,
	HOOK_CHAT_PM_OUT,

	HOOK_TIMER_SECOND,
	HOOK_TIMER_MINUTE,

	HOOK_HUB_ONLINE,
	HOOK_HUB_OFFLINE,

	HOOK_NETWORK_HUB_IN,
	HOOK_NETWORK_HUB_OUT,
	HOOK_NETWORK_CONN_IN,
	HOOK_NETWORK_CONN_OUT,
	HOOK_NETWORK_UDP_IN,
	HOOK_NETWORK_UDP_OUT,

	HOOK_QUEUE_ADDED,
	HOOK_QUEUE_MOVED,
	HOOK_QUEUE_REMOVED,
	HOOK_QUEUE_FINISHED,

	HOOK_UI_CREATED,
	HOOK_UI_CHAT_TAGS,
	HOOK_UI_CHAT_DISPLAY,
	HOOK_UI_CHAT_COMMAND,
	HOOK_UI_CHAT_COMMAND_PM,

	HOOK_DATAACESSOR_HTTP_RESOURCE_NOTIFICATION,
	HOOK_DATAACESSOR_HTTP_RESOURCE_NOTIFICATION_FAILED,
	HOOK_DATAACESSOR_HTTP_RESOURCE_STREAM
};

static const char* hostName = APPNAME;

DCHooks PluginApiImpl::dcHooks = {
	DCINTF_HOOKS_VER,

	&PluginApiImpl::createHook,
	&PluginApiImpl::destroyHook,

	&PluginApiImpl::bindHook,
	&PluginApiImpl::runHook,
	&PluginApiImpl::releaseHook
};

DCConfig PluginApiImpl::dcConfig = {
	DCINTF_CONFIG_VER,

	&PluginApiImpl::getPath,
	&PluginApiImpl::getInstallPath,

	&PluginApiImpl::setConfig,
	&PluginApiImpl::getConfig,

	&PluginApiImpl::getLanguage,

	&PluginApiImpl::copyData,
	&PluginApiImpl::releaseData
};

DCLog PluginApiImpl::dcLog = {
	DCINTF_LOGGING_VER,

	&PluginApiImpl::log
};

DCConnection PluginApiImpl::dcConnection = {
	DCINTF_DCPP_CONNECTIONS_VER,

	&PluginApiImpl::sendUdpData,
	&PluginApiImpl::sendProtocolCmd,
	&PluginApiImpl::terminateConnection,

	&PluginApiImpl::getUserFromConn
};

DCHub PluginApiImpl::dcHub = {
	DCINTF_DCPP_HUBS_VER,

	&PluginApiImpl::addHub,
	&PluginApiImpl::findHub,
	&PluginApiImpl::removeHub,

	&PluginApiImpl::emulateProtocolCmd,
	&PluginApiImpl::sendProtocolCmd,

	&PluginApiImpl::sendHubMessage,
	&PluginApiImpl::sendLocalMessage,
	&PluginApiImpl::sendPrivateMessage,

	&PluginApiImpl::findUser,
	&PluginApiImpl::copyData,
	&PluginApiImpl::releaseData,

	&PluginApiImpl::copyData,
	&PluginApiImpl::releaseData
};

DCQueue PluginApiImpl::dcQueue = {
	DCINTF_DCPP_QUEUE_VER,

	&PluginApiImpl::addList,
	&PluginApiImpl::addDownload,
	&PluginApiImpl::findDownload,
	&PluginApiImpl::removeDownload,

	&PluginApiImpl::setPriority,
	&PluginApiImpl::pause,

	&PluginApiImpl::copyData,
	&PluginApiImpl::releaseData
};

DCUtils PluginApiImpl::dcUtils = {
	DCINTF_DCPP_UTILS_VER,

	&PluginApiImpl::toUtf8,
	&PluginApiImpl::fromUtf8,

	&PluginApiImpl::Utf8toWide,
	&PluginApiImpl::WidetoUtf8,

	&PluginApiImpl::toBase32,
	&PluginApiImpl::fromBase32
};

DCTagger PluginApiImpl::dcTagger = {
	DCINTF_DCPP_TAGGER_VER,

	&PluginApiImpl::getText,

	&PluginApiImpl::addTag,
	&PluginApiImpl::replaceText
};

DCDataAccess PluginApiImpl::dcDataAccess = {
	DCINTF_DCPP_DATAACCESSOR_VER,

	&PluginApiImpl::getHTTPResource,

	&PluginApiImpl::copyData,
	&PluginApiImpl::releaseData
};

Socket* PluginApiImpl::udpSocket = nullptr;
Socket& PluginApiImpl::getUdpSocket() {
	if(!udpSocket) {
		udpSocket = new Socket(Socket::TYPE_UDP);
	}
	return *udpSocket;
}

void PluginApiImpl::init() {
	auto& dcCore = *PluginManager::getInstance()->getCore();

	dcCore.apiVersion = DCAPI_CORE_VER;

	// Interface registry
	dcCore.register_interface = &PluginApiImpl::registerInterface;
	dcCore.query_interface = &PluginApiImpl::queryInterface;
	dcCore.release_interface = &PluginApiImpl::releaseInterface;

	// Core functions
	dcCore.has_plugin = &PluginApiImpl::isLoaded;
	dcCore.host_name = &PluginApiImpl::hostName;

	// Interfaces (since these outlast any plugin they don't need to be explictly released)
	dcCore.register_interface(DCINTF_HOOKS, &dcHooks);
	dcCore.register_interface(DCINTF_CONFIG, &dcConfig);
	dcCore.register_interface(DCINTF_LOGGING, &dcLog);

	dcCore.register_interface(DCINTF_DCPP_CONNECTIONS, &dcConnection);
	dcCore.register_interface(DCINTF_DCPP_HUBS, &dcHub);
	dcCore.register_interface(DCINTF_DCPP_QUEUE, &dcQueue);
	dcCore.register_interface(DCINTF_DCPP_UTILS, &dcUtils);
	dcCore.register_interface(DCINTF_DCPP_TAGGER, &dcTagger);
	dcCore.register_interface(DCINTF_DCPP_DATAACCESSOR, &dcDataAccess);

	// Create provided hooks (since these outlast any plugin they don't need to be explictly released)
	for(int i = 0; i < IMPL_HOOKS_COUNT; ++i)
		dcHooks.create_hook(hookGuids[i], NULL);
}

void PluginApiImpl::shutdown() {
	if(udpSocket) {
		udpSocket->disconnect();
		delete udpSocket;
		udpSocket = nullptr;
	}
}

// Functions for DCCore
intfHandle PluginApiImpl::registerInterface(const char* guid, dcptr_t pInterface) {
	return PluginManager::getInstance()->registerInterface(guid, pInterface);
}

DCInterfacePtr PluginApiImpl::queryInterface(const char* guid, uint32_t version) {
	// we only return the registered interface if it is same or newer than requested
	DCInterface* dci = (DCInterface*)PluginManager::getInstance()->queryInterface(guid);
	return (!dci || dci->apiVersion >= version) ? dci : nullptr;
}

Bool PluginApiImpl::releaseInterface(intfHandle hInterface) {
	return PluginManager::getInstance()->releaseInterface(hInterface) ? True : False;
}

Bool PluginApiImpl::isLoaded(const char* guid) {
	return PluginManager::getInstance()->isLoaded(guid) ? True : False;
}

const char* PluginApiImpl::hostName() {
	return dcpp::hostName;
}

// Functions for DCHook
hookHandle PluginApiImpl::createHook(const char* guid, DCHOOK defProc) {
	return PluginManager::getInstance()->createHook(guid, defProc);
}

Bool PluginApiImpl::destroyHook(hookHandle hHook) {
	Bool bRes = PluginManager::getInstance()->destroyHook(reinterpret_cast<PluginHook*>(hHook)) ? True : False;
	hHook = NULL;
	return bRes;
}

subsHandle PluginApiImpl::bindHook(const char* guid, DCHOOK hookProc, void* pCommon) {
	return PluginManager::getInstance()->bindHook(guid, hookProc, pCommon);
}

Bool PluginApiImpl::runHook(hookHandle hHook, dcptr_t pObject, dcptr_t pData) {
	return PluginManager::getInstance()->runHook(reinterpret_cast<PluginHook*>(hHook), pObject, pData) ? True : False;
}

size_t PluginApiImpl::releaseHook(subsHandle hHook) {
	return PluginManager::getInstance()->releaseHook(reinterpret_cast<HookSubscriber*>(hHook));
}

// Functions for DCConfig
ConfigStrPtr DCAPI PluginApiImpl::getPath(PathType type) {
	auto str = Util::getPath(static_cast<Util::Paths>(type));
	ConfigStr value = { CFG_TYPE_STRING, str.c_str() };
	return reinterpret_cast<ConfigStrPtr>(copyData(reinterpret_cast<ConfigValuePtr>(&value)));
}

ConfigStrPtr DCAPI PluginApiImpl::getInstallPath(const char* guid) {
	auto str = PluginManager::getInstallPath(guid);
	ConfigStr value = { CFG_TYPE_STRING, str.c_str() };
	return reinterpret_cast<ConfigStrPtr>(copyData(reinterpret_cast<ConfigValuePtr>(&value)));
}

void PluginApiImpl::setConfig(const char* guid, const char* setting, ConfigValuePtr val) {
	PluginManager* pm = PluginManager::getInstance();
	switch(val->type) {
		case CFG_TYPE_REMOVE:
			pm->removePluginSetting(guid, setting);
			break;
		case CFG_TYPE_STRING: {
			auto str = (ConfigStrPtr)val;
			pm->setPluginSetting(guid, setting, str->value ? str->value : "");
			break;
		}
		case CFG_TYPE_INT: {
			auto num = (ConfigIntPtr)val;
			pm->setPluginSetting(guid, setting, Util::toString(num->value));
			break;
		}
		case CFG_TYPE_BOOL: {
			auto num = (ConfigBoolPtr)val;
			pm->setPluginSetting(guid, setting, Util::toString(num->value));
			break;
		}
		case CFG_TYPE_INT64: {
			auto num = (ConfigInt64Ptr)val;
			pm->setPluginSetting(guid, setting, Util::toString(num->value));
			break;
		}
		default:
			dcassert(0);
	}
}

ConfigValuePtr PluginApiImpl::getConfig(const char* guid, const char* setting, ConfigType type) {
	// Attempt to retrieve core setting...
	if(strcmp(guid, "CoreSetup") == 0) {
		int n;
		SettingsManager::Types type;
		if(SettingsManager::getInstance()->getType(setting, n, type)) {
			switch(type) {
				case SettingsManager::TYPE_STRING: {
					string settingValue = SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(n));
					ConfigStr value = { CFG_TYPE_STRING, settingValue.c_str() };

					return copyData((ConfigValuePtr)&value);
				}
				case SettingsManager::TYPE_INT: {
					ConfigInt value = { CFG_TYPE_INT, SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(n)) };
					return copyData((ConfigValuePtr)&value);
				}
				case SettingsManager::TYPE_BOOL: {
					ConfigBool value = { CFG_TYPE_BOOL, SettingsManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(n)) ? True : False };
					return copyData((ConfigValuePtr)&value);
				}
				case SettingsManager::TYPE_INT64: {
					ConfigInt64 value = { CFG_TYPE_INT64, SettingsManager::getInstance()->get(static_cast<SettingsManager::Int64Setting>(n)) };
					return copyData((ConfigValuePtr)&value);
				}
				default:
					dcassert(0);
			}

			return NULL;
		}
	}

	PluginManager* pm = PluginManager::getInstance();
	switch(type) {
		case CFG_TYPE_STRING: {
			ConfigStr value = { type, pm->getPluginSetting(guid, setting).c_str() };
			return copyData((ConfigValuePtr)&value);
		}
		case CFG_TYPE_INT: {
			ConfigInt value = { type, Util::toInt(pm->getPluginSetting(guid, setting)) };
			return copyData((ConfigValuePtr)&value);
		}
		case CFG_TYPE_BOOL: {
			ConfigBool value = { type, pm->getPluginSetting(guid, setting) != "0" ? True : False };
			return copyData((ConfigValuePtr)&value);
		}
		case CFG_TYPE_INT64: {
			ConfigInt64 value = { type, Util::toInt64(pm->getPluginSetting(guid, setting)) };
			return copyData((ConfigValuePtr)&value);
		}
		default:
			dcassert(0);
	}

	return NULL;
}

ConfigStrPtr DCAPI PluginApiImpl::getLanguage() {
	auto str = Util::getIETFLang();
	ConfigStr value = { CFG_TYPE_STRING, str.c_str() };
	return reinterpret_cast<ConfigStrPtr>(copyData(reinterpret_cast<ConfigValuePtr>(&value)));
}

ConfigValuePtr PluginApiImpl::copyData(const ConfigValuePtr val) {
	switch(val->type) {
		case CFG_TYPE_STRING: {
			auto str = (ConfigStrPtr)val;
			auto copy = (ConfigStrPtr)malloc(sizeof(ConfigStr));
			memcpy(copy, str, sizeof(ConfigStr));

			size_t bufLen = strlen(str->value) + 1;
			copy->value = (char*)malloc(bufLen);
			strncpy((char*)copy->value, str->value, bufLen);

			return (ConfigValuePtr)copy;
		}
		case CFG_TYPE_INT: {
			auto num = (ConfigIntPtr)val;
			auto copy = (ConfigIntPtr)malloc(sizeof(ConfigInt));
			memcpy(copy, num, sizeof(ConfigInt));

			return (ConfigValuePtr)copy;
		}
		case CFG_TYPE_BOOL: {
			auto num = (ConfigBoolPtr)val;
			auto copy = (ConfigBoolPtr)malloc(sizeof(ConfigBool));
			memcpy(copy, num, sizeof(ConfigBool));
			
			return (ConfigValuePtr)copy;
		}
		case CFG_TYPE_INT64: {
			auto num = (ConfigInt64Ptr)val;
			auto copy = (ConfigInt64Ptr)malloc(sizeof(ConfigInt64));
			memcpy(copy, num, sizeof(ConfigInt64));

			return (ConfigValuePtr)copy;
		}
		default:
			dcassert(0);
	}

	return NULL;
}

void PluginApiImpl::releaseData(ConfigValuePtr val) {
	switch(val->type) {
		case CFG_TYPE_STRING: {
			auto str = (ConfigStrPtr)val;
			free((char*)str->value);
			free(str);
			break;
		}
		case CFG_TYPE_INT: {
			auto num = (ConfigIntPtr)val;
			free(num);
			break;
		}
		case CFG_TYPE_BOOL: {
			auto num = (ConfigBoolPtr)val;
			free(num);
			break;
		}
		case CFG_TYPE_INT64: {
			auto num = (ConfigInt64Ptr)val;
			free(num);
			break;
		}
		default:
			dcassert(0);
	}
}

// Functions for DCLog
void PluginApiImpl::log(const char* msg) {
	LogManager::getInstance()->message(msg);
}

// Functions for DCConnection
void PluginApiImpl::sendProtocolCmd(ConnectionDataPtr conn, const char* cmd) {
	reinterpret_cast<UserConnection*>(conn->object)->sendRaw(cmd);
}

void PluginApiImpl::terminateConnection(ConnectionDataPtr conn, Bool graceless) {
	reinterpret_cast<UserConnection*>(conn->object)->disconnect(graceless != False);
}

void PluginApiImpl::sendUdpData(const char* ip, uint32_t port, dcptr_t data, size_t n) {
	try { getUdpSocket().writeTo(ip, Util::toString(port), data, n); } catch (const Exception&) { /* ... */ }
}

UserDataPtr PluginApiImpl::getUserFromConn(ConnectionDataPtr conn) {
	auto user = reinterpret_cast<UserConnection*>(conn->object)->getHintedUser();
	if(!user.user) { return nullptr; }

	auto lock = ClientManager::getInstance()->lock();
	auto ou = ClientManager::getInstance()->findOnlineUser(user);
	if(!ou) { return nullptr; }
	return ou->copyPluginObject();
}

// Functions for DCUtils
size_t PluginApiImpl::toUtf8(char* dst, const char* src, size_t n) {
	string str(Text::toUtf8(src));
	if(n >= str.size()) {
		strncpy(dst, str.c_str(), str.size());
	}
	return str.size();
}

size_t PluginApiImpl::fromUtf8(char* dst, const char* src, size_t n) {
	string str(Text::fromUtf8(src));
	if(n >= str.size()) {
		strncpy(dst, str.c_str(), str.size());
	}
	return str.size();
}

size_t PluginApiImpl::Utf8toWide(wchar_t* dst, const char* src, size_t n) {
	wstring str(Text::utf8ToWide(src));
	if(n >= str.size()) {
		wcsncpy(dst, str.c_str(), str.size());
	}
	return str.size();
}

size_t PluginApiImpl::WidetoUtf8(char* dst, const wchar_t* src, size_t n) {
	string str(Text::wideToUtf8(src));
	if(n >= str.size()) {
		strncpy(dst, str.c_str(), str.size());
	}
	return str.size();
}

size_t PluginApiImpl::toBase32(char* dst, const uint8_t* src, size_t n) {
	string str(Encoder::toBase32(src, n));
	if(n >= str.size()) {
		strncpy(dst, str.c_str(), str.size());
	}
	return str.size();
}

size_t PluginApiImpl::fromBase32(uint8_t* dst, const char* src, size_t n) {
	Encoder::fromBase32(src, dst, n);
	return n;
}

// Functions for DCTagger
const char* PluginApiImpl::getText(TagDataPtr hTags) {
	return reinterpret_cast<Tagger*>(hTags->object)->getText().c_str();
}

void PluginApiImpl::addTag(TagDataPtr hTags, size_t start, size_t end, const char* id, const char* attributes) {
	reinterpret_cast<Tagger*>(hTags->object)->addTag(start, end, id, attributes);
}

void PluginApiImpl::replaceText(TagDataPtr hTags, size_t start, size_t end, const char* replacement) {
	reinterpret_cast<Tagger*>(hTags->object)->replaceText(start, end, replacement);
}

// Functions for DCQueue
QueueDataPtr PluginApiImpl::addList(UserDataPtr user, Bool silent) {
	auto u = ClientManager::getInstance()->findUser(CID(user->cid));
	if(!u) return NULL;

	QueueData* data = NULL;
	try {
		QueueManager::getInstance()->addList(HintedUser(u, user->hubHint), silent ? 0 : QueueItem::FLAG_CLIENT_VIEW);
		data = findDownload(QueueManager::getInstance()->getListPath(HintedUser(u, user->hubHint)).c_str());
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());
	}

	return data;
}

QueueDataPtr PluginApiImpl::addDownload(const char* hash, uint64_t size, const char* target) {
	QueueData* data = NULL;
	try {
		string sTarget = File::isAbsolute(target) ? target : SETTING(DOWNLOAD_DIRECTORY) + target;
		QueueManager::getInstance()->add(sTarget, size, TTHValue(hash), HintedUser(UserPtr(), Util::emptyString));
		data = findDownload(sTarget.c_str());
	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());
	}

	return data;
}

QueueDataPtr PluginApiImpl::findDownload(const char* target) {
	QueueData* data = NULL;
	QueueManager::getInstance()->lockedOperation([&data, target](const QueueItem::StringMap& lst) {
		string sTarget = target;
		auto i = lst.find(&sTarget);
		if (i != lst.end()) {
			data = i->second->copyPluginObject();
		}
	});

	return data;
}

void PluginApiImpl::removeDownload(QueueDataPtr qi) {
	QueueManager::getInstance()->remove(qi->target);
}

void PluginApiImpl::setPriority(QueueDataPtr qi, QueuePrio priority) {
	reinterpret_cast<QueueItem*>(qi->object)->setPriority(static_cast<QueueItem::Priority>(priority));
}

Bool PluginApiImpl::pause(QueueDataPtr qi) {
	auto item = reinterpret_cast<QueueItem*>(qi->object);
	bool paused = (item->getPriority() == QueueItem::PAUSED);

	item->setPriority(paused ? QueueItem::DEFAULT : QueueItem::PAUSED);
	return (!paused ? True : False);
}

QueueDataPtr PluginApiImpl::copyData(const QueueDataPtr qi) {
	QueueDataPtr copy = (QueueDataPtr)malloc(sizeof(QueueData));
	memcpy(copy, qi, sizeof(QueueData));

	size_t bufLen = strlen(qi->target) + 1;
	copy->target = (char*)malloc(bufLen);
	strncpy((char*)copy->target, qi->target, bufLen);

	bufLen = strlen(qi->location) + 1;
	copy->location = (char*)malloc(bufLen);
	strncpy((char*)copy->location, qi->location, bufLen);

	bufLen = strlen(qi->hash) + 1;
	copy->hash = (char*)malloc(bufLen);
	strncpy((char*)copy->hash, qi->hash, bufLen);

	copy->isManaged = False;
	return copy;
}

void PluginApiImpl::releaseData(QueueDataPtr qi) {
	if(qi->isManaged) {
		LogManager::getInstance()->message("Plugin trying to free a managed object !");
		return;
	}

	free((char*)qi->target);
	free((char*)qi->location);
	free((char*)qi->hash);

	free(qi);
}

// Functions for DCHub
HubDataPtr PluginApiImpl::addHub(const char* url, const char* nick, const char* password) {
	HubData* data = findHub(url);

	if(data == NULL && !SETTING(NICK).empty()) {
		Client* client = ClientManager::getInstance()->getClient(url);
		client->connect();
		client->setPassword(password);
		client->get(HubSettings::Nick) = nick;

		// check that socket is waitting for connection...
		if(client->isConnected()) {
			return client->copyPluginObject();
		}
	}

	return data;
}

HubDataPtr PluginApiImpl::findHub(const char* url) {
	auto lock = ClientManager::getInstance()->lock();
	const auto& list = ClientManager::getInstance()->getClients();

	for(auto i = list.begin(); i != list.end(); ++i) {
		if(((*i)->getHubUrl() == url) && (*i)->isConnected())
			 return (*i)->copyPluginObject();
	}

	return NULL;
}

void PluginApiImpl::removeHub(HubDataPtr hub) {
	ClientManager::getInstance()->putClient(reinterpret_cast<Client*>(hub->object));
}

void PluginApiImpl::emulateProtocolCmd(HubDataPtr hub, const char* cmd) {
	reinterpret_cast<Client*>(hub->object)->emulateCommand(cmd);
}

void PluginApiImpl::sendProtocolCmd(HubDataPtr hub, const char* cmd) {
	reinterpret_cast<Client*>(hub->object)->send(cmd);
}

void PluginApiImpl::sendHubMessage(HubDataPtr hub, const char* message, Bool thirdPerson) {
	if(hub && hub->object) {
		Client* client = reinterpret_cast<Client*>(hub->object);

		// Lets make plugins life easier...
		ParamMap params;
		client->getMyIdentity().getParams(params, "my", true);
		client->getHubIdentity().getParams(params, "hub", false);

		client->hubMessage(Util::formatParams(message, params), thirdPerson != False);
	}
}

void PluginApiImpl::sendLocalMessage(HubDataPtr hub, const char* msg, MsgType type) {
	Client* client = reinterpret_cast<Client*>(hub->object);
	client->fire(ClientListener::ClientLine(), client, msg, type);
}

Bool PluginApiImpl::sendPrivateMessage(UserDataPtr user, const char* message, Bool thirdPerson) {
	if(user) {
		auto lock = ClientManager::getInstance()->lock();
		OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(CID(user->cid), user->hubHint);
		if(!ou) return False;

		// Lets make plugins life easier...
		ParamMap params;
		ou->getIdentity().getParams(params, "user", true);
		ou->getClient().getMyIdentity().getParams(params, "my", true);
		ou->getClient().getHubIdentity().getParams(params, "hub", false);

		// plugins do not get notified about this... todo: fix note 1.
		ou->getClient().privateMessage(*ou, Util::formatParams(message, params), thirdPerson != False);
		return True;
	}
	return False;
}

HubDataPtr PluginApiImpl::copyData(const HubDataPtr hub) {
	HubDataPtr copy = (HubDataPtr)malloc(sizeof(HubData));
	memcpy(copy, hub, sizeof(HubData));

	size_t bufLen = strlen(hub->url) + 1;
	copy->url = (char*)malloc(bufLen);
	strncpy((char*)copy->url, hub->url, bufLen);

	bufLen = strlen(hub->ip) + 1;
	copy->ip = (char*)malloc(bufLen);
	strncpy((char*)copy->ip, hub->url, bufLen);

	copy->isManaged = False;
	return copy;
}

void PluginApiImpl::releaseData(HubDataPtr hub) {
	if(hub->isManaged) {
		LogManager::getInstance()->message("Plugin trying to free a managed object !");
		return;
	}

	free((char*)hub->url);
	free((char*)hub->ip);

	free(hub);
}

UserDataPtr PluginApiImpl::findUser(const char* cid, const char* hubUrl) {
	auto lock = ClientManager::getInstance()->lock();
	OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(CID(cid), hubUrl);
	if(!ou) return NULL;
	return ou->copyPluginObject();
}

UserDataPtr PluginApiImpl::copyData(const UserDataPtr user) {
	UserDataPtr copy = (UserDataPtr)malloc(sizeof(UserData));
	memcpy(copy, user, sizeof(UserData));

	size_t bufLen = strlen(user->nick) + 1;
	copy->nick = (char*)malloc(bufLen);
	strncpy((char*)copy->nick, user->nick, bufLen);

	bufLen = strlen(user->hubHint) + 1;
	copy->hubHint = (char*)malloc(bufLen);
	strncpy((char*)copy->hubHint, user->hubHint, bufLen);

	bufLen = strlen(user->cid) + 1;
	copy->cid = (char*)malloc(bufLen);
	strncpy((char*)copy->cid, user->cid, bufLen);

	copy->isManaged = False;
	return copy;
}

void PluginApiImpl::releaseData(UserDataPtr user) {
	if(user->isManaged) {
		LogManager::getInstance()->message("Plugin trying to free a managed object !");
		return;
	}

	free((char*)user->nick);
	free((char*)user->hubHint);
	free((char*)user->cid);

	free(user);
}

/* Functions for DataAccess */
void PluginApiImpl::getHTTPResource(const char* uri, const char* localPath) {
	try {
		class HTTPDownloader : private dcpp::HttpManagerListener
		{
		public:

			HTTPDownloader() { }
			virtual ~HTTPDownloader() { };

			void getResource(const char* uri, const char* file)
			{
				HttpManager::getInstance()->addListener(this);
				if(strlen(file) == 0)
				{
					HttpManager::getInstance()->download(uri);
				}
				else
				{
					HttpManager::getInstance()->download(uri, file);
				}
			}

		private:

			DataArrayPtr getDataArrayPtr(const std::string& str)
			{
				return getDataArrayPtr((void*)str.c_str(), str.size() + 1);
			};

			DataArrayPtr getDataArrayPtr(const uint8_t* data, size_t len)
			{
				return getDataArrayPtr((void*)data, len);
			};

			DataArrayPtr getDataArrayPtr(const void* data, size_t len)
			{
				DataArrayPtr pDataArray = (DataArrayPtr)malloc(sizeof(DataArray));
				memset(pDataArray, 0, sizeof(DataArray));

				size_t bufLen = len;
				void* pData = (void*)malloc(bufLen);
				memcpy(pData, data, bufLen);
				pDataArray->pData = pData;

				pDataArray->size = bufLen;

				return pDataArray;
			};

			void runHookCompleted(const std::string& url)
			{
				PluginManager::getInstance()->runHook(HOOK_DATAACESSOR_HTTP_RESOURCE_NOTIFICATION, const_cast<char*>(url.c_str()));

				HttpManager::getInstance()->removeListener(this);

				delete this;
			}

			void runHookFailed(const std::string& url)
			{
				PluginManager::getInstance()->runHook(HOOK_DATAACESSOR_HTTP_RESOURCE_NOTIFICATION_FAILED, const_cast<char*>(url.c_str()));

				HttpManager::getInstance()->removeListener(this);

				delete this;
			}

			void runHookPartial(const std::string& url, const uint8_t* data, size_t len)
			{
				DataArrayPtr streamArray = getDataArrayPtr(data, len);

				PluginManager::getInstance()->runHook(HOOK_DATAACESSOR_HTTP_RESOURCE_STREAM, const_cast<char*>(url.c_str()), streamArray);
				
			}

			// HttpManagerListener
			void on(HttpManagerListener::Failed, HttpConnection* c, const string&) noexcept
			{
				runHookFailed(c->getUrl());
			}

			void on(HttpManagerListener::Complete, HttpConnection* c, OutputStream* stream) noexcept
			{
				runHookCompleted(c->getUrl());
			}

			void on(HttpManagerListener::Updated, HttpConnection* c, const uint8_t* data, size_t len) noexcept
			{
				runHookPartial(c->getUrl(), data, len);
			}
		};

		HTTPDownloader* downloader = new HTTPDownloader();
		downloader->getResource(uri, localPath);

	} catch(const Exception& e) {
		LogManager::getInstance()->message(e.getError());
	}
}

DataArrayPtr PluginApiImpl::copyData(const DataArrayPtr val)
{
	DataArrayPtr copy = (DataArrayPtr)malloc(sizeof(DataArray));
	memcpy(copy, val, sizeof(DataArray));

	size_t bufLen = strlen((char*)val->pData) + 1;
	char* pData = (char*)malloc(bufLen);
	strncpy(pData, (char*)val->pData, bufLen);
	copy->pData = pData;

	return copy;
}

void PluginApiImpl::releaseData(DataArrayPtr val)
{
	free(const_cast<void*>(val->pData));

	free(val);
}

} // namespace dcpp
