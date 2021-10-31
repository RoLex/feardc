/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#include <dwt/util/win32/Version.h>

#include <dwt/WindowsHeaders.h>

namespace dwt { namespace util { namespace win32 {

bool ensureVersion(Version version) {
	static DWORD major = 0;
	static DWORD minor = 0;

	if(major == 0) {
		OSVERSIONINFOEX ver = { sizeof(OSVERSIONINFOEX) };
		if(!::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ver)))
			ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		if(::GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&ver))) {
			major = ver.dwMajorVersion;
			minor = ver.dwMinorVersion;
		}
	}

	switch(version) {
	case XP: return (major == 5 && minor >= 1) || major > 5;
	case VISTA: return major >= 6;
	case SEVEN: return (major == 6 && minor >= 1) || major > 6;
	default: return false;
	}
}

} } }
