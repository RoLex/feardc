/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#ifndef DWT_BRUSH_H_
#define DWT_BRUSH_H_

#include "../WindowsHeaders.h"
#include "../forward.h"
#include "Handle.h"

namespace dwt {

class Brush : public Handle<GdiPolicy<HBRUSH> > {
public:
	enum SysColor
	{
		Scrollbar = COLOR_SCROLLBAR,
		Background = COLOR_BACKGROUND,
		ActiveCaption = COLOR_ACTIVECAPTION,
		InActiveCaption = COLOR_INACTIVECAPTION,
		Menu = COLOR_MENU,
		Window = COLOR_WINDOW,
		WindowFrame = COLOR_WINDOWFRAME,
		MenuText = COLOR_MENUTEXT,
		WindowText = COLOR_WINDOWTEXT,
		CaptionText = COLOR_CAPTIONTEXT,
		ActiveBorder = COLOR_ACTIVEBORDER,
		InActiveBorder = COLOR_INACTIVEBORDER,
		AppWorkSpace = COLOR_APPWORKSPACE,
		HighLight = COLOR_HIGHLIGHT,
		HighLightText = COLOR_HIGHLIGHTTEXT,
		BtnFace = COLOR_BTNFACE,
		BtnShadow = COLOR_BTNSHADOW,
		GrayText = COLOR_GRAYTEXT,
		BtnText = COLOR_BTNTEXT,
		InActiveCaptionText = COLOR_INACTIVECAPTIONTEXT,
		BtnHighLight = COLOR_BTNHIGHLIGHT,
		Face3D = COLOR_3DFACE,
		Highlight3D = COLOR_3DHIGHLIGHT,
		Shadow3D = COLOR_3DSHADOW,
		InfoText = COLOR_INFOTEXT,
		InfoBk = COLOR_INFOBK,
		Static = COLOR_BACKGROUND, // try ?
		StaticText = COLOR_BTNTEXT,
		GradientActiveCaption = COLOR_GRADIENTACTIVECAPTION,
		GradientInActiveCaption = COLOR_GRADIENTINACTIVECAPTION
	};

	Brush(HBRUSH h, bool own = true);

	Brush(SysColor color);

	Brush(COLORREF color);

private:
	friend class Handle<GdiPolicy<HBRUSH> >;
	typedef Handle<GdiPolicy<HBRUSH> > ResourceType;

};
}
#endif /*BRUSH_H_*/
