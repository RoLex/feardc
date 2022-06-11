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

#ifndef DCPLUSPLUS_DCPP_DOWNLOAD_H_
#define DCPLUSPLUS_DCPP_DOWNLOAD_H_

#include <string>
#include <memory>

#include "forward.h"
#include "Transfer.h"
#include "MerkleTree.h"
#include "Flags.h"
#include "GetSet.h"

namespace dcpp {

using std::string;
using std::unique_ptr;

/**
 * Comes as an argument in the DownloadManagerListener functions.
 * Use it to retrieve information about the ongoing transfer.
 */
class Download : public Transfer, public Flags
{
public:
	enum {
		FLAG_ZDOWNLOAD = 1 << 1,
		FLAG_TREE_TRIED = 1 << 2,
		FLAG_TTH_CHECK = 1 << 3,
		FLAG_XML_BZ_LIST = 1 << 4
	};

	Download(UserConnection& conn, QueueItem& qi) noexcept;

	virtual void getParams(const UserConnection& aSource, ParamMap& params);

	virtual ~Download();

	/** @return Target filename without path. */
	string getTargetFileName() const;

	/** Open the target output for writing */
	void open(int64_t bytes, bool z);

	/** Release the target output */
	void close();

	/** @internal */
	TigerTree& getTigerTree() { return tt; }
	const string& getPFS() const { return pfs; }
	/** @internal */
	AdcCommand getCommand(bool zlib);

	const unique_ptr<OutputStream>& getOutput() const { return output; }

	GETSET(string, tempTarget, TempTarget);
	GETSET(bool, treeValid, TreeValid);
private:
	const string& getDownloadTarget() const;

	unique_ptr<OutputStream> output;
	TigerTree tt;
	string pfs;
};

} // namespace dcpp

#endif /*DOWNLOAD_H_*/
