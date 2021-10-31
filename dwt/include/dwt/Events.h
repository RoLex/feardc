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

#ifndef DWT_EVENTS_H_
#define DWT_EVENTS_H_

#include "WindowsHeaders.h"
#include "Point.h"

namespace dwt {

struct SizedEvent {
	SizedEvent(const MSG& msg);

	/// Size
	/** New size of the window
	  */
	Point size;

	/// is window maximized
	/** true if window was being maximized, otherwise false
	  */
	bool isMaximized;

	/// is window minimized
	/** true if window was being minimized, otherwise false
	  */
	bool isMinimized;

	/// is window restored
	/** true if window was being restored, otherwise false
	  */
	bool isRestored;
};

/// Mouse Event structure
/** Several event handlers supply an object of this type as one or more parameters to
  * their Event Handler. <br>
  * E.g. the "onLeftMouseUp" Event Handler takes an object of this type to give
  * extensive information regarding the Event.
  */
struct MouseEvent {
	MouseEvent(const MSG& msg);

	/// Types of buttons
	enum Button {
		OTHER, LEFT, RIGHT, MIDDLE, X1, X2
	};

	/// Position of mouse
	/** Position of mouse when event was raised
	  */
	ScreenCoordinate pos;

	/// is the CTRL key pressed
	/** true if CTRL key is pressed, otherwise false
	  */
	bool isControlPressed;

	/// is the SHIFT key pressed
	/** true if SHIFT key is pressed, otherwise false
	  */
	bool isShiftPressed;

	/// Indicates which mouse button was actually pressed
	Button ButtonPressed;
};


}

#endif /*EVENTS_H_*/
