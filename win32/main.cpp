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

#include "stdafx.h"

#include <exception>

#include "CrashLogger.h"
#include "dwtTexts.h"
#include "MainWindow.h"
#include "PluginApiWin.h"
#include "SingleInstance.h"
#include "SplashWindow.h"
#include "WinUtil.h"

#include <dcpp/DCPlusPlus.h>
#include <dcpp/MerkleTree.h>
#include <dcpp/File.h>
#include <dcpp/Text.h>
#include <dcpp/Thread.h>
#include <dcpp/MappingManager.h>
#include <dcpp/ResourceManager.h>
#include <dcpp/version.h>

#include <dwt/Application.h>

#define WMU_WHERE_ARE_YOU_MSG _T("WMU_WHERE_ARE_YOU-{885D4B75-6606-4add-A8DE-EEEDC04181F1}")

const UINT SingleInstance::WMU_WHERE_ARE_YOU = ::RegisterWindowMessage(WMU_WHERE_ARE_YOU_MSG);

static void sendCmdLine(HWND hOther, const tstring& cmdLine) {
	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = sizeof(TCHAR)*(cmdLine.length() + 1);
	cpd.lpData = (void *)cmdLine.c_str();
	::SendMessage(hOther, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cpd));
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	ULONG_PTR result;
	if(::SendMessageTimeout(hWnd, SingleInstance::WMU_WHERE_ARE_YOU, 0, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result) &&
		result == SingleInstance::WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

#ifdef _DEBUG
void (*old_handler)();

// Dummy function to have something to break at
void term_handler() {
	old_handler();
}
#endif

int dwtMain(dwt::Application& app) {
#ifdef _DEBUG
	old_handler = std::set_terminate(&term_handler);

#ifndef CONSOLE
	// attach to the parent console, or create one
	if(AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
		freopen("conout$", "w", stdout);
#endif
#endif

	// https://www.kb.cert.org/vuls/id/707943 part III, "For Developers".
	::SetDllDirectory(_T(""));

	Util::initialize();

	CrashLogger crashLogger;

	string configPathHash;
	{
		std::string configPath = Util::getPath(Util::PATH_USER_CONFIG);
		TigerTree tth(TigerTree::calcBlockSize(configPath.size(), 1));
		tth.update(configPath.data(), configPath.size());
		tth.finalize();
		configPathHash = tth.getRoot().toBase32();
	}
#ifndef _DEBUG
	SingleInstance dcapp(Text::toT(string("{DCPLUSPLUS-AEE8350A-B49A-4753-AB4B-E55479A48351}") + configPathHash).c_str());
	if(dcapp.isRunning()) {
#else
	SingleInstance dcapp(Text::toT(string("{DCPLUSPLUS-AEE8350A-B49A-4753-AB4B-E55479A48350}") + configPathHash).c_str());
	if(dcapp.isRunning() && app.getCommandLine().getParams().size() > 1) {
#endif
		HWND hOther = NULL;
		::EnumWindows(&searchOtherInstance, (LPARAM)&hOther);

		if(hOther) {
			// pop up
			::SetForegroundWindow(hOther);

			if( ::IsIconic(hOther)) {
				// restore
				::ShowWindow(hOther, SW_RESTORE);
			}
			sendCmdLine(hOther, app.getCommandLine().getParamsRaw());
		}

		return 1;
	}

	try {
		std::string module = File(Text::fromT(app.getModuleFileName()), File::READ, File::OPEN).read();
		TigerTree tth(TigerTree::calcBlockSize(module.size(), 1));
		tth.update(module.data(), module.size());
		tth.finalize();
		WinUtil::tth = Text::toT(tth.getRoot().toBase32());
	} catch(const FileException&) {
		dcdebug("Failed reading exe\n");
	}

	try {
		auto splash = new SplashWindow();

		startup();
		PluginApiWin::init();

		bindtextdomain(PACKAGE, LOCALEDIR);
		textdomain(PACKAGE);

		// load in a separate thread to avoid freezing the GUI thread.
		struct Loader : Thread {
			Loader(SplashWindow& splash) : Thread(), splash(splash) { }
			int run() {
				load([this](const string& str) { this->splash(str); }, [this](float progress) { this->splash(progress); });

				splash(Text::fromT(_T(APPNAME)));

				dwt::Application::instance().callAsync([this] {
					if(ResourceManager::getInstance()->isRTL()) {
						SetProcessDefaultLayout(LAYOUT_RTL);
					}

					dwtTexts::init();

					WinUtil::init();

					MainWindow* wnd = new MainWindow;
					WinUtil::mainWindow = wnd;
					//WinUtil::mdiParent = wnd->getMDIParent();

					this->splash.close();
				});
				return 0;
			}
			SplashWindow& splash;
		} loader { *splash };

		loader.start();
		app.run();

	} catch(const std::exception& e) {
		dcdebug("Exception: %s\n", e.what());
	} catch(...) {
		dcdebug("Unknown exception");
	}
	WinUtil::uninit();

	shutdown();

	return 0;
}

