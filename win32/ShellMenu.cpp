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

// Based on <https://www.codeproject.com/Articles/4025/Use-Shell-ContextMenu-in-your-applications> by R. Engels.

#include "stdafx.h"
#include "ShellMenu.h"

#include <dwt/util/StringUtils.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::util::escapeMenu;

ShellMenu::ShellMenu(dwt::Widget* parent) :
BaseType(parent),
handler(0),
sel_id(0)
{
}

void ShellMenu::appendShellMenu(const StringList& paths) {
#define check(x) if(!(x)) { return; }

	// get IShellFolder interface of Desktop (root of Shell namespace)
	IShellFolder* desktop = 0;
	HRESULT hr = ::SHGetDesktopFolder(&desktop);
	check(hr == S_OK && desktop);

	// get interface to IMalloc used to free PIDLs
	LPMALLOC lpMalloc = 0;
	hr = ::SHGetMalloc(&lpMalloc);
	check(hr == S_OK && lpMalloc);

#undef check
#define check(x) if(!(x)) { continue; }

	// stores allocated PIDLs to free them afterwards.
	std::vector<LPITEMIDLIST> pidls;

	// stores paths for which we have managed to get a valid IContextMenu3 interface.
	std::vector<std::pair<string, LPCONTEXTMENU3>> valid;

	for(auto& i: paths) {
		// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
		// but since we use the Desktop as our interface and the Desktop is the namespace root
		// that means that it's a fully qualified PIDL, which is what we need
		LPITEMIDLIST pidl = 0;
		hr = desktop->ParseDisplayName(0, 0, const_cast<LPWSTR>(Text::utf8ToWide(i).c_str()), 0, &pidl, 0);
		check(hr == S_OK && pidl);
		pidls.push_back(pidl);

		// get the parent IShellFolder interface of pidl and the relative PIDL
		IShellFolder* folder = 0;
		LPCITEMIDLIST pidlItem = 0;
		hr = ::SHBindToParent(pidl, IID_IShellFolder, reinterpret_cast<LPVOID*>(&folder), &pidlItem);
		check(hr == S_OK && folder && pidlItem);

		// first we retrieve the normal IContextMenu interface (every object should have it)
		LPCONTEXTMENU handler1 = 0;
		hr = folder->GetUIObjectOf(0, 1, &pidlItem, IID_IContextMenu, 0, reinterpret_cast<LPVOID*>(&handler1));
		folder->Release();
		check(hr == S_OK && handler1);

		// then try to get the version 3 interface
		LPCONTEXTMENU3 handler3 = 0;
		hr = handler1->QueryInterface(IID_IContextMenu3, reinterpret_cast<LPVOID*>(&handler3));
		handler1->Release();
		check(hr == S_OK && handler3);

		valid.emplace_back(i, handler3);
	}

#undef check

	for(auto& i: pidls)
		lpMalloc->Free(i);
	lpMalloc->Release();

	desktop->Release();

	if(valid.empty())
		return;

	appendSeparator();

	if(valid.size() == 1)
		handlers.emplace_back(appendPopup(T_("Shell menu"), dwt::IconPtr(), false), valid[0].second);
	else {
		auto popup = appendPopup(T_("Shell menus"));
		for(auto& i: valid)
			handlers.emplace_back(popup->appendPopup(escapeMenu(Text::toT(i.first)), dwt::IconPtr(), false), i.second);
	}

	auto addCB = [this](UINT messageId, auto callback) {
		dwt::Message message { messageId };
		callbacks.emplace_back(message, getParent()->addCallback(message,
			[=](auto&... args) { return (this->*callback)(args...); }));
	};
	addCB(WM_DRAWITEM, &ShellMenu::handleDrawItem);
	addCB(WM_MEASUREITEM, &ShellMenu::handleMeasureItem);
	addCB(WM_MENUCHAR, &ShellMenu::dispatch);
	addCB(WM_INITMENUPOPUP, &ShellMenu::handleInitMenuPopup);
	addCB(WM_UNINITMENUPOPUP, &ShellMenu::handleUnInitMenuPopup);
	addCB(WM_MENUSELECT, &ShellMenu::handleMenuSelect); /// @todo no LRESULT& 2nd arg here
}

ShellMenu::~ShellMenu() {
	for(auto& i: callbacks)
		getParent()->clearCallback(i.first, i.second);

	for(auto& i: handlers)
		i.second->Release();
}

void ShellMenu::open(const dwt::ScreenCoordinate& pt, unsigned flags) {
	BaseType::open(pt, flags);

	if(sel_id >= ID_SHELLCONTEXTMENU_MIN && sel_id <= ID_SHELLCONTEXTMENU_MAX && handler) {
		CMINVOKECOMMANDINFO cmi = { sizeof(CMINVOKECOMMANDINFO) };
		cmi.lpVerb = (LPSTR)MAKEINTRESOURCE(sel_id - ID_SHELLCONTEXTMENU_MIN);
		cmi.nShow = SW_SHOWNORMAL;
		handler->InvokeCommand(&cmi);
	}
}

bool ShellMenu::handleDrawItem(const MSG& msg, LRESULT& ret) {
	if(msg.wParam)
		return false;

	LPDRAWITEMSTRUCT t = reinterpret_cast<LPDRAWITEMSTRUCT>(msg.lParam);
	if(!t)
		return false;
	if(t->CtlType != ODT_MENU)
		return false;

	const unsigned& id = t->itemID;
	if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX)
		return dispatch(msg, ret);

	return false;
}

bool ShellMenu::handleMeasureItem(const MSG& msg, LRESULT& ret) {
	if(msg.wParam)
		return false;

	LPMEASUREITEMSTRUCT t = reinterpret_cast<LPMEASUREITEMSTRUCT>(msg.lParam);
	if(!t)
		return false;
	if(t->CtlType != ODT_MENU)
		return false;

	const unsigned& id = t->itemID;
	if(id >= ID_SHELLCONTEXTMENU_MIN && id <= ID_SHELLCONTEXTMENU_MAX)
		return dispatch(msg, ret);

	return false;
}

bool ShellMenu::handleInitMenuPopup(const MSG& msg, LRESULT& ret) {
	HMENU menu = reinterpret_cast<HMENU>(msg.wParam);
	for(auto& i: handlers) {
		if(i.first->handle() == menu) {
			handler = i.second;
			handler->QueryContextMenu(i.first->handle(), 0, ID_SHELLCONTEXTMENU_MIN, ID_SHELLCONTEXTMENU_MAX, CMF_NORMAL | CMF_EXPLORE);
			break;
		}
	}

	return dispatch(msg, ret);
}

bool ShellMenu::handleUnInitMenuPopup(const MSG& msg, LRESULT& ret) {
	HMENU menu = reinterpret_cast<HMENU>(msg.wParam);
	for(auto& i: handlers)
		if(i.first->handle() == menu)
			i.first->clear();

	return dispatch(msg, ret);
}

bool ShellMenu::handleMenuSelect(const MSG& msg, LRESULT&) {
	// make sure this isn't a "menu closed" signal
	if((HIWORD(msg.wParam) == 0xFFFF) && (msg.lParam == 0))
		return false;

	// save the currently selected id in case we need to dispatch it later on
	sel_id = LOWORD(msg.wParam);
	return false;
}

bool ShellMenu::dispatch(const MSG& msg, LRESULT& ret) {
	if(handler) {
		handler->HandleMenuMsg2(msg.message, msg.wParam, msg.lParam, &ret);
		return true;
	}
	return false;
}
