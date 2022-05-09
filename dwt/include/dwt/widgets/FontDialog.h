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

#ifndef WINCE
#ifndef DWT_FontDialog_h
#define DWT_FontDialog_h

#include "../Widget.h"

namespace dwt {

class FontDialog
{
public:
	/// Constructor Taking pointer to parent
	explicit FontDialog(Widget* parent = 0);

	struct Options {
		bool strikeout; /// if false, the "Strikeout" check-box will be hidden
		bool underline; /// if false, the "Underline" check-box will be hidden
		bool color; /// if false, the "Color" drop-down box will be hidden

		COLORREF bgColor; /// background color of the preview control, or NaC for the default

		Options() : strikeout(true), underline(true), color(true), bgColor(NaC) { }
	};

	/** Shows the dialog
	* @param font initial and returned font (only changed if the user presses OK)
	* @param color initial and returned color (only changed if the user presses OK)
	* @param flags additional flags besides those that dwt automatically adds (see the CHOOSEFONT doc)
	* @return whether the user pressed OK
	*/
	bool open(LOGFONT& font, COLORREF& color, Options* options = 0, DWORD flags = 0);

private:
	Widget* itsParent;

	HWND getParentHandle() const { return itsParent ? itsParent->handle() : NULL; }

	static UINT_PTR CALLBACK CFHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

}

#endif
#endif //! WINCE
