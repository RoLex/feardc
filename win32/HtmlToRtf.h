/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_HTML_TO_RTF_H
#define DCPLUSPLUS_WIN32_HTML_TO_RTF_H

#include <dcpp/typedefs.h>

#include <dwt/forward.h>

/** Convert an HTML string to an RTF string, suitable for insertion within a Rich Edit control.
Only simple HTML tags (those that are marked as "phrasing content" in the HTML5 spec) are
supported. */
class HtmlToRtf {
public:
	static tstring convert(const string& html, dwt::RichTextBox* box);
};

#endif
