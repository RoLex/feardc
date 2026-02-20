/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

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

#ifndef DWT_ASPECTDIALOG_H_
#define DWT_ASPECTDIALOG_H_

#include "../tstring.h"
#include "../WidgetCreator.h"

#include <type_traits>

namespace dwt { namespace aspects {

template<typename WidgetType>
class Dialog {
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	HWND H() { return W().handle(); }

public:
	HWND getItem(int id) {
		return ::GetDlgItem(H(), id);
	}

	void setItemText(int id, const tstring& text) {
		::SetDlgItemText(H(), id, text.c_str());
	}

	template<typename T>
	void attachChild(T& childPtr, int id) {
		childPtr = attachChild<typename std::remove_pointer<T>::type >(id);
	}

	template<typename T>
	typename T::ObjectType attachChild(int id) {
		return WidgetCreator<T>::attach(&W(), id);
	}

protected:
	static INT_PTR CALLBACK dialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

/**
 * Dummy dialog procedure - we superclass the dialog window class and handle the message
 * loop outside of the dialog box procedure.
 *
 * This is similar to https://blogs.msdn.microsoft.com/oldnewthing/20031113-00/?p=41843
 */
template<typename WidgetType>
INT_PTR CALLBACK Dialog<WidgetType>::dialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return FALSE;
}

} }

#endif /*ASPECTDIALOG_H_*/
