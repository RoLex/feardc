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

#ifndef DCPLUSPLUS_DCPP_FINISHED_ITEM_H
#define DCPLUSPLUS_DCPP_FINISHED_ITEM_H

#include <boost/core/noncopyable.hpp>

#include "forward.h"
#include "typedefs.h"

#include "Pointer.h"
#include "GetSet.h"

namespace dcpp {

class FinishedItemBase : boost::noncopyable {
public:
	explicit FinishedItemBase(
		int64_t transferred_,
		int64_t milliSeconds_,
		time_t time_
		);

	void update(
		int64_t transferred_,
		int64_t milliSeconds_,
		time_t time_
		);

	int64_t getAverageSpeed() const;

	GETSET(int64_t, transferred, Transferred);
	GETSET(int64_t, milliSeconds, MilliSeconds);
	GETSET(time_t, time, Time);
};

class FinishedFileItem : public FinishedItemBase, public intrusive_ptr_base<FinishedFileItem> {
public:
	explicit FinishedFileItem(
		int64_t transferred_,
		int64_t milliSeconds_,
		time_t time_,
		int64_t fileSize_,
		int64_t actual_,
		bool crc32Checked_,
		const HintedUser& user
		);

	void update(
		int64_t transferred_,
		int64_t milliSeconds_,
		time_t time_,
		int64_t actual_,
		bool crc32Checked_,
		const HintedUser& user
		);

	double getTransferredPercentage() const;
	bool isFull() const;

	GETSET(HintedUserList, users, Users);
	GETSET(int64_t, fileSize, FileSize);
	GETSET(int64_t, actual, Actual);
	GETSET(bool, crc32Checked, Crc32Checked);
};

class FinishedUserItem : public FinishedItemBase, public intrusive_ptr_base<FinishedUserItem> {
public:
	explicit FinishedUserItem(
		int64_t transferred_,
		int64_t milliSeconds_,
		time_t time_,
		const string& file
		);

	void update(
		int64_t transferred_,
		int64_t milliSeconds_,
		time_t time_,
		const string& file
		);

	GETSET(StringList, files, Files);
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_FINISHED_ITEM_H)
