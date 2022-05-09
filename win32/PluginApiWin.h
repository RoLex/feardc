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

/** @file implement UI-specific functions of the plugin API. */

#ifndef DCPLUSPLUS_WIN32_PLUGINAPIWIN_H
#define DCPLUSPLUS_WIN32_PLUGINAPIWIN_H

#include <string>

#include <dcpp/PluginDefs.h>

using std::string;
namespace dwt { class Widget; }

class PluginApiWin {
public:
	static void init();

private:
	// Functions for DCUI
	static void DCAPI addCommand(const char* guid, const char* name, DCCommandFunc command, const char* icon);
	static void DCAPI removeCommand(const char* guid, const char* name);

	static void DCAPI playSound(const char* path);
	static void DCAPI notify(const char* title, const char* message);

	static DCUI dcUI;
};

struct PluginUtils {
	static void addPlugin(dwt::Widget* w);
	static void configPlugin(const string& guid, dwt::Widget* w);
	static void enablePlugin(const string& guid, dwt::Widget* w);
	static void disablePlugin(const string& guid, dwt::Widget*);
};

#endif
