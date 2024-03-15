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

#ifndef DCPLUSPLUS_DCPP_TRIBOOL_H
#define DCPLUSPLUS_DCPP_TRIBOOL_H

#include <boost/logic/tribool.hpp>

using boost::indeterminate;
using boost::tribool;

// conversions between tribools and ints, with 0 being the indeterminate value
namespace {
	inline tribool to3bool(int x) { if(x) { return tribool(x == 1); } return tribool(indeterminate); }
	inline int toInt(tribool x) { return x ? 1 : !x ? 2 : 0; }
}

#endif
