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

#include <dwt/CommandLine.h>
#include <dwt/WindowsHeaders.h>

namespace dwt {

const std::vector< tstring > & CommandLine::getParams() const
{
	if(itsCmdLine.empty()) {
		// Maybe we haven't converted them yet...
		bool inEscape = false;
		bool inQuote = false;
		tstring param;
		for(size_t i = 0; i < getParamsRaw().size(); ++i) {
			tstring::value_type c = getParamsRaw()[i];
			if(c == '\\') {
				if(inEscape) {
					param.push_back(c);
				}
				inEscape = !inEscape;
			} else if(c == '"') {
				if(inEscape) {
					param.push_back(c);
				} else {
					inQuote = !inQuote;
				}
				inEscape = false;
			} else if(c == ' ' && !inQuote && !inEscape) {
				itsCmdLine.push_back(param);
				param.clear();
			} else {
				param.push_back(c);
				inEscape = false;
			}
		}
		if(!param.empty())
			itsCmdLine.push_back(param);
	}
	return itsCmdLine;
}

const tstring& CommandLine::getParamsRaw() const {
	if(itsRawCmdLine.empty()) {
		itsRawCmdLine = GetCommandLine();
	}
	return itsRawCmdLine;
}

}
