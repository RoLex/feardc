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

#ifndef DCPLUSPLUS_WIN32_FAV_HUB_PROPERTIES_H
#define DCPLUSPLUS_WIN32_FAV_HUB_PROPERTIES_H

#include <dcpp/forward.h>
#include <dcpp/Text.h>

#include "GridDialog.h"

#include <map>

class FavHubProperties : public GridDialog
{
public:
	FavHubProperties(dwt::Widget* parent, FavoriteHubEntry *_entry);
	virtual ~FavHubProperties();

private:
	TextBoxPtr name;
	TextBoxPtr address;
	TextBoxPtr hubDescription;
	TextBoxPtr nick;
	TextBoxPtr password;
	TextBoxPtr description;
	TextBoxPtr email;
	TextBoxPtr userIp;
	TextBoxPtr userIp6;
	ComboBoxPtr encoding;
	ComboBoxPtr showJoins;
	ComboBoxPtr favShowJoins;
	ComboBoxPtr logMainChat;
	ComboBoxPtr groups;

	FavoriteHubEntry *entry;

	bool handleInitDialog();
	void handleGroups();
	void handleOKClicked();

	void fillGroups();

	void fillEncodings();

	static std::map<UINT, std::wstring> encodings;
	static BOOL CALLBACK EnumCodePageProc(LPTSTR lpCodePageString);
};

#endif // !defined(DCPLUSPLUS_WIN32_FAV_HUB_PROPERTIES_H)
