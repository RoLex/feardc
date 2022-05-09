/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
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

#ifndef DWT_ASPECTCLOSE_H_
#define DWT_ASPECTCLOSE_H_

#include "../Message.h"

namespace dwt { namespace aspects {

template< class WidgetType >
class Closeable {
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

public:
	/// Closes the window
	/** Call this function to raise the "Closing" event. <br>
	  * This will normally try to close the window. <br>
	  * Note! <br>
	  * If this event is trapped and we in that event handler state that we DON'T
	  * want to close the window (by returning false) the window will not be close.
	  * <br>
	  * Note! <br>
	  * If the asyncron argument is true the message will be posted to the message
	  * que meaning that the close event will be done asyncronously and therefore the
	  * function will return immediately and the close event will be handled when the
	  * close event pops up in the event handler que.
	  */
	void close(bool asyncron = false) {
		if(asyncron)
			W().postMessage(WM_CLOSE); // Return now
		else
			W().sendMessage(WM_CLOSE); // Return after close is done.
	}

	/// Event Handler setter for the Closing Event
	/** If supplied event handler is called before the window is closed. <br>
	  * Signature of event handler must be "bool foo()" <br>
	  * If you return true from your event handler the window is closed, otherwise
	  * the window is NOT allowed to actually close!!
	  */
	void onClosing(std::function<bool ()> f) {
		W().addCallback(Message(WM_CLOSE), [f](const MSG&, LRESULT&) -> bool {
			return !f();
		});
	}
};

} }

#endif /*ASPECTCLOSE_H_*/
