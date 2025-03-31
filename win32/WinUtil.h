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

#ifndef DCPLUSPLUS_WIN32_WIN_UTIL_H
#define DCPLUSPLUS_WIN32_WIN_UTIL_H

#include <unordered_map>

#include <dcpp/StringTokenizer.h>
#include <dcpp/Util.h>
#include <dcpp/User.h>
#include <dcpp/forward.h>
#include <dcpp/MerkleTree.h>
#include <dcpp/HintedUser.h>
#include <dcpp/Encoder.h>

#include <dwt/forward.h>
#include <dwt/widgets/Button.h>
#include <dwt/widgets/CheckBox.h>
#include <dwt/widgets/ComboBox.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Table.h>
#include <dwt/widgets/TabView.h>
#include <dwt/widgets/Tree.h>

#include "forward.h"
#include "RichTextBox.h"

using std::unordered_map;

using dwt::Button;
using dwt::CheckBox;
using dwt::ComboBox;
using dwt::GroupBox;
using dwt::Label;
using dwt::Menu;
using dwt::TextBox;
using dwt::Table;
using dwt::TabView;
using dwt::Tree;

// Some utilities for handling HLS colors, taken from Jean-Michel LE FOL's codeproject
// article on WTL OfficeXP Menus
typedef DWORD HLSCOLOR;
#define HLS(h,l,s) ((HLSCOLOR)(((BYTE)(h)|((WORD)((BYTE)(l))<<8))|(((DWORD)(BYTE)(s))<<16)))
#define HLS_H(hls) ((BYTE)(hls))
#define HLS_L(hls) ((BYTE)(((WORD)(hls)) >> 8))
#define HLS_S(hls) ((BYTE)((hls)>>16))

HLSCOLOR RGB2HLS (COLORREF rgb);
COLORREF HLS2RGB (HLSCOLOR hls);

COLORREF HLS_TRANSFORM (COLORREF rgb, int percent_L, int percent_S);

// rudimentary functions to adjust the red / green / blue value of a color.
COLORREF modRed(COLORREF col, int16_t mod);
COLORREF modGreen(COLORREF col, int16_t mod);
COLORREF modBlue(COLORREF col, int16_t mod);

class MainWindow;

struct ColumnInfo {
	const char* name;
	const int size;
	const bool numerical;
};

class WinUtil {
public:
	// pre-defined icon indexes used by the "fileImages" image list - see also getFileIcon.
	// also contains icons used by the transfer list.
	enum {
		DIR_ICON,
		DIR_ICON_INCOMPLETE,

		FILE_ICON_GENERIC,

		TRANSFER_ICON_USER,
		TRANSFER_ICON_PM
	};

	// icon indexes to use with the "userImages" image list.
	enum {
		// base icons
		USER_ICON,
		USER_ICON_AWAY,
		USER_ICON_BOT,

		// modifiers
		USER_ICON_MOD_START,
		USER_ICON_NOCON = USER_ICON_MOD_START,
		USER_ICON_NOSLOT,
		USER_ICON_OP,

		USER_ICON_LAST
	};

	enum NotificationType {
		NOTIFICATION_FINISHED_DL,
		NOTIFICATION_FINISHED_FL,
		NOTIFICATION_MAIN_CHAT,
		NOTIFICATION_PM,
		NOTIFICATION_PM_WINDOW,

		NOTIFICATION_LAST
	};

	struct Notification {
		int sound;
		int balloon;
		const char* title;
		unsigned icon;
	};

	static Notification notifications[NOTIFICATION_LAST];

	static tstring tth;

	static dwt::BrushPtr bgBrush;
	static COLORREF textColor;
	static COLORREF bgColor;
	static dwt::FontPtr font;
	static dwt::FontPtr uploadFont;
	static dwt::FontPtr downloadFont;
private:
	static unordered_map<string, dwt::FontPtr> userMatchFonts; // use getUserMatchFont to access
public:
	static tstring commands;
	static dwt::ImageListPtr fileImages;
	static dwt::ImageListPtr userImages;
	static unordered_map<string, size_t> fileIndexes;
	static TStringList lastDirs;
	static MainWindow* mainWindow;

	struct Seeds {
		static const Button::Seed button;
		static const ComboBox::Seed comboBox;
		static const ComboBox::Seed comboBoxEdit;
		static const CheckBox::Seed checkBox;
		static const CheckBox::Seed splitCheckBox; // +/-
		static const GroupBox::Seed group;
		static const Label::Seed label;
		static const Menu::Seed menu;
		static const Table::Seed table;
		static const TextBox::Seed textBox;
		static const RichTextBox::Seed richTextBox;
		static const TabView::Seed tabs;
		static const Tree::Seed treeView;

		struct Dialog {
			static const ComboBox::Seed comboBox;
			static const ComboBox::Seed comboBoxEdit;
			static const TextBox::Seed textBox;
			static const TextBox::Seed intTextBox;
			static const RichTextBox::Seed richTextBox;
			static const Table::Seed table;
			static const Table::Seed optionsTable;
			static const CheckBox::Seed checkBox;
			static const Button::Seed button;
		};
	};

	static void init();
	static void uninit();

	static void initSeeds();

	static void initFont();
	static tstring encodeFont(LOGFONT const& font);
	static void decodeFont(const tstring& setting, LOGFONT &dest);
	static void updateFont(dwt::FontPtr& font, int setting);
	static void updateUserMatchFonts();
	static dwt::FontPtr getUserMatchFont(const string& key);

	static void updateUploadFont();
	static void updateDownloadFont();

	static void setStaticWindowState(const string& id, bool open);

	static bool checkNick();
	static void handleDblClicks(dwt::TextBoxBase* box);

	static tstring getDescriptiveCommands();
	/**
	 * Check if this is a common /-command.
	 * @param cmd The whole text string, will be updated to contain only the command.
	 * @param param Set to any parameters.
	 * @param message Message that should be sent to the chat.
	 * @param status Message that should be shown in the status line.
	 * @param thirdPerson True if the /me command was used.
	 * @return True if the command was processed, false otherwise.
	 */
	static bool checkCommand(tstring& cmd, tstring& param, tstring& message, tstring& status, bool& thirdPerson);

	static void notify(NotificationType notification, const tstring& balloonText, const std::function<void ()>& balloonCallback = nullptr);
	static void playSound(const tstring& sound);

	static void openFile(const tstring& file);
	static void openFolder(const tstring& file);

	static void makeColumns(dwt::TablePtr table, const ColumnInfo* columnInfo, size_t columnCount,
		const string& order = Util::emptyString, const string& widths = Util::emptyString);
	static void addCopyMenu(Menu* menu, dwt::TablePtr table);

	static void addSearchMenu(Menu* menu, tstring& searchText, const string& hash = Util::emptyString);
	static void getChatSelText(dwt::TextBoxBase* box, tstring& text, const dwt::ScreenCoordinate& pt);

	/* functions to get / set table column sorting. a little trick is used to encode both the
	column index & the "ascending sort" flag into an int. */
private:
	static int tableSortSetting(int column, bool ascending);
	static pair<int, bool> tableSortSetting(int columnCount, int setting, int defaultCol, bool defaultAscending);
public:
	template<typename T>
	static int getTableSort(T table) {
		return tableSortSetting(table->getSortColumn(), table->isAscending());
	}
	template<typename T>
	static void setTableSort(T table, int columnCount, int setting, int defaultCol, bool defaultAscending = true) {
		auto sort = tableSortSetting(columnCount, setting, defaultCol, defaultAscending);
		table->setSort(sort.first, sort.second);
	}

	static std::string toString(const std::vector<int>& tokens);
	static void toInts(const string& str, std::vector<int>& tokens);

	// GUI related
private:
	static pair<ButtonPtr, ButtonPtr> addDlgButtons(GridPtr grid);
public:
	/// @return pair of the ok and cancel buttons.
	template<typename Tok, typename Tcancel>
	static pair<ButtonPtr, ButtonPtr> addDlgButtons(GridPtr grid, Tok f_ok, Tcancel f_cancel) {
		auto ret = addDlgButtons(grid);
		ret.first->onClicked(f_ok);
		ret.second->onClicked(f_cancel);
		return ret;
	}
	static ButtonPtr addHelpButton(GridPtr grid);
	static void addSearchIcon(TextBoxPtr box);
	static void addFilterMethods(ComboBoxPtr box);
	static void fillTriboolCombo(ComboBoxPtr box);
	static void preventSpaces(TextBoxPtr box);

	static void setColor(dwt::Control* widget);

	static size_t getFileIcon(const string& fileName);
	static size_t getFileIcon(const tstring& fileName);

	static bool isShift() { return (::GetKeyState(VK_SHIFT) & 0x8000) > 0; }
	static bool isAlt() { return (::GetKeyState(VK_MENU) & 0x8000) > 0; }
	static bool isCtrl() { return (::GetKeyState(VK_CONTROL) & 0x8000) > 0; }

	static tstring getNicks(const CID& cid);
	/** @return Pair of hubnames as a string and a bool representing the user's online status */
	static pair<tstring, bool> getHubNames(const CID& cid);

	static tstring getNick(const HintedUser& user);
	static tstring getHubName(const HintedUser& user);

	static void reducePaths(string& message);

	// Hash related
	static void addHashItems(Menu* menu, const TTHValue& tth, const tstring& filename, int64_t size);
	static void copyMagnet(const TTHValue& aHash, const tstring& aFile, int64_t size);
	static void searchAny(const tstring& aSearch);
	static void searchHash(const TTHValue& aHash);
	static void searchHub(const tstring& aUrl);
	static bool checkTTH(const tstring& aText) { return aText.size() == 39 && Encoder::isBase32(Text::fromT(aText)); }

	static string makeMagnet(const TTHValue& aHash, const string& aFile, int64_t size);

	static void addLastDir(const tstring& dir);

	static void openLink(const tstring& url);
	static bool browseSaveFile(dwt::Widget* parent, tstring& file);
	static bool browseFileList(dwt::Widget* parent, tstring& file);

	static void setClipboard(const tstring& str);

	static bool getUCParams(dwt::Widget* parent, const UserCommand& cmd, ParamMap& params) noexcept;

	static bool parseLink(const tstring& str, bool followExternal = true);

	static void help(dwt::Control* widget);
	static void helpId(dwt::Control* widget, unsigned id);
	static void helpTooltip(dwt::Control* widget, const dwt::Point& pos);
	static void killHelpTooltip();
	static pair<bool, string> getHelpText(unsigned id);

	// file type & protocol associations
	static void registerHubHandlers();
	static void registerMagnetHandler();
	static void registerDcextHandler();

	static void setApplicationStartupRegister();
	static void setApplicationStartupUnregister();
	static void setApplicationStartup();

	static void addUserItems(Menu* menu, const HintedUserList& users, TabViewPtr parent, const StringList& dirs = StringList());

	/* utility functions to create icons. use these throughout the prog to make it easier to change
	sizes globally should the need arise to later on. */
	static dwt::IconPtr createIcon(unsigned id, long size);
	static inline dwt::IconPtr buttonIcon(unsigned id) { return createIcon(id, 16); }
	static inline dwt::IconPtr menuIcon(unsigned id) { return createIcon(id, 16); }
	static inline dwt::IconPtr statusIcon(unsigned id) { return createIcon(id, 16); }
	static inline dwt::IconPtr tabIcon(unsigned id) { return createIcon(id, 16); }
	static dwt::IconPtr toolbarIcon(unsigned id);

	static dwt::IconPtr mergeIcons(const std::vector<int>& iconIds);

	static void getHubStatus(const string& aUrl, tstring& statusText, int& statusIcon);

private:
	static void initUserMatching();
	static void initHelpPath();

	static DWORD helpCookie;
	static tstring helpPath;
	static StringList helpTexts;

	static bool urlDcADCRegistered;
	static bool urlMagnetRegistered;
	static bool dcextRegistered;
};

#endif // !defined(WIN_UTIL_H)
