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

#ifndef DCPLUSPLUS_WIN32_STYLES_PAGE_H
#define DCPLUSPLUS_WIN32_STYLES_PAGE_H

#include <dcpp/UserMatch.h>

#include "PropPage.h"

class StylesPage : public PropPage
{
public:
	StylesPage(dwt::Widget* parent);
	virtual ~StylesPage();

	virtual void layout();
	virtual void write();

	void updateUserMatches(std::vector<UserMatch>& userMatches);

private:
	enum {
		COLUMN_TEXT,

		COLUMN_LAST
	};

	enum {
		GROUP_GENERAL,
		GROUP_TRANSFERS,
		GROUP_CHAT,
		GROUP_USERS,

		GROUP_LAST
	};

	class Data : public Flags {
	protected:
		typedef pair<dwt::FontPtr, LOGFONT> Font;

	public:
		enum { FONT_CHANGEABLE = 1 << 0, TEXT_COLOR_CHANGEABLE = 1 << 1, BG_COLOR_CHANGEABLE = 1 << 2 };

		Data(tstring&& text, const unsigned helpId);
		virtual ~Data() { }

		const tstring& getText(int) const;
		int getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const;

		virtual const Font& getFont() const { return defaultFont; }
		virtual int getTextColor() const { return -1; }
		virtual int getBgColor() const { return -1; }

		virtual void update() { }
		virtual void write() { }

		const tstring text;
		const unsigned helpId;

		bool customFont;
		Font font;
		Font defaultFont;

		bool customTextColor;
		int textColor;

		bool customBgColor;
		int bgColor;

	protected:
		static void makeFont(Font& dest, const string& setting);
	};

	class SettingsData : public Data {
	public:
		SettingsData(tstring&& text, unsigned helpId, int fontSetting, int textColorSetting, int bgColorSetting);

		const Font& getFont() const;
		int getTextColor() const;
		int getBgColor() const;

		void write();

	private:
		const int fontSetting;
		const int textColorSetting;
		const int bgColorSetting;
	};

	class UserMatchData : public Data {
	public:
		UserMatchData(UserMatch& matcher);

		const Font& getFont() const;
		int getTextColor() const;
		int getBgColor() const;

		void update();

	private:
		UserMatch& matcher;
	};

	Data* globalData;

	typedef TypedTable<Data> Table;
	Table* table;

	LabelPtr preview;

	CheckBoxPtr customFont;
	ButtonPtr font;

	CheckBoxPtr customTextColor;
	ButtonPtr textColor;

	CheckBoxPtr customBgColor;
	ButtonPtr bgColor;

	CheckBoxPtr showGen;

	void handleSelectionChanged();
	void handleTableHelpId(unsigned& id);

	void handleCustomFont();
	void handleFont();

	void handleCustomTextColor();
	void handleTextColor();

	void handleCustomBgColor();
	void handleBgColor();

	int colorDialog(COLORREF color);
	void initFont(Data* const data) const;
	COLORREF getTextColor(const Data* const data) const;
	COLORREF getBgColor(const Data* const data) const;
	void update(Data* const data);
	void updatePreview(Data* const data);
};

#endif
