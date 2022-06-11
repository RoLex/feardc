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

#ifndef DCPLUSPLUS_WIN32_CONNECTIVITY_PAGE_H
#define DCPLUSPLUS_WIN32_CONNECTIVITY_PAGE_H

#include <dcpp/ConnectivityManager.h>

#include "PropPage.h"

class ConnectivityPage : public PropPage, private ConnectivityManagerListener
{
public:
	ConnectivityPage(dwt::Widget* parent);
	virtual ~ConnectivityPage();

private:
	ItemList items;

	CheckBoxPtr autoDetect;
	ButtonPtr detectNow;
	RichTextBoxPtr log;
	ButtonPtr edit;

	enum State {
		STATE_UNKNOWN,
		STATE_DETECTING,
		STATE_FAILED,
		STATE_SUCCEED
	};

	void handleAutoClicked();
	void handleEdit();

	void updateAuto();
	void addLogLine(const tstring& msg);

	// ConnectivityManagerListener
	void on(Message, const string& message) noexcept;
	void on(Started) noexcept;
	void on(Finished) noexcept;
	void on(SettingChanged) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_CONNECTIVITY_PAGE_H)
