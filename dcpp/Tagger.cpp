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
#include "Tagger.h"

#include "SimpleXML.h"

#include <vector>

#include <boost/range/adaptor/reversed.hpp>

namespace dcpp {

using std::vector;

Tagger::Tagger(const string& text) : text(text)
{
}

Tagger::Tagger(string&& text) : text(move(text))
{
}

const string& Tagger::getText() const {
	return text;
}

void Tagger::addTag(size_t start, size_t end, string id, string attributes) {
	Tag openingTag { start, "<" + id + " " + move(attributes) + ">", true };
	Tag closingTag { end, "</" + move(id) + ">", false };

	tags.push_back(move(openingTag));
	auto& opening = tags.back();

	tags.push_back(move(closingTag));
	auto& closing = tags.back();

	opening.otherTag = &closing;
	closing.otherTag = &opening;
}

void Tagger::replaceText(size_t start, size_t end, const string& replacement) {
	text.replace(start, end - start, replacement);

	const auto delta = static_cast<int>(replacement.size()) - static_cast<int>(end - start);

	for(auto& tag: tags) {
		if(tag.pos >= end) {
			tag.pos += delta;
		} else if(tag.pos > start) {
			tag.pos = start;
		}
	}
}

string Tagger::merge(string& tmp) {
	tags.sort([](const Tag& a, const Tag& b) { return a.pos < b.pos; });

	string ret;

	size_t pos = 0;
	vector<Tag*> openTags;

	for(auto& tag: tags) {
		ret += SimpleXML::escape(text.substr(pos, tag.pos - pos), tmp, false);
		pos = tag.pos;

		if(tag.opening) {
			ret += tag.s;
			openTags.push_back(&tag);

		} else {
			if(openTags.back() == tag.otherTag) {
				// common case: no entangled tag; just write the closing tag.
				ret += tag.s;
				openTags.pop_back();

			} else {
				// there are entangled tags: write closing & opening tags of currently open tags.
				for(auto openTag: openTags | boost::adaptors::reversed) { if(openTag->pos >= tag.otherTag->pos) {
					ret += openTag->otherTag->s;
				} }
				openTags.erase(remove(openTags.begin(), openTags.end(), tag.otherTag), openTags.end());
				for(auto openTag: openTags) { if(openTag->pos >= tag.otherTag->pos) {
					ret += openTag->s;
				} }
			}
		}
	}

	if(pos != text.size()) {
		ret += SimpleXML::escape(text.substr(pos), tmp, false);
	}

	return ret;
}

} // namespace dcpp
