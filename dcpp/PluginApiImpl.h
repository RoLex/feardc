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

/**
 * The PluginApiImpl class contains implementations of certain callback functions,
 * they are simply separated here.
 */

#ifndef DCPLUSPLUS_DCPP_PLUGIN_API_IMPL_H
#define DCPLUSPLUS_DCPP_PLUGIN_API_IMPL_H

#include "forward.h"
#include "typedefs.h"

#include "PluginDefs.h"

namespace dcpp {

class PluginApiImpl
{
public:
	static void init();
	static void shutdown();

	static HubDataPtr DCAPI copyData(const HubDataPtr hub);
	static void DCAPI releaseData(HubDataPtr hub);

	static UserDataPtr DCAPI copyData(const UserDataPtr user);
	static void DCAPI releaseData(UserDataPtr user);

	static QueueDataPtr DCAPI copyData(const QueueDataPtr qi);
	static void DCAPI releaseData(QueueDataPtr qi);

	static ConfigValuePtr DCAPI copyData(const ConfigValuePtr val);
	static void DCAPI releaseData(ConfigValuePtr val);

	static DataArrayPtr DCAPI copyData(const DataArrayPtr val);
	static void DCAPI releaseData(DataArrayPtr val);

private:
	// Functions for DCCore
	static intfHandle DCAPI registerInterface(const char* guid, dcptr_t funcs);
	static DCInterfacePtr DCAPI queryInterface(const char* guid, uint32_t version);
	static Bool DCAPI releaseInterface(intfHandle hInterface);

	static Bool DCAPI isLoaded(const char* guid);
	static const char* DCAPI hostName();

	// Functions for DCHooks
	static hookHandle DCAPI createHook(const char* guid, DCHOOK defProc);
	static Bool DCAPI destroyHook(hookHandle hHook);

	static subsHandle DCAPI bindHook(const char* guid, DCHOOK hookProc, void* pCommon);
	static Bool DCAPI runHook(hookHandle hHook, dcptr_t pObject, dcptr_t pData);
	static size_t DCAPI releaseHook(subsHandle hHook);

	// Functions For DCConfig
	static ConfigStrPtr DCAPI getPath(PathType type);
	static ConfigStrPtr DCAPI getInstallPath(const char* guid);
	static void DCAPI setConfig(const char* guid, const char* setting, ConfigValuePtr val);
	static ConfigValuePtr DCAPI getConfig(const char* guid, const char* setting, ConfigType type);
	static ConfigStrPtr DCAPI getLanguage();

	// Functions for DCLog
	static void DCAPI log(const char* msg);

	// Functions for DCConnection
	static void DCAPI sendProtocolCmd(ConnectionDataPtr conn, const char* cmd);
	static void DCAPI terminateConnection(ConnectionDataPtr conn, Bool graceless);
	static void DCAPI sendUdpData(const char* ip, uint32_t port, dcptr_t data, size_t n);

	static UserDataPtr DCAPI getUserFromConn(ConnectionDataPtr conn);

	// Functions for DCUtils
	static size_t DCAPI toUtf8(char* dst, const char* src, size_t n);
	static size_t DCAPI fromUtf8(char* dst, const char* src, size_t n);

	static size_t DCAPI Utf8toWide(wchar_t* dst, const char* src, size_t n);
	static size_t DCAPI WidetoUtf8(char* dst, const wchar_t* src, size_t n);

	static size_t DCAPI toBase32(char* dst, const uint8_t* src, size_t n);
	static size_t DCAPI fromBase32(uint8_t* dst, const char* src, size_t n);

	// Functions for DCTagger
	static const char* DCAPI getText(TagDataPtr hTags);

	static void DCAPI addTag(TagDataPtr hTags, size_t start, size_t end, const char* id, const char* attributes);
	static void DCAPI replaceText(TagDataPtr hTags, size_t start, size_t end, const char* replacement);

	// Functions for DCQueue
	static QueueDataPtr DCAPI addList(UserDataPtr user, Bool silent);
	static QueueDataPtr DCAPI addDownload(const char* hash, uint64_t size, const char* target);
	static QueueDataPtr DCAPI findDownload(const char* target);
	static void DCAPI removeDownload(QueueDataPtr qi);

	static void DCAPI setPriority(QueueDataPtr qi, QueuePrio priority);
	static Bool DCAPI pause(QueueDataPtr qi);

	// Functions for DCHub
	static HubDataPtr DCAPI addHub(const char* url, const char* nick, const char* password);
	static HubDataPtr DCAPI findHub(const char* url);
	static void DCAPI removeHub(HubDataPtr hub);

	static void DCAPI emulateProtocolCmd(HubDataPtr hub, const char* cmd);
	static void DCAPI sendProtocolCmd(HubDataPtr hub, const char* cmd);

	static void DCAPI sendHubMessage(HubDataPtr hub, const char* message, Bool thirdPerson);
	static void DCAPI sendLocalMessage(HubDataPtr hub, const char* msg, MsgType type);
	static Bool DCAPI sendPrivateMessage(UserDataPtr user, const char* message, Bool thirdPerson);

	static UserDataPtr DCAPI findUser(const char* cid, const char* hubUrl);

	static void DCAPI getHTTPResource(const char* uri, const char* localPath);

	static DCHooks dcHooks;
	static DCConfig dcConfig;
	static DCLog dcLog;

	static DCConnection dcConnection;
	static DCHub dcHub;
	static DCQueue dcQueue;
	static DCUtils dcUtils;
	static DCTagger dcTagger;
	static DCDataAccess dcDataAccess;

	static Socket* udpSocket;
	static Socket& getUdpSocket();
};

} // namepsace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_PLUGIN_API_H)
