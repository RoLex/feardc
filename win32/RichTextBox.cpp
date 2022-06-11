/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdafx.h"
#include "RichTextBox.h"

#include <boost/scoped_array.hpp>

#include <dwt/WidgetCreator.h>
#include <dwt/widgets/Menu.h>
#include <dwt/widgets/ToolTip.h>
#include <dwt/util/StringUtils.h>

#include "ParamDlg.h"
#include "resource.h"
#include "WinUtil.h"

RichTextBox::Seed::Seed() : 
BaseType::Seed()
{
}

RichTextBox::RichTextBox(dwt::Widget* parent) :
BaseType(parent),
linkTip(0),
linkTipPos(0),
linkF(nullptr)
{
}

void RichTextBox::create(const Seed& seed) {
	BaseType::create(seed);

	if((seed.events & ENM_LINK) == ENM_LINK) {
		linkTip = dwt::WidgetCreator<dwt::ToolTip>::create(this, dwt::ToolTip::Seed());
		linkTip->setTool(this, [this](tstring& text) { handleLinkTip(text); });
		onDestroy([this] { linkTip->close(); linkTip = nullptr; });

		onRaw([this](WPARAM, LPARAM lParam) { return handleLink(*reinterpret_cast<ENLINK*>(lParam)); },
			dwt::Message(WM_NOTIFY, EN_LINK));
	}
}

bool RichTextBox::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(msg.message == WM_KEYDOWN) {
		switch(static_cast<int>(msg.wParam)) {
		case 'E': case 'J': case 'L': case 'R':
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			// these don't play well with DC++ since it is sometimes impossible to revert the change
			if(isControlPressed() && !isAltPressed())
				return true;
		}
	}

	if(msg.message == WM_SETCURSOR && !currentLink.empty() && ::GetMessagePos() != linkTipPos) {
		linkTip->setActive(false);
		currentLink.clear();
	}

	if(BaseType::handleMessage(msg, retVal))
		return true;

	switch(msg.message)
	{
		// we process these messages here to give the host a chance to handle them differently.

	case WM_KEYDOWN:
		{
			// imitate aspects::Keyboard
			return handleKeyDown(static_cast<int>(msg.wParam));
		}
	}

	return false;
}

MenuPtr RichTextBox::getMenu() {
	auto menu = BaseType::getMenu();

	menu->appendSeparator();
	auto enabled = !getText().empty();
	menu->appendItem(T_("&Find...\tCtrl+F"), [this] { findTextNew(); }, dwt::IconPtr(), enabled);
	menu->appendItem(T_("Find &Next\tF3"), [this] { findTextNext(); }, dwt::IconPtr(), enabled);

	if(!currentLink.empty()) {
		menu->appendSeparator();
		auto text = currentLink;
		auto linkMenu = menu->appendPopup(dwt::util::escapeMenu(text), WinUtil::menuIcon(IDI_LINKS));
		linkMenu->appendItem(T_("&Open"), [this, text] { openLink(text); }, WinUtil::menuIcon(IDI_RIGHT), true, true);
		linkMenu->appendItem(T_("&Copy"), [this, text] { WinUtil::setClipboard(text); });
	}

	return menu;
}

tstring RichTextBox::findTextPopup() {
	ParamDlg lineFind(this, T_("Search"), T_("Specify search string"));
	if(lineFind.run() == IDOK) {
		return lineFind.getValue();
	}
	return Util::emptyStringT;
}

void RichTextBox::findTextNew() {
	findText(findTextPopup());
}

void RichTextBox::findTextNext() {
	findText(currentNeedle.empty() ? findTextPopup() : currentNeedle);
}

bool RichTextBox::handleKeyDown(int c) {
	switch(c) {
	case VK_F3:
		findTextNext();
		return true;
	case VK_ESCAPE:
		setSelection(-1, -1);
		scrollToBottom();
		clearCurrentNeedle();
		return true;
	}
	return false;
}

void RichTextBox::onLink(LinkF f) {
	linkF = f;
}

LRESULT RichTextBox::handleLink(ENLINK& link) {
	/* the control doesn't handle click events, just "mouse down" & "mouse up". so we have to make
	sure the mouse hasn't moved between "down" & "up". */
	static LPARAM clickPos = 0;

	switch(link.msg) {
	case WM_LBUTTONDOWN:
		{
			clickPos = link.lParam;
			break;
		}

	case WM_LBUTTONUP:
		{
			if(link.lParam != clickPos)
				break;

			openLink(getLinkText(link));
			break;
		}

	case WM_SETCURSOR:
		{
			auto pos = ::GetMessagePos();
			if(pos == linkTipPos)
				break;
			linkTipPos = pos;

			currentLink = getLinkText(link);
			linkTip->refresh();
			break;
		}
	}
	return 0;
}

void RichTextBox::handleLinkTip(tstring& text) {
	text = currentLink;
}

tstring RichTextBox::getLinkText(const ENLINK& link) {
	boost::scoped_array<TCHAR> buf(new TCHAR[link.chrg.cpMax - link.chrg.cpMin + 1]);
	TEXTRANGE text = { link.chrg, buf.get() };
	sendMessage(EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&text));
	return buf.get();
}

void RichTextBox::openLink(const tstring& text) {
	if(!linkF || !linkF(text)) {
		WinUtil::parseLink(text);
	}
}
