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

#ifndef DWT_aspects_Keyboard_h
#define DWT_aspects_Keyboard_h

#include "../Message.h"

namespace dwt { namespace aspects {

/**
 * Base functionality that doesn't depend on template parameters
 */
class KeyboardBase {
public:
	static bool isKeyPressed(int vkey) {
		return ::GetKeyState(vkey) < 0;
	}

	static bool isShiftPressed() { return isKeyPressed(VK_SHIFT); }
	static bool isControlPressed() { return isKeyPressed(VK_CONTROL); }
	static bool isAltPressed() { return isKeyPressed(VK_MENU); }

	static bool isAnyKeyPressed() {
		BYTE keys[256];
		if(::GetKeyboardState(keys)) {
			for(size_t i = 0; i < 256; ++i) {
				// ignore virtual key codes for mouse buttons. for the rest, look at the high-order bit.
				if(i != VK_LBUTTON && i != VK_RBUTTON && i != VK_MBUTTON && i != VK_XBUTTON1 && i != VK_XBUTTON2 && keys[i] & 0x80) {
					return true;
				}
			}
		}
		return false;
	}

	/// Checks if Caps Lock is on
	/** Use this function if you need to determine if Caps Lock is ON
	  */
	static bool isCapsLockOn() { return 0x1 == ( 0x1 & ::GetKeyState( VK_CAPITAL ) ); }

	/// Get ascii character from a Virtual Key
	/** Use this to convert from the input to the response to onKeyPressed to a
	  * character. <br>
	  * Virtual Keys do not take into account the shift status of the keyboard, and
	  * always report UPPERCASE letters.
	  */
	static char virtualKeyToChar( int vkey ) {
		char theChar = 0;
		// Doing Alphabetic keys separately is not needed, but saves some steps.
		if ( ( vkey >= 'A' ) && ( vkey <= 'Z' ) )
		{
			// Left or Right shift key pressed
			bool shifted = isShiftPressed();
			bool caps_locked = isCapsLockOn(); // Caps lock toggled on.

			// The vkey comes as uppercase, if that is desired, leave it.
			if ( ( shifted || caps_locked ) && shifted != caps_locked )
			{
				theChar = vkey;
			}
			else
			{
				theChar = ( vkey - 'A' ) + 'a'; // Otherwise, convert to lowercase
			}
		}
		else
		{
			BYTE keyboardState[256];
			::GetKeyboardState( keyboardState );

			WORD wordchar;
			int retv = ::ToAscii( vkey, ::MapVirtualKey( vkey, 0 ), keyboardState, & wordchar, 0 );
			if ( 1 == retv )
			{
				theChar = wordchar & 0xff;
			}
		}
		return theChar;
	}
};

/// Aspect class used by Widgets that have the possibility of trapping keyboard events.
/** \ingroup aspects::Classes
  * E.g. the Table can trap "key pressed events" therefore they realize the aspects::Keyboard through inheritance.
  */
template< class WidgetType >
class Keyboard : public KeyboardBase
{
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef std::function<bool (int)> KeyF;

public:
	/// Gives the Widget the keyboard focus
	/** Use this function if you wish to give the Focus to a specific Widget
	  */
	void setFocus() {
		::SetFocus(H());
	}

	/// Retrieves the focus property of the Widget
	/** Use this function to check if the Widget has focus or not. If the Widget has
	  * focus this function will return true.
	  */
	bool hasFocus() const {
		return ::GetFocus() == H();
	}

	/** set the function to be called when the control gains focus. */
	void onFocus(std::function<void ()> f) {
		W().addCallback(Message(WM_SETFOCUS), [f](const MSG&, LRESULT&) -> bool {
			f();
			return false;
		});
	}

	/** set the function to be called before the control looses focus. the control being activated,
	if available, will be passed as a parameter. */
	void onKillFocus(std::function<void (Widget*)> f) {
		W().addCallback(Message(WM_KILLFOCUS), [f](const MSG& msg, LRESULT&) -> bool {
			f(msg.wParam ? hwnd_cast<Widget*>(reinterpret_cast<HWND>(msg.wParam)) : nullptr);
			return false;
		});
	}

	/// \ingroup EventHandlersaspects::Keyboard
	/// Setting the event handler for the "key pressed" event
	/** If supplied event handler is called when control has the focus and a key is
	  * being pressed (before it is released) <br>
	  * parameter passed is int which is the virtual-key code of the nonsystem key
	  * being pressed. Return value must be of type bool, if event handler returns
	  * true event is defined as "handled" meaning the system will not try itself to
	  * handle the event.<br>
	  *
	  * Certain widgets, such as TextBox, will not report VK_RETURN unless you
	  * include ES_WANTRETURN in the style field of of the creational structure
	  * passed when you createTextBox( cs ).
	  *
	  * Use virtualKeyToChar to transform virtual key code to a char, though this
	  * will obviously not work for e.g. arrow keys etc...
	  */
	void onKeyDown(KeyF f) {
		onKey(WM_KEYDOWN, f);
	}

	void onChar(KeyF f) {
		onKey(WM_CHAR, f);
	}

	void onKeyUp(KeyF f) {
		onKey(WM_KEYUP, f);
	}

	void onSysKeyDown(KeyF f) {
		onKey(WM_SYSKEYDOWN, f);
	}

	void onSysKeyUp(KeyF f) {
		onKey(WM_SYSKEYUP, f);
	}

private:
	void onKey(unsigned message, KeyF f) {
		W().addCallback(Message(message), [f](const MSG& msg, LRESULT&) -> bool {
			return f(static_cast<int>(msg.wParam));
		});
	}
};

} }

#endif
