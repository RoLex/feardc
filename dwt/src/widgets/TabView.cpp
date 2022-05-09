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

#include <dwt/widgets/TabView.h>

#include <algorithm>

#include <vsstyle.h>

#include <dwt/widgets/Container.h>
#include <dwt/widgets/ToolTip.h>
#include <dwt/WidgetCreator.h>
#include <dwt/util/StringUtils.h>
#include <dwt/util/win32/Version.h>
#include <dwt/DWTException.h>
#include <dwt/resources/Brush.h>
#include <dwt/Texts.h>

namespace dwt {

const TCHAR TabView::windowClass[] = WC_TABCONTROL;

TabView::Seed::Seed(unsigned widthConfig_, bool toggleActive_, bool ctrlTab_) :
BaseType::Seed(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
	TCS_FOCUSNEVER | TCS_HOTTRACK | TCS_MULTILINE | TCS_OWNERDRAWFIXED | TCS_RAGGEDRIGHT | TCS_TOOLTIPS),
tabStyle(WinDefault),
font(0),
widthConfig(widthConfig_),
toggleActive(toggleActive_),
ctrlTab(ctrlTab_)
{
}

TabView::TabView(Widget* w) :
BaseType(w, ChainingDispatcher::superClass<TabView>()),
Taskbar(),
tip(0),
toggleActive(false),
font(0),
boldFont(0),
inTab(false),
highlighted(-1),
highlightClose(false),
closeAuthorized(false),
active(-1),
middleClosing(0),
dragging(0)
{
}

/* general drag & drop COM interface. this selects tabs when the mouse hovers their header during
a drag & drop operation. nothing happens when a drop actually occurs however. */

class TabView::Dropper : public IDropTarget {
public:
	Dropper(TabView* const w) : IDropTarget(), w(w), ref(0), dragging(false) { }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out]  _COM_Outptr_*/ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if(!ppvObject) { return E_POINTER; }
		if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget)) {
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return ++ref;
	}

	virtual ULONG STDMETHODCALLTYPE Release( void)
	{
		if(--ref == 0) { delete this; }
		return ref;
	}

	virtual HRESULT STDMETHODCALLTYPE DragEnter(
		/* [unique][in]  __RPC__in_opt*/ IDataObject * /*pDataObj*/,
		/* [in] */ DWORD /*grfKeyState*/,
		/* [in] */ POINTL pt,
		/* [out][in]  __RPC__inout*/ DWORD *pdwEffect)
	{
		setPoint(pt);
		*pdwEffect = dragging ? DROPEFFECT_COPY : DROPEFFECT_NONE;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragOver(
		/* [in] */ DWORD /*grfKeyState*/,
		/* [in] */ POINTL pt,
		/* [out][in]  __RPC__inout*/ DWORD *pdwEffect)
	{
		setPoint(pt);
		*pdwEffect = dragging ? DROPEFFECT_COPY : DROPEFFECT_NONE;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragLeave( void)
	{
		dragging = false;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Drop(
		/* [unique][in]  __RPC__in_opt*/ IDataObject * /*pDataObj*/,
		/* [in] */ DWORD /*grfKeyState*/,
		/* [in] */ POINTL /*pt*/,
		/* [out][in]  __RPC__inout*/ DWORD *pdwEffect)
	{
		*pdwEffect = DROPEFFECT_NONE;
		return S_OK;
	}

private:
	inline void setPoint(const POINTL& pt) {
		auto i = w->hitTest(ScreenCoordinate(Point(pt.x, pt.y)));
		auto tab = w->getTabInfo(i);
		dragging = tab;
		if(dragging && tab->w != w->getActive()) {
			w->setActive(i);
		}
	}

	TabView* const w;
	ULONG ref;
	bool dragging;
};

void TabView::create(const Seed & cs) {
	if(cs.ctrlTab) {
		addAccel(FCONTROL, VK_TAB, [this] { handleCtrlTab(false); });
		addAccel(FCONTROL | FSHIFT, VK_TAB, [this] { handleCtrlTab(true); });
	}

	BaseType::create(cs);
	setFont(cs.font);

	widthConfig = cs.widthConfig;
	toggleActive = cs.toggleActive;

	if(cs.style & TCS_OWNERDRAWFIXED) {
		dwtassert(dynamic_cast<Control*>(getParent()), "Owner-drawn tabs must have a parent derived from dwt::Control");

		if(widthConfig < 100)
			widthConfig = 100;
		TabCtrl_SetMinTabWidth(handle(), widthConfig);

		closeIcon = cs.closeIcon;

		if(cs.tabStyle == Seed::WinBrowser && util::win32::ensureVersion(util::win32::VISTA)) {
			theme.load(std::wstring(L"BrowserTab::") + std::wstring(VSCLASS_TAB), this);
			if(!theme)
				theme.load(VSCLASS_TAB, this, false);
		} else
			theme.load(VSCLASS_TAB, this);

		if(!(cs.style & TCS_BUTTONS)) {
			// we don't want pre-drawn borders to get in the way here, so we fully take over painting.
			onPainting([this](PaintCanvas& pc) { handlePainting(pc); });
		}

		// TCS_HOTTRACK seems to have no effect in owner-drawn tabs, so do the tracking ourselves.
		onMouseMove([this](const MouseEvent& me) { return handleMouseMove(me); });

	} else {
		if(widthConfig <= 3)
			widthConfig = 0;
	}

	icons = new ImageList(Point(16, 16));
	TabCtrl_SetImageList(handle(), icons->handle());

	onSelectionChanged([this] { handleTabSelected(); });
	onLeftMouseDown([this](const MouseEvent& me) { return handleLeftMouseDown(me); });
	onLeftMouseUp([this](const MouseEvent& me) { return handleLeftMouseUp(me); });
	onContextMenu([this](const ScreenCoordinate& sc) { return handleContextMenu(sc); });
	onMiddleMouseDown([this](const MouseEvent& me) { return handleMiddleMouseDown(me); });
	onMiddleMouseUp([this](const MouseEvent& me) { return handleMiddleMouseUp(me); });
	onXMouseUp([this](const MouseEvent& me) { return handleXMouseUp(me); });
	onMouseWheel([this](const MouseEvent&, int delta) { handleMouseWheel(delta); });

	if(cs.style & TCS_TOOLTIPS) {
		tip = WidgetCreator<ToolTip>::attach(this, TabCtrl_GetToolTips(handle())); // created and managed by the tab control thanks to the TCS_TOOLTIPS style
		tip->addRemoveStyle(TTS_NOPREFIX, true);
		tip->onRaw([this](WPARAM, LPARAM lParam) { return handleToolTip(lParam); }, Message(WM_NOTIFY, TTN_GETDISPINFO));
	}

	auto dropper = new Dropper(this);
	if(::RegisterDragDrop(handle(), dropper) == S_OK) {
		onDestroy([this] { ::RevokeDragDrop(handle()); });
	} else {
		delete dropper;
	}
}

void TabView::add(ContainerPtr w, const IconPtr& icon) {
	const size_t pos = size();

	TabInfo* ti = new TabInfo(this, w, icon);
	TCITEM item = { TCIF_PARAM };
	item.lParam = reinterpret_cast<LPARAM>(ti);

	if(!hasStyle(TCS_OWNERDRAWFIXED)) {
		ti->text = formatTitle(w->getText());
		item.mask |= TCIF_TEXT;
		item.pszText = const_cast<LPTSTR>(ti->text.c_str());
	}

	if(icon) {
		item.mask |= TCIF_IMAGE;
		item.iImage = addIcon(icon);
	}

	int newIdx = TabCtrl_InsertItem(handle(), pos, &item);
	if ( newIdx == - 1 ) {
		throw Win32Exception("Error while trying to add page into Tab Sheet");
	}

	if(taskbar) {
		addToTaskbar(w);
		if(icon) {
			setTaskbarIcon(w, icon);
		}
	}

	viewOrder.push_front(w);

	if(viewOrder.size() == 1 || w->hasStyle(WS_VISIBLE)) {
		if(viewOrder.size() > 1) {
			swapWidgets(viewOrder.back(), w);
		} else {
			swapWidgets(0, w);
		}
		setActive(pos);
	}

	layout();

	w->onTextChanging([this, w](const tstring& t) { handleTextChanging(w, t); });
}

ContainerPtr TabView::getActive() const {
	TabInfo* ti = getTabInfo(getSelected());
	return ti ? ti->w : 0;
}

void TabView::remove(ContainerPtr w) {
	auto i = findTab(w);
	if(i == -1) {
		return;
	}

	int cur = getSelected();

	if(viewOrder.size() > 1 && i == cur) {
		next();
	}

	viewOrder.remove(w);

	if(w == middleClosing)
		middleClosing = 0;
	if(w == dragging)
		dragging = 0;

	removeIcon(i);

	delete getTabInfo(i);
	erase(i);

	if(size() == 0) {
		active = -1;
		if(titleChangedFunction)
			titleChangedFunction(tstring());
	} else {
		if(i < cur) {
			active--;
		}
	}

	layout();

	if(taskbar) {
		removeFromTaskbar(w);
	}
}

IconPtr TabView::getIcon(ContainerPtr w) const {
	auto ti = getTabInfo(w);
	return ti ? ti->icon : 0;
}

void TabView::setIcon(ContainerPtr w, const IconPtr& icon) {
	auto i = findTab(w);
	auto ti = getTabInfo(i);
	if(ti) {
		removeIcon(i);
		ti->icon = 0;

		TCITEM item = { TCIF_IMAGE };
		item.iImage = icon ? addIcon(icon) : -1;
		if(TabCtrl_SetItem(handle(), i, &item)) {
			ti->icon = icon;
		}

		if(taskbar) {
			setTaskbarIcon(w, icon);
		}
	}
}

void TabView::onTabContextMenu(ContainerPtr w, const ContextMenuFunction& f) {
	TabInfo* ti = getTabInfo(w);
	if(ti) {
		ti->handleContextMenu = f;
	}
}

const TabView::ChildList TabView::getChildren() const {
	ChildList ret;
	for(size_t i = 0; i < size(); ++i) {
		ret.push_back(getTabInfo(i)->w);
	}
	return ret;
}

void TabView::setActive(ContainerPtr w) {
	setActive(findTab(w));
}

void TabView::setActive(int i) {
	if(i == -1)
		return;

	setSelected(i);
	handleTabSelected();
}

bool TabView::activateLeftTab() {
	if(active > 0) {
		setActive(active - 1);
		return true;
	}
	return false;
}

bool TabView::activateRightTab() {
	if(active < static_cast<int>(size()) - 1) {
		setActive(active + 1);
		return true;
	}
	return false;
}

void TabView::swapWidgets(ContainerPtr oldW, ContainerPtr newW) {
	newW->resize(clientSize);
	newW->setVisible(true);
	if(oldW) {
		oldW->setVisible(false);
	}
	newW->setFocus();
}

void TabView::handleTabSelected() {
	int i = getSelected();
	if(i == active) {
		return;
	}

	TabInfo* old = getTabInfo(active);
	TabInfo* ti = getTabInfo(i);
	active = i;

	if(!ti || ti == old)
		return;

	swapWidgets(old ? old->w : 0, ti->w);

	if(!inTab)
		setTop(ti->w);

	if(ti->marked) {
		ti->marked = false;
		if(hasStyle(TCS_OWNERDRAWFIXED)) {
			redraw(i);
		} else {
			TabCtrl_HighlightItem(handle(), i, 0);
		}
	}

	if(titleChangedFunction)
		titleChangedFunction(ti->w->getText());

	if(taskbar) {
		setActiveOnTaskbar(ti->w);
	}
}

void TabView::mark(ContainerPtr w) {
	int i = findTab(w);
	if(i != -1 && i != getSelected()) {
		bool& marked = getTabInfo(w)->marked;
		if(!marked) {
			marked = true;
			if(hasStyle(TCS_OWNERDRAWFIXED)) {
				redraw(i);
			} else {
				TabCtrl_HighlightItem(handle(), i, 1);
			}
		}
	}
}

int TabView::findTab(ContainerPtr w) const {
	for(size_t i = 0; i < size(); ++i) {
		if(getTabInfo(i)->w == w) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

TabView::TabInfo* TabView::getTabInfo(ContainerPtr w) const {
	return getTabInfo(findTab(w));
}

TabView::TabInfo* TabView::getTabInfo(int i) const {
	if(i != -1) {
		TCITEM item = { TCIF_PARAM };
		if(TabCtrl_GetItem(handle(), i, &item)) {
			return reinterpret_cast<TabInfo*>(item.lParam);
		}
	}
	return 0;
}

void TabView::handleTextChanging(ContainerPtr w, const tstring& newText) {
	int i = findTab(w);
	if(i != -1) {
		if(hasStyle(TCS_OWNERDRAWFIXED)) {
			redraw(i);
		} else {
			setText(i, formatTitle(newText));
			layout();
		}

		if((i == active) && titleChangedFunction)
			titleChangedFunction(newText);
	}
}

tstring TabView::formatTitle(tstring title) {
	if(widthConfig)
		util::cutStr(title, widthConfig);
	return util::escapeMenu(title);
}

void TabView::layout() {
	Rectangle tmp = getUsableArea(true);
	if(!(tmp == clientSize)) {
		BaseType::redraw(Rectangle(Widget::getClientSize()));
		clientSize = tmp;

		// the client area has changed, update the selected tab synchronously.
		ContainerPtr sel = 0;
		TabInfo* ti = getTabInfo(getSelected());
		if(ti) {
			sel = ti->w;
			ti->w->resize(clientSize);
		}

		// update background tabs too, but only asynchronously.
		callAsync([this, sel]() {
			for(auto w: viewOrder) {
				if(w != sel) {
					w->resize(clientSize);
				}
			}
		});
	}
}

void TabView::next(bool reverse) {
	if(viewOrder.size() < 2) {
		return;
	}
	ContainerPtr wnd = getActive();
	if(!wnd) {
		return;
	}

	WindowIter i;
	if(inTab) {
		i = std::find(viewOrder.begin(), viewOrder.end(), wnd);
		if(i == viewOrder.end()) {
			return;
		}

		if(!reverse) {
			if(i == viewOrder.begin()) {
				i = viewOrder.end();
			}
			--i;
		} else {
			if(++i == viewOrder.end()) {
				i = viewOrder.begin();
			}
		}
	} else {
		if(!reverse) {
			i = --(--viewOrder.end());
		} else {
			i = ++viewOrder.begin();
		}
	}

	setActive(*i);
	return;
}

void TabView::setTop(ContainerPtr wnd) {
	viewOrder.remove(wnd);
	viewOrder.push_back(wnd);
}

int TabView::addIcon(const IconPtr& icon) {
	// see if one of the current tabs already has the icon; in that case, reuse it.
	for(size_t i = 0, n = size(); i < n; ++i) {
		auto ti = getTabInfo(i);
		if(ti && ti->icon && *ti->icon == *icon) {
			auto image = getImage(i);
			if(image != -1) {
				return image;
			}
		}
	}

	auto ret = icons->size();
	icons->add(*icon);
	return ret;
}

int TabView::getImage(unsigned index) {
	TCITEM item = { TCIF_IMAGE };
	if(TabCtrl_GetItem(handle(), index, &item)) {
		return item.iImage;
	}
	return -1;
}

void TabView::removeIcon(unsigned index) {
	auto t = getTabInfo(index);
	if(!t || !t->icon)
		return;

	// make sure no other tab is still using this icon.
	for(size_t i = 0, n = size(); i < n; ++i) {
		if(i == index) {
			continue;
		}
		auto ti = getTabInfo(i);
		if(ti && ti->icon && *ti->icon == *t->icon) {
			return;
		}
	}

	auto image = getImage(index);
	if(image != -1) {
		TabCtrl_RemoveImage(handle(), image);
	}
}

LRESULT TabView::handleToolTip(LPARAM lParam) {
	LPNMTTDISPINFO ttdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);
	TabInfo* ti = getTabInfo(ttdi->hdr.idFrom); // here idFrom corresponds to the index of the tab
	if(ti) {
		tipText = highlightClose ? Texts::get(Texts::close) : ti->w->getText();
		ttdi->lpszText = const_cast<LPTSTR>(tipText.c_str());
	}
	return 0;
}

bool TabView::handleLeftMouseDown(const MouseEvent& mouseEvent) {
	TabInfo* ti = getTabInfo(hitTest(mouseEvent.pos));
	if(ti) {
		dragging = ti->w;
		::SetCapture(handle());
		if(hasStyle(TCS_OWNERDRAWFIXED)) {
			int index = findTab(dragging);
			if(index == active) {
				closeAuthorized = inCloseRect(mouseEvent.pos);
				redraw(index);
			}
		}
	}
	return true;
}

bool TabView::handleLeftMouseUp(const MouseEvent& mouseEvent) {
	::ReleaseCapture();

	bool closeAuth = closeAuthorized;
	closeAuthorized = false;

	if(dragging) {
		int dragPos = findTab(dragging);
		dragging = 0;

		if(dragPos == -1)
			return true;

		int dropPos = hitTest(mouseEvent.pos);

		if(dropPos == -1) {
			// not on any of the current tabs; see if the tab should be moved to the end.
			Point pt = ClientCoordinate(mouseEvent.pos, this).getPoint();
			if(pt.x >= 0 && pt.x <= clientSize.right() && pt.y >= 0 && pt.y <= clientSize.top())
				dropPos = size() - 1;
			else
				return true;
		}

		if(dropPos == dragPos) {
			// the tab hasn't moved; handle the click
			if(dropPos == active) {
				if(mouseEvent.isShiftPressed || (closeAuth && inCloseRect(mouseEvent.pos))) {
					TabInfo* ti = getTabInfo(active);
					if(ti)
						ti->w->close();
				} else if(toggleActive)
					next();
			} else
				setActive(dropPos);
			return true;
		}

		// save some information about the tab before we erase it
		TCITEM item = { TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE };
		TCHAR buf[1024] = { 0 };
		item.pszText = buf;
		item.cchTextMax = (sizeof(buf) / sizeof(TCHAR)) - 1;
		TabCtrl_GetItem(handle(), dragPos, &item);

		erase(dragPos);

		TabCtrl_InsertItem(handle(), dropPos, &item);

		active = getSelected();

		layout();

		if(taskbar) {
			moveOnTaskbar(getTabInfo(dropPos)->w, (dropPos < static_cast<int>(size()) - 1) ? getTabInfo(dropPos + 1)->w : 0);
		}
	}

	return true;
}

bool TabView::handleContextMenu(ScreenCoordinate pt) {
	TabInfo* ti = 0;
	if(pt.x() == -1 && pt.y() == -1) {
		int i = getSelected();

		RECT rc;
		if(i == -1 || !TabCtrl_GetItemRect(handle(), i, &rc)) {
			return false;
		}
		pt = ScreenCoordinate(Point(rc.left, rc.top));
		ti = getTabInfo(i);
	} else {
		int i = hitTest(pt);
		if(i == -1) {
			return false;
		}
		ti = getTabInfo(i);
	}

	if(ti->handleContextMenu && ti->handleContextMenu(pt)) {
		return true;
	}

	return false;
}

bool TabView::handleMiddleMouseDown(const MouseEvent& mouseEvent) {
	TabInfo* ti = getTabInfo(hitTest(mouseEvent.pos));
	if(ti) {
		middleClosing = ti->w;
		::SetCapture(handle());
	}
	return true;
}

bool TabView::handleMiddleMouseUp(const MouseEvent& mouseEvent) {
	::ReleaseCapture();
	if(middleClosing) {
		TabInfo* ti = getTabInfo(hitTest(mouseEvent.pos));
		if(ti && ti->w == middleClosing) {
			middleClosing->close();
		}
		middleClosing = 0;
	}
	return true;
}

bool TabView::handleXMouseUp(const MouseEvent& mouseEvent) {
	switch(mouseEvent.ButtonPressed) {
	case MouseEvent::X1: next(); break;
	case MouseEvent::X2: next(true); break;

	// these are only here to satisfy compilers; they will never be called.
	case MouseEvent::OTHER: break;
	case MouseEvent::LEFT: break;
	case MouseEvent::RIGHT: break;
	case MouseEvent::MIDDLE: break;
	}
	return true;
}

void TabView::handleMouseWheel(int delta) {
	if(active == -1) {
		return; // no active tab; ignore.
	}

	// find out where the mouse is. don't trust the MouseEvent data; it contains garbage when switching windows...
	auto pt = ClientCoordinate(ScreenCoordinate(Point::fromLParam(::GetMessagePos())), this).getPoint();
	if(pt.x < 0 || pt.x > clientSize.right() || pt.y < 0 || pt.y > clientSize.top()) {
		return; // outside of the tab control itself; ignore.
	}

	// note: we don't handle small increment aggregates (when delta < 120).

	if(delta > 0) { activateLeftTab(); } // go left when scrolling upwards.
	else if(delta < 0) { activateRightTab(); } // go right when scrolling downwards.
}

bool TabView::handleMouseMove(const MouseEvent& mouseEvent) {
	int i = hitTest(mouseEvent.pos);
	if(highlighted != -1 && i != highlighted) {
		redraw(highlighted);
		highlighted = -1;
		highlightClose = false;
	}
	if(i != -1 && i != highlighted) {
		redraw(i);
		highlighted = i;
		onMouseLeave([this]() { handleMouseLeave(); });
	}
	if(i != -1 && i == active) {
		if(highlightClose ^ inCloseRect(mouseEvent.pos)) {
			highlightClose = !highlightClose;
			if(tip)
				tip->refresh();
			redraw(i);
		}
	}
	return false;
}

void TabView::handleMouseLeave() {
	if(highlighted != -1) {
		redraw(highlighted);
		highlighted = -1;
	}
}

bool TabView::handlePainting(DRAWITEMSTRUCT& info, TabInfo* ti) {
	FreeCanvas canvas { info.hDC };
	auto bkMode(canvas.setBkMode(true));

	Rectangle rect { info.rcItem };
	if(theme) {
		// remove some borders
		rect.pos.x -= 1;
		rect.pos.y -= 1;
		rect.size.x += 2;
		rect.size.y += 1;
	}

	draw(canvas, info.itemID, std::move(rect), (info.itemState & ODS_SELECTED) == ODS_SELECTED);
	return true;
}

void TabView::handlePainting(PaintCanvas& canvas) {
	Rectangle rect { canvas.getPaintRect() };
	if(rect.width() == 0 || rect.height() == 0)
		return;

	auto bkMode(canvas.setBkMode(true));

	int sel = getSelected();
	Rectangle selRect;

	for(size_t i = 0; i < size(); ++i) {
		RECT rc;
		if(TabCtrl_GetItemRect(handle(), i, &rc) &&
			(rc.right >= rect.left() || rc.left <= rect.right()) &&
			(rc.bottom >= rect.top() || rc.top <= rect.bottom()))
		{
			if(static_cast<int>(i) == sel) {
				rc.top -= 2;
				rc.bottom += 1;
				rc.left -= 1;
				rc.right += 1;
				selRect = Rectangle(rc);
			} else {
				draw(canvas, i, Rectangle(rc), false);
			}
		}
	}

	// draw the selected tab last because it might need to step on others
	if(selRect.height() > 0)
		draw(canvas, sel, std::move(selRect), true);
}

void TabView::draw(Canvas& canvas, unsigned index, Rectangle&& rect, bool isSelected) {
	TabInfo* ti = getTabInfo(index);
	if(!ti)
		return;

	bool isHighlighted = static_cast<int>(index) == highlighted || ti->marked;

	int part, state;
	if(theme) {
		part = TABP_TABITEM;
		state = isSelected ? TIS_SELECTED : isHighlighted ? TIS_HOT : TIS_NORMAL;

		theme.drawBackground(canvas, part, state, rect);

	} else {
		canvas.fill(rect, Brush(isSelected ? Brush::Window : isHighlighted ? Brush::HighLight : Brush::BtnFace));
	}

	if(isSelected && theme && !hasStyle(TCS_BUTTONS)) {
		rect.pos.y += 1;
		rect.size.y -= 1;
	}

	const Point margin { 4, 1 };
	rect.pos += margin;
	rect.size -= margin + margin;

	if(ti->icon) {
		Point size = ti->icon->getSize();
		Point pos = rect.pos;
		if(size.y < rect.size.y)
			pos.y += (rect.size.y - size.y) / 2; // center the icon vertically
		canvas.drawIcon(ti->icon, Rectangle(pos, size));

		size.x += margin.x;
		rect.pos.x += size.x;
		rect.size.x -= size.x;
	}

	if(isSelected)
		rect.size.x -= margin.x + 16; // keep some space for the 'X' button

	const tstring text = ti->w->getText();
	const unsigned dtFormat = DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_WORD_ELLIPSIS;
	auto select(canvas.select(*((isSelected || ti->marked) ? boldFont : font)));
	if(theme) {
		theme.drawText(canvas, part, state, text, dtFormat, rect);
	} else {
		canvas.setTextColor(Color::predefined(isSelected ? COLOR_WINDOWTEXT : isHighlighted ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
		canvas.drawText(text, rect, dtFormat);
	}

	if(isSelected) {
		rect.pos.x = rect.right() + margin.x;
		rect.size.x = 16;
		if(16 < rect.size.y)
			rect.pos.y += (rect.size.y - 16) / 2; // center the icon vertically
		rect.size.y = 16;

		if(closeIcon) {
			Rectangle drawRect = rect;
			if(isHighlighted && highlightClose) {
				drawRect.pos.x -= 1;
				drawRect.pos.y -= 1;
				if(closeAuthorized) {
					drawRect.size.x += 1;
					drawRect.size.y += 1;
				}
			}
			canvas.drawIcon(closeIcon, drawRect);

		} else {
			UINT format = DFCS_CAPTIONCLOSE | DFCS_FLAT;
			if(isHighlighted && highlightClose) {
				format |= DFCS_HOT;
				if(closeAuthorized)
					format |= DFCS_PUSHED;
			}
			::RECT rc(rect);
			::DrawFrameControl(canvas.handle(), &rc, DFC_CAPTION, format);
		}

		closeRect = rect;
		closeRect.pos = ScreenCoordinate(ClientCoordinate(closeRect.pos, this)).getPoint();
	}
}

bool TabView::inCloseRect(const ScreenCoordinate& pos) const {
	if(closeRect.width() > 0 && closeRect.height() > 0) {
		return closeRect.contains(pos.getPoint());
	}
	return false;
}

void TabView::setFontImpl() {
	BaseType::setFontImpl();
	font = getFont();
	if(hasStyle(TCS_OWNERDRAWFIXED)) {
		boldFont = font->makeBold();
	}
}

void TabView::helpImpl(unsigned& id) {
	// we have the help id of the whole tab control; convert to the one of the specific tab the user just clicked on
	TabInfo* ti = getTabInfo(hitTest(ScreenCoordinate(Point::fromLParam(::GetMessagePos()))));
	if(ti)
		id = ti->w->getHelpId();
}

void TabView::handleCtrlTab(bool shift) {
	inTab = true;
	next(shift);
}

bool TabView::filter(const MSG& msg) {
	if(tip)
		tip->relayEvent(msg);

	/* handle Ctrl+PageUp, Ctrl+PageDown, Alt+Left, Alt+Right here instead of setting up global
	 * accelerators in order to be able to allow further dispatching if we can't move more to the
	 * left or to the right. this is of importance when imbricating TabView widgets. */
	if(msg.message == WM_KEYDOWN && active != -1) {
		switch(static_cast<int>(msg.wParam)) {
		case VK_PRIOR: if(isControlPressed() && activateLeftTab()) { return true; } break;
		case VK_NEXT: if(isControlPressed() && activateRightTab()) { return true; } break;
		}
	}
	if(msg.message == WM_SYSKEYDOWN && active != -1) {
		switch(static_cast<int>(msg.wParam)) {
		case VK_LEFT: if(isAltPressed() && activateLeftTab()) { return true; } break;
		case VK_RIGHT: if(isAltPressed() && activateRightTab()) { return true; } break;
		}
	}

	if(inTab && msg.message == WM_KEYUP && msg.wParam == VK_CONTROL) {
		inTab = false;

		TabInfo* ti = getTabInfo(getSelected());
		if(ti) {
			setTop(ti->w);
		}
	}

	/* WM_MOUSEWHEEL have a special dispatching mechanism; they start from the window that has
	 * focus and the caller then handles their forwarding. we catch them all here (assuming the
	 * main application does call this "filter" method, which it should) and handle them as regular
	 * messages. */
	if(msg.message == WM_MOUSEWHEEL) {
		LRESULT dispachResult = 0;
		BaseType::handleMessage(msg, dispachResult);
	}

	return false;
}

void TabView::setText(unsigned index, const tstring& text) {
	TabInfo* ti = getTabInfo(index);
	if(ti) {
		ti->text = text;
		TCITEM item = { TCIF_TEXT };
		item.pszText = const_cast<LPTSTR>(ti->text.c_str());
		TabCtrl_SetItem(handle(), index, &item);
	}
}

void TabView::redraw(unsigned index) {
	RECT rect;
	if(TabCtrl_GetItemRect(handle(), index, &rect)) {
		BaseType::redraw(Rectangle(rect));
	}
}

dwt::Rectangle TabView::getUsableArea(bool cutBorders) const
{
	RECT rc;
	::GetClientRect(handle(), &rc);
	TabCtrl_AdjustRect( this->handle(), false, &rc );
	Rectangle rect { rc };
	if(cutBorders) {
		Rectangle rctabs { Widget::getClientSize() };
		// Get rid of ugly border...assume y border is the same as x border
		const long border = (rctabs.width() - rect.width()) / 2;
		const long upStretching = hasStyle(TCS_BUTTONS) ? 4 : 2;
		rect.pos.x = rctabs.x();
		rect.pos.y -= upStretching;
		rect.size.x = rctabs.width();
		rect.size.y += border + upStretching;
	}
	return rect;
}

int TabView::hitTest(const ScreenCoordinate& pt) {
	TCHITTESTINFO tci = { ClientCoordinate(pt, this).getPoint() };
	return TabCtrl_HitTest(handle(), &tci);
}

bool TabView::handleMessage( const MSG & msg, LRESULT & retVal ) {
	if(msg.message == WM_PAINT && !theme) {
		// let the tab control draw the borders of unthemed tabs (and revert to classic owner-draw callbacks).
		return false;
	}

	bool handled = BaseType::handleMessage(msg, retVal);

	if(msg.message == WM_SIZE) {
		// We need to let the tab control window proc handle this first, otherwise getUsableArea will not return
		// correct values on mulitrow tabs (since the number of rows might change with the size)
		retVal = getDispatcher().chain(msg);

		layout();

		return true;
	}

	if(!handled && msg.message == WM_COMMAND && getActive()) {
		// Forward commands to the active tab
		handled = getActive()->handleMessage(msg, retVal);
	}

	return handled;
}

}
