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

#include "stdafx.h"

#include "WinUtil.h"

#include "resource.h"
#include <mmsystem.h>

#include <boost/lexical_cast.hpp>

#include <dcpp/ClientManager.h>
#include <dcpp/debug.h>
#include <dcpp/File.h>
#include <dcpp/HashManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/Magnet.h>
#include <dcpp/QueueManager.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/StringMatch.h>
#include <dcpp/StringTokenizer.h>
#include <dcpp/ThrottleManager.h>
#include <dcpp/UserCommand.h>
#include <dcpp/UploadManager.h>
#include <dcpp/UserMatchManager.h>
#include <dcpp/version.h>

#include <dwt/Clipboard.h>
#include <dwt/DWTException.h>
#include <dwt/LibraryLoader.h>
#include <dwt/WidgetCreator.h>
#include <dwt/util/GDI.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/SaveDialog.h>

#include "ParamDlg.h"
#include "MagnetDlg.h"
#include "HubFrame.h"
#include "SearchFrame.h"
#include "MainWindow.h"
#include "PrivateFrame.h"

#ifdef HAVE_HTMLHELP_H
#include <htmlhelp.h>

#ifndef _MSC_VER
// HTML Help libs from recent MS SDKs are compiled with /GS, so link in the following.
#ifdef _WIN64
#define HH_GS_CALL __cdecl
#else
#define HH_GS_CALL __fastcall
#endif
extern "C" {
	void HH_GS_CALL __GSHandlerCheck() { }
	void HH_GS_CALL __security_check_cookie(uintptr_t) { }
#ifdef HAVE_OLD_MINGW
	uintptr_t __security_cookie;
#endif
}
#undef HH_GS_CALL
#endif

#endif

using dwt::Container;
using dwt::Grid;
using dwt::GridInfo;
using dwt::LoadDialog;
using dwt::SaveDialog;
using dwt::Widget;

WinUtil::Notification WinUtil::notifications[NOTIFICATION_LAST] = {
	{ SettingsManager::SOUND_FINISHED_DL, SettingsManager::BALLOON_FINISHED_DL, N_("Download finished"), IDI_DOWNLOAD },
	{ SettingsManager::SOUND_FINISHED_FL, SettingsManager::BALLOON_FINISHED_FL, N_("File list downloaded"), IDI_DIRECTORY },
	{ SettingsManager::SOUND_MAIN_CHAT, SettingsManager::BALLOON_MAIN_CHAT, N_("Main chat message received"), IDI_BALLOON },
	{ SettingsManager::SOUND_PM, SettingsManager::BALLOON_PM, N_("Private message received"), IDI_PRIVATE },
	{ SettingsManager::SOUND_PM_WINDOW, SettingsManager::BALLOON_PM_WINDOW, N_("Private message window opened"), IDI_PRIVATE }
};

tstring WinUtil::tth;
dwt::BrushPtr WinUtil::bgBrush;
COLORREF WinUtil::textColor = 0;
COLORREF WinUtil::bgColor = 0;
dwt::FontPtr WinUtil::font;
dwt::FontPtr WinUtil::uploadFont;
dwt::FontPtr WinUtil::downloadFont;
decltype(WinUtil::userMatchFonts) WinUtil::userMatchFonts;
dwt::ImageListPtr WinUtil::fileImages;
dwt::ImageListPtr WinUtil::userImages;
decltype(WinUtil::fileIndexes) WinUtil::fileIndexes;
TStringList WinUtil::lastDirs;
MainWindow* WinUtil::mainWindow = 0;
bool WinUtil::urlDcADCRegistered = false;
bool WinUtil::urlMagnetRegistered = false;
bool WinUtil::dcextRegistered = false;
DWORD WinUtil::helpCookie = 0;
tstring WinUtil::helpPath;
StringList WinUtil::helpTexts;

const Button::Seed WinUtil::Seeds::button;
const ComboBox::Seed WinUtil::Seeds::comboBox;
const ComboBox::Seed WinUtil::Seeds::comboBoxEdit;
const CheckBox::Seed WinUtil::Seeds::checkBox;
const CheckBox::Seed WinUtil::Seeds::splitCheckBox;
const GroupBox::Seed WinUtil::Seeds::group;
const Label::Seed WinUtil::Seeds::label;
const Menu::Seed WinUtil::Seeds::menu;
const Table::Seed WinUtil::Seeds::table;
const TextBox::Seed WinUtil::Seeds::textBox;
const RichTextBox::Seed WinUtil::Seeds::richTextBox;
const TabView::Seed WinUtil::Seeds::tabs;
const Tree::Seed WinUtil::Seeds::treeView;

const ComboBox::Seed WinUtil::Seeds::Dialog::comboBox;
const ComboBox::Seed WinUtil::Seeds::Dialog::comboBoxEdit;
const TextBox::Seed WinUtil::Seeds::Dialog::textBox;
const TextBox::Seed WinUtil::Seeds::Dialog::intTextBox;
const RichTextBox::Seed WinUtil::Seeds::Dialog::richTextBox;
const Table::Seed WinUtil::Seeds::Dialog::table;
const Table::Seed WinUtil::Seeds::Dialog::optionsTable;
const CheckBox::Seed WinUtil::Seeds::Dialog::checkBox;
const Button::Seed WinUtil::Seeds::Dialog::button;

void WinUtil::init() {

	SettingsManager::getInstance()->setDefault(SettingsManager::BACKGROUND_COLOR, dwt::Color::predefined(COLOR_WINDOW));
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_COLOR, dwt::Color::predefined(COLOR_WINDOWTEXT));

	textColor = SETTING(TEXT_COLOR);
	bgColor = SETTING(BACKGROUND_COLOR);
	bgBrush = dwt::BrushPtr(new dwt::Brush(bgColor));

	{
		NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
		SettingsManager::getInstance()->setDefault(SettingsManager::MAIN_FONT, Text::fromT(encodeFont(metrics.lfMessageFont)));
	}

	initFont();
	updateUploadFont();
	updateDownloadFont();

	initUserMatching();

	{
		// default link color: shift the hue, set lum & sat to middle values
		auto hls = RGB2HLS(textColor);
		SettingsManager::getInstance()->setDefault(SettingsManager::LINK_COLOR, HLS2RGB(HLS((HLS_H(hls) + 60) % 239, 120, 120)));
	}

	{
		// default log color: more grey than the text color
		auto hls = RGB2HLS(textColor);
		SettingsManager::getInstance()->setDefault(SettingsManager::LOG_COLOR, HLS2RGB(HLS(HLS_H(hls), 120, HLS_S(hls) / 2)));
	}

	fileImages = dwt::ImageListPtr(new dwt::ImageList(dwt::Point(16, 16)));

	// get the directory icon (DIR_ICON).
	if(SETTING(USE_SYSTEM_ICONS)) {
		::SHFILEINFO info;
		if(::SHGetFileInfo(_T("./"), FILE_ATTRIBUTE_DIRECTORY, &info, sizeof(info),
			SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES) && info.hIcon)
		{
			dwt::Icon icon { info.hIcon };
			fileImages->add(icon);
		}
	}
	if(fileImages->empty()) {
		fileImages->add(*createIcon(IDI_DIRECTORY, 16));
	}

	// create the incomplete directory icon (DIR_ICON_INCOMPLETE).
	{
		dwt::ImageList icons(dwt::Point(16, 16));
		icons.add(*fileImages->getIcon(DIR_ICON));
		icons.add(*createIcon(IDI_EXEC, 16));
		fileImages->add(*dwt::util::merge(icons));
	}

	// add the generic file icon (FILE_ICON_GENERIC).
	fileImages->add(*createIcon(IDI_FILE, 16));

	// add icons used by the transfer list (TRANSFER_ICON_*).
	fileImages->add(*createIcon(IDI_USER, 16));
	fileImages->add(*createIcon(IDI_PRIVATE, 16));

	{
		const dwt::Point size(16, 16);
		userImages = dwt::ImageListPtr(new dwt::ImageList(size));

		const unsigned baseCount = USER_ICON_MOD_START;
		const unsigned modifierCount = USER_ICON_LAST - USER_ICON_MOD_START;

		auto userIcon = [](unsigned id) { return createIcon(id, 16); };
		dwt::IconPtr bases[baseCount] = { userIcon(IDI_USER), userIcon(IDI_USER_AWAY), userIcon(IDI_USER_BOT) };
		dwt::IconPtr modifiers[modifierCount] = { userIcon(IDI_USER_NOCON), userIcon(IDI_USER_NOSLOT), userIcon(IDI_USER_OP) };

		for(size_t iBase = 0; iBase < baseCount; ++iBase) {
			for(size_t i = 0, n = modifierCount * modifierCount; i < n; ++i) {
				dwt::ImageList icons(size);

				icons.add(*bases[iBase]);

				for(size_t iMod = 0; iMod < modifierCount; ++iMod)
					if(i & (1 << iMod))
						icons.add(*modifiers[iMod]);

				userImages->add(*dwt::util::merge(icons));
			}
		}
	}

	registerHubHandlers();
	registerMagnetHandler();
	registerDcextHandler();

	initHelpPath();

	if(!helpPath.empty()) {
		// load up context-sensitive help texts
		try {
			helpTexts = StringTokenizer<string> (File(Util::getFilePath(Text::fromT(helpPath)) + "cshelp.rtf",
				File::READ, File::OPEN).read(), "\r\n").getTokens();
		} catch (const FileException&) {
		}
	}

#ifdef HAVE_HTMLHELP_H
	::HtmlHelp(NULL, NULL, HH_INITIALIZE, reinterpret_cast<DWORD_PTR> (&helpCookie));
#endif
}

void WinUtil::initSeeds() {
	// Const so that noone else will change them after they've been initialized
	Button::Seed& xbutton = const_cast<Button::Seed&> (Seeds::button);
	ComboBox::Seed& xcomboBoxEdit = const_cast<ComboBox::Seed&> (Seeds::comboBoxEdit);
	ComboBox::Seed& xcomboBox = const_cast<ComboBox::Seed&> (Seeds::comboBox);
	CheckBox::Seed& xCheckBox = const_cast<CheckBox::Seed&> (Seeds::checkBox);
	CheckBox::Seed& xSplitCheckBox = const_cast<CheckBox::Seed&> (Seeds::splitCheckBox);
	GroupBox::Seed& xgroup = const_cast<GroupBox::Seed&> (Seeds::group);
	Label::Seed& xlabel = const_cast<Label::Seed&> (Seeds::label);
	Menu::Seed& xmenu = const_cast<Menu::Seed&> (Seeds::menu);
	Table::Seed& xTable = const_cast<Table::Seed&> (Seeds::table);
	TextBox::Seed& xtextBox = const_cast<TextBox::Seed&> (Seeds::textBox);
	RichTextBox::Seed& xRichTextBox = const_cast<RichTextBox::Seed&> (Seeds::richTextBox);
	TabView::Seed& xTabs = const_cast<TabView::Seed&> (Seeds::tabs);
	Tree::Seed& xtreeView = const_cast<Tree::Seed&> (Seeds::treeView);
	ComboBox::Seed& xdComboBox = const_cast<ComboBox::Seed&> (Seeds::Dialog::comboBox);
	TextBox::Seed& xdTextBox = const_cast<TextBox::Seed&> (Seeds::Dialog::textBox);
	TextBox::Seed& xdintTextBox = const_cast<TextBox::Seed&> (Seeds::Dialog::intTextBox);
	RichTextBox::Seed& xdRichTextBox = const_cast<RichTextBox::Seed&> (Seeds::Dialog::richTextBox);
	Table::Seed& xdTable = const_cast<Table::Seed&> (Seeds::Dialog::table);
	Table::Seed& xdoptionsTable = const_cast<Table::Seed&> (Seeds::Dialog::optionsTable);

	xbutton.font = font;

	xcomboBox.style |= CBS_DROPDOWNLIST;
	xcomboBox.font = font;

	xcomboBoxEdit.font = font;

	xCheckBox.font = font;

	xSplitCheckBox = xCheckBox;
	xSplitCheckBox.caption = _T("+/-");
	xSplitCheckBox.style &= ~WS_TABSTOP;

	xgroup.font = font;

	xlabel.font = font;

	xmenu.ownerDrawn = SETTING(OWNER_DRAWN_MENUS);
	if(xmenu.ownerDrawn)
		xmenu.font = font;

	xTable.style |= WS_HSCROLL | WS_VSCROLL | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS;
	xTable.exStyle = WS_EX_CLIENTEDGE;
	xTable.lvStyle |= LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
	xTable.font = font;

	xtextBox.exStyle = WS_EX_CLIENTEDGE;
	xtextBox.font = font;

	xRichTextBox.exStyle = WS_EX_CLIENTEDGE;
	xRichTextBox.font = font;

	xTabs.font = font;

	xtreeView.style |= TVS_HASBUTTONS | TVS_LINESATROOT;
	xtreeView.exStyle = WS_EX_CLIENTEDGE;
	xtreeView.font = font;

	xdComboBox.style |= CBS_DROPDOWNLIST;

	xdTextBox.style |= ES_AUTOHSCROLL;

	xdintTextBox = xdTextBox;
	xdintTextBox.style |= ES_NUMBER;

	xdRichTextBox.style |= ES_READONLY | ES_SUNKEN;
	xdRichTextBox.exStyle = WS_EX_CLIENTEDGE;

	xdTable.style |= WS_HSCROLL | WS_VSCROLL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
	xdTable.exStyle = WS_EX_CLIENTEDGE;
	xdTable.lvStyle |= LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;

	xdoptionsTable = xdTable;
	xdoptionsTable.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
	xdoptionsTable.lvStyle |= LVS_EX_CHECKBOXES;

	xtextBox.menuSeed = Seeds::menu;
	xdTextBox.menuSeed = Seeds::menu;
	xRichTextBox.menuSeed = Seeds::menu;
	xdRichTextBox.menuSeed = Seeds::menu;
}

void WinUtil::initHelpPath() {
	// find the current locale
	auto lang = Util::getIETFLang();

	// find the path to the help file
	string path;
	if(lang != "en" && lang != "en-US") {
		while(true) {
			path = Util::getPath(Util::PATH_LOCALE) + lang
				+ PATH_SEPARATOR_STR "help" PATH_SEPARATOR_STR "DCPlusPlus.chm";
			if(File::getSize(path) != -1)
				break;
			// if the lang has extra information, try to remove it
			string::size_type pos = lang.rfind('-');
			if(pos == string::npos)
				break;
			lang = lang.substr(0, pos);
		}
	}
	if(path.empty() || File::getSize(path) == -1) {
		path = Util::getPath(Util::PATH_RESOURCES) + "DCPlusPlus.chm";
		if(File::getSize(path) == -1) {
			/// @todo also check that the file is up-to-date
			/// @todo alert the user that the help file isn't found/up-to-date
			return;
		}
	}
	helpPath = Text::toT(path);
}

void WinUtil::uninit() {
#ifdef HAVE_HTMLHELP_H
	::HtmlHelp(NULL, NULL, HH_UNINITIALIZE, helpCookie);
#endif
}

void WinUtil::initFont() {
	updateFont(font, SettingsManager::MAIN_FONT);

	initSeeds();
}

tstring WinUtil::encodeFont(LOGFONT const& font) {
	tstring res(font.lfFaceName);
	res += _T(',');
	res += Text::toT(Util::toString(static_cast<int>(std::floor(static_cast<float>(font.lfHeight) / dwt::util::dpiFactor()))));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfWeight));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfItalic));
	res += _T(',');
	res += Text::toT(Util::toString(font.lfCharSet));
	return res;
}

std::string WinUtil::toString(const std::vector<int>& tokens) {
	std::string ret;
	auto start = tokens.cbegin();
	for(auto i = start, iend = tokens.cend(); i != iend; ++i) {
		if(i != start)
			ret += ',';
		ret += Util::toString(*i);
	}
	return ret;
}

void WinUtil::decodeFont(const tstring& setting, LOGFONT &dest) {
	StringTokenizer<tstring> st(setting, _T(','));
	TStringList &sl = st.getTokens();

	NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
	dest = metrics.lfMessageFont;

	tstring face;
	if(sl.size() >= 4) {
		face = sl[0];
		dest.lfHeight = std::ceil(static_cast<float>(Util::toInt(Text::fromT(sl[1]))) * dwt::util::dpiFactor());
		dest.lfWeight = Util::toInt(Text::fromT(sl[2]));
		dest.lfItalic = static_cast<BYTE>(Util::toInt(Text::fromT(sl[3])));
		if(sl.size() >= 5) {
			dest.lfCharSet = static_cast<BYTE>(Util::toInt(Text::fromT(sl[4])));
		}
	}

	if(!face.empty()) {
		_tcscpy(dest.lfFaceName, face.c_str());
	}
}

void WinUtil::updateFont(dwt::FontPtr& font, int setting) {
	auto text = SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(setting));
	if(text.empty()) {
		font.reset();
	} else {
		LOGFONT lf;
		decodeFont(Text::toT(text), lf);
		font.reset(new dwt::Font(lf));
	}
}

void WinUtil::initUserMatching() {
	// make sure predefined definitions are here.
	auto here = UserMatchManager::getInstance()->checkPredef();

	if(!here.first || !here.second) {
		auto newList = UserMatchManager::getInstance()->getList();

		if(!here.first) {
			// add a matcher for favs.
			UserMatch matcher;
			matcher.setFlag(UserMatch::PREDEFINED);
			matcher.setFlag(UserMatch::FAVS);
			matcher.name = str(F_("Favorite users (added by %1%)") % APPNAME);
			matcher.style.font = Text::fromT(encodeFont(font->makeBold()->getLogFont()));
			matcher.style.textColor = modRed(SETTING(TEXT_COLOR), 127); // more red
			newList.push_back(std::move(matcher));
		}

		if(!here.second) {
			// add a matcher for ops.
			UserMatch matcher;
			matcher.setFlag(UserMatch::PREDEFINED);
			matcher.setFlag(UserMatch::OPS);
			matcher.name = str(F_("Operators (added by %1%)") % APPNAME);
			matcher.style.textColor = modBlue(SETTING(TEXT_COLOR), 127); // more blue
			newList.push_back(std::move(matcher));
		}

		UserMatchManager::getInstance()->setList(std::move(newList));
	}

	updateUserMatchFonts();
}

void WinUtil::updateUserMatchFonts() {
	userMatchFonts.clear();

	for(auto& font: UserMatchManager::getInstance()->getFonts()) {
		LOGFONT lf;
		decodeFont(Text::toT(font), lf);
		userMatchFonts[font] = new dwt::Font(lf);
	}
}

dwt::FontPtr WinUtil::getUserMatchFont(const string& key) {
	// cache lookup might fail when refreshing the list of user matching defs...
	auto cached = userMatchFonts.find(key);
	if(cached != userMatchFonts.end()) {
		return cached->second;
	}
	return nullptr;
}

void WinUtil::updateUploadFont() {
	updateFont(uploadFont, SettingsManager::UPLOAD_FONT);
}

void WinUtil::updateDownloadFont() {
	updateFont(downloadFont, SettingsManager::DOWNLOAD_FONT);
}

void WinUtil::setStaticWindowState(const string& id, bool open) {
	mainWindow->setStaticWindowState(id, open);
}

bool WinUtil::checkNick() {
	if(SETTING(NICK).empty()) {
		dwt::MessageBox(mainWindow).show(T_("Please enter a nickname in the settings dialog!"),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
		mainWindow->handleSettings();
		return false;
	}

	return true;
}

void WinUtil::handleDblClicks(dwt::TextBoxBase* box) {
	box->onLeftMouseDblClick([box](const dwt::MouseEvent& me) { return parseLink(box->textUnderCursor(me.pos)); });
}

#define LINE2 _T("-- https://dcplusplus.sourceforge.io <DC++ ") _T(VERSIONSTRING) _T(">")
const TCHAR
	*msgs[] = {
		_T("\r\n-- I'm a happy DC++ user. You could be happy too.\r\n") LINE2,
		_T("\r\n-- Neo-...what? Nope...never heard of it...\r\n") LINE2,
		_T("\r\n-- Evolution of species: Ape --> Man\r\n-- Evolution of science: \"The Earth is Flat\" --> \"The Earth is Round\"\r\n-- Evolution of DC: NMDC --> ADC\r\n") LINE2,
		_T("\r\n-- I share, therefore I am.\r\n") LINE2,
		_T("\r\n-- I came, I searched, I found...\r\n") LINE2,
		_T("\r\n-- I came, I shared, I sent...\r\n") LINE2,
		_T("\r\n-- I can fully configure the toolbar, can you?\r\n") LINE2,
		_T("\r\n-- I don't have to see any ads, do you?\r\n") LINE2,
		_T("\r\n-- My client supports passive-passive downloads, does yours?\r\n") LINE2,
		_T("\r\n-- I can refresh my opened file lists, can you?\r\n") LINE2,
		_T("\r\n-- I can get help for every part of GUI and settings in my client, can you?\r\n") LINE2,
		_T("\r\n-- My client automatically detects the connection, does yours?\r\n") LINE2,
		_T("\r\n-- My client keeps track of all the recent opened windows, does yours?\r\n") LINE2,
		_T("\r\n-- These addies are pretty annoying, aren't they? Get revenge by sending them yourself!\r\n") LINE2,
		_T("\r\n-- My client supports taskbar thumbnails and Aero Peek previews, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports secure communication and transfers, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports grouping favorite hubs, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports segmented downloading, does yours?\r\n") LINE2,
		_T("\r\n-- My client supports browsing file lists, does yours?\r\n") LINE2 };

#define MSGS 19

/// @todo improve - commands could be stored in a map...

const char* command_strings[] = {
	"/refresh",
	"/search <string>",
	"/close",
	"Closes the current window.",
	"/clear [lines to keep]",
	"/dc++",
	"/away [message]",
	"/back",
	"/d <search string>",
	"/g <search string>",
	"/imdb <imdb query>",
	"/rebuild",
	"/log <status, system, downloads, uploads>",
	"/help",
	"/u <url>",
	"/f <search string>"
};

const char* command_helps[] = {
	N_("Manually refreshes DC++'s share list by going through the shared directories and adding new folders and files. DC++ automatically refreshes once an hour by default, and also refreshes after the list of shared directories is changed."),
	N_("Sets the current number of upload slots to the number you specify. If this is less than the current number of slots, no uploads are cancelled."),
	N_("Sets the current number of download slots to the number you specify. If this is less than the current number of slots, no downloads are cancelled."),
	N_("Opens a new search window with the specified search string. It does not automatically send the search."),
	N_("Clears the current window of all text. Optionally, you can specify how many of the latest (most recent) lines should be kept."),
	N_("Sends a random DC++ advertising message to the chat, including a link to the DC++ homepage and the version number."),
	N_("Sets Away status. New private message windows will be responded to, once, with the message you specified, or the default away message configured in the Personal information settings page."),
	N_("Un-sets Away status."),
	N_("Launches your default web browser to the DuckDuckGo search engine with the specified search."),
	N_("Launches your default web browser to the Google search engine with the specified search."),
	N_("Launches your default web browser to the Internet Movie Database (imdb) with the specified query."),
	N_("Rebuilds the HashIndex.xml and HashData.dat files, removing entries to files that are no longer shared, or old hashes for files that have since changed. This runs in the main DC++ thread, so the interface will freeze until the rebuild is finished."),
	N_("If no parameter is specified, it launches the log for the hub or private chat with the associated application in Windows. If one of the parameters is specified it opens that log file. The status log is available only in the hub frame."),
	N_("Displays available commands. (The ones listed on this page.) Optionally, you can specify &quot;brief&quot; to have a brief listing."),
	N_("Launches your default web browser with the given URL."),
	N_("Highlights the last occourrence of the specified search string in the chat window.")
};

tstring WinUtil::getDescriptiveCommands() {
	tstring ret;

	int counter = 0;
	for(auto& command_string: command_strings) {
		ret +=
			_T("\r\n") + Text::toT(command_string) +
			_T("\r\n\t") + T_(command_helps[counter++]);
	}

	return ret;
}

tstring
	WinUtil::commands =
		_T("/refresh, /me <msg>, /clear [lines to keep], /slots #, /dslots #, /search <string>, /f <string>, /dc++, /away <msg>, /back, /d <searchstring>, /g <searchstring>, /imdb <imdbquery>, /u <url>, /rebuild, /download, /upload");

bool WinUtil::checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson) {
	string::size_type i = cmd.find(' ');
	if(i != string::npos) {
		param = cmd.substr(i + 1);
		cmd = cmd.substr(1, i - 1);
	} else {
		cmd = cmd.substr(1);
	}

	if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
		if(Util::stricmp(param.c_str(), _T("system")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::SYSTEM))));
		} else if(Util::stricmp(param.c_str(), _T("downloads")) == 0) {
			WinUtil::openFile(Text::toT(
				Util::validateFileName(LogManager::getInstance()->getPath(LogManager::DOWNLOAD))));
		} else if(Util::stricmp(param.c_str(), _T("uploads")) == 0) {
			WinUtil::openFile(Text::toT(Util::validateFileName(LogManager::getInstance()->getPath(LogManager::UPLOAD))));
		} else {
			return false;
		}
	} else if(Util::stricmp(cmd.c_str(), _T("me")) == 0) {
		message = param;
		thirdPerson = true;
	} else if(Util::stricmp(cmd.c_str(), _T("refresh")) == 0) {
		try {
			ShareManager::getInstance()->setDirty();
			ShareManager::getInstance()->refresh(true);
		} catch (const ShareException& e) {
			status = Text::toT(e.getError());
		}
	} else if(Util::stricmp(cmd.c_str(), _T("slots")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if(j > 0) {
			ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::SLOTS), j);
			status = T_("Slots set");
		} else {
			status = T_("Invalid number of slots");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("dslots")) == 0) {
		int j = Util::toInt(Text::fromT(param));
		if(j >= 0) {
			SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SLOTS, j);
			status = T_("Download slots set");
		} else {
			status = T_("Invalid number of slots");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("search")) == 0) {
		if(!param.empty()) {
			SearchFrame::openWindow(mainWindow->getTabView(), param, SearchManager::TYPE_ANY);
		} else {
			status = T_("Specify a search string");
		}
	} else if(Util::stricmp(cmd.c_str(), _T("dc++")) == 0) {
		message = msgs[GET_TICK() % MSGS];
	} else if(Util::stricmp(cmd.c_str(), _T("away")) == 0) {
		if(Util::getAway() && param.empty()) {
			Util::setAway(false);
			status = T_("Away mode off");
		} else {
			Util::setAway(true);
			Util::setAwayMessage(Text::fromT(param));
			auto awayMessage = Util::getAwayMessage();
			status = str(TF_("Away mode on: %1%") % (awayMessage.empty() ? T_("No away message") : Text::toT(awayMessage)));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("back")) == 0) {
		Util::setAway(false);
		status = T_("Away mode off");
	} else if(Util::stricmp(cmd.c_str(), _T("d")) == 0) {
		if(param.empty()) {
			status = T_("Specify a search string");
		} else {
			WinUtil::openLink(_T("https://www.duckduckgo.com/?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("g")) == 0) {
		if(param.empty()) {
			status = T_("Specify a search string");
		} else {
			WinUtil::openLink(_T("https://www.google.com/search?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("imdb")) == 0) {
		if(param.empty()) {
			status = T_("Specify a search string");
		} else {
			WinUtil::openLink(_T("https://www.imdb.com/find?q=") + Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("u")) == 0) {
		if(param.empty()) {
			status = T_("Specify a URL");
		} else {
			WinUtil::openLink(Text::toT(Util::encodeURI(Text::fromT(param))));
		}
	} else if(Util::stricmp(cmd.c_str(), _T("rebuild")) == 0) {
		HashManager::getInstance()->rebuild();
	} else if(Util::stricmp(cmd.c_str(), _T("upload")) == 0) {
		auto value = Util::toInt(Text::fromT(param));
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::MAX_UPLOAD_SPEED_MAIN), value);
		status = value ? str(TF_("Upload limit set to %1% KiB/s") % value) : T_("Upload limit disabled");
	} else if(Util::stricmp(cmd.c_str(), _T("download")) == 0) {
		auto value = Util::toInt(Text::fromT(param));
		ThrottleManager::setSetting(ThrottleManager::getCurSetting(SettingsManager::MAX_DOWNLOAD_SPEED_MAIN), value);
		status = value ? str(TF_("Download limit set to %1% KiB/s") % value) : T_("Download limit disabled");
	} else {
		return false;
	}

	return true;
}

void WinUtil::notify(NotificationType notification, const tstring& balloonText, const std::function<void ()>& balloonCallback) {
	const auto& n = notifications[notification];

	const string& s = SettingsManager::getInstance()->get((SettingsManager::StrSetting)n.sound);
	if(!s.empty()) {
		playSound(Text::toT(s));
	}

	int b = SettingsManager::getInstance()->get((SettingsManager::IntSetting)n.balloon);
	if(b == SettingsManager::BALLOON_ALWAYS || (b == SettingsManager::BALLOON_BACKGROUND && !mainWindow->onForeground())) {
		mainWindow->notify(T_(n.title), balloonText, balloonCallback, createIcon(n.icon, 16));
	}
}

void WinUtil::playSound(const tstring& sound) {
	if(sound == _T("beep")) {
		::MessageBeep(MB_OK);
	} else {
		::PlaySound(sound.c_str(), 0, SND_FILENAME | SND_ASYNC);
	}
}

void WinUtil::openFile(const tstring& file) {
	::ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void WinUtil::openFolder(const tstring& file) {
	if(File::getSize(Text::fromT(file)) != -1)
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, /select, \"" + (Text::fromT(file))
			+ "\"").c_str(), NULL, SW_SHOWNORMAL);
	else
		::ShellExecute(NULL, NULL, Text::toT("explorer.exe").c_str(), Text::toT("/e, \"" + Util::getFilePath(
			Text::fromT(file)) + "\"").c_str(), NULL, SW_SHOWNORMAL);
}

tstring WinUtil::getNicks(const CID& cid) {
	return Text::toT(Util::toString(ClientManager::getInstance()->getNicks(cid)));
}

pair<tstring, bool> WinUtil::getHubNames(const CID& cid) {
	auto hubs = ClientManager::getInstance()->getHubNames(cid);
	if(hubs.empty()) {
		return make_pair(T_("Offline"), false);
	} else {
		return make_pair(Text::toT(Util::toString(hubs)), true);
	}
}

tstring WinUtil::getNick(const HintedUser& user) {
	return Text::toT(ClientManager::getInstance()->getNick(user));
}

tstring WinUtil::getHubName(const HintedUser& user) {
	return Text::toT(ClientManager::getInstance()->getHubName(user));
}

size_t WinUtil::getFileIcon(const string& fileName) {
	if(SETTING(USE_SYSTEM_ICONS)) {
		string ext = Text::toLower(Util::getFileExt(fileName));
		if(!ext.empty()) {
			auto index = fileIndexes.find(ext);
			if(index != fileIndexes.end())
				return index->second;
		}

		::SHFILEINFO info;
		if(::SHGetFileInfo(Text::toT(Text::toLower(Util::getFileName(fileName))).c_str(), FILE_ATTRIBUTE_NORMAL,
			&info, sizeof(info), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES) && info.hIcon)
		{
			size_t ret = fileImages->size();
			fileIndexes[ext] = ret;

			dwt::Icon icon { info.hIcon };
			fileImages->add(icon);

			return ret;
		}
	}

	return FILE_ICON_GENERIC;
}

size_t WinUtil::getFileIcon(const tstring& fileName) {
	return getFileIcon(Text::fromT(fileName));
}

void WinUtil::reducePaths(string& message) {
	string::size_type start = 0;
	while((start = message.find('<', start)) != string::npos) {
		string::size_type end = message.find('>', start);
		if(end == string::npos)
			return;

		if(count(message.begin() + start, message.begin() + end, PATH_SEPARATOR) >= 2) {
			message.erase(end, 1);
			message.erase(start, message.rfind(PATH_SEPARATOR, message.rfind(PATH_SEPARATOR, end) - 1) - start);
			message.insert(start, "...");
		}

		start++;
	}
}

void WinUtil::addHashItems(Menu* menu, const TTHValue& tth, const tstring& filename, int64_t size) {
	if(!tth) { return; }
	menu->appendItem(T_("Search for alternates"), [=] { searchHash(tth); }, menuIcon(IDI_SEARCH));
	menu->appendItem(T_("Copy magnet link to clipboard"), [=] { copyMagnet(tth, filename, size); }, menuIcon(IDI_MAGNET));
}

void WinUtil::copyMagnet(const TTHValue& aHash, const tstring& aFile, int64_t size) {
	if(!aFile.empty()) {
		setClipboard(Text::toT(makeMagnet(aHash, Text::fromT(aFile), size)));
	}
}

string WinUtil::makeMagnet(const TTHValue& aHash, const string& aFile, int64_t size) {
	string ret = "magnet:?xt=urn:tree:tiger:" + aHash.toBase32();
	if(size > 0)
		ret += "&xl=" + Util::toString(size);
	return ret + "&dn=" + Util::encodeURI(aFile);
}

void WinUtil::searchAny(const tstring& aSearch) {
	SearchFrame::openWindow(mainWindow->getTabView(), aSearch, SearchManager::TYPE_ANY);
}

void WinUtil::searchHash(const TTHValue& aHash) {
	SearchFrame::openWindow(mainWindow->getTabView(), Text::toT(aHash.toBase32()), SearchManager::TYPE_TTH);
}

void WinUtil::searchHub(const tstring& aUrl) {
	SearchFrame::openWindow(mainWindow->getTabView(), Util::emptyStringT, SearchManager::TYPE_ANY, aUrl);
}

void WinUtil::addLastDir(const tstring& dir) {
	auto i = find(lastDirs.begin(), lastDirs.end(), dir);
	if(i != lastDirs.end()) {
		lastDirs.erase(i);
	}
	while(lastDirs.size() >= 10) {
		lastDirs.erase(lastDirs.begin());
	}
	lastDirs.push_back(dir);
}

bool WinUtil::browseSaveFile(dwt::Widget* parent, tstring& file) {
	auto ext = Util::getFileExt(file);
	auto path = Util::getFilePath(file);

	SaveDialog dlg(parent);

	if(!ext.empty()) {
		ext = ext.substr(1); // remove leading dot so default extension works when browsing for file
		dlg.addFilter(str(TF_("%1% files") % ext), _T("*.") + ext);
		dlg.setDefaultExtension(ext);
	}
	dlg.addFilter(T_("All files"), _T("*.*"));
	dlg.setInitialDirectory(path);

	return dlg.open(file);
}

bool WinUtil::browseFileList(dwt::Widget* parent, tstring& file) {
	return LoadDialog(parent).addFilter(T_("File Lists"), _T("*.xml.bz2;*.xml")) .addFilter(T_("All files"), _T("*.*")) .setInitialDirectory(
		Text::toT(Util::getListPath())) .open(file);
}

void WinUtil::setClipboard(const tstring& str) {
	dwt::Clipboard::setData(str, mainWindow);
}

bool WinUtil::getUCParams(dwt::Widget* parent, const UserCommand& uc, ParamMap& params) noexcept {
	ParamDlg dlg(parent, Text::toT(Util::toString(" > ", uc.getDisplayName())));

	string::size_type i = 0;
	StringList names;

	while((i = uc.getCommand().find("%[line:", i)) != string::npos) {
		i += 7;
		string::size_type j = uc.getCommand().find(']', i);
		if(j == string::npos)
			break;

		const string name = uc.getCommand().substr(i, j - i);
		if(find(names.begin(), names.end(), name) == names.end()) {
			tstring caption = Text::toT(name);
			if(uc.adc()) {
				Util::replace(_T("\\\\"), _T("\\"), caption);
				Util::replace(_T("\\s"), _T(" "), caption);
			}

			// let's break between slashes (while ignoring double-slashes) to see if it's a combo
			int combo_sel = -1;
			tstring combo_caption = caption;
			Util::replace(_T("//"), _T("\t"), combo_caption);
			TStringList combo_values = StringTokenizer<tstring>(combo_caption, _T('/')).getTokens();
			if(combo_values.size() > 2) { // must contain at least: caption, default sel, 1 value

				auto first = combo_values.begin();
				combo_caption = *first;
				combo_values.erase(first);

				first = combo_values.begin();
				try { combo_sel = boost::lexical_cast<size_t>(Text::fromT(*first)); }
				catch(const boost::bad_lexical_cast&) { combo_sel = -1; }
				combo_values.erase(first);
				if(static_cast<size_t>(combo_sel) >= combo_values.size())
					combo_sel = -1; // default selection value too high
			}

			if(combo_sel >= 0) {
				for(auto& i: combo_values)
					Util::replace(_T("\t"), _T("/"), i);

				// if the combo has already been displayed before, retrieve the prev value and bypass combo_sel
				auto prev = find(combo_values.begin(), combo_values.end(), Text::toT(boost::get<string>(params["line:" + name])));
				if(prev != combo_values.end())
					combo_sel = prev - combo_values.begin();

				dlg.addComboBox(combo_caption, combo_values, combo_sel);

			} else {
				dlg.addTextBox(caption, Text::toT(boost::get<string>(params["line:" + name])));
			}

			names.push_back(name);
		}
		i = j + 1;
	}

	if(names.empty())
		return true;

	if(dlg.run() == IDOK) {
		const TStringList& values = dlg.getValues();
		for(size_t i = 0, iend = values.size(); i < iend; ++i) {
			params["line:" + names[i]] = Text::fromT(values[i]);
		}
		return true;
	}
	return false;
}

dwt::Control* helpPopup = nullptr; // only have 1 help popup at any given time.

/** Display a help popup.
Help popups display RTF text within a Rich Edit control. They visually look similar to a regular
tooltip, but with a yellow background and thicker borders.
@tparam tooltip Whether this help popup should act as an inactive tooltip that doesn't receive
input but closes itself once its timer runs out. */
template<bool tooltip>
class HelpPopup : private RichTextBox {
	typedef HelpPopup<tooltip> ThisType;
	typedef RichTextBox BaseType;

public:
	HelpPopup(dwt::Widget* parent, const tstring& text, const dwt::Point& pos = dwt::Point(), bool multiline = false) :
		BaseType(parent)
	{
		// create the box as an invisible popup window.
		auto seed = WinUtil::Seeds::richTextBox;
		seed.style = WS_POPUP | ES_READONLY;
		if(multiline)
			seed.style |= ES_MULTILINE;
		seed.exStyle = WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_CLIENTEDGE;
		seed.location.size.x = std::min(getParent()->getDesktopSize().width(), static_cast<long>(maxWidth * dwt::util::dpiFactor()));
		seed.location.size.y = 1; // so that Rich Edit 8 sends EN_REQUESTRESIZE
		seed.events |= ENM_REQUESTRESIZE;
		create(seed);

		const auto margins = sendMessage(EM_GETMARGINS);
		sendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(LOWORD(margins) + margin, HIWORD(margins) + margin));

		// let the control figure out what the best size is.
		onRaw([this, text, pos](WPARAM, LPARAM l) { return this->resize(l, text, pos); }, dwt::Message(WM_NOTIFY, EN_REQUESTRESIZE));
		setText(text);
	}

private:
	LRESULT resize(LPARAM lParam, const tstring& text, dwt::Point pos) {
		dwt::Rectangle rect { reinterpret_cast<REQRESIZE*>(lParam)->rc };

		if(rect.width() > getWindowRect().width() && !hasStyle(ES_MULTILINE)) {
			// can't add ES_MULTILINE at run time, so create the control again.
			new ThisType(getParent(), text, pos, true);
			close();
			return 0;
		}

		// ok, ready to show the popup! make sure there's only 1 at any given time.
		if(helpPopup) {
			helpPopup->close();
		}
		helpPopup = this;
		onDestroy([this] { if(this == helpPopup) helpPopup = nullptr; });

		// where to position the popup.
		if(!tooltip) {
			if(isAnyKeyPressed()) {
				auto rect = getParent()->getWindowRect();
				pos.x = rect.left() + rect.width() / 2;
				pos.y = rect.bottom() + margin;
			} else {
				pos = dwt::Point::fromLParam(::GetMessagePos());
			}
		}

		// adjust the size to account for borders and margins.
		rect.pos = pos;
		rect.size.x += ::GetSystemMetrics(SM_CXEDGE) * 2;
		rect.size.y += ::GetSystemMetrics(SM_CYEDGE) * 2 + margin;

		// make sure the window fits in within the desktop of the parent widget.
		rect.ensureVisibility(getParent());

		setColor(dwt::Color::predefined(COLOR_INFOTEXT), dwt::Color::predefined(COLOR_INFOBK));

		if(tooltip) {
			// this help popup acts as a tooltip; it will close by itself.
			onMouseMove([this](const dwt::MouseEvent&) { return this->close(); });

		} else {
			// capture the mouse.
			onLeftMouseDown([this](const dwt::MouseEvent&) { return this->close(); });
			::SetCapture(handle());
			onDestroy([] { ::ReleaseCapture(); });

			// capture the keyboard.
			auto focus = dwt::hwnd_cast<dwt::Widget*>(::GetFocus());
			if(focus) {
				auto cb1 = focus->addCallback(dwt::Message(WM_KEYDOWN), [this](const MSG&, LRESULT&) { return this->close(); });
				auto cb2 = focus->addCallback(dwt::Message(WM_HELP), [this](const MSG&, LRESULT&) { return this->close(); });
				onDestroy([this, focus, cb1, cb2] {
					auto cb1_ = cb1; focus->clearCallback(dwt::Message(WM_KEYDOWN), cb1_);
					auto cb2_ = cb2; focus->clearCallback(dwt::Message(WM_HELP), cb2_);
				});
			}
		}

		// go live!
		::SetWindowPos(handle(), 0, rect.left(), rect.top(), rect.width(), rect.height(),
			SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_SHOWWINDOW);

		return 0;
	}

	bool close() {
		BaseType::close(true);
		return true;
	}

	static const long margin = 6;
	static const long maxWidth = 400;
};

void WinUtil::help(dwt::Control* widget) {
	helpId(widget, widget->getHelpId());
}

void WinUtil::helpId(dwt::Control* widget, unsigned id) {
	if(id >= IDH_CSHELP_BEGIN && id <= IDH_CSHELP_END) {
		// context-sensitive help
		new HelpPopup<false>(widget, Text::toT(getHelpText(id).second));

	} else {
#ifdef HAVE_HTMLHELP_H
		if(id < IDH_BEGIN || id > IDH_END)
			id = IDH_INDEX;
		::HtmlHelp(widget->handle(), helpPath.c_str(), HH_HELP_CONTEXT, id);
#endif
	}
}

void WinUtil::helpTooltip(dwt::Control* widget, const dwt::Point& pos) {
	killHelpTooltip();

	auto id = widget->getHelpId();
	if(id >= IDH_CSHELP_BEGIN && id <= IDH_CSHELP_END) {
		// context-sensitive help
		auto text = getHelpText(id);
		if(text.first) {
			new HelpPopup<true>(widget, Text::toT(text.second), pos);
		}
	}
}

void WinUtil::killHelpTooltip() {
	if(helpPopup) {
		helpPopup->close();
		helpPopup = nullptr;
	}
}

pair<bool, string> WinUtil::getHelpText(unsigned id) {
	if(id >= IDH_CSHELP_BEGIN) {
		id -= IDH_CSHELP_BEGIN;

		if(id < helpTexts.size()) {
			const auto& ret = helpTexts[id];
			if(!ret.empty()) {
				return make_pair(true, ret);
			}
		}
	}

	return make_pair(false, _("No help information available"));
}

void WinUtil::toInts(const string& str, std::vector<int>& array) {
	StringTokenizer<string> t(str, _T(','));
	StringList& l = t.getTokens();

	for(size_t i = 0; i < l.size() && i < array.size(); ++i) {
		array[i] = Util::toInt(l[i]);
	}
}

pair<ButtonPtr, ButtonPtr> WinUtil::addDlgButtons(GridPtr grid) {
	Button::Seed seed;

	seed.caption = T_("OK");
	seed.style |= BS_DEFPUSHBUTTON;
	seed.menuHandle = reinterpret_cast<HMENU> (IDOK);
	seed.padding.x = 20;
	ButtonPtr ok = grid->addChild(seed);
	ok->setHelpId(IDH_DCPP_OK);
	ok->setImage(buttonIcon(IDI_OK));

	seed.caption = T_("Cancel");
	seed.style &= ~BS_DEFPUSHBUTTON;
	seed.menuHandle = reinterpret_cast<HMENU> (IDCANCEL);
	seed.padding.x = 10;
	ButtonPtr cancel = grid->addChild(seed);
	cancel->setHelpId(IDH_DCPP_CANCEL);
	cancel->setImage(buttonIcon(IDI_CANCEL));

	return make_pair(ok, cancel);
}

ButtonPtr WinUtil::addHelpButton(GridPtr grid) {
	Button::Seed seed(T_("Help"));
	seed.padding.x = 10;
	ButtonPtr button = grid->addChild(seed);
	button->setHelpId(IDH_DCPP_HELP);
	button->setImage(buttonIcon(IDI_HELP));
	return button;
}

void WinUtil::addSearchIcon(TextBoxPtr box) {
	// add a search icon by creating a transparent label on top of the text control.

	dcassert(box->hasStyle(WS_CLIPCHILDREN));

	// structure of the right border: text | spacing | icon | margin | right border
	const int spacing = 2, size = 16;
	const auto margin = HIWORD(box->sendMessage(EM_GETMARGINS));
	box->sendMessage(EM_SETMARGINS, EC_RIGHTMARGIN, MAKELONG(0, spacing + size + margin));

	auto label = dwt::WidgetCreator<Label>::create(box, Label::Seed(createIcon(IDI_SEARCH, 16)));
	setColor(label);
	box->onSized([box, label, size, margin](const dwt::SizedEvent&) {
		auto sz = box->getClientSize();
		label->resize(dwt::Rectangle(sz.x - margin - size, std::max(sz.y - size, 0L) / 2, size, size));
	});
}

void WinUtil::addFilterMethods(ComboBoxPtr box) {
	tstring methods[StringMatch::METHOD_LAST] = { T_("Partial match"), T_("Exact match"), T_("Regular Expression") };
	std::for_each(methods, methods + StringMatch::METHOD_LAST, [box](const tstring& str) { box->addValue(str); });
}

void WinUtil::fillTriboolCombo(ComboBoxPtr box) {
	box->addValue(T_("Default"));
	box->addValue(T_("Yes"));
	box->addValue(T_("No"));
}

void WinUtil::preventSpaces(TextBoxPtr box) {
	box->onUpdated([box] {
		auto text = box->getText();
		bool update = false;

		// Strip ' '
		tstring::size_type i;
		while((i = text.find(' ')) != string::npos) {
			text.erase(i, 1);
			update = true;
		}

		if(update) {
			// Something changed; update window text without changing cursor pos
			long caretPos = box->getCaretPos() - 1;
			box->setText(text);
			box->setSelection(caretPos, caretPos);
		}
	});
}

void WinUtil::setColor(dwt::Control* widget) {
	widget->setColor(textColor, bgColor);

	widget->onCommand([widget] {
		widget->setColor(textColor, bgColor);
		widget->redraw();
	}, ID_UPDATECOLOR);
}

HLSCOLOR RGB2HLS(COLORREF rgb) {
	unsigned char minval = min(GetRValue(rgb), min(GetGValue(rgb), GetBValue(rgb)));
	unsigned char maxval = max(GetRValue(rgb), max(GetGValue(rgb), GetBValue(rgb)));
	float mdiff = float(maxval) - float(minval);
	float msum = float(maxval) + float(minval);

	float luminance = msum / 510.0f;
	float saturation = 0.0f;
	float hue = 0.0f;

	if(maxval != minval) {
		float rnorm = (maxval - GetRValue(rgb)) / mdiff;
		float gnorm = (maxval - GetGValue(rgb)) / mdiff;
		float bnorm = (maxval - GetBValue(rgb)) / mdiff;

		saturation = (luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

		if(GetRValue(rgb) == maxval)
			hue = 60.0f * (6.0f + bnorm - gnorm);
		if(GetGValue(rgb) == maxval)
			hue = 60.0f * (2.0f + rnorm - bnorm);
		if(GetBValue(rgb) == maxval)
			hue = 60.0f * (4.0f + gnorm - rnorm);
		if(hue > 360.0f)
			hue = hue - 360.0f;
	}
	return HLS ((hue*255)/360, luminance*255, saturation*255);
}

static inline BYTE _ToRGB(float rm1, float rm2, float rh) {
	if(rh > 360.0f)
		rh -= 360.0f;
	else if(rh < 0.0f)
		rh += 360.0f;

	if(rh < 60.0f)
		rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;
	else if(rh < 180.0f)
		rm1 = rm2;
	else if(rh < 240.0f)
		rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;

	return (BYTE) (rm1 * 255);
}

COLORREF HLS2RGB(HLSCOLOR hls) {
	float hue = ((int) HLS_H(hls) * 360) / 255.0f;
	float luminance = HLS_L(hls) / 255.0f;
	float saturation = HLS_S(hls) / 255.0f;

	if(saturation == 0.0f) {
		return RGB (HLS_L(hls), HLS_L(hls), HLS_L(hls));
	}
	float rm1, rm2;

	if(luminance <= 0.5f)
		rm2 = luminance + luminance * saturation;
	else
		rm2 = luminance + saturation - luminance * saturation;
	rm1 = 2.0f * luminance - rm2;
	BYTE red = _ToRGB(rm1, rm2, hue + 120.0f);
	BYTE green = _ToRGB(rm1, rm2, hue);
	BYTE blue = _ToRGB(rm1, rm2, hue - 120.0f);

	return RGB (red, green, blue);
}

COLORREF HLS_TRANSFORM(COLORREF rgb, int percent_L, int percent_S) {
	HLSCOLOR hls = RGB2HLS(rgb);
	BYTE h = HLS_H(hls);
	BYTE l = HLS_L(hls);
	BYTE s = HLS_S(hls);

	if(percent_L > 0) {
		l = BYTE(l + ((255 - l) * percent_L) / 100);
	} else if(percent_L < 0) {
		l = BYTE((l * (100 + percent_L)) / 100);
	}
	if(percent_S > 0) {
		s = BYTE(s + ((255 - s) * percent_S) / 100);
	} else if(percent_S < 0) {
		s = BYTE((s * (100 + percent_S)) / 100);
	}
	return HLS2RGB(HLS(h, l, s));
}

// check that col0 is within COLORREF bounds; adjust col1 & col2 otherwise.
namespace { void checkColor(int16_t& col0, int16_t& col1, int16_t& col2) {
	if(col0 < 0) {
		int16_t delta = -col0;
		col0 = 0;
		col1 += delta;
		col2 += delta;
		if(col1 > 255) { col1 = 255; }
		if(col2 > 255) { col2 = 255; }
	} else if(col0 > 255) {
		int16_t delta = col0 - 255;
		col0 = 255;
		col1 -= delta;
		col2 -= delta;
		if(col1 < 0) { col1 = 0; }
		if(col2 < 0) { col2 = 0; }
	}
} }

COLORREF modRed(COLORREF col, int16_t mod) {
	int16_t r = GetRValue(col) + mod, g = GetGValue(col), b = GetBValue(col);
	checkColor(r, g, b);
	return RGB(r, g, b);
}

COLORREF modGreen(COLORREF col, int16_t mod) {
	int16_t r = GetRValue(col), g = GetGValue(col) + mod, b = GetBValue(col);
	checkColor(g, r, b);
	return RGB(r, g, b);
}

COLORREF modBlue(COLORREF col, int16_t mod) {
	int16_t r = GetRValue(col), g = GetGValue(col), b = GetBValue(col) + mod;
	checkColor(b, r, g);
	return RGB(r, g, b);
}

namespace {

void regChanged() {
	::SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

bool registerHandler_(const tstring& name, const tstring& descr, bool url, const tstring& prefix) {
	HKEY hk;
	TCHAR Buf[512];
	Buf[0] = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name + _T("\\Shell\\Open\\Command")).c_str(),
		0, KEY_WRITE | KEY_READ, &hk) == ERROR_SUCCESS)
	{
		DWORD bufLen = sizeof(Buf);
		DWORD type;
		::RegQueryValueEx(hk, NULL, 0, &type, (LPBYTE) Buf, &bufLen);
		::RegCloseKey(hk);
	}

	tstring app = _T("\"") + dwt::Application::instance().getModuleFileName() + _T("\" \"") + prefix + _T("%1\"");
	if(Util::stricmp(app.c_str(), Buf) == 0) {
		// already registered to us
		return true;
	}

	if(::RegCreateKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name).c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return false;
	}

	::RegSetValueEx(hk, NULL, 0, REG_SZ, (LPBYTE) descr.c_str(), sizeof(TCHAR) * (descr.size() + 1));
	if(url) {
		::RegSetValueEx(hk, _T("URL Protocol"), 0, REG_SZ, (LPBYTE) _T(""), sizeof(TCHAR));
	}
	::RegCloseKey(hk);

	if(::RegCreateKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name + _T("\\Shell\\Open\\Command")).c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) != ERROR_SUCCESS)
	{
		return false;
	}
	bool ret = ::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE) app.c_str(), sizeof(TCHAR) * (app.length() + 1)) == ERROR_SUCCESS;
	::RegCloseKey(hk);

	if(::RegCreateKeyEx(HKEY_CURRENT_USER, (_T("Software\\Classes\\") + name + _T("\\DefaultIcon")).c_str(),
		0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS)
	{
		app = dwt::Application::instance().getModuleFileName();
		::RegSetValueEx(hk, _T(""), 0, REG_SZ, (LPBYTE) app.c_str(), sizeof(TCHAR) * (app.length() + 1));
		::RegCloseKey(hk);
	}

	return ret;
}

/// @todo var template
bool registerHandler(const tstring& name, const tstring& description, bool url, const tstring& prefix = Util::emptyStringT) {
	bool ret = registerHandler_(name, description, url, prefix);
	if(ret) {
		regChanged();
	} else {
		LogManager::getInstance()->message(str(F_("Error registering the %1% handler") % Text::fromT(name)));
	}
	return ret;
}

} // unnamed namespace

void WinUtil::registerHubHandlers() {
	if(SETTING(URL_HANDLER)) {
		if(!urlDcADCRegistered) {
			urlDcADCRegistered = registerHandler(_T("dchub"), _T("URL:Direct Connect Protocol"), true) &&
				registerHandler(_T("adc"), _T("URL:Direct Connect Protocol"), true) &&
				registerHandler(_T("adcs"), _T("URL:Direct Connect Protocol"), true);
		}
	} else if(urlDcADCRegistered) {
		urlDcADCRegistered =
			::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\dchub")) != ERROR_SUCCESS ||
			::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\adc")) == ERROR_SUCCESS ||
			::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\adcs")) == ERROR_SUCCESS;
		regChanged();
	}
}

void WinUtil::registerMagnetHandler() {
	if(SETTING(MAGNET_REGISTER)) {
		if(!urlMagnetRegistered) {
			urlMagnetRegistered = registerHandler(_T("magnet"), _T("URL:Magnet"), true);
		}
	} else if(urlMagnetRegistered) {
		urlMagnetRegistered = ::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\magnet")) != ERROR_SUCCESS;
		regChanged();
	}
}

void WinUtil::registerDcextHandler() {
	if(SETTING(DCEXT_REGISTER)) {
		if(!dcextRegistered) {
			dcextRegistered = registerHandler(_T(".dcext"), _T("DC plugin"), false, _T("dcext:"));
		}
	} else if(dcextRegistered) {
		dcextRegistered = ::SHDeleteKey(HKEY_CURRENT_USER, _T("Software\\Classes\\.dcext")) != ERROR_SUCCESS;
		regChanged();
	}
}

void WinUtil::setApplicationStartupRegister()
{
	HKEY hk;
	DWORD ret = 0;

	if(::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
		0, KEY_WRITE | KEY_READ, &hk) != ERROR_SUCCESS)
	{
		ret = ::RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
							   0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL);

		if(ret != ERROR_SUCCESS)
		{
			LogManager::getInstance()->message(str(F_("Error registering DC++ for automatic startup (could not find or create key)")));
			return;
		}
	}

	tstring app = _T("\"") + dwt::Application::instance().getModuleFileName() + _T("\"");

	ret = ::RegSetValueEx(hk, _T("DC++"), 0, REG_SZ, (LPBYTE) app.c_str(), sizeof(TCHAR) * (app.length() + 1));
	if(ret != ERROR_SUCCESS)
	{
		LogManager::getInstance()->message(str(F_("Error registering DC++ for automatic startup (could not set key value)")));
	}

	::RegCloseKey(hk);
}

void WinUtil::setApplicationStartupUnregister()
{
	HKEY hk;
	DWORD ret = 0;

	ret = ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
						 0, KEY_WRITE | KEY_READ, &hk);
	if(ret != ERROR_SUCCESS)
	{
		// Could not find key, well, that's OK.
		return;
	}

	tstring app = _T("\"") + dwt::Application::instance().getModuleFileName() + _T("\"");

	TCHAR Buf[512];
	Buf[0] = 0;
	DWORD bufLen = sizeof(Buf);
	DWORD type;

	ret = ::RegQueryValueEx(hk, _T("DC++"), 0, &type, (LPBYTE) Buf, &bufLen);
	if(ret == ERROR_SUCCESS)
	{
		bool bEqualApplications = Util::stricmp(app.c_str(), Buf) == 0;
		if(bEqualApplications) 
		{
			ret = ::RegDeleteValue(hk, _T("DC++"));
			if(ret != ERROR_SUCCESS)
			{
				LogManager::getInstance()->message(str(F_("Error unregistering DC++ for automatic startup (could not delete key value)")));
			}
		}
	}

	::RegCloseKey(hk);
}

void WinUtil::setApplicationStartup()
{
	if(SETTING(REGISTER_SYSTEM_STARTUP))
	{
		setApplicationStartupRegister();
	}
	else
	{
		setApplicationStartupUnregister();
	}
}

void WinUtil::openLink(const tstring& url) {
	::ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

bool WinUtil::parseLink(const tstring& str, bool followExternal) {
	auto url = Text::fromT(str);
	Util::sanitizeUrl(url);

	string proto, host, port, file, query, fragment;
	Util::decodeUrl(url, proto, host, port, file, query, fragment);

	if(Util::stricmp(proto.c_str(), "adc") == 0 ||
	   Util::stricmp(proto.c_str(), "adcs") == 0 ||
	   Util::stricmp(proto.c_str(), "dchub") == 0 )
	{
		HubFrame::openWindow(mainWindow->getTabView(), url);

		/// @todo parse other params when RFCs for these schemes have been published.

		return true;
	}

	if(host == "magnet") {
		string hash, name, key;
		if(Magnet::parseUri(Text::fromT(str), hash, name, key)) {
			MagnetDlg(mainWindow, Text::toT(hash), Text::toT(name), Text::toT(key)).run();
		} else {
			dwt::MessageBox(mainWindow).show(
				T_("A MAGNET link was given to DC++, but it didn't contain a valid file hash for use on the Direct Connect network.  No action will be taken."),
				T_("MAGNET Link detected"), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		}
		return true;
	}

	if(followExternal)
	{
		TStringList lst = StringTokenizer<tstring>(Text::toT(SETTING(WHITELIST_OPEN_URIS)), ';').getTokens();
		for(auto& s : lst)
		{
			if(Util::strnicmp(str.c_str(), s.c_str(), s.size()) == 0)
			{
				openLink(str);
				return true;
			}
		}

		if(!proto.empty())
		{
			auto boxResult = dwt::MessageBox(mainWindow).show(
									T_(
									"Warning: Allowing content to open a program can be useful, but it can potentially harm your computer. "
									"Do not allow it unless you trust the source of the content.") + _T("\n\n") +
									T_("Any program that is launched will have the same access rights as DC++.") + _T("\n\n") +
									T_("The requested link is ") + str + _T("\n\n") +
									T_("Do you still want to allow to open a program on your computer?"),
									T_("External protocol request"),
									dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONEXCLAMATION);
			if(boxResult == dwt::MessageBox::RETBOX_YES) {
				openLink(str);
			}
			return true;
		}
	}

	return false;
}

namespace {

typedef std::function<void(const HintedUser&, const string&)> UserFunction;

void eachUser(const HintedUserList& list, const StringList& dirs, const UserFunction& f) {
	size_t j = 0;
	for(auto& i: list) {
		try {
			f(i, (j < dirs.size()) ? dirs[j] : string());
		} catch (const Exception& e) {
			LogManager::getInstance()->message(e.getError());
		}
		++j;
	}
}

void addUsers(Menu* menu, const tstring& text, const HintedUserList& users, const UserFunction& f,
	const dwt::IconPtr& icon = dwt::IconPtr(), const StringList& dirs = StringList())
{
	if(users.empty())
		return;

	if(users.size() > 1) {
		menu = menu->appendPopup(text, icon);
		menu->appendItem(T_("All"), [=] { eachUser(users, dirs, f); });

		menu->appendSeparator();

		for(size_t i = 0, iend = users.size(); i < iend; ++i) {
			menu->appendItem(WinUtil::getNicks(users[i]), [=] { eachUser(HintedUserList(1, users[i]),
				StringList(1, (i < dirs.size()) ? dirs[i] : Util::emptyString), f); });
		}
	} else {
		menu->appendItem(text, [=] { eachUser(users, dirs, f); }, icon);
	}
}

template<typename F>
HintedUserList filter(const HintedUserList& l, F f) {
	HintedUserList ret;
	for(auto& i: l) {
		if(f(i.user)) {
			ret.push_back(i);
		}
	}
	return ret;
}

bool isFav(const UserPtr& u) {
	return !FavoriteManager::getInstance()->isFavoriteUser(u);
}

} // unnamed namespace

void WinUtil::addUserItems(Menu* menu, const HintedUserList& users, TabViewPtr parent, const StringList& dirs) {
	QueueManager* qm = QueueManager::getInstance();

	addUsers(menu, T_("&Get file list"), users, [=](const HintedUser &u, const string& s) {
		qm->addList(u, QueueItem::FLAG_CLIENT_VIEW, s); }, dwt::IconPtr(), dirs);

	addUsers(menu, T_("&Browse file list"), users, [=](const HintedUser &u, const string& s) {
		qm->addList(u, QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST, s); }, dwt::IconPtr(), dirs);

	addUsers(menu, T_("&Match queue"), users, [=](const HintedUser &u, const string& s) {
		qm->addList(u, QueueItem::FLAG_MATCH_QUEUE, Util::emptyString); });

	addUsers(menu, T_("&Send private message"), users, [parent](const HintedUser& u, const string&) {
		PrivateFrame::openWindow(parent, u); });

	addUsers(menu, T_("Add To &Favorites"), filter(users, &isFav), [=](const HintedUser &u, const string& s) {
		FavoriteManager::getInstance()->addFavoriteUser(u); }, dwt::IconPtr(new dwt::Icon(IDI_FAVORITE_USER_ON)));

	addUsers(menu, T_("Grant &extra slot"), users, [=](const HintedUser &u, const string& s) {
		UploadManager::getInstance()->reserveSlot(u); });

	addUsers(menu, T_("Remove user from queue"), users, [=](const HintedUser &u, const string& s) {
		qm->removeSource(u, QueueItem::Source::FLAG_REMOVED); });
}

void WinUtil::makeColumns(dwt::TablePtr table, const ColumnInfo* columnInfo, size_t columnCount, const string& order,
	const string& widths)
{
	std::vector<dwt::Column> n(columnCount);
	std::vector<int> o(columnCount);

	for(size_t i = 0; i < columnCount; ++i) {
		n[i].header = T_(columnInfo[i].name);
		n[i].width = columnInfo[i].size;
		n[i].alignment = columnInfo[i].numerical ? dwt::Column::RIGHT : dwt::Column::LEFT;
		o[i] = i;
	}

	toInts(order, o);
	size_t i = 0;
	Text::tokenize(widths, ',', [&] (const string& w) { if(i < n.size()) n[i++].width = Util::toInt(w); });

	table->setColumns(n);
	table->setColumnOrder(o);
}

void WinUtil::addCopyMenu(Menu* menu, dwt::TablePtr table) {
	if(!table->hasSelected()) { return; }

	menu->appendSeparator();
	menu = menu->appendPopup(T_("Copy"));

	auto cols = table->getColumns();
	auto order = table->getColumnOrder();

	auto copyF = [table](unsigned col) -> function<void ()> { return [=] {
		tstring text;
		for(auto row: table->getSelection()) {
			if(!text.empty()) { text += _T("\r\n"); }
			text += table->getText(row, col);
		}
		setClipboard(text);
	}; };
	for(auto col: order) {
		menu->appendItem(cols[col].header, copyF(col));
	}

	menu->appendSeparator();
	menu->appendItem(T_("All columns"), [=] {
		tstring text;
		for(auto row: table->getSelection()) {
			tstring rowText;
			for(auto col: order) {
				if(!rowText.empty()) { rowText += _T("\r\n"); }
				rowText += str(TF_("%1%: %2%") % cols[col].header % table->getText(row, col));
			}
			if(!text.empty()) { text += _T("\r\n\r\n"); }
			text += move(rowText);
		}
		setClipboard(text);
	});
}

int WinUtil::tableSortSetting(int column, bool ascending) {
	return ascending || column == -1 ? column : -column - 2;
}

pair<int, bool> WinUtil::tableSortSetting(int columnCount, int setting, int defaultCol, bool defaultAscending) {
	SettingsManager::getInstance()->setDefault(static_cast<SettingsManager::IntSetting>(setting),
		tableSortSetting(defaultCol, defaultAscending));
	auto s = SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(setting));
	auto ret = s >= -1 ? make_pair(s, true) : make_pair(-s - 2, false);
	if(ret.first >= columnCount) {
		return make_pair(defaultCol, defaultAscending);
	}
	return ret;
}

dwt::IconPtr WinUtil::createIcon(unsigned id, long size) {
	return new dwt::Icon(id, dwt::Point(size, size));
}

dwt::IconPtr WinUtil::toolbarIcon(unsigned id) {
	return createIcon(id, SETTING(TOOLBAR_SIZE));
}

dwt::IconPtr WinUtil::mergeIcons(const std::vector<int>& iconIds)
{
	const dwt::Point size(16, 16);
	dwt::ImageList icons(size);

	for(auto& item : iconIds)
	{
		auto icon = createIcon(item, 16);
		icons.add(*icon);
	}
	
	return dwt::util::merge(icons);
}

void WinUtil::getHubStatus(const string& url, tstring& statusText, int& statusIcon)
{
	enum {
		HUB_ON_ICON,
		HUB_OFF_ICON
	};

	statusText = Util::emptyStringT;
	statusIcon = -1;

	// This checks if the hub is opened at all, but does not say whether it's connected or not.
	if(ClientManager::getInstance()->isConnected(url))
	{
		// This will check the actual connectivity to the hub.
		if(ClientManager::getInstance()->isHubConnected(url))
		{
			statusText = T_("Connected");
			statusIcon = HUB_ON_ICON;
		}
		else
		{
			statusText = T_("Disconnected");
			statusIcon = HUB_OFF_ICON;
		}
	}
}
