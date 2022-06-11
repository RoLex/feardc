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

#ifndef DCPLUSPLUS_DCPP_WINDOW_INFO_H
#define DCPLUSPLUS_DCPP_WINDOW_INFO_H

#include <string>
#include <unordered_map>

#include "forward.h"
#include "Flags.h"
#include "GetSet.h"

namespace dcpp {

using std::string;
using std::unordered_map;

struct WindowParam : Flags {
	WindowParam() : Flags() { }
	WindowParam(const string& content, Flags::MaskType flags = 0) : Flags(flags), content(content) { }

	enum {
		FLAG_IDENTIFIES = 1 << 1, /// this WindowParam determines the uniqueness of the WindowInfo holding it.

		FLAG_CID = 1 << 2, /// this WindowParam indicates a CID for an user whose information shall be saved on exit.
		FLAG_FILELIST = 1 << 3 /// this WindowParam specifies the path to a file list that must not be deleted on exit.
	};

	string content;

	operator const string&() const { return content; }
	template<typename T> bool operator==(const T& str) const { return content == str; }
	bool empty() const { return content.empty(); }
};

typedef unordered_map<string, WindowParam> WindowParams;

class WindowInfo {
public:
	explicit WindowInfo(const string& id_, const WindowParams& params_);

	GETSET(string, id, Id);
	GETSET(WindowParams, params, Params);

	bool operator==(const WindowInfo& rhs) const;
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_WINDOW_INFO_H)
