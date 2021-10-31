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

#ifndef DCPLUSPLUS_DCPP_COMPILER_H
#define DCPLUSPLUS_DCPP_COMPILER_H

#if defined(__GNUC__)

#ifdef _WIN32

#if __GNUC__ < 6 || (__GNUC__ == 6 && __GNUC_MINOR__ < 2)
#error GCC 6.2 is required
#endif

#ifdef HAVE_OLD_MINGW
#error Regular MinGW has stability problems; use a MinGW package from mingw-w64
// see <https://bugs.launchpad.net/dcplusplus/+bug/1029629> for details
#endif

#else // _WIN32

#if __GNUC__ < 5 || (__GNUC__ == 5 && __GNUC_MINOR__ < 4)
#error GCC 5.4 is required
#endif

#endif // _WIN32

#elif defined(_MSC_VER)
#if _MSC_VER < 1800 || _MSC_FULL_VER < 180021114
#error Visual Studio 2013 with the Nov 2013 CTP is required
#endif

//disable the deprecated warnings for the CRT functions.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1

#define strtoll _strtoi64

#else
#error No supported compiler found

#endif

#if defined(_MSC_VER)
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%I64d"
#define U64_FMT "%I64u"

#elif defined(SIZEOF_LONG) && SIZEOF_LONG == 8
#define _LL(x) x##l
#define _ULL(x) x##ul
#define I64_FMT "%ld"
#define U64_FMT "%lu"
#else
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%lld"
#define U64_FMT "%llu"
#endif

#ifndef _REENTRANT
# define _REENTRANT 1
#endif

#ifdef _WIN32

#if _WIN32_WINNT < 0x600 || WINVER < 0x600
#error _WIN32_WINNT / WINVER must require Windows Vista (0x600)
#endif

#if _WIN32_IE < 0x600
#error _WIN32_IE must require Common Controls 6 (0x600)
#endif

#endif

#endif // DCPLUSPLUS_DCPP_COMPILER_H
