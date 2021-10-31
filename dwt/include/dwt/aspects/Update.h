/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_aspects_Update_h
#define DWT_aspects_Update_h

#include "../Dispatchers.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of Updating their text property
/** \ingroup aspects::Classes
  * E.g. the TextBox have an Update Aspect to it therefore TextBox
  * realize the aspects::Update through inheritance. When a Widget realizes the Update
  * Aspect it normally means that when the "value" part of the Widget changes, like
  * for instance when a TextBox changes the text value the update event fill be
  * raised.
  */
template< class WidgetType >
class Update
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	typedef Dispatchers::VoidVoid<> UpdateDispatcher;
public:
	/// \ingroup EventHandlersaspects::Update
	/// Sets the event handler for the Updated event.
	/** When the Widget value/text is being updated this event will be raised.
	  */
	void onUpdated(const UpdateDispatcher::F& f) {
		W().addCallback(WidgetType::getUpdateMessage(), UpdateDispatcher(f));
	}
};

} }

#endif
