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

#include "stdinc.h"
#include "GeoManager.h"

#include "GeoIP2.h"
#include "Util.h"

namespace dcpp {

void GeoManager::init() {
	geo = make_unique<GeoIP>(getDbPath());
}

void GeoManager::update() {
	if (geo) {
		geo->update();
		//geo->rebuild();
	}
}

/*
void GeoManager::rebuild() {
	if (geo) {
		geo->rebuild();
	}
}
*/

void GeoManager::close() {
	geo.reset();
}

string GeoManager::getCountry(const string& ip) const {
	if (ip.empty() || !geo.get())
		return Util::emptyString;

	return geo->getCountry(ip);
}

string GeoManager::getEpoch() const {
	if (!geo.get())
		return Util::emptyString;

	return geo->getEpoch();
}

string GeoManager::getVersion() const {
	if (!geo.get())
		return Util::emptyString;

	return geo->getVersion();
}

string GeoManager::getDbPath() {
	return Util::getPath(Util::PATH_USER_LOCAL) + "GeoLite2-City.mmdb";
}

} // namespace dcpp
