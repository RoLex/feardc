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

#include <dwt/widgets/VirtualTree.h>

#include <dwt/util/check.h>

namespace dwt {

VirtualTree::Seed::Seed(const BaseType::Seed& seed) :
	BaseType::Seed(seed)
{
}

VirtualTree::VirtualTree(Widget* parent) :
	BaseType(parent),
	selected(nullptr)
{
	addRoot();
}

bool VirtualTree::handleMessage(const MSG& msg, LRESULT& retVal) {
	switch(msg.message) {
	case TVM_DELETEITEM:
		{
			retVal = handleDelete(msg.lParam);
			return true;
		}
	case TVM_ENSUREVISIBLE:
		{
			retVal = handleEnsureVisible(reinterpret_cast<Item*>(msg.lParam));
			return true;
		}
	case TVM_EXPAND:
		{
			retVal = handleExpand(msg.wParam, reinterpret_cast<Item*>(msg.lParam));
			return true;
		}
	case TVM_GETCOUNT:
		{
			retVal = items.size() - 1; // don't count the fake root
			return true;
		}
	case TVM_GETITEM:
		{
			retVal = handleGetItem(*reinterpret_cast<TVITEMEX*>(msg.lParam));
			return true;
		}
	case TVM_GETITEMSTATE:
		{
			retVal = handleGetItemState(msg.lParam, reinterpret_cast<Item*>(msg.wParam));
			return true;
		}
	case TVM_GETNEXTITEM:
		{
			auto ret = handleGetNextItem(msg.wParam, reinterpret_cast<Item*>(msg.lParam));
			retVal = ret == root ? 0 : reinterpret_cast<LRESULT>(ret);
			return true;
		}
	case TVM_HITTEST:
		{
			retVal = reinterpret_cast<LRESULT>(find(reinterpret_cast<HTREEITEM>(sendTreeMsg(TVM_HITTEST, 0, msg.lParam))));
			reinterpret_cast<TVHITTESTINFO*>(msg.lParam)->hItem = reinterpret_cast<HTREEITEM>(retVal);
			return true;
		}
	case TVM_INSERTITEM:
		{
			retVal = reinterpret_cast<LRESULT>(handleInsert(*reinterpret_cast<TVINSERTSTRUCT*>(msg.lParam)));
			return true;
		}
	case TVM_SELECTITEM:
		{
			retVal = handleSelect(msg.wParam, reinterpret_cast<Item*>(msg.lParam));
			return true;
		}
	case TVM_SETITEM:
		{
			retVal = handleSetItem(*reinterpret_cast<TVITEMEX*>(msg.lParam));
			return true;
		}
	case WM_NOTIFY:
		{
			switch(reinterpret_cast<NMHDR*>(msg.lParam)->code) {
			case TVN_ITEMEXPANDED:
				{
					handleExpanded(*reinterpret_cast<NMTREEVIEW*>(msg.lParam));
					break;
				}
			case TVN_ITEMEXPANDING:
				{
					handleExpanding(*reinterpret_cast<NMTREEVIEW*>(msg.lParam));
					break;
				}
			case TVN_SELCHANGED:
				{
					handleSelected(find(reinterpret_cast<NMTREEVIEW*>(msg.lParam)->itemNew.hItem));
					break;
				}
			}
			break;
		}
	}
	return BaseType::handleMessage(msg, retVal);
}

// root node
VirtualTree::Item::Item() :
	handle(nullptr),
	parent(nullptr),
	prev(nullptr),
	next(nullptr),
	firstChild(nullptr),
	lastChild(nullptr),
	state(TVIS_EXPANDED),
	image(0),
	selectedImage(0),
	lParam(0)
{
}

VirtualTree::Item::Item(TVINSERTSTRUCT& tvis) :
	handle(nullptr),
	parent(reinterpret_cast<Item*>(tvis.hParent)),
	prev(nullptr),
	next(nullptr),
	firstChild(nullptr),
	lastChild(nullptr),
	state(tvis.itemex.state & tvis.itemex.stateMask),
	image(tvis.itemex.iImage),
	selectedImage(tvis.itemex.iSelectedImage),
	lParam(tvis.itemex.lParam)
{
	if(tvis.hInsertAfter == TVI_FIRST) {
		insertBefore(parent->firstChild);
	} else if(tvis.hInsertAfter == TVI_LAST) {
		insertAfter(parent->lastChild);
	} else if(tvis.hInsertAfter == TVI_SORT) {
		dwtDebugFail("VirtualTree TVI_SORT not implemented");
	} else {
		insertAfter(reinterpret_cast<Item*>(tvis.hInsertAfter));
	}
	setText(tvis.itemex.pszText);
}

HTREEITEM VirtualTree::Item::ptr() const {
	return reinterpret_cast<HTREEITEM>(const_cast<Item*>(this));
}

size_t VirtualTree::Item::Hash::operator()(const Item& item) const {
	return reinterpret_cast<size_t>(&item) / sizeof(Item);
}

bool VirtualTree::Item::Equal::operator()(const Item& a, const Item& b) const {
	return &a == &b;
}

void VirtualTree::Item::insertBefore(Item* sibling) {
	if(sibling) {
		if(sibling->prev) {
			sibling->prev->next = this;
		}
		prev = sibling->prev;
		sibling->prev = this;
	}
	next = sibling;
	if(parent->firstChild == sibling) {
		parent->firstChild = this;
	}
	if(!parent->lastChild) {
		parent->lastChild = this;
	}
}

void VirtualTree::Item::insertAfter(Item* sibling) {
	if(sibling) {
		if(sibling->next) {
			sibling->next->prev = this;
		}
		next = sibling->next;
		sibling->next = this;
	}
	prev = sibling;
	if(parent->lastChild == sibling) {
		parent->lastChild = this;
	}
	if(!parent->firstChild) {
		parent->firstChild = this;
	}
}

void VirtualTree::Item::setText(LPTSTR text) {
	if(text && text != LPSTR_TEXTCALLBACK) {
		this->text = text;
	} else {
		this->text.reset();
	}
}

bool VirtualTree::Item::expanded() const {
	return state & TVIS_EXPANDED;
}

VirtualTree::Item* VirtualTree::Item::lastExpandedChild() const {
	return lastChild && expanded() ? lastChild->lastExpandedChild() : const_cast<Item*>(this);
}

VirtualTree::Item* VirtualTree::Item::prevVisible() const {
	return prev ? prev->lastExpandedChild() : parent;
}

VirtualTree::Item* VirtualTree::Item::nextVisible() const {
	if(firstChild && expanded()) { return firstChild; }
	if(next) { return next; }
	auto item = this;
	while(item->parent) {
		item = item->parent;
		if(item->next) {
			return item->next;
		}
	}
	return nullptr;
}

bool VirtualTree::handleDelete(LPARAM lParam) {
	if(!lParam || lParam == reinterpret_cast<LPARAM>(TVI_ROOT)) {
		// delete all items.
		bool ret = sendTreeMsg(TVM_DELETEITEM, 0, lParam);
		items.clear();
		addRoot();
		return ret;
	}

	// delete one item.
	auto item = reinterpret_cast<Item*>(lParam);
	if(!validate(item)) { return false; }
	hide(*item);
	remove(item);
	return true;
}

bool VirtualTree::handleEnsureVisible(Item* item) {
	if(!validate(item)) { return false; }
	display(*item);
	return sendTreeMsg(TVM_ENSUREVISIBLE, 0, reinterpret_cast<LPARAM>(item->handle));
}

bool VirtualTree::handleExpand(WPARAM code, Item* item) {
	if(!validate(item)) { return false; }
	switch(code) {
	case TVE_COLLAPSE: if(item->expanded()) { item->state &= ~TVIS_EXPANDED; } break;
	case TVE_EXPAND: if(!item->expanded()) { item->state |= TVIS_EXPANDED; } break;
	default: dwtDebugFail("VirtualTree expand code not implemented"); break;
	}
	if(item->handle) {
		sendTreeMsg(TVM_EXPAND, code, reinterpret_cast<LPARAM>(item->handle));
	}
	return true;
}

void VirtualTree::handleExpanded(NMTREEVIEW& data) {
	auto item = find(data.itemNew.hItem);
	if(!validate(item)) { return; }
	switch(data.action) {
	case TVE_COLLAPSE:
		{
			if(item->expanded()) { item->state &= ~TVIS_EXPANDED; }
			// remove children of this item when collapsing it.
			auto child = item->firstChild;
			while(child) {
				hide(*child);
				child = child->next;
			}
			break;
		}
	case TVE_EXPAND:
		{
			if(!item->expanded()) { item->state |= TVIS_EXPANDED; }
			break;
		}
	default: dwtDebugFail("VirtualTree expand code not implemented"); break;
	}
}

void VirtualTree::handleExpanding(NMTREEVIEW& data) {
	auto item = find(data.itemNew.hItem);
	if(!validate(item)) { return; }
	switch(data.action) {
	case TVE_COLLAPSE:
		{
			break;
		}
	case TVE_EXPAND:
		{
			// add direct children of this item before expanding it.
			auto child = item->firstChild;
			while(child) {
				display(*child);
				child = child->next;
			}
			break;
		}
	default: dwtDebugFail("VirtualTree expand code not implemented"); break;
	}
}

bool VirtualTree::handleGetItem(TVITEMEX& tv) {
	auto item = reinterpret_cast<Item*>(tv.hItem);
	if(!validate(item)) { return false; }
	if(tv.mask & TVIF_CHILDREN) { tv.cChildren = item->firstChild ? 1 : 0; }
	if(tv.mask & TVIF_HANDLE) { tv.hItem = item->ptr(); }
	if(tv.mask & TVIF_IMAGE) { tv.iImage = item->image; }
	tv.lParam = item->lParam; // doesn't depend on TVIF_PARAM
	if(tv.mask & TVIF_SELECTEDIMAGE) { tv.iSelectedImage = item->selectedImage; }
	tv.state = item->state; // doesn't depend on TVIF_STATE nor on stateMask
	if(tv.mask & TVIF_TEXT && item->text) { item->text->copy(tv.pszText, tv.cchTextMax); }
	return true;
}

UINT VirtualTree::handleGetItemState(UINT mask, Item* item) {
	if(!validate(item)) { return 0; }
	return item->state & mask;
}

VirtualTree::Item* VirtualTree::handleGetNextItem(WPARAM code, Item* item) {
	if(item && !validate(item)) { return nullptr; }
	switch(code) {
	case TVGN_CARET: return selected;
	case TVGN_CHILD: return (item ? item : root)->firstChild;
	case TVGN_DROPHILITE: return nullptr;
	case TVGN_FIRSTVISIBLE: return root->firstChild;
	case TVGN_LASTVISIBLE: return root->lastExpandedChild();
	case TVGN_NEXT: return item ? item->next : nullptr;
	//case TVGN_NEXTSELECTED: return selected;
	case TVGN_NEXTVISIBLE: return item ? item->nextVisible() : nullptr;
	case TVGN_PARENT: return item ? item->parent : nullptr;
	case TVGN_PREVIOUS: return item ? item->prev : nullptr;
	case TVGN_PREVIOUSVISIBLE: return item ? item->prevVisible() : nullptr;
	case TVGN_ROOT: return root->firstChild;
	default: return nullptr;
	}
}

VirtualTree::Item* VirtualTree::handleInsert(TVINSERTSTRUCT& tvis) {
	if(tvis.hParent == TVI_ROOT || !tvis.hParent) {
		tvis.hParent = root->ptr();
	}
	auto& item = const_cast<Item&>(*items.emplace(tvis).first);
	if(item.parent->firstChild == item.parent->lastChild) {
		updateChildDisplay(item.parent);
	}
	if(item.parent->expanded()) {
		display(item);
	}
	return &item;
}

bool VirtualTree::handleSelect(WPARAM code, Item* item) {
	if(!validate(item)) { return sendTreeMsg(TVM_SELECTITEM, code, 0); }
	display(*item);
	return sendTreeMsg(TVM_SELECTITEM, code, reinterpret_cast<LPARAM>(item->handle));
}

void VirtualTree::handleSelected(Item* item) {
	selected = validate(item) ? item : nullptr;
}

bool VirtualTree::handleSetItem(TVITEMEX& tv) {
	auto item = reinterpret_cast<Item*>(tv.hItem);
	if(!validate(item)) { return false; }
	if(tv.mask & TVIF_IMAGE) { item->image = tv.iImage; }
	if(tv.mask & TVIF_PARAM) { item->lParam = tv.lParam; }
	if(tv.mask & TVIF_SELECTEDIMAGE) { item->selectedImage = tv.iSelectedImage; }
	if(tv.mask & TVIF_STATE) { item->state &= ~tv.stateMask; item->state |= tv.state & tv.stateMask; }
	if(tv.mask & TVIF_TEXT) { item->setText(tv.pszText); }
	if(item->handle) {
		tv.hItem = item->handle;
		sendTreeMsg(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tv));
	}
	return true;
}

void VirtualTree::addRoot() {
	root = const_cast<Item*>(&*items.emplace().first);
}

bool VirtualTree::validate(Item* item) const {
	return item && items.find(*item) != items.cend();
}

VirtualTree::Item* VirtualTree::find(HTREEITEM handle) {
	if(!handle) { return nullptr; }
	for(auto& i: items) { if(i.handle == handle) { return const_cast<Item*>(&i); } }
	return nullptr;
}

void VirtualTree::display(Item& item) {
	// insert the item in the actual tree control.
	if(item.handle) { return; }

	if(item.parent != root && !item.parent->handle) { display(*item.parent); }

	// don't insert items as expanded; instead expand them manually if they were expanded before.
	auto expanded = item.expanded();
	if(expanded) { item.state &= ~TVIS_EXPANDED; }

	TVINSERTSTRUCT tvis = { item.parent->handle, item.prev ? item.prev->handle : item.next ? TVI_FIRST : TVI_LAST, { {
		TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE | TVIF_TEXT } } };
	tvis.itemex.state = item.state;
	tvis.itemex.stateMask = item.state;
	tvis.itemex.pszText = item.text ? const_cast<TCHAR*>(item.text->c_str()) : LPSTR_TEXTCALLBACK;
	tvis.itemex.iImage = item.image;
	tvis.itemex.iSelectedImage = item.selectedImage;
	tvis.itemex.cChildren = item.firstChild ? 1 : 0;
	tvis.itemex.lParam = item.lParam;
	item.handle = reinterpret_cast<HTREEITEM>(sendTreeMsg(TVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&tvis)));

	if(expanded) { sendTreeMsg(TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(item.handle)); }
}

void VirtualTree::hide(Item& item) {
	// remove the item and its children from the actual tree control.
	if(!item.handle) { return; }
	auto child = item.firstChild;
	while(child) {
		hide(*child);
		child = child->next;
	}
	sendTreeMsg(TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(item.handle));
	item.handle = nullptr;
}

void VirtualTree::remove(Item* item) {
	while(item->firstChild) {
		remove(item->firstChild);
	}
	if(item->parent) {
		if(item->parent->firstChild == item) {
			item->parent->firstChild = item->next;
			if(!item->parent->firstChild) {
				updateChildDisplay(item->parent);
			}
		}
		if(item->parent->lastChild == item) {
			item->parent->lastChild = item->prev;
		}
	}
	if(item->prev) { item->prev->next = item->next; }
	if(item->next) { item->next->prev = item->prev; }
	if(item == selected) { selected = nullptr; }
	items.erase(*item);
}

void VirtualTree::updateChildDisplay(Item* item) {
	// this determines whether the item has a +/- sign next to it.
	if(!item->handle) { return; }
	TVITEMEX tv = { TVIF_CHILDREN, item->handle };
	tv.cChildren = item->firstChild ? 1 : 0;
	sendTreeMsg(TVM_SETITEM, 0, reinterpret_cast<LPARAM>(&tv));
}

LRESULT VirtualTree::sendTreeMsg(UINT msg, WPARAM wParam, LPARAM lParam) {
	// send with a direct dispatcher call to avoid loops (since we catch tree messages).
	MSG treeMsg { treeHandle(), msg, wParam, lParam };
	return tree->getDispatcher().chain(treeMsg);
}

}
