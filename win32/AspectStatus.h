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

#ifndef DCPLUSPLUS_WIN32_ASPECTSTATUS_H_
#define DCPLUSPLUS_WIN32_ASPECTSTATUS_H_

#include <dwt/widgets/StatusBar.h>

#include "WinUtil.h"

template<class WidgetType>
class AspectStatus {
	typedef AspectStatus<WidgetType> ThisType;

	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

	HWND H() const { return W().handle(); }

protected:
	AspectStatus() : status(0) { }

	void initStatus(bool sizeGrip = false) {
		dwt::StatusBar::Seed cs(WidgetType::STATUS_LAST, WidgetType::STATUS_STATUS, sizeGrip);
		cs.font = WinUtil::font;
		status = W().addChild(cs);
		status->onHelp(&WinUtil::help);
	}

	dwt::StatusBarPtr status;
};

#endif /*ASPECTSTATUS_H_*/
