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

#ifndef DWT_aspects_EraseBackground_h
#define DWT_aspects_EraseBackground_h

#include "../Message.h"
#include "../CanvasClasses.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of handling the erase
/// background property
/** \ingroup aspects::Classes
  * E.g. the Window have a aspects::EraseBackground Aspect to it therefore
  * Table realize the aspects::EraseBackground through  inheritance. When the
  * Widget needs to erase its background this event will be called with a Canvas
  * object which can be used for  manipulating the colors etc the system uses to
  * erase the background of the Widget with.
  */
template< class WidgetType >
class EraseBackground
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

public:
	/// \ingroup EventHandlersaspects::EraseBackground
	/// Setting the event handler for the "erase background" event
	/** This event handler will be called just before Widget is about to erase its
	  * background, the canvas passed can be used to draw upon etc to manipulate the
	  * background property of the Widget.
	  */
	void onEraseBackground(std::function<bool (Canvas&)> f) {
		W().setCallback(Message(WM_ERASEBKGND), [f](const MSG& msg, LRESULT& ret) -> bool {
			FreeCanvas canvas { reinterpret_cast<HDC>(msg.wParam) };
			ret = f(canvas);
			return ret;
		});
	}

	void noEraseBackground() {
		W().setCallback(Message(WM_ERASEBKGND), &aspects::EraseBackground<WidgetType>::noEraseDispatcher);
	}

protected:
	static bool noEraseDispatcher(const MSG& msg, LRESULT& ret) {
		ret = 1;
		return true;
	}
};

} }

#endif
