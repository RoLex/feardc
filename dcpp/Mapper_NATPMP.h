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

#ifndef DCPLUSPLUS_DCPP_MAPPER_NATPMP_H
#define DCPLUSPLUS_DCPP_MAPPER_NATPMP_H

#include <dcpp/Mapper.h>

namespace dcpp {

class Mapper_NATPMP : public Mapper
{
public:
	Mapper_NATPMP(string&& localIp, bool v6);

	static const string name;

private:
	bool init();
	void uninit();

	bool add(const string& port, const Protocol protocol, const string& description);
	bool remove(const string& port, const Protocol protocol);

	uint32_t renewal() const { return lifetime / 2; }

	string getDeviceName();
	string getExternalIP();

	const string& getName() const { return name; }

	string gateway;
	uint32_t lifetime; // in minutes
};

} // dcpp namespace

#endif
