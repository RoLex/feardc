/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
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

#include <dwt/Widget.h>

#include <dwt/DWTException.h>
#include <dwt/util/check.h>
#include <dwt/util/win32/ApiHelpers.h>

namespace dwt {

GlobalAtom Widget::propAtom(_T("dwt::Widget*"));

#ifdef DWT_DEBUG_WIDGETS
int widgetCount;
#endif

Widget::Widget(Widget* parent_, Dispatcher& dispatcher_) :
	hwnd(NULL), parent(parent_), dispatcher(dispatcher_)
{
#ifdef DWT_DEBUG_WIDGETS
	++widgetCount;
	printf("created a dwt widget; count: %d\n", widgetCount);
#endif
}

Widget::~Widget() {
	if(hwnd) {
		::RemoveProp(hwnd, propAtom);
	}
#ifdef DWT_DEBUG_WIDGETS
	--widgetCount;
	printf("destroying a dwt widget; count: %d\n", widgetCount);
#endif
}

void Widget::kill() {
	delete this;
}

HWND Widget::create(const Seed & cs) {
	HWND hWnd = ::CreateWindowEx(cs.exStyle, getDispatcher().getClassName(), cs.caption.c_str(), cs.style,
		cs.location.x(), cs.location.y(), cs.location.width(), cs.location.height(),
		getParentHandle(), cs.menuHandle, ::GetModuleHandle(NULL), reinterpret_cast<LPVOID>(this));

	if(!hWnd) {
		// The most common error is to forget WS_CHILD in the styles
		throw Win32Exception("Unable to create widget");
	}

	return hWnd;
}

void Widget::setHandle(HWND h) {
	if(hwnd) {
		throw DWTException("You may not attach to a widget that's already attached");
	}

	hwnd = h;

	::SetProp(hwnd, propAtom, reinterpret_cast<HANDLE>(this));

	::SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc::wndProc));
}

Widget* Widget::getRoot() const {
	return hwnd_cast<Widget*>(::GetAncestor(handle(), GA_ROOT));
}

void Widget::addRemoveStyle(DWORD addStyle, bool add) {
	util::win32::updateStyle(handle(), GWL_STYLE, addStyle, add);
}

void Widget::addRemoveExStyle(DWORD addStyle, bool add) {
	util::win32::updateStyle(handle(), GWL_EXSTYLE, addStyle, add);
}

Widget::CallbackIter Widget::addCallback(const Message& msg, const CallbackType& callback) {
	CallbackList& callbacks = handlers[msg];
	callbacks.push_back(callback);
	return --callbacks.end();
}

Widget::CallbackIter Widget::setCallback(const Message& msg, const CallbackType& callback) {
	CallbackList& callbacks = handlers[msg];
	callbacks.clear();
	callbacks.push_back(callback);
	return --callbacks.end();
}

void Widget::clearCallback(const Message& msg, const CallbackIter& i) {
	CallbackList& callbacks = handlers[msg];
	callbacks.erase(i);
	if(callbacks.empty()) {
		handlers.erase(msg);
	}
}

void Widget::clearCallbacks(const Message& msg) {
	handlers.erase(msg);
}

/// Make sure that handle is still valid before calling f
void checkCall(HWND handle, const Application::Callback& f) {
	/// @todo this might fail when the handle has already been re-used elsewhere
	if(::IsWindow(handle))
		f();
}

void Widget::callAsync(const Application::Callback& f) {
	HWND h = handle();
	Application::instance().callAsync([h, f] { checkCall(h, f); });
}

bool Widget::handleMessage(const MSG &msg, LRESULT &retVal) {
	// First we must create a "comparable" message...
	Message msgComparer(msg);
	auto i = handlers.find(msgComparer);
	bool handled = false;
	if(i != handlers.end()) {
		CallbackList& list = i->second;
		for(auto& j: list) {
			handled |= j(msg, retVal);
		}
	}
	return handled;
}

void Widget::setParent(Widget* widget) {
	::SetWindowLongPtr(handle(), GWLP_HWNDPARENT, widget->toLParam());
	parent = widget;
}

Point Widget::getPreferredSize() {
	return Point(0, 0);
}

void Widget::layout() { }

Point Widget::getPrimaryDesktopSize() {
	POINT pt = { 0 };
	return getDesktopSize(::MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY)).size;
}

Rectangle Widget::getDesktopSize() const {
	return getDesktopSize(::MonitorFromWindow(handle(), MONITOR_DEFAULTTONEAREST));
}

Rectangle Widget::getDesktopSize(HMONITOR mon) {
	MONITORINFO mi = { sizeof(MONITORINFO) };
	if(::GetMonitorInfo(mon, &mi)) {
		return Rectangle(mi.rcWork);
	}

	// the following should never be needed, but better be safe...
	RECT rc = { 0 };
	::GetWindowRect(::GetDesktopWindow(), &rc);
	return Rectangle(rc);
}

void Widget::setZOrder(HWND insertAfter) {
	::SetWindowPos(handle(), insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
}

}
