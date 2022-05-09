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

#include "stdafx.h"
#include "dwtTexts.h"

#include <dcpp/Text.h>

#include <dwt/Texts.h>

void dwtTexts::init() {
	dwt::Texts::get = [](dwt::Texts::Text text) -> tstring {
		using namespace dwt;

		switch(text) {
		case Texts::undo: return T_("&Undo\tCtrl+Z");
		case Texts::cut: return T_("Cu&t\tCtrl+X");
		case Texts::copy: return T_("&Copy\tCtrl+C");
		case Texts::paste: return T_("&Paste\tCtrl+V");
		case Texts::del: return T_("&Delete\tDel");
		case Texts::selAll: return T_("Select &All\tCtrl+A");
		case Texts::resize: return T_("Click and drag to resize");
		case Texts::close: return T_("Close");
		}

		dcassert(0);
		return dcpp::tstring();
	};
}
