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

#if defined(__GNUC__)
#if __GNUC__ < 8 || (__GNUC__ == 8 && __GNUC_MINOR__ < 1)
#error GCC 8.1 is required
#endif

#ifdef HAVE_OLD_MINGW
#error Regular MinGW may have stability problems; use a MinGW package from mingw-w64
// see <https://bugs.launchpad.net/dcplusplus/+bug/2032940> for details
#endif

#elif defined(_MSC_VER)
#if _MSC_VER < 1910 || _MSC_FULL_VER < 191025017
#error Visual Studio 2017 required
#endif

#else
#error No supported compiler found

#endif
