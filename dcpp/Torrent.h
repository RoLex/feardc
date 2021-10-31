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

#ifndef DCPLUSPLUS_DCPP_TORRENT_H_
#define DCPLUSPLUS_DCPP_TORRENT_H_

#ifndef TORRENT_H_
#define TORRENT_H_

#include <string>
#include <vector>
#include <boost/optional/optional.hpp>

#include "forward.h"
#include "typedefs.h"
#include "SHA1Hash.h"
#include "MD5Hash.h"
#include "MerkleTree.h"

namespace dcpp {

using std::string;
using std::vector;
using boost::optional;

struct TorrentReader;

/**
 * Bittorrent interop - read torrent files and provide utilites to match them
 * to TTH's.
 */
class Torrent {
public:
	struct File {
		File() : length(-1) { }
		int64_t length;
		StringList path;
		optional<MD5Value> md5sum;
		optional<TTHValue> tth;
	};

	explicit Torrent(const string& data);

	void match(const SearchResult &sr);
	bool allMatched() const;

	vector<File>& getFiles() { return files; }
	const vector<SHA1Value> getPieces() { return pieces; }

private:
	friend struct TorrentReader;

	vector<File> files;
	vector<SHA1Value> pieces;
};

}

#endif

#endif /* DCPLUSPLUS_DCPP_TORRENT_H_ */
