/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** @file
 * This file contains all common DWT includes and also configures compiler-specific
 * quirks etc.
 */
#ifndef DWT_WindowsHeaders_h
#define DWT_WindowsHeaders_h

#if _WIN32_WINNT < 0x502 || WINVER < 0x502
#error _WIN32_WINNT / WINVER must require Windows XP SP2 (0x502)
#endif

#if _WIN32_IE < 0x600
#error _WIN32_IE must require Common Controls 6 (0x600)
#endif

#include <cstdint>

// Windows API files...
#include <errno.h>
#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <commdlg.h>
#include <assert.h>

// GCC specific
#ifdef __GNUC__
#include "GCCHeaders.h"
#endif //! __GNUC__

// MSVC specific
#ifdef _MSC_VER
#include "VCDesktopHeaders.h"
#endif

// Standard headers that most classes need
#include <memory>
#include <vector>
#include <list>
#include <boost/core/noncopyable.hpp>

// Other quirks

// as of 2024-05-09, Debian sid's mingw-w64 does not define LM_GETIDEALSIZE
#ifndef LM_GETIDEALSIZE
#define LM_GETIDEALSIZE LM_GETIDEALHEIGHT
#endif

#endif // !WindowsHeaders_h
