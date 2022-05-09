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
#include "WindowInfo.h"

namespace dcpp {

WindowInfo::WindowInfo(const string& id_, const WindowParams& params_) :
id(id_),
params(params_)
{
}

bool WindowInfo::operator==(const WindowInfo& rhs) const {
	if(id != rhs.id)
		return false;

	// compare every identifying params.
	int rParams = 0;
	for(auto& i: rhs.params)
		if(i.second.isSet(WindowParam::FLAG_IDENTIFIES))
			++rParams;
	for(auto& i: params) {
		if(i.second.isSet(WindowParam::FLAG_IDENTIFIES)) {
			auto ri = rhs.params.find(i.first);
			if(ri == rhs.params.end())
				return false;
			if(i.second.content != ri->second.content)
				return false;
			--rParams;
		}
	}
	return rParams == 0;
}

} // namespace dcpp
