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

#include "stdinc.h"
#include "StringMatch.h"

#include "format.h"
#include "LogManager.h"
#include "StringTokenizer.h"

namespace dcpp {

StringMatch::Method StringMatch::getMethod() const {
	return boost::get<StringSearch::List>(&search) ? PARTIAL : boost::get<string>(&search) ? EXACT : REGEX;
}

void StringMatch::setMethod(Method method) {
	switch(method) {
	case PARTIAL: search = StringSearch::List(); break;
	case EXACT: search = string(); break;
	case REGEX: search = boost::regex(); break;
	case METHOD_LAST: break;
	}
}

bool StringMatch::operator==(const StringMatch& rhs) const {
	return pattern == rhs.pattern && getMethod() == rhs.getMethod();
}

struct Prepare : boost::static_visitor<bool> {
	Prepare(const string& pattern) : pattern(pattern) { }

	bool operator()(StringSearch::List& s) const {
		s.clear();
		StringTokenizer<string> st(pattern, ' ');
		for(auto& i: st.getTokens()) {
			if(!i.empty()) {
				s.emplace_back(i);
			}
		}
		return true;
	}

	bool operator()(string& s) const {
		s = pattern;
		return true;
	}

	bool operator()(boost::regex& r) const {
		try {
			r.assign(pattern);
			return true;
		} catch(const std::runtime_error&) {
			LogManager::getInstance()->message(str(F_("Invalid regular expression: %1%") % pattern));
			return false;
		}
	}

private:
	const string& pattern;
};

bool StringMatch::prepare() {
	return !pattern.empty() && boost::apply_visitor(Prepare(pattern), search);
}

struct Match : boost::static_visitor<bool> {
	Match(const string& str) : str(str) { }

	bool operator()(const StringSearch::List& s) const {
		for(auto& i: s) {
			if(!i.match(str)) {
				return false;
			}
		}
		return !s.empty();
	}

	bool operator()(const string& s) const {
		return str == s;
	}

	bool operator()(const boost::regex& r) const {
		try {
			return !r.empty() && boost::regex_search(str, r);
		} catch(const std::runtime_error&) {
			// most likely a stack overflow, ignore...
			return false;
		}
	}

private:
	const string& str;
};

bool StringMatch::match(const string& str) const {
	return !str.empty() && boost::apply_visitor(Match(str), search);
}

} // namespace dcpp
