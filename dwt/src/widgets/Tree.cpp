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

#include <dwt/widgets/Tree.h>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/for_each.hpp>

#include <dwt/widgets/Header.h>
#include <dwt/WidgetCreator.h>

namespace dwt {

using boost::range::for_each;
using boost::adaptors::map_values;

const TCHAR Tree::TreeView::windowClass[] = WC_TREEVIEW;

Tree::TreeView::TreeView(Widget* parent) : Control(parent, ChainingDispatcher::superClass<TreeView>()) { }

bool Tree::TreeView::handleMessage(const MSG& msg, LRESULT &retVal) {
	// give the parent control a chance to handle messages as well.
	return BaseType::handleMessage(msg, retVal) || getParent()->handleMessage(msg, retVal);
}

Tree::Seed::Seed() :
	BaseType::Seed(WS_CHILD | WS_TABSTOP | TVS_DISABLEDRAGDROP | TVS_HASLINES | TVS_NONEVENHEIGHT | TVS_SHOWSELALWAYS),
	font(0)
{
}

void Tree::create( const Seed & cs )
{
	Control::Seed mySeed(WS_CHILD, WS_EX_CONTROLPARENT);

	BaseType::create(mySeed);
	tree = WidgetCreator<TreeView>::create(this, cs);

	onSized([this](const SizedEvent& e) { layout(); });

	/* forward these messages to the tree control handle. do it with Dispatcher::chain to avoid an
	infinite loop. */
	auto forwardMsg = [this](const Message& message) {
		addCallback(message, [this](const MSG& msg, LRESULT& ret) -> bool {
			MSG treeMsg = msg;
			treeMsg.hwnd = treeHandle();
			ret = tree->getDispatcher().chain(treeMsg);
			return true;
		});
	};
	forwardMsg(Message(WM_SETFONT));
	forwardMsg(Message(WM_SETREDRAW));

	tree->onCustomDraw([this](NMTVCUSTOMDRAW& x) { return draw(x); });

	setFont(cs.font);
	layout();
}

HTREEITEM Tree::insert(const tstring& text, HTREEITEM parent, HTREEITEM insertAfter,
					   LPARAM param, bool expanded, int iconIndex, int selectedIconIndex)
{
	TVINSERTSTRUCT item = { parent, insertAfter, { { TVIF_TEXT } } };
	auto& t = item.itemex;
	t.pszText = const_cast<TCHAR*>(text.c_str());
	if(param) {
		t.mask |= TVIF_PARAM;
		t.lParam = param;
	}
	if(expanded) {
		t.mask |= TVIF_STATE;
		t.state = TVIS_EXPANDED;
		t.stateMask = TVIS_EXPANDED;
	}
	if(itsNormalImageList) {
		t.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		t.iImage = (iconIndex == -1) ? I_IMAGECALLBACK : iconIndex;
		t.iSelectedImage = (selectedIconIndex == - 1) ? t.iImage : selectedIconIndex;
	}
	return insert(item);
}

HTREEITEM Tree::insert(TVINSERTSTRUCT& tvis) {
	return TreeView_InsertItem(treeHandle(), &tvis);
}

tstring Tree::getSelectedText() {
	return getText(TreeView_GetSelection(treeHandle()));
}

tstring Tree::getText( HTREEITEM node, int column)
{
	if(node == NULL) {
		return tstring();
	}

	if(column == 0) {
		TVITEMEX item = { TVIF_HANDLE | TVIF_TEXT, node };
		TCHAR buffer[1024];
		buffer[0] = '\0';
		item.cchTextMax = 1022;
		item.pszText = buffer;
		if ( TreeView_GetItem(treeHandle(), &item) ) {
			return buffer;
		}
	} else {
		auto i = texts.find(node);
		if(i != texts.end() && i->second.size() > static_cast<size_t>(column)) {
			return i->second[column];
		}
	}

	return tstring();
}

void Tree::setText(HTREEITEM node, int column, const tstring& text) {
	if(column == 0) {
		TVITEMEX item = { TVIF_HANDLE | TVIF_TEXT, node };
		item.pszText = const_cast<LPTSTR>(text.c_str());
		TreeView_SetItem(treeHandle(), &item);
	} else if(static_cast<size_t>(column) < getColumnCount()) {
		auto &v = texts[node];
		if(v.size() <= static_cast<size_t>(column)) v.resize(column + 1);
		v[column] = text;
	}
}

void Tree::eraseChildren( HTREEITEM node )
{
	HTREEITEM next_node, current_node;

	if ( (current_node = getNext( node, TVGN_CHILD ) ) == NULL )
		return;

	while ( (next_node = getNext( current_node, TVGN_NEXT )) != NULL )
	{
		erase( current_node );
		current_node = next_node;
	}

	erase( current_node );
}

void Tree::setNormalImageList( ImageListPtr imageList ) {
	  itsNormalImageList = imageList;
	  TreeView_SetImageList(treeHandle(), imageList->getImageList(), TVSIL_NORMAL);
}

void Tree::setStateImageList( ImageListPtr imageList ) {
	  itsStateImageList = imageList;
	  TreeView_SetImageList(treeHandle(), imageList->getImageList(), TVSIL_STATE);
}

LPARAM Tree::getDataImpl(HTREEITEM item) {
	TVITEM tvitem = { TVIF_PARAM | TVIF_HANDLE, item };
	if(!TreeView_GetItem(treeHandle(), &tvitem)) {
		return 0;
	}
	return tvitem.lParam;
}

void Tree::setDataImpl(HTREEITEM item, LPARAM lParam) {
	TVITEM tvitem = { TVIF_PARAM | TVIF_HANDLE, item };
	tvitem.lParam = lParam;
	TreeView_SetItem(treeHandle(), &tvitem);
}

int Tree::getItemHeight() {
	return TreeView_GetItemHeight(treeHandle());
}

void Tree::setItemHeight(int h) {
	TreeView_SetItemHeight(treeHandle(), h);
}

ScreenCoordinate Tree::getContextMenuPos() {
	HTREEITEM item = getSelected();
	POINT pt = { 0 };
	if(item) {
		RECT trc = getItemRect(item);
		pt.x = trc.left;
		pt.y = trc.top + ((trc.bottom - trc.top) / 2);
	}
	return ClientCoordinate(pt, this);
}

void Tree::select(const ScreenCoordinate& pt) {
	HTREEITEM ht = hitTest(pt);
	if(ht != NULL && ht != getSelected()) {
		setSelected(ht);
	}
}

Rectangle Tree::getItemRect(HTREEITEM item) {
	RECT rc;
	TreeView_GetItemRect(treeHandle(), item, &rc, TRUE);
	return Rectangle(rc);
}

HeaderPtr Tree::getHeader() {
	if(!header) {
		header = WidgetCreator<Header>::create(this);
		header->setFont(getFont());

		// todo if col 0 was dragged, reset texts...
		header->onRaw([this](WPARAM, LPARAM) -> LRESULT { tree->redraw(); return 0; }, Message(WM_NOTIFY, HDN_ENDDRAG));
		header->onRaw([this](WPARAM, LPARAM) -> LRESULT { tree->redraw(); return 0; }, Message(WM_NOTIFY, HDN_ITEMCHANGED));

		layout();
	}

	return header;
}

void Tree::layout() {
	auto client = getClientSize();
	if(header) {
		auto hsize = header->getPreferredSize();
		header->resize(Rectangle(0, 0, client.x, hsize.y));
		tree->resize(Rectangle(0, hsize.y, client.x, client.y - hsize.y));
	} else {
		tree->resize(Rectangle(client));
	}
}

int Tree::insertColumnImpl(const Column& column, int after) {
	auto h = getHeader();

	return h->insert(column.header, column.width, 0, after);
}

void Tree::eraseColumnImpl(unsigned column) {
	if(!header) {
		return;
	}

	header->erase(column);
	if(getColumnCount() == 0) {
		header->close(false);
		header = 0;

		layout();
	}

	if(column == 0 && getColumnCount() >= 1) {
		for_each(texts, [&](const std::pair<HTREEITEM, std::vector<tstring>> & item) {
			setText(item.first, 0, item.second.empty() ? tstring() : item.second[0]);
		});

	}

	for_each(texts | map_values, [&](std::vector<tstring> & v) {
		if(column < v.size()) {
			v.erase(v.begin() + column);
		}
	});
}

unsigned Tree::getColumnCountImpl() const {
	return header ? header->size() : 0;
}

Column Tree::getColumnImpl(unsigned column) const {
	if(!header) {
		return Column();
	}

	HDITEM item = { HDI_FORMAT | HDI_TEXT | HDI_WIDTH };
	TCHAR buf[1024] = { 0 };
	item.pszText = buf;
	item.cchTextMax = 1023;
	Header_GetItem(header->handle(), column, &item);

	return Column(item.pszText, item.cxy); // TODO fmt
}

std::vector<Column> Tree::getColumnsImpl() const {
	std::vector<Column> ret;
	if(!header) {
		return ret;
	}

	ret.resize(getColumnCount());
	for(size_t i = 0; i < ret.size(); ++i) {
		ret[i] = getColumn(i);
	}
	return ret;
}

std::vector<int> Tree::getColumnOrderImpl() const {
	std::vector<int> ret;
	if(!header) {
		return ret;
	}
	
	ret.resize(getColumnCount());
	if(!Header_GetOrderArray(header->handle(), ret.size(), ret.data())) {
		ret.clear();
	}
	return ret;
}

void Tree::setColumnOrderImpl(const std::vector<int>& columns) {
	// TODO
}

std::vector<int> Tree::getColumnWidthsImpl() const {
	std::vector<int> ret;
	if(!header) {
		return ret;
	}
	
	ret.resize(getColumnCount());
	for(size_t i = 0; i < ret.size(); ++i) {
		ret[i] = header->getWidth(i);
	}
	return ret;
}

void Tree::setColumnWidthImpl(unsigned column, int width) {
	// TODO
}

LRESULT Tree::prePaint(NMTVCUSTOMDRAW& nmcd) {
	return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
}

LRESULT Tree::prePaintItem(NMTVCUSTOMDRAW& nmcd) {
	// Clip the default item (column 0) drawing to the column width
	auto clipRect = nmcd.nmcd.rc;
	auto w = header->getWidth(Header_OrderToIndex(header->handle(), 0));
	clipRect.right = clipRect.left + w;

	Region region { clipRect };
	POINT pt = { 0 };

	FreeCanvas canvas { nmcd.nmcd.hdc };
	::GetWindowOrgEx(canvas.handle(), &pt);
	::OffsetRgn(region.handle(), -pt.x, -pt.y);
	::SelectClipRgn(canvas.handle(), region.handle());

	::SaveDC(canvas.handle());
	return CDRF_DODEFAULT | CDRF_NOTIFYPOSTPAINT;
}

LRESULT Tree::postPaintItem(NMTVCUSTOMDRAW& nmcd) {
	FreeCanvas canvas { nmcd.nmcd.hdc };

	::RestoreDC(canvas.handle(), -1);

	// Remove previously set clip region
	::SelectClipRgn(canvas.handle(), nullptr);

	auto item = (HTREEITEM)nmcd.nmcd.dwItemSpec;

	if (item == NULL) return CDRF_DODEFAULT;

	auto clientSize = tree->getClientSize();

	int x = header->getWidth(Header_OrderToIndex(header->handle(), 0));
	auto columns = getColumnCount();

	canvas.setTextColor(nmcd.clrText);
	canvas.setBkColor(nmcd.clrTextBk);
	auto selectFont(canvas.select(*getFont()));

	for(size_t i = 1; i < columns; ++i) {
		auto index = Header_OrderToIndex(header->handle(), i);
		auto width = header->getWidth(index);

		if(width > 0) {
			Rectangle rect { x, nmcd.nmcd.rc.top, width, nmcd.nmcd.rc.bottom - nmcd.nmcd.rc.top };

			auto text = getText(item, index);
			if(!text.empty()) {
				canvas.drawText(text, rect, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
			}
		}

		x += width;
		if (x > clientSize.x) break;
	}

	return CDRF_DODEFAULT;
}

LRESULT Tree::postPaint(NMTVCUSTOMDRAW& nmcd) {
	return CDRF_DODEFAULT;
}

LRESULT Tree::draw(NMTVCUSTOMDRAW& nmcd) {
	if(!header || nmcd.nmcd.rc.left >= nmcd.nmcd.rc.right || nmcd.nmcd.rc.top >= nmcd.nmcd.rc.bottom) {
		return CDRF_DODEFAULT;
	}

	switch(nmcd.nmcd.dwDrawStage) {
	case CDDS_PREPAINT: return prePaint(nmcd);
	case CDDS_ITEMPREPAINT: return prePaintItem(nmcd);
	case CDDS_ITEMPOSTPAINT: return postPaintItem(nmcd);
	case CDDS_POSTPAINT: return postPaint(nmcd);
	}

	return CDRF_DODEFAULT;
}

}
