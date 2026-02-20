/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

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

#include <dwt/Theme.h>

#include <vssym32.h>

#include <dwt/Dispatchers.h>
#include <dwt/util/check.h>

namespace dwt {

Theme::Theme() : theme(0)
{
}

Theme::~Theme() {
	close();
}

void Theme::load(const tstring& classes, Widget* w_, bool handleThemeChanges) {
	w = w_;
	dwtassert(w, "Theme: no widget set");

	open(classes);

	if(handleThemeChanges) {
		w->addCallback(Message(WM_THEMECHANGED), Dispatchers::VoidVoid<0, false>([this, classes] {
			close();
			open(classes);
		}));
	}
}

void Theme::drawBackground(Canvas& canvas, int part, int state, const Rectangle& rect, bool drawParent, boost::optional<Rectangle> clip) {
	::RECT rc = rect;
	boost::optional<RECT> rcClip(clip);
	if(drawParent && isBackgroundPartiallyTransparent(part, state)) {
		DrawThemeParentBackground(w->handle(), canvas.handle(), clip ? &rcClip.get() : &rc);
	}
	DrawThemeBackground(theme, canvas.handle(), part, state, &rc, clip ? &rcClip.get() : 0);
}

void Theme::drawText(Canvas& canvas, int part, int state, const tstring& text, unsigned textFlags, const Rectangle& rect) {
	::RECT rc = rect;
	DrawThemeText(theme, canvas.handle(), part, state, text.c_str(), text.size(), textFlags, 0, &rc);
}

void Theme::formatRect(Canvas& canvas, int part, int state, Rectangle& rect) {
	::RECT rc = rect;
	MARGINS margins = { 0 };
	if(GetThemeMargins(theme, canvas.handle(), part, state, TMT_CONTENTMARGINS, &rc, &margins) == S_OK) {
		rc.left += margins.cxLeftWidth; rc.right -= margins.cxRightWidth;
		rc.top += margins.cyTopHeight; rc.bottom -= margins.cyBottomHeight;
		rect = Rectangle(rc);
	}
}

void Theme::formatTextRect(Canvas& canvas, int part, int state, const tstring& text, unsigned textFlags, Rectangle& rect) {
	formatRect(canvas, part, state, rect);

	::RECT rc = rect;
	::RECT rcOut;
	if(GetThemeTextExtent(theme, canvas.handle(), part, state, text.c_str(), text.size(), textFlags, &rc, &rcOut) == S_OK) {
		rect = Rectangle(rcOut);
	}
}

COLORREF Theme::getColor(int part, int state, int specifier) {
	COLORREF color;
	if(GetThemeColor(theme, part, state, specifier, &color) == S_OK)
		return color;
	return NaC;
}

bool Theme::getPartSize(Canvas& canvas, int part, int state, Point& ret) {
	SIZE size;
	if(GetThemePartSize(theme, canvas.handle(), part, state, 0, TS_TRUE, &size) == S_OK) {
		ret.x = size.cx;
		ret.y = size.cy;
		return true;
	}
	return false;
}

bool Theme::isBackgroundPartiallyTransparent(int part, int state) {
	return IsThemeBackgroundPartiallyTransparent(theme, part, state);
}

Theme::operator bool() const {
	return theme;
}

void Theme::open(const tstring& classes) {
	if(IsAppThemed()) {
		theme = OpenThemeData(w->handle(), classes.c_str());
	}
}

void Theme::close() {
	if(theme) {
		CloseThemeData(theme);
		theme = 0;
	}
}

}
