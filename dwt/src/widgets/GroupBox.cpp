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

#include <dwt/widgets/GroupBox.h>
#include <dwt/Point.h>

namespace dwt {

const TCHAR GroupBox::windowClass[] = WC_BUTTON;

GroupBox::Seed::Seed(const tstring& caption) :
BaseType::Seed(BS_GROUPBOX | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT, caption),
font(0),
padding(6, 6)
{
}

void GroupBox::create( const GroupBox::Seed & cs ) {
	BaseType::create(cs);
	setFont(cs.font);

	padding.x = ::GetSystemMetrics(SM_CXEDGE) * 2 + cs.padding.x * 2;
	padding.y = ::GetSystemMetrics(SM_CYEDGE) + cs.padding.y * 2; // ignore the top border

	onEnabled([this](bool b) { handleEnabled(b); });
	onWindowPosChanged([this](const Rectangle &) { layout(); });
}

Point GroupBox::getPreferredSize() {
	Point ret(0, 0);
	auto w = getChild();

	if(w) {
		ret = w->getPreferredSize();
	}

	return expand(ret);
}

void GroupBox::layout() {
	auto child = getChild();
	if(child) {
		auto size = getClientSize();
		auto rc = shrink(Rectangle(size));
		child->resize(rc);
	}

	BaseType::layout();
}

Rectangle GroupBox::shrink(const Rectangle& client) {
	UpdateCanvas c(this);
	auto select(c.select(*getFont()));

	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);
	int h = tmNew.tmHeight;

	if(length() == 0)
		h /= 2;

	return Rectangle(
		client.width() > padding.x / 2 ? client.x() + padding.x / 2 : client.width(),
		client.height() > padding.y / 2 + h ? client.y() + padding.y / 2 + h : client.height(),
		client.width() > padding.x ? client.width() - padding.x : 0,
		client.height() > padding.y + h ? client.height() - padding.y - h : 0
	);
}

Point GroupBox::expand(const Point& child) {
	UpdateCanvas c(this);
	auto select(c.select(*getFont()));

	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);
	int h = tmNew.tmHeight;

	Point txt;
	if(length() > 0) {
		txt = c.getTextExtent(getText());
	} else {
		h /= 2;
	}

	return Point(std::max(child.x, txt.x) + padding.x, child.y + padding.y + h);
}

void GroupBox::handleEnabled(bool enabled) {
	auto child = getChild();
	if(child)
		child->setEnabled(enabled);
}

}
