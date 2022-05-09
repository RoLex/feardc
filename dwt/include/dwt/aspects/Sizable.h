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

#ifndef DWT_aspects_Sizable_h
#define DWT_aspects_Sizable_h

#include "../Widget.h"
#include "../Dispatchers.h"
#include "../Events.h"

namespace dwt {

// g++ 10.x otherwise doesn't instantiate it, leading to a linker error.
template SizedEvent Dispatchers::convert<SizedEvent>(const MSG& msg);

namespace aspects {
namespace detail {
	inline Rectangle clientRectFromMSG(const MSG &msg) {
		RECT rc;
		::GetClientRect(msg.hwnd, &rc);
		return Rectangle(rc);
	}
}

/// \ingroup aspects::Classes
/// \ingroup WidgetLayout
/// Aspect class used by Widgets that have the possibility of setting and getting a
/// "size" property of their objects.
/** E.g. the TextBox have a "size" Aspect therefore it realizes the
  * aspects::Sizable through inheritance. <br>
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
class Sizable
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef Dispatchers::ConvertBase<Rectangle, &detail::clientRectFromMSG, 0, false> WindowPosDispatcher;
	typedef Dispatchers::ConvertBase<SizedEvent, &Dispatchers::convert<SizedEvent>, 0, false> SizeDispatcher;
	typedef Dispatchers::ConvertBase<Point, &Point::fromMSG, 0, false> MoveDispatcher;

public:
	void centerWindow() {
		// this is greatly inspired by MFC (wincore.cpp - CWnd::CenterWindow). 

		/// @todo this only works for top-level windows (ie dialogs); for child windows, coords will need conversion.
		auto root = W().getParent()->getRoot();
		auto size = W().getWindowSize();
		auto rect = root->getWindowRect();
		rect.pos.x = (rect.left() + rect.right() - size.x) / 2;
		rect.pos.y = (rect.top() + rect.bottom() - size.y) / 2;
		rect.size = size;

		resize(rect.ensureVisibility(root));
	}

	bool isIconic();
	bool isZoomed();

	void resize(const Rectangle &r);

	void onWindowPosChanged(const typename WindowPosDispatcher::F &f) {
		W().addCallback(Message( WM_WINDOWPOSCHANGED ), WindowPosDispatcher(f));
	}

	/// \ingroup EventHandlersaspects::Sizable
	// Setting the event handler for the "sized" event
	/** The size submitted to the event handler is the new client area size. The
	  * parameter passed is WidgetSizedEventResult which contains the new size
	  * information.
	  */
	void onSized(const typename SizeDispatcher::F& f) {
		W().addCallback(Message( WM_SIZE ), SizeDispatcher(f));
	}

	/// \ingroup EventHandlersaspects::Sizable
	// Setting the event handler for the "moved" event
	/** This event will be raised when the Widget is being moved. The parameter
	  * passed is Point which is the new position of the Widget
	  */
	void onMoved(const typename MoveDispatcher::F& f) {
		W().addCallback(Message( WM_MOVE ), MoveDispatcher(f));
	}
};
 
template< class WidgetType >
bool Sizable< WidgetType >::isIconic()
{
	return ::IsIconic(H()) > 0;
}

template< class WidgetType >
bool Sizable< WidgetType >::isZoomed()
{
	return ::IsZoomed(H()) > 0;
}

template<class WidgetType>
void Sizable<WidgetType>::resize(const Rectangle &r) {
	::MoveWindow(H(), r.left(), r.top(), r.width(), r.height(), TRUE);
}

} }

#endif
