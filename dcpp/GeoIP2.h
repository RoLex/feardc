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

#ifndef DCPLUSPLUS_DCPP_GEOIP2_H
#define DCPLUSPLUS_DCPP_GEOIP2_H

#include "CriticalSection.h"

#include <string>
//#include <unordered_map>
#include <vector>
#include <boost/core/noncopyable.hpp>

struct MMDB_s;

namespace dcpp {

using std::string;
//using std::unordered_map;
using std::vector;

class GeoIP : boost::noncopyable {
public:
	explicit GeoIP(string&& path);
	~GeoIP();

	string getCountry(const string& ip) const;
	void update();
	//void rebuild();

private:
	bool decompress() const;
	void open();
	void close();

	mutable CriticalSection cs;
	::MMDB_s* geo;

	const string path;
	const string language;

	//unordered_map<int, string> cache;
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_GEOIP2_H)
