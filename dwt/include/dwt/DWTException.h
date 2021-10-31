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

#ifndef DWT_EXCEPTION_H
#define DWT_EXCEPTION_H

#include "WindowsHeaders.h"

#include <string>
#include <stdexcept>

namespace dwt {

/// Exception class used in SmartWin
/** All exceptions thrown by SmartWin will be either of this class or derived from
  * this class. <br>
  * You should derive from this class yourself if you throw exceptions inside your
  * event handlers. <br>
  * Otherwise SmartWin may not be able to respond correctly to the Exception.
  */
class DWTException : public std::runtime_error {
public:

	DWTException( const std::string& err ) : std::runtime_error(err) { }
	virtual ~DWTException() throw() { };
private:
};

/// Utility class for handling win32 errors - don't use it directly...
class Win32Exception : public DWTException {
public:
	Win32Exception() : DWTException(translateLastError()), code(::GetLastError()) { }
	Win32Exception(const std::string& msg) : DWTException(msg + " (" + translateLastError() + ")"), code(::GetLastError()) { }
	virtual ~Win32Exception() throw() { }

	DWORD getCode() { return code; }
private:
	DWORD code;

	static std::string translateLastError();
};

}

#endif
