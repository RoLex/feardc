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

#include <dwt/widgets/StatusBar.h>

#include <dwt/WidgetCreator.h>
#include <dwt/widgets/ToolTip.h>

#include <numeric>

namespace dwt {

const TCHAR StatusBar::windowClass[] = STATUSCLASSNAME;

/* the minimum width of the part of the status bar that has to fill all the remaining space of the
bar (see Seed). if the bar is too narrow, only the "fill" part will be shown. */
const unsigned fillMin = 100;

StatusBar::Seed::Seed(unsigned parts_, unsigned fill_, bool sizeGrip) :
BaseType::Seed(WS_CHILD | WS_CLIPSIBLINGS),
font(0),
parts(parts_),
fill(fill_)
{
	assert(fill < parts);

	if(sizeGrip) {
		style |= SBARS_SIZEGRIP;
	}
}

StatusBar::StatusBar(Widget* parent) :
BaseType(parent, ChainingDispatcher::superClass<StatusBar>()),
fill(0),
tip(0),
tipPart(0)
{
}

void StatusBar::create(const Seed& cs) {
	parts.resize(cs.parts);
	fill = cs.fill;

	BaseType::create(cs);
	setFont(cs.font);

	tip = WidgetCreator<ToolTip>::create(this, ToolTip::Seed());
	tip->setTool(this, [this](tstring& text) { handleToolTip(text); });
	onDestroy([this] { tip->close(); tip = nullptr; });
}

void StatusBar::setSize(unsigned part, unsigned size) {
	dwtassert(part < parts.size(), "Invalid part number");
	parts[part].desiredSize = size;
}

void StatusBar::setText(unsigned part, const tstring& text, bool alwaysResize) {
	dwtassert(part < parts.size(), "Invalid part number");
	Part& info = getPart(part);
	info.text = text;
	if(part != fill) {
		info.updateSize(this, alwaysResize);
	} else if(!text.empty()) {
		lastLines.push_back(text);
		while(lastLines.size() > MAX_LINES) {
			lastLines.erase(lastLines.begin());
		}
		tstring& tip = info.tip;
		tip.clear();
		for(size_t i = 0; i < lastLines.size(); ++i) {
			if(i > 0) {
				tip += _T("\r\n");
			}
			tip += lastLines[i];
		}
	}
	sendMessage(SB_SETTEXT, static_cast<WPARAM>(part), reinterpret_cast<LPARAM>(text.c_str()));
}

void StatusBar::setIcon(unsigned part, const IconPtr& icon, bool alwaysResize) {
	dwtassert(part < parts.size(), "Invalid part number");
	Part& info = getPart(part);
	info.icon = icon;
	if(part != fill)
		info.updateSize(this, alwaysResize);
	sendMessage(SB_SETICON, part, icon ? reinterpret_cast<LPARAM>(icon->handle()) : 0);
}

void StatusBar::setToolTip(unsigned part, const tstring& text) {
	dwtassert(part < parts.size(), "Invalid part number");
	getPart(part).tip = text;
}

void StatusBar::setHelpId(unsigned part, unsigned id) {
	dwtassert(part < parts.size(), "Invalid part number");
	getPart(part).helpId = id;
}

void StatusBar::setWidget(unsigned part, Control* widget, const Rectangle& padding) {
	dwtassert(part < parts.size(), "Invalid part number");
	auto p = new WidgetPart(widget, padding);
	p->desiredSize = widget->getPreferredSize().x;
	p->helpId = widget->getHelpId();
	parts.insert(parts.erase(parts.begin() + part), p);
}

void StatusBar::onClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), "Invalid part number");
	getPart(part).clickF = f;

	// imitate the default onClicked but with a setCallback.
	setCallback(Message(WM_NOTIFY, NM_CLICK), Dispatchers::VoidVoid<>([this] { handleClicked(); }));
}

void StatusBar::onRightClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), "Invalid part number");
	getPart(part).rightClickF = f;

	// imitate the default onRightClicked but with a setCallback.
	setCallback(Message(WM_NOTIFY, NM_RCLICK), Dispatchers::VoidVoid<>([this] { handleRightClicked(); }));
}

void StatusBar::onDblClicked(unsigned part, const F& f) {
	dwtassert(part < parts.size(), "Invalid part number");
	getPart(part).dblClickF = f;

	// imitate the default onDblClicked but with a setCallback.
	setCallback(Message(WM_NOTIFY, NM_DBLCLK), Dispatchers::VoidVoid<>([this] { handleDblClicked(); }));
}

int StatusBar::refresh() {
	// The status bar will auto-resize itself - all we need to do is to layout the sections
	sendMessage(WM_SIZE);

	auto sz = BaseType::getWindowSize();
	layoutSections(sz);
	return sz.y;
}

bool StatusBar::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(tip) {
		if(msg.message == WM_MOUSEMOVE) {
			Part* part = getClickedPart();
			if(part && part != tipPart) {
				tip->refresh();
				tipPart = part;
			}
		}
		tip->relayEvent(msg);
	}

	return BaseType::handleMessage(msg, retVal);
}

void StatusBar::Part::updateSize(StatusBar* bar, bool alwaysResize) {
	unsigned newSize = 0;
	if(icon)
		newSize += icon->getSize().x;
	if(!text.empty()) {
		if(icon)
			newSize += 4; // spacing between icon & text
		newSize += bar->getTextSize(text).x;
	}
	if(newSize > 0)
		newSize += 10; // add margins
	if(newSize > desiredSize || (alwaysResize && newSize != desiredSize)) {
		desiredSize = newSize;
		bar->layoutSections();
	}
}

void StatusBar::WidgetPart::layout(POINT* offset) {
	::SetWindowPos(widget->handle(), HWND_TOP,
		offset[0].x + padding.left(), offset[0].y + padding.top(),
		offset[1].x - offset[0].x - padding.width(), offset[1].y - offset[0].y - padding.height(),
		SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

StatusBar::Part& StatusBar::getPart(unsigned part) {
	auto& ret = parts[part];
	if(!dynamic_cast<Part*>(&ret)) {
		auto p = new Part();
		parts.insert(parts.erase(parts.begin() + part), p);
		return *p;
	}
	return static_cast<Part&>(ret);
}

void StatusBar::layoutSections() {
	layoutSections(getWindowSize());
}

void StatusBar::layoutSections(const Point& sz) {
	std::vector<unsigned> sizes(parts.size());
	for(size_t i = 0, n = sizes.size(); i < n; ++i)
		sizes[i] = parts[i].desiredSize;

	sizes[fill] = 0;

	const auto total = std::accumulate(sizes.begin(), sizes.end(), 0);
	if(total + fillMin < static_cast<unsigned>(sz.x)) {
		// cool, there's enough room to fit all the parts.
		for(auto& part: parts) {
			part.actualSize = part.desiredSize;
		}
		parts[fill].actualSize = sizes[fill] = sz.x - total;

		// transform sizes into offsets
		unsigned offset = 0;
		for(auto& size: sizes) {
			offset += size;
			size = offset;
		}

	} else {
		// only show the "fill" part if the status bar is too narrow.
		for(auto& part: parts) { part.actualSize = 0; }
		for(auto& size: sizes) { size = 0; }
		parts[fill].actualSize = sizes[fill] = sz.x;
	}

	sendMessage(SB_SETPARTS, sizes.size(), reinterpret_cast<LPARAM>(sizes.data()));

	// reposition embedded widgets.
	for(auto i = parts.begin(); i != parts.end(); ++i) {
		auto wp = dynamic_cast<WidgetPart*>(&*i);
		if(wp) {
			POINT p[2];
			sendMessage(SB_GETRECT, i - parts.begin(), reinterpret_cast<LPARAM>(p));
			::MapWindowPoints(handle(), getParent()->handle(), p, 2);
			wp->layout(p);
		}
	}
}

StatusBar::Part* StatusBar::getClickedPart() {
	unsigned x = ClientCoordinate(ScreenCoordinate(Point::fromLParam(::GetMessagePos())), this).x();
	unsigned total = 0;
	for(auto& i: parts) {
		total += i.actualSize;
		if(total > x)
			return dynamic_cast<Part*>(&i);
	}

	return 0;
}

void StatusBar::handleToolTip(tstring& text) {
	Part* part = getClickedPart();
	if(part) {
		text = part->tip;
		tip->setMaxTipWidth((text.find('\n') == tstring::npos) ? -1 : part->actualSize);
	} else {
		text.clear();
	}
}

void StatusBar::handleClicked() {
	Part* part = getClickedPart();
	if(part && part->clickF)
		part->clickF();
}

void StatusBar::handleRightClicked() {
	Part* part = getClickedPart();
	if(part && part->rightClickF)
		part->rightClickF();
}

void StatusBar::handleDblClicked() {
	Part* part = getClickedPart();
	if(part && part->dblClickF)
		part->dblClickF();
}

void StatusBar::helpImpl(unsigned& id) {
	// we have the help id of the whole status bar; convert to the one of the specific part the user just clicked on
	Part* part = getClickedPart();
	if(part && part->helpId)
		id = part->helpId;
}

}
