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

#include <dwt/widgets/ModalDialog.h>

#include <dwt/Application.h>
#include <dwt/util/GDI.h>

namespace dwt {

const TCHAR* ModalDialog::windowClass = WC_DIALOG;

ModalDialog::Seed::Seed(const Point& size, DWORD styles_) :
BaseType::Seed(tstring(), styles_ | WS_POPUP | WS_CAPTION | WS_SYSMENU, WS_EX_CONTROLPARENT | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE)
{
	location.size = size;
	location.size.x *= util::dpiFactor();
	location.size.y *= util::dpiFactor();
}

ModalDialog::ModalDialog(Widget* parent) :
BaseType(parent, ChainingDispatcher::superClass<ModalDialog>()),
quit(false),
ret(0)
{
	onClosing([this] { return this->ThisType::defaultClosing(); });

	filterIter = dwt::Application::instance().addFilter([this](MSG& msg) { return this->ThisType::filter(msg); });
}

ModalDialog::~ModalDialog() {
	dwt::Application::instance().removeFilter(filterIter);
}

void ModalDialog::create(const Seed& cs) {
	Seed cs2 = cs;

	if((cs.style & DS_CONTEXTHELP) == DS_CONTEXTHELP) {
		cs2.exStyle |= WS_EX_CONTEXTHELP;
	}

	cs2.style &= ~WS_VISIBLE;

	BaseType::create(cs2);

	SetWindowLongPtr(handle(), DWLP_DLGPROC, (LPARAM)dialogProc);

	HWND hwndDefaultFocus = GetNextDlgTabItem(handle(), NULL, FALSE);
	if (sendMessage(WM_INITDIALOG, (WPARAM)hwndDefaultFocus)) {
		 sendMessage(WM_NEXTDLGCTL, (WPARAM)hwndDefaultFocus, TRUE);
	}
}

int ModalDialog::show() {
	auto root = getParent()->getRoot();
	if(!root || !root->getEnabled()) {
		::DestroyWindow(handle());
		return IDCANCEL;
	}

	root->setEnabled(false);

	setVisible(true);

	while(!quit) {
		if(!Application::instance().dispatch()) {
			quit |= !Application::instance().sleep();
		}
	}

	root->setEnabled(true);

	::DestroyWindow(handle());

	return ret;
}

bool ModalDialog::filter(MSG& msg) {
	if(::IsDialogMessage(handle(), &msg)) {
		return true;
	}
	return false;
}

void ModalDialog::kill() {
	// do nothing
}
}

