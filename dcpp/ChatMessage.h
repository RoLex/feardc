/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_CHAT_MESSAGE_H
#define DCPLUSPLUS_DCPP_CHAT_MESSAGE_H

#include "forward.h"

#include <string>

namespace dcpp {

using std::string;

/** This class holds information about one chat message. It is sent with a ClientListener::Message
notification so information about the hub itself should be known.
On creation, the class reads message information it has received from the underlying protocol and
formats it in 2 ways:
- One plain text string, suitable for logs, tooltips, etc.
- One HTML string. The tags in the HTML string are limited to those marked as "phrasing content" in
the HTML5 spec. Each part of the message (timestamp, nick, etc) is enclosed in a <span> tag in
order to facilitate identification of the part.
DC++ formats the strings first according to settings, then hands them over to plugins for further
processing. */
struct ChatMessage {
	ChatMessage(const string& text, OnlineUser* from,
		const OnlineUser* to = nullptr, const OnlineUser* replyTo = nullptr,
		bool thirdPerson = false, time_t messageTimestamp = 0);

	/** Plain text message. */
	string message;
	/** HTML representation of the message. */
	string htmlMessage;

	UserPtr from;
	UserPtr to;
	UserPtr replyTo;

	/** Time when this structure was created (ie when the message was received by DC++). */
	time_t timestamp;

	/** [ADC only] Whether the message has been written in third person mode. */
	bool thirdPerson;
	/** [ADC only] Time when the message was initially sent. */
	time_t messageTimestamp;

	/** Store context-agnostic formattings that can be applied to the given message in the tagger.
	Note that the string may be modified. */
	static void format(Tagger& tags, string& tmp);
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_CHAT_MESSAGE_H)
