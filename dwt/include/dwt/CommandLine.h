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

#ifndef DWT_CommandLine_h
#define DWT_CommandLine_h

#include <boost/core/noncopyable.hpp>
#include <vector>
#include "tstring.h"

namespace dwt {

class Application;

/// Class declaration for the CommandLine class
/** The CommandLine class is a helper class for extracting the Command line
  * parameters which are being sent into the WinMain/main function. <br>
  * If you need to retrieve the Command line parameters use this class.
  */
class CommandLine : public boost::noncopyable
{
	friend class Application;
public:
	/// Returns a vector of the actual params
	/** The parameters are split using standard argv semantics */
	const std::vector< tstring > & getParams() const;

	/// Returns the "raw" command line parameter
	/** For those of you which MUST have the actual RAW command line parameter you
	  * can use this function which will return them as given to the application.
	  */
	const tstring& getParamsRaw() const;
private:
	// Only Application which is friend can instantiate an object of this type!!
	CommandLine( ) { }

	mutable std::vector< tstring > itsCmdLine;
	mutable tstring itsRawCmdLine;
};

}

#endif
