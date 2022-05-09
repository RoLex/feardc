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

#ifndef DWT_aspects_Enabled_h
#define DWT_aspects_Enabled_h

#include "../Message.h"
#include "../Dispatchers.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of changing the enabled
/// property
/** \ingroup aspects::Classes
  * The Table has an enabled Aspect to it; therefore it realizes this
  * aspects::Enabled through inheritance. <br>
  * When a Widget is enabled it is possible to interact with it in some way, e.g. a
  * button can be pushed, a ComboBox can change value etc. When the Widget is not
  * enabled it cannot change its "value" or be interacted with but is normally still
  * visible.
  */
template< class WidgetType >
class Enabled
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	static bool isEnabled(const MSG& msg) { return msg.wParam > 0; }

	typedef Dispatchers::ConvertBase<bool, &aspects::Enabled<WidgetType>::isEnabled, 0, false> EnabledDispatcher;
	friend class Dispatchers::ConvertBase<bool, &aspects::Enabled<WidgetType>::isEnabled, 0, false>;

public:

	/// \ingroup EventHandlersaspects::Enabled
	/// Setting the event handler for the "enabled" event
	/** This event handler will be raised when the enabled property of the Widget is
	  * being changed. <br>
	  * The bool value passed to your Event Handler defines if the widget has just
	  * been enabled or if it has been disabled! <br>
	  * No parameters are passed.
	  */
	void onEnabled(const typename EnabledDispatcher::F& f) {
		W().addCallback(Message( WM_ENABLE ), EnabledDispatcher(f));
	}
};

} }

#endif
