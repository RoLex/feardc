/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#include <dwt/widgets/ToolTip.h>

namespace dwt {

const TCHAR ToolTip::windowClass[] = TOOLTIPS_CLASS;

ToolTip::Seed::Seed() :
	BaseType::Seed(WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, WS_EX_TRANSPARENT)
{
}

void ToolTip::create( const Seed & cs ) {
	dwtassert((cs.style & WS_POPUP) == WS_POPUP, "ToolTips must have the WS_POPUP style");

	BaseType::create(cs);
}

void ToolTip::relayEvent(const MSG& msg) {
	if(msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
		sendMessage(TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM>(&msg));
}

void ToolTip::setText(const tstring& text_) {
	setText(getParent(), text_);
}

void ToolTip::setText(Widget* widget, const tstring& text_) {
	text = text_;
	setTool(widget, [this](tstring& ret) { ret = text; });
}

void ToolTip::addTool(Widget* widget, LPTSTR text) {
	TOOLINFO ti = { sizeof(TOOLINFO), TTF_IDISHWND | TTF_SUBCLASS, getParent()->handle(),
		reinterpret_cast<UINT_PTR>(widget->handle()) };
	ti.lpszText = text;
	sendMessage(TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

void ToolTip::setTool(Widget* widget, F f) {
	onGetTip(f);
	addTool(widget);
}

int ToolTip::getDelay(int param) {
	return sendMessage(TTM_GETDELAYTIME, param);
}

void ToolTip::setDelay(int param, int delay) {
	sendMessage(TTM_SETDELAYTIME, param, (delay >= 0) ? MAKELPARAM(delay, 0) : -1);
}

void ToolTip::setActive(bool b) {
	sendMessage(TTM_ACTIVATE, b ? TRUE : FALSE);
}

void ToolTip::refresh() {
	setActive(false);
	setActive(true);
}

void ToolTip::onGetTip(F f) {
	setCallback(Message(WM_NOTIFY, TTN_GETDISPINFO), [f](const MSG& msg, LRESULT&) -> bool {
		auto& ttdi = *reinterpret_cast<LPNMTTDISPINFO>(msg.lParam);
		auto tip = hwnd_cast<ToolTip*>(ttdi.hdr.hwndFrom);
		if(tip) {
			f(tip->text);
			ttdi.lpszText = const_cast<LPTSTR>(tip->text.c_str());
		}
		return true;
	});
}

}
