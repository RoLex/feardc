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

#include <dwt/widgets/FontDialog.h>

#include <dlgs.h>
#include <dwt/CanvasClasses.h>
#include <dwt/resources/Brush.h>
#include <dwt/util/win32/ApiHelpers.h>

namespace dwt {

FontDialog::FontDialog(Widget* parent) :
itsParent(parent)
{
}

UINT_PTR CALLBACK FontDialog::CFHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
	case WM_INITDIALOG:
		{
			/* this may seem like hacking around the resources of a dialog we don't own, but it is
			actually quite legit as the resources for the font dialog have officially been
			published (see Font.dlg in the MS SDK). */

			Options& options = *reinterpret_cast<Options*>(reinterpret_cast<LPCHOOSEFONT>(lParam)->lCustData);

			if(!options.strikeout) {
				// remove the "Strikeout" box
				HWND box = ::GetDlgItem(hwnd, chx1);
				::EnableWindow(box, FALSE);
				::ShowWindow(box, SW_HIDE);
			}

			if(!options.underline) {
				// remove the "Underline" box
				HWND box = ::GetDlgItem(hwnd, chx2);
				::EnableWindow(box, FALSE);
				::ShowWindow(box, SW_HIDE);
			}

			if(!options.color) {
				// remove the "Color" label
				HWND box = ::GetDlgItem(hwnd, stc4);
				::EnableWindow(box, FALSE);
				::ShowWindow(box, SW_HIDE);
				// remove the "Color" box
				box = ::GetDlgItem(hwnd, cmb4);
				::EnableWindow(box, FALSE);
				::ShowWindow(box, SW_HIDE);
			}

			if(options.bgColor != NaC) {
				// store the bg color; it will be used when processing WM_PAINT.
				::SetProp(hwnd, _T("bgColor"), &options.bgColor);
			}

			break;
		}

	case WM_PAINT:
		{
			HANDLE prop = ::GetProp(hwnd, _T("bgColor"));
			if(!prop)
				break;

			/* instead of creating a standard control to show the preview, Windows chooses to
			catch WM_PAINT and paint at the position of the preview control, which is actually
			hidden. our only solution to paint a background is therefore to reproduce that painting
			by ourselves. thanks to Wine (dlls/comdlg32/fontdlg.c) for showing how this is done. */

			PaintCanvas canvas { hwnd };
			auto bkMode(canvas.setBkMode(true));

			HWND box = ::GetDlgItem(hwnd, stc5);

			::RECT rc;
			::GetClientRect(box, &rc);
			::MapWindowPoints(box, hwnd, reinterpret_cast<LPPOINT>(&rc), 2);
			Rectangle rect { rc };

			canvas.fill(rect, Brush(*reinterpret_cast<COLORREF*>(prop)));

			HWND colorCombo = ::GetDlgItem(hwnd, cmb4);
			int i = ComboBox_GetCurSel(colorCombo);
			if(i != CB_ERR)
				canvas.setTextColor(ComboBox_GetItemData(colorCombo, i));

			LOGFONT logFont;
			::SendMessage(hwnd, WM_CHOOSEFONT_GETLOGFONT, 0, reinterpret_cast<LPARAM>(&logFont));
			Font font { logFont };
			auto select(canvas.select(font));

			canvas.drawText(util::win32::getWindowText(box), rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			return 1;
		}

	case WM_DESTROY:
		{
			::RemoveProp(hwnd, _T("bgColor"));
			break;
		}
	}
	return 0;
}

bool FontDialog::open(LOGFONT& font, COLORREF& color, Options* options, DWORD flags) {
	CHOOSEFONT cf = { sizeof(CHOOSEFONT) };

	cf.hwndOwner = getParentHandle();
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | flags;
	LOGFONT font_ = font;
	cf.lpLogFont = &font_;
	cf.rgbColors = color;
	if(options) {
		cf.Flags |= CF_ENABLEHOOK;
		cf.lCustData = reinterpret_cast<LPARAM>(options);
		cf.lpfnHook = CFHookProc;
	}

	if(::ChooseFont(&cf)) {
		font = *cf.lpLogFont;
		if(!options || options->color) {
			color = cf.rgbColors;
		}
		return true;
	}
	return false;
}

}
