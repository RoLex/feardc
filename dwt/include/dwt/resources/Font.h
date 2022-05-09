/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

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

#ifndef DWT_Font_h
#define DWT_Font_h

#include "../WindowsHeaders.h"
#include "../forward.h"
#include "Handle.h"

namespace dwt {

/// Class for creating a Font object.
/** This class is the type sent to the aspects::Font realizing classes. <br>
  * Normally you would make an instance of this class and then stuff it into any
  * class object that realizes the aspects::Font Aspect. <br>
  * One instance of this class can be shared among different Widgets ( even different
  * types of Widgets )
  */
class Font : public Handle<GdiPolicy<HFONT>>
{
public:
	Font(HFONT font, bool owned);

	Font(LOGFONT& lf);

	/** Load a system font with the GetStockObject API. These fonts are obsolete and should only be
	used as a last resort. */
	enum Predefined {
		SystemFixed = SYSTEM_FIXED_FONT,
		System = SYSTEM_FONT,
		OemFixed = OEM_FIXED_FONT,
		DefaultGui = DEFAULT_GUI_FONT,
		DeviceDefault = DEVICE_DEFAULT_FONT,
		AnsiVar = ANSI_VAR_FONT,
		AnsiFixed = ANSI_FIXED_FONT
	};
	Font(Predefined predef);

	LOGFONT getLogFont() const;

	/// get a new font with the same characteristics as this one, but bold.
	FontPtr makeBold() const;

private:
	friend class Handle<GdiPolicy<HFONT>>;
	typedef Handle<GdiPolicy<HFONT>> ResourceType;
};

}

#endif
