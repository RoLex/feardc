/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_STATIC_FRAME_H_
#define DCPLUSPLUS_WIN32_STATIC_FRAME_H_

#include <dcpp/Text.h>

#include "MDIChildFrame.h"

template<class T>
class StaticFrame : public MDIChildFrame<T> {
	typedef MDIChildFrame<T> BaseType;

public:
	StaticFrame(TabViewPtr parent, const tstring& title, unsigned helpId, unsigned iconId, bool manageAccels = true) :
		MDIChildFrame<T>(parent, title, helpId, iconId, manageAccels)
	{
		WinUtil::setStaticWindowState(T::id, true);
	}

	virtual ~StaticFrame() {
		frame = 0;
		WinUtil::setStaticWindowState(T::id, false);
	}

	static void openWindow(TabViewPtr parent, bool close = true, bool activate = true) {
		if(frame) {
			if(close && parent->getActive() == frame) {
				frame->close();
			} else if(activate) {
				frame->activate();
			}
		} else {
			frame = new T(parent);
			if(activate)
				frame->activate();
		}
	}

	static void parseWindowParams(TabViewPtr parent, const WindowParams& params) {
		openWindow(parent, false, BaseType::parseActivateParam(params));
	}

protected:
	static T* frame;
};

template<class T>
T* StaticFrame<T>::frame = 0;

#endif /*STATICFRAME_H_*/
