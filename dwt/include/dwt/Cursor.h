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

#ifndef DWT_Cursor_h
#define DWT_Cursor_h

#include "WindowsHeaders.h"

namespace dwt {

/// Class for manipulating the mouse cursor
/** Use getCursor and store the returned object until you wish to change back to the
 * previous cursor
 */
class Cursor : boost::noncopyable
{
	explicit Cursor(HCURSOR old) : old(old), owner(true) { }

	Cursor(Cursor &&rhs) {*this = std::move(rhs);}

	Cursor &operator=(Cursor &&rhs) {
		if(this != &rhs) {
			old = rhs.old;
			owner = true;
			rhs.owner = false;
			rhs.old = NULL;
		}

		return *this;
	}
public:
	/// Returns a wait cursor
		/** Store the returned object and when object goes out of scope or is freed old
		 * cursor will be set again
		 */
		static Cursor getWaitCursor() {
			return Cursor(::SetCursor( ::LoadCursor( 0, IDC_WAIT ) ) );
		}

		~Cursor() {
			if(owner) {
				::SetCursor(old);
			}
		}

	private:
		HCURSOR old;
		bool owner;
	};

}

#endif
