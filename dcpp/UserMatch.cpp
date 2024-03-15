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

#include "stdinc.h"
#include "UserMatch.h"

#include "Client.h"
#include "FavoriteManager.h"
#include "OnlineUser.h"

namespace dcpp {

bool UserMatch::Rule::operator==(const Rule& rhs) const {
	return field == rhs.field && StringMatch::operator==(rhs);
}

void UserMatch::addRule(Rule&& rule) {
	if(rule.prepare()) {
		rules.push_back(move(rule));
	}
}

bool UserMatch::empty() const {
	return !isSet(FAVS) && !isSet(OPS) && !isSet(BOTS) && rules.empty();
}

bool UserMatch::match(OnlineUser& user) const {
	const auto& identity = user.getIdentity();

	if(isSet(FAVS) && !FavoriteManager::getInstance()->isFavoriteUser(identity.getUser())) {
		return false;
	}

	if(isSet(OPS) && !identity.isOp()) {
		return false;
	}

	if(isSet(BOTS) && !identity.isBot()) {
		return false;
	}

	for(auto& i: rules) {
		string str;
		switch(i.field) {
		case UserMatch::Rule::NICK: str = identity.getNick(); break;
		case UserMatch::Rule::CID: str = identity.getUser()->getCID().toBase32(); break;
		case UserMatch::Rule::IP: str = identity.getIp(); break;
		case UserMatch::Rule::HUB_ADDRESS: str = user.getClient().getHubUrl(); break;
		case UserMatch::Rule::FIELD_LAST: break;
		}
		if(!i.match(str)) {
			return false;
		}
	}

	return true;
}

} // namespace dcpp
