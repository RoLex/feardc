/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_SEARCHRESULT_H
#define DCPLUSPLUS_DCPP_SEARCHRESULT_H

#include "forward.h"
#include "AdcCommand.h"
#include "FastAlloc.h"
#include "HintedUser.h"
#include "MerkleTree.h"
#include "Pointer.h"

#include <boost/core/noncopyable.hpp>

namespace dcpp {

class SearchManager;

class SearchResult : public FastAlloc<SearchResult>, public intrusive_ptr_base<SearchResult>, boost::noncopyable {
public:
	enum Types {
		TYPE_FILE,
		TYPE_DIRECTORY
	};

	SearchResult(Types aType, int64_t aSize, const string& name, const TTHValue& aTTH);

	SearchResult(const HintedUser& aUser, Types aType, int aSlots, int aFreeSlots,
		int64_t aSize, const string& aFile, const string& aHubName,
		const string& ip, TTHValue aTTH, const string& aToken, const Style& style);

	string toSR(const Client& client) const;
	AdcCommand toRES(char type) const;

	const string& getFile() const { return file; }
	string getFileName() const;
	const string& getHubName() const { return hubName; }
	HintedUser& getUser() { return user; }
	int64_t getSize() const { return size; }
	Types getType() const { return type; }
	int getSlots() const { return slots; }
	int getFreeSlots() const { return freeSlots; }
	string getSlotString() const;
	TTHValue getTTH() const { return tth; }
	const string& getIP() const { return IP; }
	const string& getToken() const { return token; }
	const Style& getStyle() const { return style; }

private:
	friend class SearchManager;

	SearchResult();

	string file;
	string hubName;
	HintedUser user;
	int64_t size;
	Types type;
	int slots;
	int freeSlots;
	string IP;
	TTHValue tth;
	string token;
	Style style;
};

}

#endif
