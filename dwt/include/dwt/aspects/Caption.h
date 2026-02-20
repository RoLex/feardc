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

#ifndef DWT_aspects_Caption_h
#define DWT_aspects_Caption_h

#include "../tstring.h"
#include "../Dispatchers.h"
#include "../Message.h"
#include "../util/win32/ApiHelpers.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of setting their main text caption
template<typename WidgetType>
class Caption
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	static tstring getText(const MSG& msg) {
		return tstring( reinterpret_cast< TCHAR * >( msg.lParam ) );
	}

	typedef Dispatchers::ConvertBase<tstring, &Caption<WidgetType>::getText, 0, false> TextDispatcher;
	friend class Dispatchers::ConvertBase<tstring, &Caption<WidgetType>::getText, 0, false>;
public:
	/// Sets the text of the aspects::Text realizing class
	/** The txt parameter is the new text to put into the realizing object.
	  * note: the caller should make sure the string is correctly formatted for the current context
	  * (eg, make sure there are no new lines in tab controls) - DWT won't do any runtime checking.
	  */
	virtual void setText( const tstring & txt );

	/// Gets the text of the aspects::Text realizing class
	/** The Return value is the text of the realizing class.
	  */
	tstring getText() const;

	/// Length of text in characters
	size_t length() const;

	/// \ingroup EventHandlersaspects::Text
	/// Setting the event handler for the "setText" event
	/** When the text changes in the Widget this event will be raised. <br>
	  * The parameter passed is tstring & which is the new text of the
	  * Widget.
	  */
	void onTextChanging(const typename TextDispatcher::F& f) {
		W().addCallback(Message( WM_SETTEXT ), TextDispatcher(f));
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template< class WidgetType >
void Caption< WidgetType >::setText( const tstring & txt ) {
	W().sendMessage(WM_SETTEXT, 0, reinterpret_cast< LPARAM >(txt.c_str()) );
}

template< class WidgetType >
size_t Caption< WidgetType >::length( ) const {
	return util::win32::getWindowTextLength(H());
}

template< class WidgetType >
tstring Caption< WidgetType >::getText() const
{
	return util::win32::getWindowText(H());
}

} }

#endif
