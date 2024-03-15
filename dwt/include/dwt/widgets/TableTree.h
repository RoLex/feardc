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

#ifndef DWT_TABLETREE_H
#define DWT_TABLETREE_H

#include <unordered_map>
#include <vector>

#include "Table.h"
#include <dwt/Theme.h>

namespace dwt {

/** This table control provides a hierarchical organization of its items. Parency is represented by
glyphs similar to those drawn by tree controls. */
class TableTree :
	public Table
{
	typedef Table BaseType;
	friend class WidgetCreator<TableTree>;

public:
	typedef TableTree ThisType;
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(const BaseType::Seed& seed);
	};

	void create(const Seed& seed);

	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

	/** Insert a child item.
	@note The list should be resorted and redrawn for this to take effect. */
	void insertChild(LPARAM parent, LPARAM child);
	void eraseChild(LPARAM child);
	void collapse(LPARAM parent);
	void expand(LPARAM parent);

	template<typename SortFunction> void onSortItems(SortFunction f) {
		BaseType::onSortItems([this, f](LPARAM lhs, LPARAM rhs) -> int {
			auto res = handleSort(lhs, rhs);
			return res ? res : f(lhs, rhs);
		});
	}

protected:
	explicit TableTree(Widget* parent);
	virtual ~TableTree() { }

private:
	struct Item {
		std::vector<LPARAM> children;
		bool expanded;
		Rectangle glyphRect;
		Item();
		void redrawGlyph(TableTree& w);
	};
	std::unordered_map<LPARAM, Item> items;
	std::unordered_map<LPARAM, LPARAM> children; // child -> parent cache

	Theme theme;
	long indent;

	LRESULT handleCustomDraw(NMLVCUSTOMDRAW& data);
	bool handleKeyDown(int c);
	bool handleLeftMouseDown(const MouseEvent& me);
	void handleDelete(int pos);
	void handleInsert(LVITEM& lv);
	int handleSort(LPARAM& lhs, LPARAM& rhs);

	void eraseChild(decltype(children)::iterator& child, bool deleting);

	LRESULT sendMsg(UINT msg, WPARAM wParam, LPARAM lParam);
};

}

#endif
