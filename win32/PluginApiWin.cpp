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

#include "stdafx.h"
#include "PluginApiWin.h"

#include <dcpp/format.h>
#include <dcpp/PluginManager.h>
#include <dcpp/Text.h>
#include <dcpp/Util.h>
#include <dcpp/version.h>

#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/MessageBox.h>

#include "MainWindow.h"
#include "PluginInfoDlg.h"
#include "WinUtil.h"

DCUI PluginApiWin::dcUI = {
	DCINTF_DCPP_UI_VER,

	&PluginApiWin::addCommand,
	&PluginApiWin::removeCommand,

	&PluginApiWin::playSound,
	&PluginApiWin::notify
};

void PluginApiWin::init() {
	PluginManager::getInstance()->registerInterface(DCINTF_DCPP_UI, &dcUI);
}

// Functions for DCUI
void PluginApiWin::addCommand(const char* guid, const char* name, DCCommandFunc command, const char* icon) {
	MainWindow::addPluginCommand(guid, Text::toT(name), [=] { command(name); }, icon ? Text::toT(icon) : Util::emptyStringT);
}

void PluginApiWin::removeCommand(const char* guid, const char* name) {
	MainWindow::removePluginCommand(guid, Text::toT(name));
}

void PluginApiWin::playSound(const char* path) {
	WinUtil::playSound(Text::toT(path));
}

void PluginApiWin::notify(const char* title, const char* message) {
	if(WinUtil::mainWindow) {
		WinUtil::mainWindow->notify(Text::toT(title), Text::toT(message));
	}
}

void PluginUtils::addPlugin(dwt::Widget* w) {
	tstring path_t;
	if(dwt::LoadDialog(w)
		.addFilter(str(TF_("%1% files") % _T("dcext")), _T("*.dcext"))
		.addFilter(str(TF_("%1% files") % _T("dll")), _T("*.dll"))
		.open(path_t))
	{
		auto path = Text::fromT(path_t);
		if(Util::getFileExt(path) == ".dcext") {
			PluginInfoDlg(w, path).run();
			WinUtil::mainWindow->refreshPluginMenu();
		} else {
			try {
				PluginManager::getInstance()->addPlugin(path);
			} catch(const Exception& e) {
				dwt::MessageBox(w).show(tstring(T_("Cannot install the plugin:")) + _T("\r\n\r\n") + Text::toT(e.getError()),
					Text::toT(Util::getFileName(path)), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
			}
		}
	}
}

void PluginUtils::configPlugin(const string& guid, dwt::Widget* w) {
	if(!PluginManager::getInstance()->configPlugin(guid, w->handle())) {
		dwt::MessageBox(w).show(
			str(TF_("%1% doesn't need any additional configuration") % Text::toT(PluginManager::getInstance()->getPlugin(guid).name)),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONINFORMATION);
		return;
	}
	WinUtil::mainWindow->refreshPluginMenu();
}

void PluginUtils::enablePlugin(const string& guid, dwt::Widget* w) {
	try {
		PluginManager::getInstance()->enablePlugin(guid);
		WinUtil::mainWindow->refreshPluginMenu();
	} catch(const Exception& e) {
		dwt::MessageBox(w).show(tstring(T_("Cannot enable the plugin:")) + _T("\r\n\r\n") + Text::toT(e.getError()),
			Text::toT(PluginManager::getInstance()->getPlugin(guid).name), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
	}
}

void PluginUtils::disablePlugin(const string& guid, dwt::Widget*) {
	PluginManager::getInstance()->disablePlugin(guid);
	WinUtil::mainWindow->refreshPluginMenu();
}
