/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DWT_HOLDRESIZE_H_
#define DWT_HOLDRESIZE_H_

#include <vector>

#include "../Widget.h"
#include "../Rectangle.h"

namespace dwt { namespace util {

class HoldResize {
public:
	HoldResize(Widget *parent, size_t hint) : parent(parent) { sizes.reserve(hint); }

	~HoldResize() {
		if(sizes.empty()) {
			return;
		}

		if(!resizeDefered()) {
			resizeNormal();
		}
	}

	void resize(Widget *child, const Rectangle &r) {
		WINDOWPOS pos = { child->handle(), NULL, r.left(), r.top(), r.width(), r.height() };
		sizes.push_back(pos);
	}

private:

	Widget *parent;
	std::vector<WINDOWPOS> sizes;

	bool resizeDefered() {
		auto h = ::BeginDeferWindowPos(sizes.size());
		for(auto& i: sizes) {
			h = ::DeferWindowPos(h, i.hwnd, NULL, i.x, i.y, i.cx, i.cy, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
			if(h == NULL) {
				return false;
			}
		}

		if(!::EndDeferWindowPos(h)) {
			return false;
		}

		return true;
	}

	void resizeNormal() {
		for(auto& i: sizes) {
			::SetWindowPos(i.hwnd, NULL, i.x, i.y, i.cx, i.cy, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
		}
	}
};
} }

#endif /* DWT_HOLDRESIZE_H_ */
