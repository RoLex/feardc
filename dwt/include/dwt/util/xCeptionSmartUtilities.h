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

#ifndef DWT_xCeptionSmartUtilities_h
#define DWT_xCeptionSmartUtilities_h

#include <exception>

namespace dwt { namespace util {

/// Exception class for SmartUtil library
class xCeptionSmartUtilities : public std::exception
{
	public:
		/// Constructor taking a const char pointer
		/** The char * will contain the description returned from the virtual member function called what()
		  */
		xCeptionSmartUtilities( const char * err ) throw()
			: std::exception(),
			itsAsciiErrorMsg( err )
		{}

		/// Overloaded Constructor basically doing the same as the const TCHAR * version
		/** See the const char * err overloaded version
		  */
		xCeptionSmartUtilities( const std::string & err ) throw()
			: std::exception(),
			itsAsciiErrorMsg( err )
		{}

		/// Returns a descriptive error message explaining what went wrong
		/** Overridden from the std::exception::what()<br>
		  * This function will ALWAYS return a char * string since it must be compliant with std::exception::what() function
		  */
		virtual const char * what() const throw()
		{
			return itsAsciiErrorMsg.c_str();
		}

		virtual ~xCeptionSmartUtilities() throw()
		{};

	private:
		std::string itsAsciiErrorMsg;
};

} }

#endif
