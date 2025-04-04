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

#include <dwt/widgets/Button.h>

#include <dwt/CanvasClasses.h>
#include <dwt/util/check.h>

namespace dwt {

const TCHAR Button::windowClass[] = WC_BUTTON;

Button::Seed::Seed(const tstring& caption, DWORD style) :
BaseType::Seed(style | WS_CHILD | WS_TABSTOP, 0, caption),
font(0),
padding(3, 2)
{
}

void Button::create(const Seed& cs) {
	BaseType::create(cs);
	setFont(cs.font);

	::RECT rect = { cs.padding.x, cs.padding.y, cs.padding.x, cs.padding.y };
	sendMessage(BCM_SETTEXTMARGIN, 0, reinterpret_cast<LPARAM>(&rect));
}

void Button::setImage(BitmapPtr bitmap) {
	sendMessage(BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(bitmap->handle()));
}

void Button::setImage(IconPtr icon) {
	sendMessage(BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(icon->handle()));
}

Point Button::getPreferredSize() {
	SIZE size = { 0 };
	if (!sendMessage(BCM_GETIDEALSIZE, 0, reinterpret_cast<LPARAM>(&size))) {
		dwtassert(false, "Button: BCM_GETIDEALSIZE failed");
	}
	return Point(size.cx, size.cy);
}

}
