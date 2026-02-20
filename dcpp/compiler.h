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

#ifndef DCPLUSPLUS_DCPP_COMPILER_H
#define DCPLUSPLUS_DCPP_COMPILER_H

#if defined(__GNUC__)

#ifdef _WIN32

#if __GNUC__ < 8 || (__GNUC__ == 8 && __GNUC_MINOR__ < 1)
#error GCC 8.1 is required
#endif

//@todo remove when we raise WIN32 gcc version requirement to higher than this

#if __GNUC__ < 9 || (__GNUC__ == 9 && __GNUC_MINOR__ < 2)
#warning "Randomization quality will be poor using this compiler, see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85494"
#endif

#ifdef HAVE_OLD_MINGW
#error Regular MinGW may have stability problems; use a MinGW package from mingw-w64
// see <https://bugs.launchpad.net/dcplusplus/+bug/2032940> for details
#endif

#else // _WIN32

#if __GNUC__ < 7 || (__GNUC__ == 7 && __GNUC_MINOR__ < 5)
#error GCC 7.5 is required
#endif

#endif // _WIN32

#elif defined(_MSC_VER)
#if _MSC_VER < 1910 || _MSC_FULL_VER < 191025017
#error Visual Studio 2017 required
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

#if _WIN32_WINNT < 0x601 || WINVER < 0x601
#error _WIN32_WINNT / WINVER must require Windows 7 (0x601)
#endif

#if _WIN32_IE < 0x600
#error _WIN32_IE must require Common Controls 6 (0x600)
#endif

#endif

#endif // DCPLUSPLUS_DCPP_COMPILER_H
