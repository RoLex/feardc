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

#ifndef DWT_Message_h
#define DWT_Message_h

#include "WindowsHeaders.h"

#include <functional>

namespace dwt {

/// Class for comparing messages
/** Class helps out abstract away System Messages, used in e.g. aspects::Raw to let user
  * be able to handle any type of generic Message! <br>
  * To understand usage check out google or MSDN and look for Windows Message
  * Procedure or WM_ etc...
  */
struct Message
{
public:
	/// Constructor taking a System Message
	/** Use this one if you need to e.g. handle an aspects::Raw::onRaw Event and the
	  * only interesting parameter to figure out what Message to handle is the actual
	  * Message.
	  */
	explicit Message( UINT msg );

	/// Normally used with e.g. WM_COMMAND or WM_NOTIFY messages
	/** In such cases WM_COMMAND or WM_NOTIFY should be added as the msg and the
	  * notification code or command code should be added as the extraCode parameter.
	  * <br>
	  * Use this one if you need to e.g. handle an aspects::Raw::onRaw Event and it's a
	  * WM_COMMAND, WM_NOTIFY or similar type of Message
	  */
	explicit Message( UINT msg, LPARAM param );

	// Note!
	// We're asserting this constructor call comes from the mainWndProc...
	// We're also ONLY storing enough information in this struct to IDENTIFY the
	// MESSAGE, nothing more. Therefore DON'T assume that the members are pointing
	// to the structures you would expect from the given message. This class is
	// only used to do comparisons between different windows messages. The
	// forceValues will if false "manipulate" the values for easy comparison
	Message( const MSG& msg );

	bool operator<(const Message& rhs) const;
	bool operator==(const Message& rhs) const;
private:
	friend struct std::hash<Message>;

	/// Contains the actual Message
	UINT msg;

	/// Contains the param argument from the Message
	LPARAM param;
};

inline bool Message::operator==( const Message & right ) const {
	return msg == right.msg && param == right.param;
}

}

namespace std {
template<>
struct hash<dwt::Message> {
	size_t operator()(const dwt::Message& message) const { return message.msg * 31 + message.param; }
};

}

#endif
