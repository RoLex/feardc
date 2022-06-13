/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "ChatMessage.h"
#include "Client.h"
#include "format.h"
#include "Magnet.h"
#include "OnlineUser.h"
#include "PluginManager.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "Tagger.h"
#include "Util.h"

namespace dcpp {

ChatMessage::ChatMessage(const string& text, OnlineUser* from,
	const OnlineUser* to, const OnlineUser* replyTo,
	bool thirdPerson, time_t messageTimestamp) :
from(from->getUser()),
to(to ? to->getUser() : nullptr),
replyTo(replyTo ? replyTo->getUser() : nullptr),
timestamp(time(0)),
thirdPerson(thirdPerson),
messageTimestamp(messageTimestamp)
{
	/* format the message to plain text and HTML strings before handing them over to plugins for
	further	processing. */

	string tmp;
	string xmlTmp;

	auto addSpan = [&xmlTmp](string id, const string& content, const string& style) -> string {
		string ret = "<span id=\"" + move(id) + "\"";
		if(!style.empty()) { ret += " style=\"" + SimpleXML::escape(style, xmlTmp, true) + "\""; }
		ret += ">" + SimpleXML::escape(content, xmlTmp, false) + "</span>";
		return ret;
	};

	htmlMessage += "<span id=\"message\" style=\"white-space: pre-wrap;\">";

	if (SETTING(TIME_STAMPS)) {
		tmp = "[" + Util::getShortTimeString(timestamp);

		if (!from->getIdentity().isHub() && !from->getIdentity().isBot() && from->getIdentity().getIp().size()) {
			if (SETTING(SHOW_USER_IP))
				tmp += " | " + from->getIdentity().getIp();

			if (SETTING(SHOW_USER_COUNTRY) && from->getIdentity().getCountry().size())
				tmp += " | " + from->getIdentity().getCountry();
		}

		tmp += "]";
		htmlMessage += addSpan("timestamp", tmp, Util::emptyString) + " ";
	}

	if(messageTimestamp) {
		tmp = "["; tmp += str(F_("Sent %1%") % Util::getShortTimeString(messageTimestamp)); tmp += "]";
		message += tmp + " ";
		htmlMessage += addSpan("messageTimestamp", tmp, Util::emptyString) + " ";
	}

	tmp = from->getIdentity().getNick();
	// let's *not* obey the spec here and add a space after the star. :P
	tmp = thirdPerson ? "* " + tmp + " " : "<" + tmp + ">";
	message += tmp + " ";

	auto style = from->getIdentity().getStyle();
	string styleAttr;
	if(!style.font.empty()) { styleAttr += "font: " + Util::cssFont(style.font) + ";"; }
	if(style.textColor != -1) { styleAttr += "color: #" + Util::cssColor(style.textColor) + ";"; }
	if(style.bgColor != -1) { styleAttr += "background-color: #" + Util::cssColor(style.bgColor) + ";"; }
	htmlMessage += addSpan("nick", tmp, styleAttr) + " ";

	// Check all '<' and '[' after newlines as they're probably pastes...
	tmp = text;
	size_t i = 0;
	while((i = tmp.find('\n', i)) != string::npos) {
		++i;
		if(i < tmp.size()) {
			if(tmp[i] == '[' || tmp[i] == '<') {
				tmp.insert(i, "- ");
				i += 2;
			}
		}
	}

	message += tmp;

	/* format the message; this will involve adding custom tags. use the Tagger class to that end. */
	Tagger tags(move(tmp));
	format(tags, xmlTmp);

	// let plugins play with the tag list
	PluginManager::getInstance()->onChatTags(tags, from);

	htmlMessage += "<span id=\"text\">" + tags.merge(xmlTmp) + "</span></span>";

	// forward to plugins
	PluginManager::getInstance()->onChatDisplay(htmlMessage, from);
}

namespace { inline bool validSchemeChar(char c, bool first) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		(!first && c >= '0' && c <= '9') ||
		(!first && (c == '+' || c == '.' || c == '-'));
} }

void ChatMessage::format(Tagger& tags, string& tmp) {
	const auto& text = tags.getText();

	/* link formatting - optimize the lookup a bit by using the fact that every link identifier
	(except www ones) contains a colon. */

	auto addLinkStr = [&tmp, &tags](size_t begin, size_t end, const string& link) {
		tags.addTag(begin, end, "a", "href=\"" + SimpleXML::escape(link, tmp, true) + "\"");
	};

	auto addLink = [&text, &addLinkStr](size_t begin, size_t end) {
		addLinkStr(begin, end, text.substr(begin, end - begin));
	};

	static const string delimiters = " \t\r\n<>\"";

	size_t i = 0, begin, end, n = text.size();
	while((i = text.find(':', i)) != string::npos) {

		// get the left bound; make sure it's a valid scheme according to RFC 3986, section 3.1
		begin = i;
		while(begin > 0 && validSchemeChar(text[begin - 1], false)) { --begin; }
		while(begin < i && !validSchemeChar(text[begin], true)) { ++begin; }
		if(begin == i) { ++i; continue; }

		// get the right bound
		if((end = text.find_first_of(delimiters, i + 1)) == string::npos) end = n;

		if(i > 0 && (
			(i + 4 < n && text[i + 1] == '/' && text[i + 2] == '/') || // "http://", etc
			(i == begin + 6 && i + 1 <= n && !text.compare(begin, 6, "mailto"))))
		{
			addLink(begin, end);
			i = end;

		} else if(i == begin + 6 && i + 2 <= n && !text.compare(begin, 6, "magnet") && text[i + 1] == '?') {
			string link = text.substr(begin, end - begin), hash, name, key;
			if(Magnet::parseUri(link, hash, name, key)) {

				if(!name.empty()) {
					// magnet link: replace with the friendly name
					name += " (magnet)";
					tags.replaceText(begin, end, name);

					// the size of the string has changed; update counts.
					const auto delta = static_cast<int>(name.size()) - static_cast<int>(link.size());
					end += delta;
					n += delta;
				}

				addLinkStr(begin, end, link);
				i = end;

			} else {
				++i;
			}

		} else {
			++i;
		}
	}

	// check for www links.
	i = 0;
	while((i = text.find("www.", i)) != string::npos) {
		if(i + 5 <= n && (i == 0 || delimiters.find(text[i - 1]) != string::npos)) {
			if((end = text.find_first_of(delimiters, i + 4)) == string::npos) end = n;
			if(i + 5 <= end) {
				addLink(i, end);
				i = end;
				continue;
			}
		}
		i += 5;
	}
}

} // namespace dcpp
