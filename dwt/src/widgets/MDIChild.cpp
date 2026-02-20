/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

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

#include <dwt/widgets/MDIChild.h>
#include <dwt/DWTException.h>

namespace dwt {

MDIChild::Seed::Seed(const tstring& caption) :
	BaseType::Seed(caption, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS, WS_EX_MDICHILD),
	activate(true)
{
}

void MDIChild::createMDIChild( const Seed& cs ) {
	getParent()->sendMessage(WM_SETREDRAW, FALSE);
	HWND active = (HWND)(cs.activate ? NULL : getParent()->sendMessage(WM_MDIGETACTIVE));
	HWND wnd = ::CreateMDIWindow( getDispatcher().getClassName(),
		cs.caption.c_str(),
		cs.style,
		cs.location.x(), cs.location.y(), cs.location.width(), cs.location.height(),
		getParent()->handle(),
		::GetModuleHandle(NULL),
		reinterpret_cast< LPARAM >( static_cast< Widget * >( this ) ) );

	if (wnd == NULL) {
		throw Win32Exception("CreateMDIWindow failed");
	}

	if(active) {
		getParent()->sendMessage(WM_MDIACTIVATE, (WPARAM)active);
	}

	getParent()->sendMessage(WM_SETREDRAW, TRUE);
	redraw();
}

bool MDIChild::handleMessage(const MSG& msg, LRESULT& retVal) {
	// Prevent some flicker...
    if(msg.message == WM_NCPAINT || msg.message == WM_SIZE)
    {
	    if(getParent()->isActiveMaximized()) {
		    if(msg.message == WM_NCPAINT) // non client area
		    return true;

		    if(msg.message == WM_SIZE) // client area
		    {
			    if((msg.wParam == SIZE_MAXIMIZED || msg.wParam == SIZE_RESTORED) && getParent()->getActive() == this) // active and maximized
			    	return BaseType::handleMessage(msg, retVal);

			    sendMessage(WM_SETREDRAW, FALSE);
			    bool ret = BaseType::handleMessage(msg, retVal);
			    sendMessage(WM_SETREDRAW, TRUE);
			    return ret;
		    }
	    }
    }
    return BaseType::handleMessage(msg, retVal);
}

void MDIChild::activate() {
	Widget* prev = getParent()->getActive();
	if(prev == this)
		return;

	if(::IsIconic(handle())) {
		getParent()->sendMessage(WM_MDIRESTORE, reinterpret_cast<WPARAM>(this->handle()));
	}
	getParent()->sendMessage(WM_MDIACTIVATE, reinterpret_cast<WPARAM>(this->handle()));
}

}
