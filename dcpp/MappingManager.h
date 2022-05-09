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

#ifndef DCPLUSPLUS_DCPP_MAPPING_MANAGER_H
#define DCPLUSPLUS_DCPP_MAPPING_MANAGER_H

#include <memory>
#include <functional>
#include <vector>

#include "forward.h"
#include "typedefs.h"
#include "Mapper.h"
#include "TimerManager.h"
#include "atomic.h"
#include "tribool.h"

namespace dcpp {

using std::atomic;
using std::function;
using std::unique_ptr;
using std::vector;

class MappingManager :
	public Singleton<MappingManager>,
	private Thread,
	private TimerManagerListener
{
public:
	/** add an implementation derived from the base Mapper class, passed as template parameter.
	the first added mapper will be tried first, unless the "MAPPER" setting is not empty. */
	template<typename T> void addMapper(bool v6) {
		(v6 ? mappers6 : mappers4).emplace_back(T::name, [v6](string&& localIp) {
			return new T(move(localIp), v6);
		});
	}
	StringList getMappers(bool v6) const;

	bool open(bool v4, bool v6);
	void close(tribool v6 = indeterminate);
	/** whether a working port mapping implementation is currently in use. */
	bool getOpened(bool v6) const;
	/** get information about the currently working implementation, if there is one; or a status
	string stating otherwise. */
	string getStatus(bool v6) const;

private:
	friend class Singleton<MappingManager>;

	vector<pair<string, function<Mapper* (string&&)>>> mappers4, mappers6;

	static atomic_flag busy;
	atomic<bool> needsV4PortMap, needsV6PortMap;
	unique_ptr<Mapper> working4, working6; /// currently working implementations.
	uint64_t renewal; /// when the next renewal should happen, if requested by the mapper.

	MappingManager();
	virtual ~MappingManager() { }

	int run();
	void runPortMapping(bool v6, const string& conn_port, const string& secure_port, const string& search_port);

	void close(Mapper& mapper);
	void log(const string& message, tribool v6 = indeterminate);
	string deviceString(Mapper& mapper) const;
	void renewLater(Mapper& mapper);

	void on(TimerManagerListener::Minute, uint64_t tick) noexcept;
};

} // namespace dcpp

#endif
