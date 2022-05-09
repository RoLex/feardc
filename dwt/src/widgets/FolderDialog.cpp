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

#include <dwt/widgets/FolderDialog.h>

namespace dwt {

FolderDialog::FolderDialog(Widget* parent) :
parent(parent),
pidlRoot(nullptr),
pidlInitialSel(nullptr)
{
}

FolderDialog::~FolderDialog() {
	if(pidlRoot) {
		::CoTaskMemFree(pidlRoot);
	}
}

FolderDialog& FolderDialog::setRoot(const int csidl) {
	SHGetSpecialFolderLocation(getParentHandle(), csidl, &pidlRoot);
	return *this;
}

FolderDialog& FolderDialog::setTitle(const tstring& title) {
	this->title = title;
	return *this;
}

FolderDialog& FolderDialog::setInitialSelection(const tstring& sel) {
	initialSel = sel;
	return *this;
}

FolderDialog& FolderDialog::setInitialSelection(const int csidl) {
	SHGetSpecialFolderLocation(getParentHandle(), csidl, &pidlInitialSel);
	return *this;
}

bool FolderDialog::open(tstring& dir) {
	if(!dir.empty())
		setInitialSelection(dir);

	BROWSEINFO bws = { getParentHandle(), pidlRoot };
	if(!title.empty()) {
		bws.lpszTitle = title.c_str();
	}
	bws.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_EDITBOX;
	if(!initialSel.empty() || pidlInitialSel) {
		bws.lpfn = &browseCallbackProc;
		bws.lParam = reinterpret_cast<LPARAM>(this);
	}

	// Avoid errors about missing cdroms, floppies etc..
	UINT oldErrorMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);

	LPITEMIDLIST lpIDL = SHBrowseForFolder( & bws );

	::SetErrorMode(oldErrorMode);

	bool ret = false;

	if(lpIDL) {
		TCHAR buf[MAX_PATH + 1];
		if(::SHGetPathFromIDList(lpIDL, buf)) {
			dir = buf;
			if(!dir.empty() && *(dir.end() - 1) != _T('\\')) {
				dir += _T('\\');
			}
			ret = true;
		}

		::CoTaskMemFree(lpIDL);
	}

	return ret;
}

int CALLBACK FolderDialog::browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM lpData) {
	if(lpData && uMsg == BFFM_INITIALIZED) {
		auto& dialog = *reinterpret_cast<FolderDialog*>(lpData);
		auto wparam = dialog.initialSel.empty() ? FALSE : TRUE;
		auto lparam = wparam ? reinterpret_cast<LPARAM>(dialog.initialSel.c_str()) :
			reinterpret_cast<LPARAM>(dialog.pidlInitialSel);
		::SendMessage(hwnd, BFFM_SETSELECTION, wparam, lparam);
		::SendMessage(hwnd, BFFM_SETEXPANDED, wparam, lparam);
	}
	return 0;
}

}
