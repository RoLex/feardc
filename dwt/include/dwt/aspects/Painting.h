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

#ifndef DWT_aspects_Painting_h
#define DWT_aspects_Painting_h

#include "../CanvasClasses.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that can be custom painted.
/** \ingroup aspects::Classes
  * When a Painting Event is raised the Widget needs to be repainted.
  */
template< class WidgetType >
class Painting
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

public:
	/// \ingroup EventHandlersaspects::Painting
	/// Painting event handler setter
	/** If supplied, event handler is called with a PaintCanvas& which you can use to
	  * paint stuff onto the window with. <br>
	  * Parameters passed is PaintCanvas&
	  */
	void onPainting(std::function<void (PaintCanvas&)> f) {
		W().addCallback(Message(WM_PAINT), [f, this](const MSG&, LRESULT&) -> bool {
			PaintCanvas canvas { &this->W() };
			f(canvas);
			return true;
		});
	}

	void onPrinting(std::function<void (Canvas&)> f) {
		W().addCallback(Message(WM_PRINTCLIENT), [f](const MSG& msg, LRESULT&) -> bool {
			FreeCanvas canvas { reinterpret_cast<HDC>(msg.wParam) };
			f(canvas);
			return true;
		});
	}
};

} }

#endif
