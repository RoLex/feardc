/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#include <dwt/widgets/TextBox.h>

#include <dwt/CanvasClasses.h>
#include <dwt/Texts.h>
#include <dwt/WidgetCreator.h>
#include <dwt/util/check.h>

namespace dwt {

TextBoxBase::TextBoxBase(Widget *parent, Dispatcher& dispatcher) :
BaseType(parent, dispatcher),
lines(1)
{
	dwtassert(parent, "Cant have a TextBox without a parent...");
}

TextBox::TextBox(Widget* parent) :
TextBoxBase(parent, ChainingDispatcher::superClass<TextBox>())
{
}
	
const TCHAR TextBox::windowClass[] = WC_EDIT;

TextBoxBase::Seed::Seed(DWORD style, DWORD exStyle, const tstring& caption) :
BaseType::Seed(style, exStyle, caption),
lines(1)
{
}

/* drag & drop COM interface. this allows one to drop text inside this text box. during the drag,
the caret moves inside the box to show where the drop will occur. */

class TextBoxBase::Dropper : public IDropTarget {
public:
	Dropper(TextBoxBase* const w) : IDropTarget(), w(w), ref(0), dragging(false) { }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out]  _COM_Outptr_*/ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if(!ppvObject) { return E_POINTER; }
		if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget)) {
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return ++ref;
	}

	virtual ULONG STDMETHODCALLTYPE Release( void)
	{
		if(--ref == 0) { delete this; }
		return ref;
	}

	virtual HRESULT STDMETHODCALLTYPE DragEnter(
		/* [unique][in]  __RPC__in_opt*/ IDataObject *pDataObj,
		/* [in] */ DWORD /*grfKeyState*/,
		/* [in] */ POINTL /*pt*/,
		/* [out][in]  __RPC__inout*/ DWORD *pdwEffect)
	{
		if(w->hasStyle(ES_READONLY)) { return S_OK; }
		FORMATETC formatetc { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
		dragging = pDataObj->QueryGetData(&formatetc) == S_OK;
		if(dragging) {
			w->setFocus(); // focus to display the caret
			*pdwEffect = DROPEFFECT_COPY;
		} else {
			*pdwEffect = DROPEFFECT_NONE;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragOver(
		/* [in] */ DWORD /*grfKeyState*/,
		/* [in] */ POINTL pt,
		/* [out][in]  __RPC__inout*/ DWORD *pdwEffect)
	{
		if(dragging) {
			moveCaret(pt);
			*pdwEffect = DROPEFFECT_COPY;
		} else {
			*pdwEffect = DROPEFFECT_NONE;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE DragLeave( void)
	{
		dragging = false;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Drop(
		/* [unique][in]  __RPC__in_opt*/ IDataObject *pDataObj,
		/* [in] */ DWORD /*grfKeyState*/,
		/* [in] */ POINTL pt,
		/* [out][in]  __RPC__inout*/ DWORD *pdwEffect)
	{
		if(dragging) {
			FORMATETC formatetc { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM stgmedium;
			if(pDataObj->GetData(&formatetc, &stgmedium) == S_OK) {
				if(stgmedium.tymed == TYMED_HGLOBAL && stgmedium.hGlobal) {
					auto buf = reinterpret_cast<LPCTSTR>(::GlobalLock(stgmedium.hGlobal));
					if(buf) {
						tstring text { buf };
						::GlobalUnlock(stgmedium.hGlobal);
						if(w->hasStyle(ES_NUMBER) && text.find_first_not_of(_T("0123456789")) != tstring::npos) {
							w->sendMessage(WM_CHAR, 'A'); // simulate a key press to show the error popup
						} else {
							moveCaret(pt);
							w->replaceSelection(text);
						}
					}
				}
				::ReleaseStgMedium(&stgmedium);
			}
			*pdwEffect = DROPEFFECT_COPY;
		} else {
			*pdwEffect = DROPEFFECT_NONE;
		}
		return S_OK;
	}

private:
	inline void moveCaret(const POINTL& pt) {
		auto pos = w->charFromPos(ScreenCoordinate(Point(pt.x, pt.y)));
		w->setSelection(pos, pos);
	}

	TextBoxBase* const w;
	ULONG ref;
	bool dragging;
};

void TextBoxBase::create(const Seed& cs) {
	lines = cs.lines;
	menuSeed = cs.menuSeed;
	BaseType::create(cs);

	auto dropper = new Dropper(this);
	if(::RegisterDragDrop(handle(), dropper) == S_OK) {
		onDestroy([this] { ::RevokeDragDrop(handle()); });
	} else {
		delete dropper;
	}
}

TextBox::Seed::Seed(const tstring& caption) :
BaseType::Seed(WS_CHILD | WS_TABSTOP, WS_EX_CLIENTEDGE, caption),
font(0)
{
}

void TextBox::create(const Seed& cs) {
	// some text-boxes don't handle ctrl + A so we have do it ourselves...
	addAccel(FCONTROL, 'A', [this] { setSelection(); });

	BaseType::create(cs);
	setFont(cs.font);
}

void TextBox::setText(const tstring& txt) {
	BaseType::setText(txt);

	// multiline text-boxes don't fire EN_CHANGE / EN_UPDATE when they receive WM_SETTTEXT
	if(hasStyle(ES_MULTILINE)) {
		sendCommand(EN_UPDATE);
		sendCommand(EN_CHANGE);
	}
}

void TextBox::setCue(const tstring& text) {
	Edit_SetCueBannerTextFocused(handle(), text.c_str(), TRUE);
}

tstring TextBox::getLine(int line) {
	tstring tmp;
	tmp.resize(std::max(2, lineLength(lineIndex(line))));

	*reinterpret_cast<WORD*>(&tmp[0]) = static_cast<WORD>(tmp.size());
	tmp.resize(sendMessage(EM_GETLINE, static_cast<WPARAM>(line), reinterpret_cast<LPARAM>(&tmp[0])));
	return tmp;
}

tstring TextBox::textUnderCursor(const ScreenCoordinate& p, bool includeSpaces) {
	int i = charFromPos(p);
	int line = lineFromPos(p);
	int c = (i - lineIndex(line)) & 0xFFFF;

	tstring tmp = getLine(line);

	tstring::size_type start = tmp.find_last_of(includeSpaces ? _T("<\t\r\n") : _T(" <\t\r\n"), c);
	if(start == tstring::npos)
		start = 0;
	else
		start++;

	tstring::size_type end = tmp.find_first_of(includeSpaces ? _T(">\t\r\n") : _T(" >\t\r\n"), start + 1);
	if(end == tstring::npos)
		end = tmp.size();

	return tmp.substr(start, end-start);
}

void TextBox::showPopup(const tstring& title, const tstring& text, int icon) {
	EDITBALLOONTIP tip { sizeof(EDITBALLOONTIP), title.c_str(), text.c_str(), icon };
	Edit_ShowBalloonTip(handle(), &tip);
}

tstring TextBox::getSelection() const
{
	DWORD start, end;
	this->sendMessage( EM_GETSEL, reinterpret_cast< WPARAM >( & start ), reinterpret_cast< LPARAM >( & end ) );
	tstring retVal = this->getText().substr( start, end - start );
	return retVal;
}

ClientCoordinate TextBoxBase::ptFromPos(int pos) {
	LRESULT res = sendMessage(EM_POSFROMCHAR, pos);
	Point pt;
	if(res == -1) {
		Point sz = getClientSize();
		pt.x = sz.x / 2;
		pt.y = sz.y / 2;
	} else {
		pt.x = LOWORD(res);
		pt.y = HIWORD(res);
	}
	return ClientCoordinate(pt, this);
}

void TextBoxBase::scrollToBottom() {
	// this function takes care of various corner cases (not fully scrolled, scrolled too far...)

	sendMessage(WM_VSCROLL, SB_BOTTOM);
	sendMessage(WM_VSCROLL, SB_BOTTOM);
}

ScreenCoordinate TextBoxBase::getContextMenuPos() {
	return ptFromPos(getCaretPos());
}

Point TextBoxBase::getPreferredSize() {
	// Taken from https://support.microsoft.com/kb/124315
	UpdateCanvas c(this);

	TEXTMETRIC tmSys = { 0 };
	{
		Font sysFont(Font::System);
		auto select(c.select(sysFont));
		c.getTextMetrics(tmSys);
	}

	auto select(c.select(*getFont()));
	TEXTMETRIC tmNew = { 0 };
	c.getTextMetrics(tmNew);

	Point ret = c.getTextExtent(getText());
	ret.x += GetSystemMetrics(SM_CXEDGE) * 2;
	ret.y = lines * tmNew.tmHeight + std::min(tmNew.tmHeight, tmSys.tmHeight) / 2 + GetSystemMetrics(SM_CYEDGE) * 2;
	return ret;
}

MenuPtr TextBoxBase::getMenu() {
	const bool writable = !hasStyle(ES_READONLY);
	const bool text = !getText().empty();
	const bool selection = !getSelection().empty();

	MenuPtr menu(WidgetCreator<Menu>::create(getParent(), menuSeed));

	if(writable) {
		menu->appendItem(Texts::get(Texts::undo), [this] { this->sendMessage(WM_UNDO); },
			IconPtr(), sendMessage(EM_CANUNDO));
		menu->appendSeparator();
		menu->appendItem(Texts::get(Texts::cut), [this] { this->sendMessage(WM_CUT); },
			IconPtr(), selection);
	}
	menu->appendItem(Texts::get(Texts::copy), [this] { this->sendMessage(WM_COPY); },
		IconPtr(), selection);
	if(writable) {
		menu->appendItem(Texts::get(Texts::paste), [this] { this->sendMessage(WM_PASTE); });
		menu->appendItem(Texts::get(Texts::del), [this] { this->sendMessage(WM_CLEAR); },
			IconPtr(), selection);
	}
	menu->appendSeparator();
	menu->appendItem(Texts::get(Texts::selAll), [this] { this->setSelection(0, -1);},
		IconPtr(), text);

	return menu;
}

bool TextBoxBase::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(msg.message == WM_RBUTTONDOWN && !hasFocus())
		setFocus();

	bool handled = BaseType::handleMessage(msg, retVal);

	// keep the scroll position at the end if it already was at the end
	if((msg.message == WM_SIZE || msg.message == WM_MOVE) && hasStyle(WS_VSCROLL) && scrollIsAtEnd()) {
		retVal = getDispatcher().chain(msg);
		redraw(true);
		scrollToBottom();
		return true;
	}

	if(!handled && msg.message == WM_CONTEXTMENU) {
		// process this here to give the host a chance to handle it differently.

		// imitate aspects::ContextMenu
		ScreenCoordinate pt(Point::fromLParam(msg.lParam));
		if(pt.x() == -1 || pt.y() == -1) {
			pt = getContextMenuPos();
		}

		getMenu()->open(pt);
		return true;
	}

	return handled;
}

void TextBoxBase::setFontImpl() {
	BaseType::setFontImpl();
	menuSeed.font = getFont();
}

bool TextBox::handleMessage(const MSG& msg, LRESULT& retVal) {
	if(BaseType::handleMessage(msg, retVal))
		return true;

	if(msg.message == WM_GETDLGCODE) {
		/*
		* multiline edit controls add the DLGC_WANTALLKEYS flag but go haywire when they actually
		* try to handle keys like tab/enter/escape; especially when hosted in a modeless dialog or
		* when one of their parents has the WS_EX_CONTROLPARENT style.
		*/
		retVal = getDispatcher().chain(msg);
		if(retVal & DLGC_WANTALLKEYS) {
			retVal &= ~DLGC_WANTALLKEYS;

			if(msg.wParam == VK_RETURN)
				retVal |= DLGC_WANTMESSAGE;
		}

		return true;
	}

	return false;
}

}
