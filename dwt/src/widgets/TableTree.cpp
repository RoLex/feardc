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

#include <dwt/widgets/TableTree.h>

#include <vsstyle.h>

#include <dwt/CanvasClasses.h>
#include <dwt/resources/ImageList.h>
#include <dwt/resources/Pen.h>
#include <dwt/util/check.h>
#include <dwt/util/HoldRedraw.h>

namespace dwt {

TableTree::Seed::Seed(const BaseType::Seed& seed) :
	BaseType::Seed(seed)
{
}

TableTree::TableTree(Widget* parent) :
	BaseType(parent),
	indent(0)
{
}

void TableTree::create(const Seed& seed) {
	dwtassert((seed.style & LVS_REPORT) == LVS_REPORT, "TableTree requires LVS_REPORT");

	BaseType::create(seed);

	theme.load(VSCLASS_TREEVIEW, this);

	onCustomDraw([this](NMLVCUSTOMDRAW& data) { return handleCustomDraw(data); });
	onKeyDown([this](int c) { return handleKeyDown(c); });
	onLeftMouseDown([this](const MouseEvent& me) { return handleLeftMouseDown(me); });
}

bool TableTree::handleMessage(const MSG& msg, LRESULT& retVal) {
	switch(msg.message) {
	case LVM_DELETEITEM:
		{
			handleDelete(msg.wParam);
			break;
		}
	case LVM_DELETEALLITEMS:
		{
			items.clear();
			children.clear();
			break;
		}
	case LVM_INSERTITEM:
		{
			handleInsert(*reinterpret_cast<LVITEM*>(msg.lParam));
			break;
		}
	case LVM_SETIMAGELIST:
		{
			if(msg.wParam == LVSIL_SMALL && msg.lParam) {
				indent = ImageList(reinterpret_cast<HIMAGELIST>(msg.lParam), false).getImageSize().x;
			}
			break;
		}
	case LVM_SETITEM:
		{
			dwtDebugFail("TableTree LVM_SETITEM not implemented");
			break;
		}
	}
	return BaseType::handleMessage(msg, retVal);
}

void TableTree::insertChild(LPARAM parent, LPARAM child) {
	items[parent].children.push_back(child);
	children[child] = parent;

	if(items[parent].expanded) {
		LVITEM item = { LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT, findData(parent) + 1 };
		item.pszText = LPSTR_TEXTCALLBACK;
		item.iImage = I_IMAGECALLBACK;
		item.lParam = child;
		item.iIndent = 2;
		sendMsg(LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
	}
}

void TableTree::eraseChild(LPARAM child) {
	auto i = children.find(child);
	if(i != children.end()) {
		eraseChild(i, false);
	}
}

void TableTree::collapse(LPARAM parent) {
	util::HoldRedraw hold { this };

	auto pos = findData(parent);
	auto n = items[parent].children.size();

	// assume children are all at the right pos.
	for(size_t i = 0; i < n; ++i) {
		sendMsg(LVM_DELETEITEM, pos + 1, 0);
	}

	items[parent].expanded = false;
	items[parent].redrawGlyph(*this);

	// special case, see TableTreeTest
	if(n == 1 && pos == static_cast<int>(size()) - 1) {
		Control::redraw(false);
	}
}

void TableTree::expand(LPARAM parent) {
	util::HoldRedraw hold { this };

	LVITEM item = { LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT, findData(parent) };
	item.pszText = LPSTR_TEXTCALLBACK;
	item.iImage = I_IMAGECALLBACK;
	item.iIndent = 2;
	for(auto child: items[parent].children) {
		++item.iItem;
		item.lParam = child;
		sendMsg(LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
	}

	resort();

	items[parent].expanded = true;
	items[parent].redrawGlyph(*this);
}

TableTree::Item::Item() : expanded(false)
{
}

void TableTree::Item::redrawGlyph(TableTree& w) {
	::RECT rect = glyphRect;
	::InvalidateRect(w.handle(), &rect, FALSE);
}

LRESULT TableTree::handleCustomDraw(NMLVCUSTOMDRAW& data) {
	if(data.nmcd.dwDrawStage == CDDS_PREPAINT) {
		return CDRF_NOTIFYITEMDRAW;
	}

	if(data.nmcd.dwDrawStage == CDDS_ITEMPREPAINT && data.dwItemType == LVCDI_ITEM) {
		return CDRF_NOTIFYSUBITEMDRAW;
	}

	if(data.nmcd.dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM) && data.dwItemType == LVCDI_ITEM && data.iSubItem == 0) {
		FreeCanvas canvas { data.nmcd.hdc };

		auto rect = getRect(data.nmcd.dwItemSpec, 0, LVIR_BOUNDS);

		{
			// draw tree lines.
			LOGBRUSH lb { BS_SOLID, Color::predefined(COLOR_GRAYTEXT) };
			Pen pen { ::ExtCreatePen(PS_COSMETIC | PS_ALTERNATE, 1, &lb, 0, nullptr) };
			auto selectPen(canvas.select(pen));

			Point mid { rect.left() + indent / 2, rect.top() + indent / 2 };

			auto lastChild = false;
			if(children.find(data.nmcd.lItemlParam) != children.end()) {
				// this is a child item; draw a second vertical line to link surrounding parents.
				canvas.line(mid.x, rect.top(), mid.x, rect.bottom()); // vertical
				rect.pos.x += indent;
				mid.x += indent;
				lastChild = children.find(getData(data.nmcd.dwItemSpec + 1)) == children.end();
			}

			canvas.line(mid, Point(rect.left() + indent, mid.y)); // horizontal
			canvas.line(mid.x, rect.top(), mid.x, lastChild ? mid.y : rect.bottom()); // vertical
		}

		auto parent = items.find(data.nmcd.lItemlParam);
		if(parent != items.end() && !parent->second.children.empty()) {
			// this is a parent item; draw the +/- glyph.

			if(theme) {
				int part = TVP_GLYPH, state = parent->second.expanded ? GLPS_OPENED : GLPS_CLOSED;
				theme.getPartSize(canvas, part, state, rect.size);
				rect.pos.x += std::max(indent - rect.size.x, 0L) / 2;
				rect.pos.y += std::max(indent - rect.size.y, 0L) / 2;
				theme.drawBackground(canvas, part, state, rect);
				parent->second.glyphRect = rect;

			} else {
				const long glyphSize = 9, padding = 2;
				rect.pos.x += std::max(indent - glyphSize, 0L) / 2;
				rect.pos.y += std::max(indent - glyphSize, 0L) / 2;
				rect.size.x = rect.size.y = glyphSize;

				::RECT rc = rect;
				::DrawEdge(canvas.handle(), &rc, EDGE_BUMP, BF_RECT | BF_MIDDLE | BF_FLAT);

				Point mid { rect.left() + glyphSize / 2, rect.top() + glyphSize / 2 };

				Pen pen { Color::predefined(COLOR_GRAYTEXT), Pen::Solid };
				auto selectPen(canvas.select(pen));

				canvas.line(rect.left() + padding, mid.y, rect.right() - padding, mid.y); // horizontal
				if(!parent->second.expanded) {
					canvas.line(mid.x, rect.top() + padding, mid.x, rect.bottom() - padding); // vertical
				}

				parent->second.glyphRect = rect;
			}
		}
	}

	return CDRF_DODEFAULT;
}

bool TableTree::handleKeyDown(int c) {
	bool handled = false;
	switch(c) {
	case VK_LEFT:
		{
			for(auto sel: getSelection()) {
				auto item = items.find(getData(sel));
				if(item != items.end()) {
					// a parent is selected; collapse it.
					handled = true;
					if(item->second.expanded) {
						collapse(item->first);
						continue;
					}
				}
				auto child = children.find(getData(sel));
				if(child != children.end()) {
					// a child is selected; select its parent.
					handled = true;
					ListView_SetItemState(handle(), sel, 0, LVIS_SELECTED | LVIS_FOCUSED);
					setSelected(findData(child->second));
				}
			}
			break;
		}
	case VK_RIGHT:
		{
			for(auto sel: getSelection()) {
				auto item = items.find(getData(sel));
				if(item != items.end()) {
					// a parent is selected; either expand it or select its first child.
					handled = true;
					if(item->second.expanded) {
						if(!item->second.children.empty()) {
							ListView_SetItemState(handle(), sel, 0, LVIS_SELECTED | LVIS_FOCUSED);
							setSelected(sel + 1);
						}
					} else {
						expand(item->first);
					}
				}
			}
			break;
		}
	}
	return handled;
}

bool TableTree::handleLeftMouseDown(const MouseEvent& me) {
	auto hit = hitTest(me.pos);
	if(hit.second == 0) { // first column
		auto it = items.find(getData(hit.first));
		if(it != items.end() && it->second.glyphRect.contains(ClientCoordinate(me.pos, this).getPoint())) {
			it->second.expanded ? collapse(it->first) : expand(it->first);
			return true;
		}
	}
	return false;
}

void TableTree::handleDelete(int pos) {
	auto param = getData(pos);

	auto parent = items.find(param);
	if(parent != items.end()) {
		if(parent->second.expanded) {
			collapse(param);
		}
		for(auto child: parent->second.children) {
			children.erase(child);
		}
		items.erase(parent);
	}

	auto child = children.find(param);
	if(child != children.end()) {
		eraseChild(child, true);
	}
}

void TableTree::handleInsert(LVITEM& lv) {
	if((lv.mask & LVIF_TEXT) == LVIF_TEXT && lv.pszText != LPSTR_TEXTCALLBACK) {
		dwtDebugFail("TableTree non-callback texts not implemented");
	}
	if((lv.mask & LVIF_IMAGE) == LVIF_IMAGE && lv.iImage != I_IMAGECALLBACK) {
		dwtDebugFail("TableTree non-callback images not implemented");
	}
	if((lv.mask & LVIF_PARAM) != LVIF_PARAM || !lv.lParam) {
		dwtDebugFail("TableTree null LPARAM not implemented");
	}

	// add indentation to draw tree lines.
	if((lv.mask & LVIF_INDENT) != LVIF_INDENT) {
		lv.mask |= LVIF_INDENT;
	}
	lv.iIndent = 1;
}

int TableTree::handleSort(LPARAM& lhs, LPARAM& rhs) {
	/* return 1 or -1 when the 2 items have a direct parency relationship; otherwise, return 0 and
	let the host proceed with comparing. */

	auto c1 = children.find(lhs), c2 = children.find(rhs);

	if(c1 != children.end() && c2 != children.end() && c1->second == c2->second) {
		return 0; // rhs & lhs have the same parent
	}

	if(c1 != children.end()) {
		if(c1->second == rhs) { return isAscending() ? 1 : -1; } // rhs is lhs' parent
		lhs = c1->second; // lhs now points to its parent
	}

	if(c2 != children.end()) {
		if(c2->second == lhs) { return isAscending() ? -1 : 1; } // lhs is rhs' parent
		rhs = c2->second; // rhs now points to its parent
	}

	return 0;
}

void TableTree::eraseChild(decltype(children)::iterator& child, bool deleting) {
	auto& item = items[child->second];
	auto& cont = item.children;
	if(!deleting && item.expanded) {
		sendMsg(LVM_DELETEITEM, findData(child->first), 0);
	}
	cont.erase(std::remove(cont.begin(), cont.end(), child->first), cont.end());
	if(cont.empty()) {
		item.expanded = false;
		item.glyphRect = Rectangle();
		item.redrawGlyph(*this);
	}
	children.erase(child);
}

LRESULT TableTree::sendMsg(UINT msg, WPARAM wParam, LPARAM lParam) {
	// send with a direct dispatcher call to avoid loops (since we catch messages in handleMessage).
	MSG directMsg { handle(), msg, wParam, lParam };
	return getDispatcher().chain(directMsg);
}

}
