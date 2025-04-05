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

#ifndef DCPLUSPLUS_DCPP_GEO_MANAGER_H
#define DCPLUSPLUS_DCPP_GEO_MANAGER_H

#include "forward.h"
#include "GeoIP2.h"
#include "Singleton.h"

#include <memory>
#include <string>

namespace dcpp {

using std::string;
using std::unique_ptr;

/** Manages IP->country mappings. */
class GeoManager : public Singleton<GeoManager>
{
public:
	/** Prepare the database. */
	void init();
	/** Update a database. Call after a new one have been downloaded. */
	void update();
	/** Rebuild the internal caches. Call after a change of country settings. */
	//void rebuild();
	/** Unload database. */
	void close();

	/** Map an IP address to a country. */
	string getCountry(const string& ip) const;

	string getEpoch() const;
	string getVersion() const;
	static string getDbPath();

private:
	friend class Singleton<GeoManager>;

	// for now city is enough for supported parameters
	unique_ptr<GeoIP> geo;

	GeoManager() { }
	virtual ~GeoManager() { }
};

} // namespace dcpp

#endif
