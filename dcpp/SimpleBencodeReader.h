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

#ifndef DCPLUSPLUS_DCPP_SIMPLEBENCODEREADER_H_
#define DCPLUSPLUS_DCPP_SIMPLEBENCODEREADER_H_

#include <cstdint>
#include <string>
#include <boost/core/noncopyable.hpp>

namespace dcpp {

using std::string;

class SimpleBencodeReader {
public:
	struct Callback : boost::noncopyable {
		virtual ~Callback() { }

		virtual void intValue(int64_t) { }
		virtual void stringValue(const string &) { }

		virtual void startList() { }
		virtual void endList() { }

		virtual void startDictEntry(const string &) { }
		virtual void endDictEntry() { }
	};

	SimpleBencodeReader(Callback &cb) : cb(cb) { }

	void parse(const string& data);
private:
	static int64_t readInt(const char **data);
	static string readString(const char **data, const char *end);

	void decode(const char **data, const char *end);

	Callback &cb;
};

}

#endif /* DCPLUSPLUS_DCPP_SIMPLEBENCODEREADER_H_ */
