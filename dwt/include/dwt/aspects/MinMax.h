/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

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

#ifndef DWT_aspects_MinMax_h
#define DWT_aspects_MinMax_h

#include "../WindowsHeaders.h"

namespace dwt { namespace aspects {

/// \ingroup aspects::Classes
/// \ingroup WidgetLayout
/// Aspect class used by Widgets that have the possibility of setting and getting a
/// "size" property of their objects.
/** E.g. the TextBox have a "size" Aspect therefore it realizes the
  * aspects::MinMax through inheritance. <br>
  * Note! <br>
  * All coordinates have zenith top-left corner of either the desktop display or the
  * client area of the parent Widget. <br>
  * Note! <br>
  * There are two different ways to calculate the position of a Widget, one is in
  * screen coordinates which starts top left of the desktop window, the other way is
  * relative to its parent Widget which starts at the top left of the parent Widgets
  * client area which is the total area of the Widget after the border, menu, toolbar
  * etc have been taken away. <br>
  * In addition all bounding Rectangles dealt with through this class are giving
  * their down right coordinates in SIZES and not in POSITIONS!
  */
template< class WidgetType >
class MinMax
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

public:
	/// Maximize your window
	/** This will make the window fill the whole area that the window has available.
	  * <br>
	  * This function cannot be called for any Widget types other than those derived
	  * from Frame.
	  */
	void maximize();

	/// Minimize your window
	/** This will make the window become minimized. <br>
	  * This function cannot be called for any Widget types other than those derived
	  * from Frame.
	  */
	void minimize();

	/// Restores your window
	/** This will make the window become restored. <br>
	  * This function cannot be called for any Widget types other than those derived
	  * from Frame.
	  */
	void restore();
};

template< class WidgetType >
void MinMax< WidgetType >::maximize()
{
	::ShowWindow(H(), SW_SHOWMAXIMIZED );
}

template< class WidgetType >
void MinMax< WidgetType >::minimize()
{
	::ShowWindow(H(), SW_MINIMIZE );
}

template< class WidgetType >
void MinMax< WidgetType >::restore()
{
	::ShowWindow(H(), SW_RESTORE );
}

} }

#endif
