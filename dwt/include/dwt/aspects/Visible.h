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

#ifndef DWT_aspects_Visible_h
#define DWT_aspects_Visible_h

#include "../Dispatchers.h"
#include "../Rectangle.h"

namespace dwt { namespace aspects {

/// \ingroup aspects::Classes
/// Aspect class used by Widgets that have the possibility of manipulating the
/// visibility property
/** E.g. the Table have a Visibility Aspect to it therefore Table
  * realizes aspects::Visible through inheritance. <br>
  * Most Widgets realize this Aspect since they can become visible and invisible.
  * When the visibilty state of the Widget changes in one way or another the visible
  * event is raised. <br>
  * Use the onVisibilityChanged function to set an event handler for trapping this
  * event.
  */
template< class WidgetType >
class Visible
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	HWND H() const { return W().handle(); }

	static bool isVisible(const MSG& msg) { return msg.wParam > 0; }

	typedef Dispatchers::ConvertBase<bool, &Visible<WidgetType>::isVisible> VisibleDispatcher;
	friend class Dispatchers::ConvertBase<bool, &Visible<WidgetType>::isVisible>;

public:
	/// Sets the visibility property of the Widget
	/** Changes the visibility property of the Widget. <br>
	  * Use this function to change the visibility property of the Widget
	  */
	void setVisible(bool visible);

	/// Retrieves the visible property of the Widget
	/** Use this function to check if the Widget is visible or not. <br>
	  * If the Widget is visible this function will return true.
	  */
	bool getVisible() const;

	/** Intercept visibility changes of this widget (when it is shown / hidden). */
	void onVisibilityChanged(typename VisibleDispatcher::F f) {
		W().addCallback(Message(WM_SHOWWINDOW), VisibleDispatcher(f));
	}

	/** Intercept WM_SETREDRAW messages (used to temporarily disable updating of a widget to
	implement them better. Return true from the callback to disable regular processing of the
	message. */
	void onRedrawChanged(std::function<bool (bool)> f) {
		W().addCallback(Message(WM_SETREDRAW), [f](const MSG& msg, LRESULT&) -> bool {
			return f(msg.wParam);
		});
	}

	/// Repaints the whole window
	/** Invalidate the window and repaints it.
	  */
	void redraw(bool now = false);

	void redraw(const Rectangle& r, bool now = false);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< class WidgetType >
void Visible< WidgetType >::setVisible( bool visible )
{
	::ShowWindow( H(), visible ? SW_SHOW : SW_HIDE );
}

template< class WidgetType >
bool Visible< WidgetType >::getVisible() const
{
	return ::IsWindowVisible( H() ) != 0;
}

template<class WidgetType>
void Visible<WidgetType>::redraw(bool now) {
	::RedrawWindow(H(), NULL, NULL, RDW_ERASE | RDW_INVALIDATE | (now ? RDW_UPDATENOW : 0));
}

template<class WidgetType>
void Visible<WidgetType>::redraw(const Rectangle& r, bool now) {
	RECT rc = r;
	::RedrawWindow(H(), &rc, NULL, RDW_ERASE | RDW_INVALIDATE | (now ? RDW_UPDATENOW : 0));
}

} }

#endif
