/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

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

#include <dwt/widgets/Control.h>

#include <dwt/DWTException.h>
#include <dwt/util/check.h>

#include <dwt/widgets/Menu.h>
#include <dwt/widgets/TabView.h>

namespace dwt {

Control::Seed::Seed(DWORD style, DWORD exStyle, const tstring& caption) :
BaseType::Seed(style | WS_VISIBLE, exStyle, caption)
{
}

Control::Control(Widget* parent, Dispatcher& dispatcher) :
BaseType(parent, dispatcher),
accel(0)
{
}

void Control::addAccel(BYTE fVirt, WORD key, const CommandDispatcher::F& f) {
	const size_t id = id_offset + accels.size();
	ACCEL a = { static_cast<BYTE>(fVirt | FVIRTKEY), key, static_cast<WORD>(id) };
	accels.push_back(a);
	onCommand([this, f] { callAsync(f); }, id);
}

void Control::initAccels() {
	dwtassert(!accel, "Control::initAccels called twice on the same control");
	accel = ::CreateAcceleratorTable(&accels[0], accels.size());
	if(!accel) {
		throw Win32Exception("Control::initAccels: CreateAcceleratorTable failed");
	}
}

void Control::create(const Seed& seed) {
	BaseType::create(seed);

	if(!accels.empty()) {
		initAccels();
	}
}

Control::~Control() {
	if(accel) {
		::DestroyAcceleratorTable(accel);
	}
}

bool Control::filter(MSG& msg) {
	return accel && ::TranslateAccelerator(handle(), accel, &msg);
}

namespace { template<typename T> bool forwardPainting(const MSG& msg) {
	T* t = reinterpret_cast<T*>(msg.lParam);
	if(!t)
		return false;

	switch(t->CtlType) {
	case ODT_MENU: if(msg.wParam == 0) return Menu::handlePainting(*t); break;
	case ODT_TAB: return TabView::handlePainting(*t); break;
	}
	return false;
} }

bool Control::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(msg.message == WM_CLOSE && !getRoot()->getEnabled()) {
		// disallow closing disabled windows.
		return true;
	}

	bool handled = BaseType::handleMessage(msg, retVal);
	if(handled)
		return handled;

	switch(msg.message)
	{
		/* messages that allow Windows controls to owner-draw are sent as notifications to the
		parent; therefore, we catch them here assuming that the parent of an owner-drawn control
		will have this class as a base. they are then forwarded to the relevant control for further
		processing. */
	case WM_DRAWITEM:
		{
			if(forwardPainting<DRAWITEMSTRUCT>(msg)) {
				retVal = TRUE;
				return true;
			}
			break;
		}
	case WM_MEASUREITEM:
		{
			if(forwardPainting<MEASUREITEMSTRUCT>(msg)) {
				retVal = TRUE;
				return true;
			}
			break;
		}

		/* menus have their own message loop. to peek into it, one can either install a hook or
		wait for WM_ENTERIDLE messages; we choose the latter. this allows async callbakcs to keep
		running and updating the application while a menu is up. */
	case WM_ENTERIDLE:
		{
			if(msg.wParam == MSGF_MENU) {
				Application::instance().dispatch();
			}
			break;
		}
	}

	return handled;
}

}
