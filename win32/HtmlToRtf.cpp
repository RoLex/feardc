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

// inspired by Bjarke Viksoe's Simple HTML Viewer <https://www.viksoe.dk/code/simplehtmlviewer.htm>.

#include "stdafx.h"
#include "HtmlToRtf.h"

#include <cmath>

#include <boost/algorithm/string/trim.hpp>

#include <dcpp/debug.h>
#include <dcpp/Flags.h>
#include <dcpp/ScopedFunctor.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/Text.h>

#include <dwt/util/GDI.h>
#include <dwt/widgets/RichTextBox.h>

struct Parser : SimpleXMLReader::CallBack {
	Parser(dwt::RichTextBox* box);
	void startTag(const string& name, StringPairList& attribs, bool simple);
	void data(const string& data);
	void endTag(const string& name);
	tstring finalize();

private:
	struct Context : Flags {
		enum { Bold = 1 << 0, Italic = 1 << 1, Underlined = 1 << 2 };
		size_t font; // index in the "fonts" table
		int fontSize;
		size_t textColor; // index in the "colors" table
		size_t bgColor; // index in the "colors" table
		string link;

		Context(dwt::RichTextBox* box, Parser& parser);

		tstring getBegin() const;
		tstring getEnd() const;
	};

	size_t addFont(string&& font);
	static int rtfFontSize(float px);
	size_t addColor(COLORREF color);

	void parseFont(const string& s);
	void parseColor(size_t& contextColor, const string& s);
	void parseDecoration(const string& s);

	tstring ret;

	StringList fonts;
	StringList colors;

	vector<Context> contexts;
};

tstring HtmlToRtf::convert(const string& html, dwt::RichTextBox* box) {
	Parser parser(box);
	try { SimpleXMLReader(&parser).parse(html); }
	catch(const SimpleXMLException& e) { return Text::toT(e.getError()); }
	return parser.finalize();
}

Parser::Parser(dwt::RichTextBox* box) {
	// create a default context with the Rich Edit control's current formatting.
	contexts.emplace_back(box, *this);
}

void Parser::startTag(const string& name_, StringPairList& attribs, bool simple) {
	auto name = boost::algorithm::trim_copy(name_);

	if(name == "br") {
		ret += _T("\\line\n");
	}

	if(simple) {
		return;
	}

	contexts.push_back(contexts.back());
	ScopedFunctor([this] { ret += contexts.back().getBegin(); });

	if(name == "b") {
		contexts.back().setFlag(Context::Bold);
	} else if(name == "i") {
		contexts.back().setFlag(Context::Italic);
	} else if(name == "u") {
		contexts.back().setFlag(Context::Underlined);
	}

	if(attribs.empty()) {
		return;
	}

	if(name == "a") {
		const auto& link = getAttrib(attribs, "href", 0);
		if(!link.empty()) {
			auto& context = contexts.back();
			context.link = link;
			context.setFlag(Context::Underlined);
			context.textColor = addColor(SETTING(LINK_COLOR)); /// @todo move to styles
		}
	}

	const auto& style = getAttrib(attribs, "style", 0);

	enum { Declaration, Font, Decoration, TextColor, BgColor, Unknown } state = Declaration;

	string tmp;
	size_t i = 0, j;
	while((j = style.find_first_of(":;", i)) != string::npos) {
		tmp = style.substr(i, j - i);
		i = j + 1;

		boost::algorithm::trim(tmp);

		switch(state) {
		case Declaration:
			{
				if(tmp == "font") { state = Font; }
				else if(tmp == "color") { state = TextColor; }
				else if(tmp == "text-decoration") { state = Decoration; }
				else if(tmp == "background-color") { state = BgColor; }
				else { state = Unknown; }
				break;
			}

		case Font:
			{
				parseFont(tmp);
				state = Declaration;
				break;
			}

		case Decoration:
			{
				parseDecoration(tmp);
				state = Declaration;
				break;
			}

		case TextColor:
			{
				parseColor(contexts.back().textColor, tmp);
				state = Declaration;
				break;
			}

		case BgColor:
			{
				parseColor(contexts.back().bgColor, tmp);
				state = Declaration;
				break;
			}

		case Unknown:
			{
				state = Declaration;
				break;
			}
		}
	}
}

void Parser::data(const string& data) {
	ret += dwt::RichTextBox::rtfEscape(Text::toT(Text::toDOS(data)));
}

void Parser::endTag(const string& name) {
	ret += contexts.back().getEnd();
	contexts.pop_back();
}

tstring Parser::finalize() {
	return Text::toT("{{\\fonttbl" + Util::toString(Util::emptyString, fonts) +
		"}{\\colortbl" + Util::toString(Util::emptyString, colors) + "}") + ret + _T("}");
}

Parser::Context::Context(dwt::RichTextBox* box, Parser& parser) {
	// create a default context with the Rich Edit control's current formatting.
	auto lf = box->getFont()->getLogFont();
	font = parser.addFont("\\fnil\\fcharset" + std::to_string(lf.lfCharSet) + " " + Text::fromT(lf.lfFaceName));
	fontSize = rtfFontSize(static_cast<float>(std::abs(lf.lfHeight)) / dwt::util::dpiFactor());
	if(lf.lfWeight >= FW_BOLD) { setFlag(Bold); }
	if(lf.lfItalic) { setFlag(Italic); }

	textColor = parser.addColor(box->getTextColor());
	bgColor = parser.addColor(box->getBgColor());
}

tstring Parser::Context::getBegin() const {
	string ret = "{";

	if(!link.empty()) {
		/* Wine doesn't support chat links so display them as plain text.
		 * See <https://bugs.winehq.org/show_bug.cgi?id=34824>. */
		if(SETTING(CLICKABLE_CHAT_LINKS)) {
			ret += "\\field{\\*\\fldinst HYPERLINK \"" + link + "\"}{\\fldrslt";
		} else {
			ret += "{";
		}
	}

	ret += "\\f" + std::to_string(font) + "\\fs" + std::to_string(fontSize) +
		"\\cf" + std::to_string(textColor) + "\\highlight" + std::to_string(bgColor);
	if(isSet(Bold)) { ret += "\\b"; }
	if(isSet(Italic)) { ret += "\\i"; }
	if(isSet(Underlined)) { ret += "\\ul"; }

	ret += " ";

	if(!link.empty()) {
		// add an invisible space; otherwise link formatting may get lost...
		ret += "{\\v  }";

		// Throw the link before its label when in the Wine workaround...
		if(!SETTING(CLICKABLE_CHAT_LINKS)) { ret += "<" + link + "> "; }
	}

	return Text::toT(ret);
}

tstring Parser::Context::getEnd() const {
	return link.empty() ? _T("}") : _T("}}");
}

size_t Parser::addFont(string&& font) {
	auto ret = fonts.size();
	fonts.push_back("{\\f" + std::to_string(ret) + move(font) + ";}");
	return ret;
}

int Parser::rtfFontSize(float px) {
	// the px value must not take DPI settings into account; the Rich Edit control handles that.
	return std::floor(px
		* 72.0 / 96.0 // px -> font points
		* 2.0); // RTF font sizes are expressed in half-points
}

size_t Parser::addColor(COLORREF color) {
	auto ret = colors.size();
	colors.push_back("\\red" + std::to_string(GetRValue(color)) +
		"\\green" + std::to_string(GetGValue(color)) +
		"\\blue" + std::to_string(GetBValue(color)) + ";");
	return ret;
}

void Parser::parseFont(const string& s) {
	// this contains multiple params separated by spaces.
	StringTokenizer<string> tok(s, ' ');
	auto& l = tok.getTokens();

	// remove empty strings.
	l.erase(std::remove_if(l.begin(), l.end(), [](const string& s) { return s.empty(); }), l.end());

	if(l.size() < 2) // the last 2 params (font size & font family) are compulsory.
		return;

	// the last param (font family) may contain spaces; merge if that is the case.
	while(*(l.back().end() - 1) == '\'' && (l.back().size() <= 1 || *l.back().begin() != '\'')) {
		*(l.end() - 2) += ' ' + std::move(l.back());
		l.erase(l.end() - 1);
		if(l.size() < 2)
			return;
	}

	// parse the last param (font family).
	/// @todo handle multiple fonts separated by commas...
	auto& family = l.back();
	family.erase(std::remove(family.begin(), family.end(), '\''), family.end());
	if(family.empty())
		return;
	contexts.back().font = addFont("\\fnil " + std::move(family));

	// parse the second to last param (font size).
	/// @todo handle more than px sizes
	auto& size = *(l.end() - 2);
	if(size.size() > 2 && *(size.end() - 2) == 'p' && *(size.end() - 1) == 'x') { // 16px
		contexts.back().fontSize = rtfFontSize(Util::toFloat(size.substr(0, size.size() - 2)));
	}

	// parse the optional third to last param (font weight).
	if(l.size() > 2 && Util::toInt(*(l.end() - 3)) >= FW_BOLD) {
		contexts.back().setFlag(Context::Bold);
	}

	// parse the optional first param (font style).
	if(l.size() > 2 && l[0] == "italic") {
		contexts.back().setFlag(Context::Italic);
	}
}

void Parser::parseDecoration(const string& s) {
	if(s == "underline") {
		contexts.back().setFlag(Context::Underlined);
	}
}

void Parser::parseColor(size_t& contextColor, const string& s) {
	auto sharp = s.find('#');
	if(sharp != string::npos && s.size() > sharp + 6) {
		try {
#if defined(__MINGW32__) && defined(_GLIBCXX_HAVE_BROKEN_VSWPRINTF)
			/// @todo use stol on MinGW when it's available
			unsigned int color = 0;
			auto colStr = s.substr(sharp + 1, 6);
			sscanf(colStr.c_str(), "%X", &color);
#else
			size_t pos = 0;
			auto color = std::stol(s.substr(sharp + 1, 6), &pos, 16);
#endif
			contextColor = addColor(RGB((color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF));
		} catch(const std::exception& e) { dcdebug("color parsing exception: %s with str: %s\n", e.what(), s.c_str()); }
	}
}
