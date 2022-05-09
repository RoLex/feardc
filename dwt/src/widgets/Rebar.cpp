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

#include <dwt/widgets/Rebar.h>

namespace dwt {

const TCHAR Rebar::windowClass[] = REBARCLASSNAME;

Rebar::Seed::Seed() :
BaseType::Seed(WS_CHILD | WS_CLIPCHILDREN | CCS_NODIVIDER | RBS_AUTOSIZE | RBS_VARHEIGHT, WS_EX_CONTROLPARENT)
{
}

Rebar::Rebar(Widget* parent) :
BaseType(parent, ChainingDispatcher::superClass<Rebar>())
{
}

void Rebar::create(const Seed& cs) {
	BaseType::create(cs);
}

int Rebar::refresh() {
	// use dummy sizes to avoid flickering; the rebar will figure out the proper sizes by itself.
	::MoveWindow(handle(), 0, 0, 0, 0, TRUE);
	return BaseType::getWindowSize().y;
}

void Rebar::add(Widget* w, unsigned style, const tstring& text) {
	if(size() == 0)
		setVisible(true);

	w->addRemoveStyle(CCS_NORESIZE, true);

	REBARBANDINFO info = { sizeof(REBARBANDINFO), RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE, style };

	if(!text.empty()) {
		info.fMask |= RBBIM_TEXT;
		info.lpText = const_cast<LPTSTR>(text.c_str());
	}

	info.hwndChild = w->handle();

	const Point size = w->getPreferredSize();
	info.cxMinChild = size.x;
	info.cyMinChild = size.y;

	sendMessage(RB_INSERTBAND, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(&info));
}

void Rebar::remove(Widget* w) {
	for(unsigned i = 0, n = size(); i < n; ++i) {
		REBARBANDINFO info = { sizeof(REBARBANDINFO), RBBIM_CHILD };
		if(sendMessage(RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&info)) && info.hwndChild == w->handle()) {
			sendMessage(RB_DELETEBAND, i);
			break;
		}
	}

	if(size() == 0)
		setVisible(false);
}

bool Rebar::empty() const {
	return size() == 0;
}

unsigned Rebar::size() const {
	return sendMessage(RB_GETBANDCOUNT);
}

}
