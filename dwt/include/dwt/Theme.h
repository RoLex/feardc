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

#ifndef DWT_THEME_H
#define DWT_THEME_H

#include "CanvasClasses.h"

#include <uxtheme.h>

#include <boost/optional/optional.hpp>

namespace dwt {

/** helper class to manage a theme. this class:
- manages calls to visual styles APIs without having to link to uxtheme.lib.
- handles opening and closing the theme on destruction and on WM_THEMECHANGED.
*/
class Theme {
public:
	Theme();
	virtual ~Theme();

	void load(const tstring& classes, Widget* w_, bool handleThemeChanges = true);

	/**
	* @param drawParent if false, you have to call isThemeBackgroundPartiallyTransparent and handle
	* drawing the transparent bits yourself.
	* @param clip only draw within this rectangle (useful for partial painting from a PaintCanvas).
	*/
	void drawBackground(Canvas& canvas, int part, int state, const Rectangle& rect,
		bool drawParent = true, boost::optional<Rectangle> clip = boost::none);
	/// @param textFlags see the DrawText doc for possible values.
	void drawText(Canvas& canvas, int part, int state, const tstring& text, unsigned textFlags, const Rectangle& rect);
	void formatRect(Canvas& canvas, int part, int state, Rectangle& rect);
	/// @param textFlags see the DrawText doc for possible values.
	void formatTextRect(Canvas& canvas, int part, int state, const tstring& text, unsigned textFlags, Rectangle& rect);
	/**
	* @param specifier color property of the theme, see the TMT_COLOR doc for possible values.
	* may return NaC if no color is available.
	*/
	COLORREF getColor(int part, int state, int specifier);
	bool getPartSize(Canvas& canvas, int part, int state, Point& ret);
	bool isBackgroundPartiallyTransparent(int part, int state);

	explicit operator bool() const;

private:
	void open(const tstring& classes);
	void close();

	HTHEME theme;
	Widget* w;
};

}

#endif
