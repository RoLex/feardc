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

#ifndef DWT_aspects_Scrollable_h
#define DWT_aspects_Scrollable_h

#include "../Message.h"

namespace dwt { namespace aspects {

/// Aspect class used by Widgets that have the possibility of scrolling through a scroll bar.
template<typename WidgetType>
class Scrollable
{
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef std::function<void ()> F;

public:
	/// @ refer to the ::GetScrollInfo doc for information on the params.
	SCROLLINFO getScrollInfo(int fnBar, unsigned mask = SIF_ALL) const;
	bool scrollIsAtEnd() const;

	/** catch horizontal scrolling events.
	@param sbFlag one of the SB_ flags defined in the WM_HSCROLL doc, or -1 as a catch-all. */
	void onScrollHorz(F f, int sbFlag = -1) {
		onScroll(WM_HSCROLL, f, sbFlag);
	}

	/** catch vertical scrolling events.
	@param sbFlag one of the SB_ flags defined in the WM_VSCROLL doc, or -1 as a catch-all. */
	void onScrollVert(F f, int sbFlag = -1) {
		onScroll(WM_VSCROLL, f, sbFlag);
	}

private:
	/// override if the derived widget needs to be adjusted when determining the scroll pos.
	virtual int scrollOffsetImpl() const {
		return 0;
	}

	void onScroll(UINT message, F f, int sbFlag) {
		W().addCallback(Message(message), [f, sbFlag](const MSG& msg, LRESULT&) -> bool {
			if(sbFlag == -1 || LOWORD(msg.wParam) == sbFlag) {
				f();
			}
			return false;
		});
	}
};

template<typename WidgetType>
SCROLLINFO Scrollable<WidgetType>::getScrollInfo(int fnBar, unsigned mask) const {
	SCROLLINFO info = { sizeof(SCROLLINFO), mask };
	::GetScrollInfo(H(), fnBar, &info);
	return info;
}

template<typename WidgetType>
bool Scrollable<WidgetType>::scrollIsAtEnd() const {
	SCROLLINFO scrollInfo = getScrollInfo(SB_VERT, SIF_RANGE | SIF_PAGE | SIF_POS);
	return !scrollInfo.nPage || scrollInfo.nPos >= static_cast<int>(scrollInfo.nMax - scrollInfo.nPage) + scrollOffsetImpl();
}

} }

#endif
