/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#include <dwt/util/win32/ApiHelpers.h>

namespace dwt { namespace util { namespace win32 {

size_t getWindowTextLength(HWND hWnd) {
	return static_cast<size_t>(::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0));
}

tstring getWindowText(HWND hWnd) {
	size_t textLength = getWindowTextLength(hWnd);
	if (textLength == 0)
		return tstring();
	tstring retVal(textLength + 1, 0);
	retVal.resize(::SendMessage(hWnd, WM_GETTEXT, static_cast<WPARAM>(textLength + 1), reinterpret_cast<LPARAM>(&retVal[0])));
	return retVal;
}

void updateStyle(HWND hwnd, int which, DWORD style, bool add) {
	DWORD newStyle = ::GetWindowLongPtr(hwnd, which);
	bool mustUpdate = false;
	if(add && (newStyle & style) != style) {
		mustUpdate = true;
		newStyle |= style;
	} else if(!add && (newStyle & style) == style) {
		mustUpdate = true;
		newStyle ^= style;
	}

	if(mustUpdate) {
		::SetWindowLongPtr(hwnd, which, newStyle);

		// Faking a recheck in the window to read new style... (hack)
		::SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

} } }
