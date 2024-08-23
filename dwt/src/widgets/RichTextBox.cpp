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

#include <dwt/widgets/RichTextBox.h>

#include <algorithm>
#include <cmath>

#include <boost/lambda/lambda.hpp>
#include <boost/format.hpp>
#include <boost/scoped_array.hpp>

#include <dwt/Point.h>
#include <dwt/util/check.h>
#include <dwt/util/HoldRedraw.h>

#include <dwt/LibraryLoader.h>

/// @todo remove when MinGW has these - defined here instead of GCCHeaders.h because they're from richedit.h
#ifndef MSFTEDIT_CLASS
#define MSFTEDIT_CLASS L"RICHEDIT50W"
#endif
#ifndef CFM_BACKCOLOR
#define CFM_BACKCOLOR 0x04000000
#endif
#ifndef EM_SETLANGOPTIONS
#define EM_SETLANGOPTIONS (WM_USER + 120)
#endif
#ifndef EM_GETLANGOPTIONS
#define EM_GETLANGOPTIONS (WM_USER + 121)
#endif
#ifndef IMF_AUTOKEYBOARD
#define IMF_AUTOKEYBOARD 0x0001
#endif

namespace dwt {

const TCHAR RichTextBox::windowClass[] = MSFTEDIT_CLASS;

RichTextBox::Seed::Seed() :
	BaseType::Seed(WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL),
	font(0),
	scrollBarHorizontallyFlag(false),
	scrollBarVerticallyFlag(false),
	events(ENM_LINK)
{
}

Dispatcher& RichTextBox::makeDispatcher() {
	// msftedit is the DLL containing Rich Edit 4.1, available from XP SP1 onwards.
	static LibraryLoader richEditLibrary(_T("msftedit.dll"));
	return ChainingDispatcher::superClass<RichTextBox>();
}

void RichTextBox::create(const Seed& cs) {
	BaseType::create(cs);
	setFont(cs.font);

	setColor(Color::predefined(COLOR_WINDOWTEXT), Color::predefined(COLOR_WINDOW));

	setScrollBarHorizontally(cs.scrollBarHorizontallyFlag);
	setScrollBarVertically(cs.scrollBarVerticallyFlag);
	sendMessage(EM_SETEVENTMASK, 0, cs.events);
	sendMessage(EM_AUTOURLDETECT, FALSE);

	/* after special chars are added to the control, it sets the IMF_AUTOKEYBOARD flag which
	results in the Win keyboard language switching. the fix is to remove that flag when the control
	gains focus (which is when the presence of the flag actually matters). */
	onFocus([this] {
		auto opts = sendMessage(EM_GETLANGOPTIONS);
		if((opts & IMF_AUTOKEYBOARD) == IMF_AUTOKEYBOARD)
			sendMessage(EM_SETLANGOPTIONS, 0, opts & ~IMF_AUTOKEYBOARD);
	});

	/* unlike other common controls, Rich Edits ignore WM_PRINTCLIENT messages. as per
	<https://msdn.microsoft.com/en-us/library/bb787875(VS.85).aspx>, we have to handle the printing
	by ourselves. this is crucial for taskbar thumbnails and "Aero Peek" previews. */
	onPrinting([this](Canvas& canvas) {
		Rectangle rect { getClientSize() };

		// paint a background in case the text doesn't span the whole box.
		canvas.fill(rect, Brush(bgColor));

		FORMATRANGE format { canvas.handle(), canvas.handle() };
		format.rc = rect;
		format.rc.bottom += std::abs(getFont()->getLogFont().lfHeight); // make room for the last line
		// convert to twips and respect DPI settings.
		format.rc.right *= 1440 / canvas.getDeviceCaps(LOGPIXELSX);
		format.rc.bottom *= 1440 / canvas.getDeviceCaps(LOGPIXELSY);
		format.rcPage = format.rc;

		// find the first fully visible line (sometimes they're partially cut).
		bool found = false;
		for(long line = getFirstVisibleLine(), n = getLineCount(); line < n; ++line) {
			format.chrg.cpMin = lineIndex(line);
			if(posFromChar(format.chrg.cpMin).y >= 0) {
				found = true;
				break;
			}
		}
		if(!found) {
			format.chrg.cpMin = 0;
		}
		format.chrg.cpMax = -1;

		sendMessage(EM_FORMATRANGE, 1, reinterpret_cast<LPARAM>(&format));
		sendMessage(EM_FORMATRANGE); // "free the cached information" as MSDN recommends.
	});
}

RichTextBox::RichTextBox(dwt::Widget* parent) :
TextBoxBase(parent, makeDispatcher())
{
}

inline int RichTextBox::charFromPos(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	// Unlike edit control: "The return value specifies the zero-based character index of the character
	// nearest the specified point. The return value indicates the last character in the edit control if the
	// specified point is beyond the last character in the control."
	POINTL lp;
	lp.x = cc.x();
	lp.y = cc.y();
	return sendMessage(EM_CHARFROMPOS, 0, (LPARAM)&lp);
}

inline Point RichTextBox::posFromChar(int charOffset)
{
	POINTL pt;
	sendMessage(EM_POSFROMCHAR, (WPARAM)&pt, (LPARAM)charOffset);
	return Point(pt.x, pt.y);
}

inline int RichTextBox::lineFromPos(const ScreenCoordinate& pt) {
	return lineFromChar(charFromPos(pt));
}

inline int RichTextBox::lineFromChar(int c) {
	return sendMessage(EM_EXLINEFROMCHAR, 0, c);
}

tstring RichTextBox::getSelection() const {
	std::pair<int, int> range = getCaretPosRange();

	DWORD size = ((range.second - range.first) * sizeof(TCHAR) * 2) + 1; // Double buf size due to possible CRLFs

	if (size < 2)
		return tstring();

	boost::scoped_array< TCHAR > buf(new TCHAR[size]);

	// This gets text with consistent line endigs and without rtf hidden control fields content across
	// all modern Windows versions. 
	// Wine doesn't support GT_NOHIDDENTEXT as of 2024.05 but nor does those rtf fields so there's nothing to avoid...
	GETTEXTEX gte { size, GT_SELECTION | GT_NOHIDDENTEXT | GT_USECRLF, 1200, NULL, NULL };
	sendMessage(EM_GETTEXTEX, reinterpret_cast< WPARAM >(&gte), reinterpret_cast< LPARAM >(buf.get()));

	return buf.get();
}

Point RichTextBox::getScrollPos() const {
	POINT scrollPos;
	sendMessage(EM_GETSCROLLPOS, 0, reinterpret_cast< LPARAM >(&scrollPos));
	return Point(scrollPos);
}

void RichTextBox::setScrollPos(Point& scrollPos) {
	sendMessage(EM_SETSCROLLPOS, 0, reinterpret_cast< LPARAM >(&scrollPos));
}

tstring RichTextBox::textUnderCursor(const ScreenCoordinate& p, bool includeSpaces) {
	tstring tmp = getText();

	tstring::size_type start = tmp.find_last_of(includeSpaces ? _T("<\t\r\n") : _T(" <\t\r\n"),
		fixupLineEndings(tmp.begin(), tmp.end(), charFromPos(p)));
	if(start == tstring::npos)
		start = 0;
	else
		start++;

	tstring::size_type end = tmp.find_first_of(includeSpaces ? _T(">\t\r\n") : _T(" >\t\r\n"), start + 1);
	if(end == tstring::npos)
		end = tmp.size();

	return tmp.substr(start, end - start);
}

int RichTextBox::fixupLineEndings(tstring::const_iterator begin, tstring::const_iterator end, tstring::difference_type ibo) const {
	// https://web.archive.org/web/20140518113109/http://rubyforge.org/pipermail/wxruby-users/2006-August/002116.html
	// ("TE_RICH2 RichEdit control"). Otherwise charFromPos will be increasingly
	// off from getText with each new line by one character.
	// @todo test whether this fixup is still needed for currently supported Windows versions and under Wine.
	int cur = 0;
	return std::find_if(begin, end, (cur += (boost::lambda::_1 != static_cast< TCHAR >('\r')),
		boost::lambda::var(cur) > ibo)) - begin;
}

void RichTextBox::setTextA(const std::string& txt) {
	{
		util::HoldRedraw hold { this };
		setTextEx(txt, ST_DEFAULT);
		sendMessage(WM_VSCROLL, SB_TOP);
	}
	redraw();
}

void RichTextBox::setTextEx(const std::string& txt, DWORD flags) {
	SETTEXTEX config = { flags, CP_ACP };
	sendMessage(EM_SETTEXTEX, reinterpret_cast<WPARAM>(&config), reinterpret_cast<LPARAM>(txt.c_str()));
}

void RichTextBox::setText(const tstring& txt) {
	setTextA("{\\urtf " + escapeUnicode(txt) + "}");
}

void RichTextBox::addText(const std::string & txt) {
	setSelection(-1, -1);
	setTextEx(txt, ST_SELECTION);
}

void RichTextBox::addTextSteady(const tstring& txtRaw) {
	HoldScroll hold { this };

	std::pair<int, int> cr = getCaretPosRange();
	std::string txt = escapeUnicode(txtRaw);

	int charsToRemove = 0;

	/* this will include more chars than there actually are because of RTF codes. not a problem
	here; accuracy isn't necessary since whole lines are getting chopped anyway. */
	size_t prevLen = length();
	size_t addedLen = txtRaw.size();
	size_t limit = getTextLimit();

	if(prevLen + addedLen > limit) {
		/* adding text would overflow the char limit. -> remove some (or all) existing lines. */

		if(addedLen >= limit) {
			/* adding more text than the box can contain. -> remove all previous lines. */
			charsToRemove = prevLen;
			hold.scroll = true;

		} else {
			/* the text being added fits within the box, but not when appended to current contents.
			 * -> find out how many lines have to be removed. we try from 10 % to 80 % of the text
			 *    limit. */
			for(auto multiplier = 1; multiplier <= 8; ++multiplier) {
				auto charsToDivLimit = lineIndex(lineFromChar(limit * multiplier / 10));
				if(charsToDivLimit >= 0 && prevLen + addedLen - charsToDivLimit < limit) {
					/* good, got enough room for the new text! */
					charsToRemove = charsToDivLimit;
					if(!hold.scroll) {
						/* when not in auto-scroll mode, adjust the scroll pos to stay on the same
						 * line, despite removing some at the top of the box. adjust based on the
						 * origin of the box (posFromChar 0) as posFromChar coordinates are
						 * relative to the current scroll pos (we want coordinates relative to the
						 * origin of the box). */
						hold.scrollPos.y -= posFromChar(charsToRemove).y - posFromChar(0).y;
					}
					break;
				}
			}
			if(charsToRemove <= 0) {
				/* clearing out 80 % of the current text would not be enough. -> remove all
				 * previous lines. */
				charsToRemove = prevLen;
				hold.scroll = true;
			}
		}

		setSelection(0, charsToRemove);
		replaceSelection(_T(""));
	}

	addText(txt);
	setSelection(cr.first - charsToRemove, cr.second - charsToRemove);
}

void RichTextBox::findText(tstring const& needle) {
	// The code here is slightly longer than a pure getText/STL approach
	// might allow, but it also ducks entirely the line-endings debacle.
	int max = length();

	// a new search? reset cursor to bottom
	if(needle != currentNeedle || currentNeedlePos == -1) {
		currentNeedle = needle;
		currentNeedlePos = max;
	}

	// set current selection
	FINDTEXT ft = { {currentNeedlePos, 0}, NULL };	// reversed
	tstring::size_type len = needle.size();
	boost::scoped_array< TCHAR > needleCTstr( new TCHAR[len+1] );
	needleCTstr[len] = 0;
	copy(needle.begin(), needle.begin()+len, needleCTstr.get());
	ft.lpstrText = needleCTstr.get();

	// empty search? stop
	if(needle.empty())
		return;

	// find upwards
	currentNeedlePos = sendMessage(EM_FINDTEXTW, 0, reinterpret_cast< LPARAM >(&ft));

	// not found? try again on full range
	if(currentNeedlePos == -1 && ft.chrg.cpMin != max) { // no need to search full range twice
		currentNeedlePos = max;
		ft.chrg.cpMin = currentNeedlePos;
		currentNeedlePos = sendMessage(EM_FINDTEXTW, 0, reinterpret_cast< LPARAM >(&ft));
	}

	// found? set selection
	if(currentNeedlePos != -1) {
		ft.chrg.cpMin = currentNeedlePos;
		ft.chrg.cpMax = currentNeedlePos + needle.length();
		setFocus();
		sendMessage(EM_EXSETSEL, 0, reinterpret_cast< LPARAM >(&ft.chrg));
	} else {
#ifdef PORT_ME
		addStatus(T_("String not found: ") + needle);
#endif
		clearCurrentNeedle();
	}
}

void RichTextBox::clearCurrentNeedle()
{
	currentNeedle.clear();
}

std::string RichTextBox::unicodeEscapeFormatter(const tstring_range& match) {
	if(match.empty())
		return std::string();
	return (boost::format("\\ud\\u%dh")%(int(*match.begin()))).str();
}

std::string RichTextBox::escapeUnicode(const tstring& str) {
	std::string ret;
	boost::find_format_all_copy(std::back_inserter(ret), str,
		boost::first_finder(L"\x7f", std::greater<TCHAR>()), unicodeEscapeFormatter);
	return ret;
}

tstring RichTextBox::rtfEscapeFormatter(const tstring_range& match) {
	if(match.empty())
		return tstring();
	tstring s(1, *match.begin());
	if (s == _T("\r")) return _T("");
	if (s == _T("\n")) return _T("\\line\n");
	return _T("\\") + s;
}

tstring RichTextBox::rtfEscape(const tstring& str) {
	using boost::lambda::_1;
	tstring escaped;
	boost::find_format_all_copy(std::back_inserter(escaped), str,
		boost::first_finder(L"\x7f", _1 == '{' || _1 == '}' || _1 == '\\' || _1 == '\n' || _1 == '\r'), rtfEscapeFormatter);
	return escaped;
}

void RichTextBox::updateColors(COLORREF text, COLORREF background, bool updateFont) {
	util::HoldRedraw hold { this };

	/* when changing the global formatting of a Rich Edit, its per-character formatting properties
	may become funky depending on how they were initially set (the RTF context depth, for example,
	seems to matter). the solution is to gather colors beforehand and re-apply them after the
	global changes. */
	std::vector<CHARFORMAT2> formats(length());
	size_t sel = 0;
	for(auto& format: formats) {
		setSelection(sel, sel + 1);
		++sel;
		format.cbSize = sizeof(CHARFORMAT2);
		sendMessage(EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
	}

	if(updateFont) {
		BaseType::setFontImpl();
	}

	// change the global text color.
	auto prevText = textColor;
	textColor = text;
	CHARFORMAT textFormat = { sizeof(CHARFORMAT), CFM_COLOR };
	textFormat.crTextColor = textColor;
	sendMessage(EM_SETCHARFORMAT, SCF_DEFAULT, reinterpret_cast<LPARAM>(&textFormat));

	// change the global background color.
	auto prevBg = bgColor;
	bgColor = background;
	sendMessage(EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(bgColor));

	// restore custom colors.
	sel = 0;
	for(auto& format: formats) {
		format.dwMask &= CFM_COLOR | CFM_BACKCOLOR | CFM_LINK | CFM_UNDERLINE;
		if(format.dwMask) {
			if(format.crTextColor == prevText) { format.crTextColor = textColor; }
			if(format.crBackColor == prevBg) { format.crBackColor = bgColor; }
			setSelection(sel, sel + 1);
			sendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
		}
		++sel;
	}
}

void RichTextBox::setColorImpl(COLORREF text, COLORREF background) {
	if(text != textColor || background != bgColor) {
		updateColors(text, background);
		redraw();
	}
}

void RichTextBox::setFontImpl() {
	// changing the default font resets default colors.
	updateColors(textColor, bgColor, true);
	redraw();
}

bool RichTextBox::handleMessage(const MSG& msg, LRESULT& retVal) {
	bool handled = BaseType::handleMessage(msg, retVal);

	/* when scrolling downwards, the content of the box sometimes scrolls up too far, leaving blank
	space below the last line. this behavior seems to be specific to Rich Edit 4.1. */
	if(msg.message == WM_VSCROLL && msg.wParam == SB_PAGEDOWN) {
		retVal = getDispatcher().chain(msg);
		if(scrollIsAtEnd())
			scrollToBottom();
		return true;
	}

	return handled;
}

RichTextBox::HoldScroll::HoldScroll(RichTextBox* box) : box(box) {
	scrollPos = box->getScrollPos();
	scroll = box->scrollIsAtEnd();

	box->sendMessage(WM_SETREDRAW, FALSE);
}

RichTextBox::HoldScroll::~HoldScroll() {
	if(scroll) {
		box->scrollToBottom();
	} else {
		box->setScrollPos(scrollPos);
	}

	box->sendMessage(WM_SETREDRAW, TRUE);
	box->redraw();
}

}
