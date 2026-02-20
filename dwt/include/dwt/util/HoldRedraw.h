/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DWT_HOLDREDRAW_H_
#define DWT_HOLDREDRAW_H_

#include "../Widget.h"

namespace dwt { namespace util {

class HoldRedraw {
public:
	HoldRedraw(Widget* w_, bool reallyHold = true) {
		if(reallyHold) {
			w = w_;
			w->sendMessage(WM_SETREDRAW, FALSE);
		} else {
			w = nullptr;
		}
	}
	~HoldRedraw() {
		if(w)
			w->sendMessage(WM_SETREDRAW, TRUE);
	}
private:
	Widget* w;
};

} }

#endif /* DWT_HOLDREDRAW_H_*/
