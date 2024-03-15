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

#ifndef DWT_aspects_Mouse_h
#define DWT_aspects_Mouse_h

#include "../Events.h"
#include "../Message.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of trapping "mouse
/// clicked" events.
/** \ingroup aspects::Classes
* E.g. the Window can trap "mouse clicked events" therefore it realize the
* aspects::Mouse through inheritance.
*/
template< class WidgetType >
class Mouse
{
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef std::function<bool (const MouseEvent&)> F;

public:
	/// \ingroup EventHandlersaspects::Mouse
	/// Left mouse button pressed event handler setter
	/** If supplied, function will be called when user press the Left Mouse button in
	* the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onLeftMouseDown(F f) {
		onMouse(WM_LBUTTONDOWN, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Left mouse button pressed and released event handler setter
	/** If supplied, function will be called when user releases the Left Mouse button
	* after clicking onto the client area of the Widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onLeftMouseUp(F f) {
		onMouse(WM_LBUTTONUP, f);
	}

	/// Left mouse button double-clicked event handler setter
	/** If supplied, function will be called when user double clicks the Left mouse button
	* in the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onLeftMouseDblClick(F f) {
		onMouse(WM_LBUTTONDBLCLK, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Right mouse button pressed event handler setter
	/** If supplied, function will be called when user press the Right Mouse button
	* in the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onRightMouseDown(F f) {
		onMouse(WM_RBUTTONDOWN, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Right mouse button pressed and released event handler setter
	/** If supplied, function will be called when user releases the Right Mouse
	* button after clicking onto the client area of the Widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onRightMouseUp(F f) {
		onMouse(WM_RBUTTONUP, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Right mouse button double-clicked event handler setter
	/** If supplied, function will be called when user  double clicks the Right mouse button
	* in the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onRightMouseDblClick(F f) {
		onMouse(WM_RBUTTONDBLCLK, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Middle mouse button pressed event handler setter
	/** If supplied, function will be called when user press the Middle Mouse button
	* in the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onMiddleMouseDown(F f) {
		onMouse(WM_MBUTTONDOWN, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Middle mouse button pressed and released event handler setter
	/** If supplied, function will be called when user releases the middle Mouse
	* button after clicking onto the client area of the Widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onMiddleMouseUp(F f) {
		onMouse(WM_MBUTTONUP, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Middle mouse button double-clicked event handler setter
	/** If supplied, function will be called when user double clicks the Middle mouse button
	* in the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onMiddleDblClick(F f) {
		onMouse(WM_MBUTTONDBLCLK, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// X mouse button pressed event handler setter
	/** If supplied, function will be called when user press the X Mouse button in
	* the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onXMouseDown(F f) {
		onXMouse(WM_XBUTTONDOWN, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// X mouse button pressed and released event handler setter
	/** If supplied, function will be called when user releases the X Mouse button
	* after clicking onto the client area of the Widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onXMouseUp(F f) {
		onXMouse(WM_XBUTTONUP, f);
	}

	/// X mouse button double-clicked event handler setter
	/** If supplied, function will be called when user double clicks the X mouse button
	* in the client area of the widget. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onXMouseDblClick(F f) {
		onXMouse(WM_XBUTTONDBLCLK, f);
	}

	/// \ingroup EventHandlersaspects::Mouse
	/// Mouse moved event handler setter
	/** If supplied, function will be called when user moves the mouse. <br>
	* The parameter passed is const MouseEvent & which contains the state of
	* the mouse.
	*/
	void onMouseMove(F f) {
		onMouse(WM_MOUSEMOVE, f);
	}

	/** Handle mouse wheel events.
	 *
	 * In addition to a regular MouseEvent structure, a scrolling delta
	 * is also provided. On Windows, that delta is a multiple of the WHEEL_DELTA macro, which is
	 * 120. See the WM_MOUSEWHEEL doc for more information.
	 *
	 * Note: Dispatching rules for this message on Windows are special: the OS starts with the
	 * focus window and goes up the chain on its own - check the doc for more information. */
	void onMouseWheel(std::function<void (const MouseEvent&, int)> f) {
		W().addCallback(Message(WM_MOUSEWHEEL), [f](const MSG& msg, LRESULT&) {
			f(MouseEvent(msg), static_cast<int>(GET_WHEEL_DELTA_WPARAM(msg.wParam)));
			return true;
		});
	}

	void onMouseLeave(std::function<void ()> f) {
		TRACKMOUSEEVENT t = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, H() };
		if(::TrackMouseEvent(&t)) {
			W().setCallback(Message(WM_MOUSELEAVE), [f](const MSG&, LRESULT&) -> bool {
				f();
				return true;
			});
		}
	}

private:
	void onMouse(UINT msg, F f) {
		W().addCallback(Message(msg), [f](const MSG& msg, LRESULT&) -> bool {
			return f(MouseEvent(msg));
		});
	}

	void onXMouse(UINT msg, F f) {
		W().addCallback(Message(msg), [f](const MSG& msg, LRESULT& ret) -> bool {
			ret = TRUE;
			return f(MouseEvent(msg));
		});
	}
};

} }

#endif
