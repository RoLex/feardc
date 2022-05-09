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

#include "stdinc.h"
#include "Magnet.h"

#include "StringTokenizer.h"
#include "Text.h"
#include "Util.h"

namespace dcpp {

using std::map;

bool Magnet::parseUri(const string& uri, string& hash, string& name, string& key) {
	if(Util::strnicmp(uri.c_str(), "magnet:?", 8)) {
		return false;
	}

	// official types that are of interest to us
	//  xt = exact topic
	//  xs = exact substitute
	//  as = acceptable substitute
	//  dn = display name

	StringTokenizer<string> mag(uri.substr(8), '&');
	map<string, string> hashes;
	string type, param;
	for(auto& idx: mag.getTokens()) {

		// break into pairs
		auto pos = idx.find('=');
		if(pos != string::npos) {
			type = Text::toLower(Util::encodeURI(idx.substr(0, pos), true));
			param = Util::encodeURI(idx.substr(pos + 1), true);
		} else {
			type = Util::encodeURI(idx, true);
			param.clear();
		}

		// extract what is of value
		if(param.size() == 85 && Util::strnicmp(param.c_str(), "urn:bitprint:", 13) == 0) {
			hashes[type] = param.substr(46);
		} else if(param.size() == 54 && Util::strnicmp(param.c_str(), "urn:tree:tiger:", 15) == 0) {
			hashes[type] = param.substr(15);
		} else if(param.size() == 55 && Util::strnicmp(param.c_str(), "urn:tree:tiger/:", 16) == 0) {
			hashes[type] = param.substr(16);
		} else if(param.size() == 59 && Util::strnicmp(param.c_str(), "urn:tree:tiger/1024:", 20) == 0) {
			hashes[type] = param.substr(20);
		} else if(type.size() == 2 && Util::strnicmp(type.c_str(), "dn", 2) == 0) {
			name = param;
		} else if(type.size() == 2 && Util::strnicmp(type.c_str(), "kt", 2) == 0) {
			key = param;
		}
	}

	// pick the most authoritative hash out of all of them.
	if(hashes.find("xt") != hashes.end()) {
		hash = hashes["xt"];
	} else if(hashes.find("xs") != hashes.end()) {
		hash = hashes["xs"];
	} else if(hashes.find("as") != hashes.end()) {
		hash = hashes["as"];
	}

	if(!hash.empty() || !key.empty()) {
		return true;
	}
	return false;
}

} // namespace dcpp
