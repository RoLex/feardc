/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

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

#include <dwt/widgets/ToolBar.h>

namespace dwt {

const TCHAR ToolBar::windowClass[] = TOOLBARCLASSNAME;

ToolBar::Seed::Seed() :
BaseType::Seed(WS_CHILD | WS_CLIPCHILDREN | CCS_ADJUSTABLE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS)
{
}

ToolBar::ToolBar(Widget* parent) :
BaseType(parent, ChainingDispatcher::superClass<ToolBar>()),
itsNormalImageList(0),
itsHotImageList(0),
itsDisabledImageList(0),
customizing(false),
customized(nullptr),
customizeHelp(nullptr)
{
}

void ToolBar::create(const Seed& cs) {
	BaseType::create(cs);

	sendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);

	//// Telling the toolbar what the size of the TBBUTTON struct is
	sendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON));

	onRaw([this](WPARAM, LPARAM lParam) { return handleDropDown(lParam); }, Message(WM_NOTIFY, TBN_DROPDOWN));
	onRaw([this](WPARAM, LPARAM lParam) { return handleToolTip(lParam); }, Message(WM_NOTIFY, TBN_GETINFOTIP));

	if((cs.style & CCS_ADJUSTABLE) == CCS_ADJUSTABLE) {
		// customization-related messages
		onRaw([this](WPARAM, LPARAM) { return handleBeginAdjust(); }, Message(WM_NOTIFY, TBN_BEGINADJUST));
		onRaw([this](WPARAM, LPARAM) { return handleChange(); }, Message(WM_NOTIFY, TBN_TOOLBARCHANGE));
		onRaw([this](WPARAM, LPARAM) { return handleCustHelp(); }, Message(WM_NOTIFY, TBN_CUSTHELP));
		onRaw([this](WPARAM, LPARAM) { return handleEndAdjust(); }, Message(WM_NOTIFY, TBN_ENDADJUST));
		onRaw([this](WPARAM, LPARAM lParam) { return handleGetButtonInfo(lParam); }, Message(WM_NOTIFY, TBN_GETBUTTONINFO));
		onRaw([this](WPARAM, LPARAM) { return handleInitCustomize(); }, Message(WM_NOTIFY, TBN_INITCUSTOMIZE));
		onRaw([this](WPARAM, LPARAM) { return handleQuery(); }, Message(WM_NOTIFY, TBN_QUERYINSERT));
		onRaw([this](WPARAM, LPARAM) { return handleQuery(); }, Message(WM_NOTIFY, TBN_QUERYDELETE));
		onRaw([this](WPARAM, LPARAM) { return handleReset(); }, Message(WM_NOTIFY, TBN_RESET));
	}
}

Point ToolBar::getPreferredSize() {
	// get the rect of the last item
	RECT rect;
	sendMessage(TB_GETITEMRECT, size() - 1, reinterpret_cast<LPARAM>(&rect));
	return Point(rect.right, rect.bottom - rect.top);
}

void ToolBar::addButton(const std::string& id, const IconPtr& icon, const IconPtr& hotIcon, const tstring& text, bool showText,
	unsigned helpId, const Dispatcher::F& f, const DropDownFunction& dropDownF)
{
	if(icon) {
		if(!itsNormalImageList)
			setNormalImageList(new ImageList(icon->getSize()));
		itsNormalImageList->add(*icon);
	}
	if(hotIcon) {
		if(!itsHotImageList)
			setHotImageList(new ImageList(hotIcon->getSize()));
		itsHotImageList->add(*hotIcon);
	}

	addButton(id, icon ? itsNormalImageList->size() - 1 : I_IMAGENONE, text, showText, helpId, f, dropDownF);
}

void ToolBar::addButton(const std::string& id, int image, const tstring& text, bool showText,
	unsigned helpId, const Dispatcher::F& f, const DropDownFunction& dropDownF)
{
	TBBUTTON tb = { image, static_cast<int>(id_offset + buttons.size()), TBSTATE_ENABLED, BTNS_AUTOSIZE };
	if(dropDownF)
		tb.fsStyle |= f ? BTNS_DROPDOWN : BTNS_WHOLEDROPDOWN;
	if(showText) {
		tb.fsStyle |= BTNS_SHOWTEXT;
		tstring str = text;
		str.push_back('\0'); // terminated by 2 nulls
		tb.iString = sendMessage(TB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
	} else {
		static tstring emptyString;
		tb.iString = reinterpret_cast<INT_PTR>(emptyString.c_str());
	}

	Button button = { tb, id, text, helpId, f, dropDownF };
	buttons.push_back(button);
}

std::vector<std::string> ToolBar::getLayout() const {
	std::vector<std::string> ret;

	// imitate getButton except we also care about separators and we cache the TBBUTTONINFO struct
	TBBUTTONINFO tb = { sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE };
	for(unsigned i = 0, iend = size(); i < iend; ++i) {
		if(sendMessage(TB_GETBUTTONINFO, i, reinterpret_cast<LPARAM>(&tb)) != -1) {
			if((tb.fsStyle & BTNS_SEP) == BTNS_SEP)
				ret.emplace_back();
			else {
				size_t index = tb.idCommand - id_offset;
				if(index < buttons.size())
					ret.push_back(buttons[index].id);
			}
		}
	}

	return ret;
}

void ToolBar::setLayout(const std::vector<std::string>& ids) {
	while(size() > 0)
		removeButton(0);

	for(auto& id: ids) {
		const TBBUTTON* ptb = 0;
		if(id.empty())
			ptb = &getSeparator();
		else {
			for(auto& button: buttons) {
				if(button.id == id) {
					ptb = &(button.button);
				}
			}
		}

		if(ptb && !sendMessage(TB_ADDBUTTONS, 1, reinterpret_cast<LPARAM>(ptb))) {
			throw Win32Exception("Error when trying to add a button in ToolBar::setLayout");
		}
	}
}

void ToolBar::customize() {
	sendMessage(TB_CUSTOMIZE);
}

void ToolBar::setButtonChecked(unsigned id, bool check) {
	sendMessage(TB_CHECKBUTTON, id, check ? TRUE : FALSE);
}

void ToolBar::setButtonChecked(const std::string& id, bool check) {
	int intId = getIntId(id);
	if(intId != -1)
		setButtonChecked(intId, check);
}

unsigned ToolBar::size() const {
	return sendMessage(TB_BUTTONCOUNT);
}

int ToolBar::hitTest(const ScreenCoordinate& pt) {
	POINT point = ClientCoordinate(pt, this).getPoint();
	return sendMessage(TB_HITTEST, 0, reinterpret_cast<LPARAM>(&point));
}

void ToolBar::removeButton(unsigned index) {
	sendMessage(TB_DELETEBUTTON, index);
}

bool ToolBar::handleMessage( const MSG & msg, LRESULT & retVal ) {
	if(msg.message == WM_COMMAND && msg.lParam == reinterpret_cast<LPARAM>(handle())) {
		size_t index = LOWORD(msg.wParam) - id_offset;
		if(index < buttons.size()) {
			const Dispatcher::F& f = buttons[index].f;
			if(f) {
				f();
				return true;
			}
		}
	}
	return BaseType::handleMessage(msg, retVal);
}

LRESULT ToolBar::handleDropDown(LPARAM lParam) {
	LPNMTOOLBAR info = reinterpret_cast<LPNMTOOLBAR>(lParam);
	size_t index = info->iItem - id_offset;
	if(index < buttons.size()) {
		const DropDownFunction& f = buttons[index].dropDownF;
		if(f) {
			f(ScreenCoordinate(ClientCoordinate(Point(info->rcButton.left, info->rcButton.bottom), this)));
			return TBDDRET_DEFAULT;
		}
	}
	return TBDDRET_NODEFAULT;
}

LRESULT ToolBar::handleToolTip(LPARAM lParam) {
	LPNMTBGETINFOTIP info = reinterpret_cast<LPNMTBGETINFOTIP>(lParam);
	size_t index = info->iItem - id_offset;
	if(index < buttons.size()) {
		_tcsncpy(info->pszText, buttons[index].text.c_str(), info->cchTextMax);
	}
	return 0;
}

LRESULT ToolBar::handleBeginAdjust() {
	prevLayout = getLayout();
	customizing = true;
	return 0;
}

LRESULT ToolBar::handleChange() {
	if(!customizing && customized)
		customized();
	return 0;
}

LRESULT ToolBar::handleCustHelp() {
	if(customizeHelp)
		customizeHelp();
	return 0;
}

LRESULT ToolBar::handleEndAdjust() {
	if(customized)
		customized();
	customizing = false;
	prevLayout.clear();
	return 0;
}

LRESULT ToolBar::handleGetButtonInfo(LPARAM lParam) {
	LPTBNOTIFY info = reinterpret_cast<LPTBNOTIFY>(lParam);
	size_t index = info->iItem; // no need to use id_offset here, this is already a 0-based index
	if(index < buttons.size()) {
		const Button& button = buttons[index];
		info->tbButton = button.button;
		_tcsncpy(info->pszText, button.text.c_str(), info->cchText);
		return TRUE;
	}
	return FALSE;
}

LRESULT ToolBar::handleInitCustomize() {
	// hide the "Help" button of the dialog if no help function is defined.
	return customizeHelp ? 0 : TBNRF_HIDEHELP;
}

LRESULT ToolBar::handleQuery() {
	return TRUE;
}

LRESULT ToolBar::handleReset() {
	setLayout(prevLayout);
	return 0;
}

void ToolBar::helpImpl(unsigned& id) {
	// we have the help id of the whole toolbar; convert to the one of the specific button the user just clicked on
	int position = hitTest(ScreenCoordinate(Point::fromLParam(::GetMessagePos())));
	if(position >= 0) {
		const Button* button = getButton(position);
		if(button)
			id = button->helpId;
	}
}

const TBBUTTON& ToolBar::getSeparator() const {
	static TBBUTTON sep = { 0, 0, 0, BTNS_SEP };
	return sep;
}

const ToolBar::Button* ToolBar::getButton(unsigned position) const {
	TBBUTTONINFO tb = { sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_COMMAND };
	if(sendMessage(TB_GETBUTTONINFO, position, reinterpret_cast<LPARAM>(&tb)) != -1) {
		size_t index = tb.idCommand - id_offset;
		if(index < buttons.size())
			return &buttons[index];
	}
	return 0;
}

int ToolBar::getIntId(const std::string& id) const {
	for(auto& i: buttons) {
		if(i.id == id)
			return i.button.idCommand;
	}
	return -1;
}

}
