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

// Based on <https://www.codeproject.com/Articles/4025/Use-Shell-ContextMenu-in-your-applications> by R. Engels.

#ifndef DCPLUSPLUS_WIN32_SHELL_MENU_H
#define DCPLUSPLUS_WIN32_SHELL_MENU_H

#include <dcpp/typedefs.h>

#include <dwt/widgets/Menu.h>

#include "forward.h"

class ShellMenu : public dwt::Menu
{
	typedef Menu BaseType;

public:
	typedef ShellMenu ThisType;
	typedef ShellMenuPtr ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(const BaseType::Seed& seed) : BaseType::Seed(seed) {
		}
	};

	explicit ShellMenu(dwt::Widget* parent);
	~ShellMenu();

	void appendShellMenu(const StringList& paths);
	void open(const dwt::ScreenCoordinate& pt, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON);

private:
	std::vector<std::pair<Menu*, LPCONTEXTMENU3>> handlers;

	LPCONTEXTMENU3 handler;
	unsigned sel_id;

	std::vector<std::pair<dwt::Message, dwt::Widget::CallbackIter>> callbacks;

	bool handleDrawItem(const MSG& msg, LRESULT& ret);
	bool handleMeasureItem(const MSG& msg, LRESULT& ret);
	bool handleInitMenuPopup(const MSG& msg, LRESULT& ret);
	bool handleUnInitMenuPopup(const MSG& msg, LRESULT& ret);
	bool handleMenuSelect(const MSG& msg, LRESULT&);

	bool dispatch(const MSG& msg, LRESULT& ret);
};

/// @todo rename to just Menu to override dwt::Menu

#endif // !defined(DCPLUSPLUS_WIN32_SHELL_MENU_H)
