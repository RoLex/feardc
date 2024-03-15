/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#ifndef DWT_ASPECTCOMMAND_
#define DWT_ASPECTCOMMAND_

#include "../Message.h"
#include "../Dispatchers.h"

namespace dwt { namespace aspects {

template<typename WidgetType>
class Command {
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

public:
	typedef Dispatchers::VoidVoid<> CommandDispatcher;

	void onCommand(const CommandDispatcher::F& f, unsigned id) {
		W().addCallback(Message(WM_COMMAND, id), CommandDispatcher(f));
	}

	void onCommand(const CommandDispatcher::F& f, unsigned controlId, unsigned code) {
		W().addCallback(Message(WM_COMMAND, MAKEWPARAM(controlId, code)), CommandDispatcher(f));
	}

	void onSysCommand(const CommandDispatcher::F& f, unsigned id) {
		W().addCallback(Message(WM_SYSCOMMAND, id), CommandDispatcher(f));
	}

	void sendCommand(unsigned id) {
		W().sendMessage(WM_COMMAND, MAKEWPARAM(0, id), reinterpret_cast<LPARAM>(H()));
	}
};

} }

#endif /*ASPECTCOMMAND_*/
