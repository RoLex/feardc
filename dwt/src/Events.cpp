/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
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

#include <dwt/Events.h>

namespace dwt {

SizedEvent::SizedEvent( const MSG& msg ) :
	size(Point::fromLParam(msg.lParam)),
	isMaximized(msg.wParam == SIZE_MAXIMIZED),
	isMinimized(msg.wParam == SIZE_MINIMIZED),
	isRestored(msg.wParam == SIZE_RESTORED)
{
}

MouseEvent::MouseEvent(const MSG& msg) {
	POINT pt = { GET_X_LPARAM( msg.lParam ), GET_Y_LPARAM( msg.lParam ) };
	::ClientToScreen(msg.hwnd, &pt);

	pos = ScreenCoordinate(Point(pt));

	DWORD keys;

	if(msg.message == WM_XBUTTONDOWN || msg.message == WM_XBUTTONUP || msg.message == WM_XBUTTONDBLCLK) {
		keys = GET_KEYSTATE_WPARAM(msg.wParam);

		DWORD button = GET_XBUTTON_WPARAM(msg.wParam);
		ButtonPressed = (
			XBUTTON1 & button ? MouseEvent::X1 : (
				XBUTTON2 & button ? MouseEvent::X2 : MouseEvent::OTHER
			)
		);

	} else {
		// Special case for WM_MOUSEWHEEL, where keys do not make up the whole WPARAM.
		keys = (msg.message != WM_MOUSEWHEEL) ? msg.wParam : GET_KEYSTATE_WPARAM(msg.wParam);

		// These might be an issue when porting to Windows CE since CE does only support LEFT and RIGHT (or something...)
		ButtonPressed = (
			MK_LBUTTON & keys ? MouseEvent::LEFT : (
				MK_RBUTTON & keys ? MouseEvent::RIGHT : (
					MK_MBUTTON & keys ? MouseEvent::MIDDLE : (
						MK_XBUTTON1 & keys ? MouseEvent::X1 : (
							MK_XBUTTON2 & keys ? MouseEvent::X2 : MouseEvent::OTHER
						)
					)
				)
			)
		);
	}

	isControlPressed = keys & MK_CONTROL;
	isShiftPressed = keys & MK_SHIFT;
}

}

