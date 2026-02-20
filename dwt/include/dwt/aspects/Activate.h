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

#ifndef DWT_aspects_Activate_h
#define DWT_aspects_Activate_h

#include "../Message.h"
#include "../Dispatchers.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that can be activated.
/** \ingroup aspects::Classes
  * When a Widget is being activated it means that it becomes the "active" Widget
  * meaning that it receives keyboard input and normally if it is a text Widget gets
  * to own the caret. This Aspect is closely related to the aspects::Focus Aspect.
  */
template< class WidgetType >
class Activate
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	static bool isActive(const MSG& msg) {
		return LOWORD( msg.wParam ) == WA_ACTIVE || LOWORD( msg.wParam ) == WA_CLICKACTIVE;
	}

	typedef Dispatchers::ConvertBase<bool, &Activate<WidgetType>::isActive> ActivateDispatcher;
	friend class Dispatchers::ConvertBase<bool, &Activate<WidgetType>::isActive>;
public:
	/// Activates the Widget
	/** Changes the activated property of the Widget. <br>
	  * Use this function to change the activated property of the Widget to true or
	  * with other words make this Widget  the currently active Widget.
	  */
	void setActive();

	/// Retrieves the activated property of the Widget
	/** Use this function to check if the Widget is active or not. <br>
	  * If the Widget is active this function will return true.
	  */
	bool isActive() const;

	/// \ingroup EventHandlersaspects::Activate
	/// Setting the member event handler for the "activated" event
	/** Sets the event handler for changes to the active property of the Widget, if
	  * the active status of the Widget changes the supplied event handler will be
	  * called with either true or false indicating the active state of the Widget.
	  * Parameter passed is bool
	  */
	void onActivate(const typename ActivateDispatcher::F& f) {
		W().addCallback(Message(WM_ACTIVATE), ActivateDispatcher(f));
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< class WidgetType >
void Activate< WidgetType >::setActive()
{
	::SetActiveWindow( H() );
}

template< class WidgetType >
bool Activate< WidgetType >::isActive() const
{
	return ::GetActiveWindow() == H();
}

} }

#endif
