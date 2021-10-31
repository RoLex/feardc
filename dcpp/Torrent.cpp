/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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
#include "Torrent.h"

#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/for_each.hpp>

#include "Exception.h"
#include "debug.h"

#include "SimpleBencodeReader.h"
#include "SearchResult.h"

namespace dcpp {

using boost::range::find;
using boost::range::for_each;

struct TorrentReader : SimpleBencodeReader::Callback {
	enum State {
		INFO,
		PIECES,
		NAME,
		LENGTH,
		MD5_SUM,
		FILES,
		PATH,
		UNKNOWN
	};

	TorrentReader(Torrent &t) : t(t), f(0) {
		t.files.emplace_back();
		f = &t.files.back();
	}

	virtual ~TorrentReader() { }

	virtual void intValue(int64_t v) {
		if(!inState(INFO)) {
			return;
		}

		if(inState(LENGTH)) {
			f->length = v;
			return;
		}
	}

	virtual void stringValue(const string &v) {
		if(!inState(INFO)) {
			return;
		}

		if(inState(NAME) || inState(PATH)) {
			f->path.push_back(v);
		} else if(inState(PIECES)) {
			if(v.size() % 20 != 0) {
				throw Exception("Unexpected hash length");
			}

			t.pieces.resize(v.size() / 20);
			for(size_t i = 0; i < t.pieces.size(); ++i) {
				t.pieces[i] = SHA1Value(reinterpret_cast<const uint8_t*>(v.data() + i*20));
			}
		} else if(inState(MD5_SUM)) {
			if(v.size() != 32) {
				// Skip
				return;
			}

			f->md5sum = MD5Value();
			Encoder::fromBase16(v.c_str(), f->md5sum->data, MD5Value::BYTES);
		}
	}

	virtual void startList() {
		if(!inState(INFO)) {
			return;
		}

		if(inState(FILES)) {
			t.files.emplace_back();
			f = &t.files.back();
		}
	}

	virtual void endList() {
		if(inState(INFO) && inState(FILES)) {
			f = &t.files[0];
		}
	}

	virtual void startDictEntry(const string &name) {
		if(name == "info") {
			states.push_back(INFO);
		} else if(name == "pieces") {
			states.push_back(PIECES);
		} else if(name == "name") {
			states.push_back(NAME);
		} else if(name == "length") {
			states.push_back(LENGTH);
		} else if(name == "md5sum") {
			states.push_back(MD5_SUM);
		} else if(name == "files") {
			states.push_back(FILES);
		} else if(name == "path") {
			states.push_back(PATH);
		} else {
			states.push_back(UNKNOWN);
		}
	}

	virtual void endDictEntry() { dcassert(!states.empty()); states.pop_back(); }

	Torrent &t;
	Torrent::File *f;

	std::vector<State> states;

	bool inState(State state) {
		return !states.empty() && find(states, state) != states.end();
	}

	size_t skipDepth;
};

Torrent::Torrent(const string &data) {
	TorrentReader rd(*this);
	SimpleBencodeReader(rd).parse(data);
}

void Torrent::match(const SearchResult &sr) {
	for_each(files, [&](File& f) {
		if(f.length == sr.getSize() && Util::stricmp(sr.getFileName(), f.path.back()) == 0) {
			if(f.tth) {
				// ???
			} else {
				f.tth = sr.getTTH();
			}
		}
	});
}

} // namespace dcpp
