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

#ifndef DCPLUSPLUS_DCPP_PLUGIN_ENTITY_H
#define DCPLUSPLUS_DCPP_PLUGIN_ENTITY_H

#include <list>

#include "CriticalSection.h"
#include "PluginApiImpl.h"

namespace dcpp {

using std::list;

template<typename PluginType>
class PluginEntity
{
public:
	PluginEntity() : pod() {
		pod.isManaged = True;
	}
	virtual ~PluginEntity() { }

	PluginType* copyPluginObject() {
		Lock l(cs);
		return PluginApiImpl::copyData(getPluginObject());
	}

	void freePluginObject(PluginType* pObject) {
		PluginApiImpl::releaseData(pObject);
	}

protected:
	void resetEntity() { psCache.clear(); }

	const char* pluginString(const string& str) {
		psCache.push_back(str);
		return psCache.back().c_str();
	}

	PluginType pod;

private:
	friend class PluginManager;

	virtual PluginType* getPluginObject() noexcept = 0;

	CriticalSection cs;
	list<string> psCache;
};

} // namespace dcpp

#endif // DCPLUSPLUS_DCPP_PLUGIN_ENTITY_H
