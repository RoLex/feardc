/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_aspects_Fonts_h
#define DWT_aspects_Fonts_h

#include "../resources/Font.h"
#include "../CanvasClasses.h"

namespace dwt { namespace aspects {

/** Aspect class used by Widgets that have the possibility of changing their main font. By default,
this is done by sending a WM_SETFONT message. Widgets that want to customize this behavior can
provide a void setFontImpl() function. */
template<typename WidgetType>
class Fonts
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

public:
	/// Fills a Point with the size of text to be drawn in the Widget's font.
	/** getTextSize determines the height and width that text will take. <br>
	  * This is useful if you want to allocate enough space to fit known text. <br>
	  * It accounts for the set font too.
	  */
	Point getTextSize(const tstring& text) {
		UpdateCanvas c(&W());
		auto select(c.select(*W().getFont()));

		Rectangle rect;
		c.drawText(text, rect, DT_CALCRECT | DT_NOPREFIX);

		return rect.size;
	}

	/// Sets the font used by the Widget
	void setFont(FontPtr font) {
		this->font = font ? font : new Font(Font::DefaultGui);
		W().setFontImpl();
	}

	/// Returns the font used by the Widget
	const FontPtr& getFont() {
		if(!font) {
			font = new Font(reinterpret_cast<HFONT>(W().sendMessage(WM_GETFONT)), false);
		}
		return font;
	}

protected:
	virtual void setFontImpl() {
		W().sendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(font->handle()), TRUE);
	}

private:
	// Keep a reference around so it doesn't get deleted
	FontPtr font;
};

} }

#endif
