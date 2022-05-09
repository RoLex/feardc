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

#ifndef DCPLUSPLUS_WIN32_I_RECENT_H
#define DCPLUSPLUS_WIN32_I_RECENT_H

#include <dcpp/WindowManager.h>

#include "MDIChildFrame.h"

/** Recent instances of windows implementing this interface are tracked using dcpp::WindowManager. */
template<typename T>
class IRecent {
	T& t() { return *static_cast<T*>(this); }
	const T& t() const { return *static_cast<const T*>(this); }

protected:
	void addRecent() {
		WindowManager::getInstance()->addRecent(t().getId(), t().getWindowParams());
	}

	void updateRecent() {
		WindowManager::getInstance()->updateRecent(t().getId(), t().getWindowParams());
	}

	void setText(const tstring& text) {
		t().MDIChildFrame<T>::setText(text);
		updateRecent();
	}

	void addRecentParams(WindowParams& params) const {
		params["Title"] = WindowParam(Text::fromT(t().getText()));
	}
};

#endif // !defined(DCPLUSPLUS_WIN32_I_RECENT_H)
