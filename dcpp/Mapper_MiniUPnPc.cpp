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
#include "Mapper_MiniUPnPc.h"

extern "C" {
#ifndef MINIUPNP_STATICLIB
#define MINIUPNP_STATICLIB
#endif
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
}

namespace dcpp {

const string Mapper_MiniUPnPc::name = "MiniUPnP";

Mapper_MiniUPnPc::Mapper_MiniUPnPc(string&& localIp, bool v6) :
Mapper(move(localIp), v6)
{
}

bool Mapper_MiniUPnPc::init() {
	if(!url.empty())
		return true;

	UPNPDev* devices = upnpDiscover(2000, localIp.empty() ? nullptr : localIp.c_str(), nullptr, 0, v6, 2, nullptr);
	if(!devices)
		return false;

	UPNPUrls urls;
	IGDdatas data;

	auto ret = UPNP_GetValidIGD(devices, &urls, &data, 0, 0, 0, 0);

	bool ok = ret == 1;
	if(ok) {
		url = urls.controlURL;
		service = data.first.servicetype;
		device = data.CIF.friendlyName;
	}

	if(ret > 0) {
		FreeUPNPUrls(&urls);
		freeUPNPDevlist(devices);
	}

	return ok;
}

void Mapper_MiniUPnPc::uninit() {
}

bool Mapper_MiniUPnPc::add(const string& port, const Protocol protocol, const string& description) {
	return UPNP_AddPortMapping(url.c_str(), service.c_str(), port.c_str(), port.c_str(),
		localIp.c_str(), description.c_str(), protocols[protocol], 0, 0) == UPNPCOMMAND_SUCCESS;
}

bool Mapper_MiniUPnPc::remove(const string& port, const Protocol protocol) {
	return UPNP_DeletePortMapping(url.c_str(), service.c_str(), port.c_str(), protocols[protocol], 0) == UPNPCOMMAND_SUCCESS;
}

string Mapper_MiniUPnPc::getDeviceName() {
	return device;
}

string Mapper_MiniUPnPc::getExternalIP() {
	char buf[16] = { 0 };
	if(UPNP_GetExternalIPAddress(url.c_str(), service.c_str(), buf) == UPNPCOMMAND_SUCCESS)
		return buf;
	return string();
}

} // dcpp namespace
