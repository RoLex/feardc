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

#ifndef DCPLUSPLUS_DCPP_ATOMIC_HPP_
#define DCPLUSPLUS_DCPP_ATOMIC_HPP_

// GCC has issues with atomic - see https://bugs.launchpad.net/dcplusplus/+bug/735512
/// @todo check this again when GCC improves their threading support
#if 0

#include <boost/atomic.hpp>

namespace dcpp
{

using boost::atomic;
using boost::atomic_flag;

}

#ifndef ATOMIC_FLAG_INIT
#define ATOMIC_FLAG_INIT { }
#endif

#else

#include <atomic>

namespace dcpp
{

using std::atomic;
using std::atomic_flag;

}

#endif



#endif /* DCPLUSPLUS_DCPP_ATOMIC_HPP_ */
