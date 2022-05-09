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

#ifndef DWT_VIRTUALTREE_H
#define DWT_VIRTUALTREE_H

#include <unordered_set>

#include <boost/optional.hpp>

#include "Tree.h"

namespace dwt {

/** This is a tree control designed to hold a large amount of items. Instead of adding all of its
nodes to the control, it stores them internally and only adds them when required (when expanding
parent nodes). */
/* Implementation: Fake HTREEITEMs are delivered to the callers of a VirtualTree. Instead of
pointing to actual nodes of the tree control, they point to an internal structure. Tree control
messages that depend on HTREEITEMs are captured and handled internally by the VirtualTree.
Many implementation details are inspired by Wine (dlls/comctl32/treeview.c). */
class VirtualTree :
	public Tree
{
	typedef Tree BaseType;
	friend class WidgetCreator<VirtualTree>;

public:
	typedef VirtualTree ThisType;
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(const BaseType::Seed& seed);
	};

	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

protected:
	explicit VirtualTree(Widget* parent);
	virtual ~VirtualTree() { }

private:
	/* info about each item; similar to TVINSERTSTRUCT / TVITEMEX. a pointer to this structure is
	given to callers of a VirtualTree as an HTREEITEM; they can then manipulate it as usual. */
	struct Item : boost::noncopyable {
		HTREEITEM handle; /* the handle in the actual tree control when this item is visible, which
						  is different from the fake handle given to callers of a VirtualTree. */
		Item* parent;
		Item* prev;
		Item* next;
		Item* firstChild;
		Item* lastChild;
		UINT state;
		boost::optional<tstring> text;
		int image;
		int selectedImage;
		LPARAM lParam;

		Item();
		Item(TVINSERTSTRUCT& tvis);

		HTREEITEM ptr() const;
		struct Hash { size_t operator()(const Item& item) const; };
		struct Equal { bool operator()(const Item& a, const Item& b) const; };

		void insertBefore(Item* sibling);
		void insertAfter(Item* sibling);
		void setText(LPTSTR text);

		bool expanded() const;
		Item* lastExpandedChild() const;
		Item* prevVisible() const;
		Item* nextVisible() const;
	};

	std::unordered_set<Item, Item::Hash, Item::Equal> items;
	Item* root;
	Item* selected;

	bool handleDelete(LPARAM lParam);
	bool handleEnsureVisible(Item* item);
	bool handleExpand(WPARAM code, Item* item);
	void handleExpanded(NMTREEVIEW& data);
	void handleExpanding(NMTREEVIEW& data);
	bool handleGetItem(TVITEMEX& tv);
	UINT handleGetItemState(UINT mask, Item* item);
	Item* handleGetNextItem(WPARAM code, Item* item);
	Item* handleInsert(TVINSERTSTRUCT& tvis);
	bool handleSelect(WPARAM code, Item* item);
	void handleSelected(Item* item);
	bool handleSetItem(TVITEMEX& tv);

	void addRoot();
	bool validate(Item* item) const;
	Item* find(HTREEITEM handle);
	void display(Item& item);
	void hide(Item& item);
	void remove(Item* item);
	void updateChildDisplay(Item* item);
	LRESULT sendTreeMsg(UINT msg, WPARAM wParam, LPARAM lParam);
};

}

#endif
