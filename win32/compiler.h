/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#if defined(__GNUC__)
#if __GNUC__ < 6 || (__GNUC__ == 6 && __GNUC_MINOR__ < 2)
#error GCC 6.2 is required
#endif

#ifdef HAVE_OLD_MINGW
#error Regular MinGW has stability problems; use a MinGW package from mingw-w64
// see <https://bugs.launchpad.net/dcplusplus/+bug/1029629> for details
#endif

#elif defined(_MSC_VER)
#if _MSC_VER < 1800 || _MSC_FULL_VER < 180021114
#error Visual Studio 2013 with the Nov 2013 CTP is required
#endif

#else
#error No supported compiler found

#endif
