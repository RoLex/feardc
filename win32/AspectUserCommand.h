/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_USER_COMMAND_H
#define DCPLUSPLUS_WIN32_USER_COMMAND_H

#include <dcpp/FavoriteManager.h>
#include <dwt/widgets/Menu.h>

#include "resource.h"

using dwt::Menu;

template<class T>
class AspectUserCommand {
public:
	AspectUserCommand() { }
	virtual ~AspectUserCommand() { }

	void prepareMenu(Menu* menu, int ctx, const string& hubUrl) {
		prepareMenu(menu, ctx, StringList(1, hubUrl));
	}

	void prepareMenu(Menu* menu, int ctx, const StringList& hubs) {
		userCommands = FavoriteManager::getInstance()->getUserCommands(ctx, hubs);

		if(!userCommands.empty()) {
			menu->appendSeparator();
			auto cur = menu;
			for(size_t n = 0; n < userCommands.size(); ++n) {
				UserCommand* uc = &userCommands[n];

				if(uc->getType() == UserCommand::TYPE_SEPARATOR) {
					// Avoid double separators...
					auto count = cur->size();
					if(count > 0 && !cur->isSeparator(count - 1u)) {
						cur->appendSeparator();
					}

				} else if(uc->isRaw() || uc->isChat()) {
					cur = menu;

					for(auto i = uc->getDisplayName().begin(), iend = uc->getDisplayName().end(); i != iend; ++i) {
						tstring name = Text::toT(*i);
						if(i + 1 == iend) {
							cur->appendItem(name, [=] { static_cast<T*>(this)->runUserCommand(*uc); });
						} else {
							bool found = false;
							// Let's see if we find an existing item...
							for(size_t k = 0; k < cur->size(); k++) {
								if(cur->isPopup(k) && Util::stricmp(cur->getText(k), name) == 0) {
									found = true;
									cur = cur->getChild(k);
								}
							}
							if(!found) {
								cur = cur->appendPopup(name);
							}
						}
					}
				}
			}
		}
	}
private:
	UserCommand::List userCommands;
};

#endif // !defined(UC_HANDLER_H)
