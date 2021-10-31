/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_ASPECTTIMER_H_
#define DWT_ASPECTTIMER_H_

#include "../Message.h"

namespace dwt { namespace aspects {

template< class WidgetType >
class Timer {
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

public:
	/// Creates a timer object.
	/** The event function will be called when at least milliSeconds seconds have elapsed.
	  * If your event handler returns true, it will keep getting called periodically, otherwise
	  * it will be removed.
	  */
	void setTimer(std::function<bool ()> f, unsigned int milliSeconds, unsigned int id = 0) {
		if(milliSeconds) {
			::SetTimer(H(), id, milliSeconds, 0);
			W().setCallback(Message(WM_TIMER, id), [f](const MSG& msg, LRESULT&) -> bool {
				if(!f()) {
					/// @todo remove from message map as well...
					::KillTimer(msg.hwnd, msg.wParam);
				}
				return true;
			});
		} else {
			::KillTimer(H(), id);
		}
	}
};

} }

#endif /*ASPECTTIMER_H_*/
