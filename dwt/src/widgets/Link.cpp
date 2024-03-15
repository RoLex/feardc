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

#include <dwt/widgets/Link.h>

namespace dwt {

const TCHAR Link::windowClass[] = WC_LINK;

Link::Seed::Seed(const tstring& caption, bool link, std::function<bool (const tstring&, bool)> linkManager) :
	BaseType::Seed(WS_CHILD | WS_TABSTOP, 0, link ? _T("<a href=\"") + caption + _T("\">") + caption + _T("</a>") : caption),
	font(0),
	fpLinkManager(linkManager)
{
}

Link::Link(Widget* parent) :
	BaseType(parent, ChainingDispatcher::superClass<Link>())
{
}

void Link::create(const Seed& seed) {
	BaseType::create(seed);
	setFont(seed.font);

	auto openLink = [seed](WPARAM, LPARAM lParam) -> LRESULT {
		if(seed.fpLinkManager) {
			seed.fpLinkManager(reinterpret_cast<NMLINK*>(lParam)->item.szUrl, true);
		} else {
			::ShellExecute(0, 0, reinterpret_cast<NMLINK*>(lParam)->item.szUrl, 0, 0, SW_SHOWNORMAL);
		}
		return 0;
	};
	onRaw(openLink, Message(WM_NOTIFY, NM_CLICK));
	onRaw(openLink, Message(WM_NOTIFY, NM_RETURN));
}

void Link::setLink(const tstring& link, size_t index) {
	LITEM item = { LIF_ITEMINDEX | LIF_URL, static_cast<int>(index) };
	link.copy(item.szUrl, std::min(link.size(), static_cast<size_t>(L_MAX_URL_LENGTH - 1)));
	sendMessage(LM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
}

Point Link::getPreferredSize() {
	SIZE size = { 0 };
	sendMessage(LM_GETIDEALSIZE, getRoot()->getClientSize().x, reinterpret_cast<LPARAM>(&size));
	return Point(size.cx, size.cy);
}

}
