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

#ifndef DCPLUSPLUS_DCPP_TAGGER_H
#define DCPLUSPLUS_DCPP_TAGGER_H

#include "forward.h"

#include <list>
#include <string>

namespace dcpp {

using std::list;
using std::string;

/** Adds XML tags to a plain text string. The tags are added all at once. The tagger supports
entangled tags, such as: <a> <b> </a> </b> -> <a> <b> </b></a><b> </b> */
class Tagger {
public:
	Tagger(const string& text);
	Tagger(string&& text);

	const string& getText() const;

	void addTag(size_t start, size_t end, string id, string attributes);
	void replaceText(size_t start, size_t end, const string& replacement);
	string merge(string& tmp);

private:
	string text;

	struct Tag { size_t pos; string s; bool opening; Tag* otherTag; };
	list<Tag> tags; // this table holds the tags to be added along with their position.
};

} // namespace dcpp

#endif
