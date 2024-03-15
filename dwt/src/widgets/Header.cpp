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

#include <dwt/widgets/Header.h>

namespace dwt {

const TCHAR Header::windowClass[] = WC_HEADER;

Header::Seed::Seed() :
	/// @todo add HDS_DRAGDROP when the tree has better support for col ordering
	BaseType::Seed(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)
{
}

void Header::create( const Header::Seed & cs ) {
	BaseType::create(cs);
}

Point Header::getPreferredSize() {
	RECT rc = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };
	WINDOWPOS wp = { 0 };
	HDLAYOUT hl = { &rc, &wp };
	if(Header_Layout(handle(), &hl)) {
		return Point(wp.cx, wp.cy);
	}

	return Point(0, 0);
}

int Header::insert(const tstring& header, int width, LPARAM lParam, int after) {
	if(after == -1) after = size();

	HDITEM item = { HDI_FORMAT };
	item.fmt = HDF_LEFT;// TODO
	if(!header.empty()) {
		item.mask |= HDI_TEXT;
		item.pszText = const_cast<LPTSTR>(header.c_str());
	}

	if(width >= 0) {
		item.mask |= HDI_WIDTH;
		item.cxy = width;
	}

	return Header_InsertItem(handle(), after, &item);
}

int Header::findDataImpl(LPARAM data, int start) {
    LVFINDINFO fi = { LVFI_PARAM, NULL, data };
    return ListView_FindItem(handle(), start, &fi);
}

LPARAM Header::getDataImpl(int idx) {
	HDITEM item = { HDI_LPARAM };

	if(!Header_GetItem(handle(), idx, &item)) {
		return 0;
	}
	return item.lParam;
}

void Header::setDataImpl(int idx, LPARAM data) {
	LVITEM item = { HDI_LPARAM };
	item.lParam = data;

	Header_SetItem(handle(), idx, &item);
}

int Header::getWidth(int idx) const {
	HDITEM item = { HDI_WIDTH };

	if(!Header_GetItem(handle(), idx, &item)) {
		return 0;
	}
	return item.cxy;
}

}
