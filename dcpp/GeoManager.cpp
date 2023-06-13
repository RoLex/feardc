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

#include "stdinc.h"
#include "GeoManager.h"

#include "GeoIP.h"
#include "Util.h"

namespace dcpp {

void GeoManager::init() {
	geo6.reset(new GeoIP(getDbPath(true)));
	geo4.reset(new GeoIP(getDbPath(false)));

	rebuild();
}

void GeoManager::update(bool v6) {
	auto geo = (v6 ? geo6 : geo4).get();
	if(geo) {
		geo->update();
		geo->rebuild();
	}
}

void GeoManager::rebuild() {
	geo6->rebuild();
	geo4->rebuild();
}

void GeoManager::close() {
	geo6.reset();
	geo4.reset();
}

const string& GeoManager::getCountry(const string& ip, int flags) {
	if(!ip.empty()) {

		/*Check for the IP version when the caller didn't specify it.
		  This hadn't needed for years but with GeoIP databases released 
		  somewhere in mid 2018 and on for V4 addresses (and for any arbitrary
		  string for the matter) the GeoIP_id_by_addr_v6 call, unlike before
		  when it returned no hits, now always returns a specific id that is 
		  actually in the database. This resulted the same invalid country
		  returned by this function for V4 addresses in cases when called 
		  with IP version unspecified.
		  This also ensures that V4 IPs never looked up unnecessary in the 
		  V6 database which should improve performance.*/

		if((flags & V6) && (flags & V4)) {
			flags = Util::isIpV4(ip) ? V4 : V6;
		}

		if((flags & V6) && geo6.get()) {
			const auto& ret = geo6->getCountry(ip);
			if(!ret.empty())
				return ret;
		}

		if((flags & V4) && geo4.get()) {
			return geo4->getCountry(ip);
		}
	}

	return Util::emptyString;
}

string GeoManager::getDbPath(bool v6) {
	return Util::getPath(Util::PATH_USER_LOCAL) + (v6 ? "GeoIPv6.dat" : "GeoIP.dat");
}

string GeoManager::getRegionDbPath() {
	return Util::getPath(Util::PATH_USER_LOCAL) + "GeoIP_Regions.csv";
}

} // namespace dcpp
