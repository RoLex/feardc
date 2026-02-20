/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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

#include "TextFrame.h"

#include <dcpp/File.h>
#include <dcpp/Text.h>
#include <dcpp/WindowInfo.h>

#include <dwt/widgets/FontDialog.h>
#include <dwt/widgets/Grid.h>

#include "MainWindow.h"
#include "WinUtil.h"

using dwt::FontDialog;
using dwt::Grid;
using dwt::GridInfo;

const string TextFrame::id = "Text";
const string& TextFrame::getId() const { return id; }

static const size_t MAX_TEXT_LEN = 64*1024;

void TextFrame::openWindow(TabViewPtr parent, const string& fileName, bool activate, bool temporary) {
	auto window = new TextFrame(parent, fileName, temporary);
	if(activate)
		window->activate();
}

WindowParams TextFrame::getWindowParams() const {
	WindowParams ret;
	ret["Path"] = WindowParam(path, WindowParam::FLAG_IDENTIFIES);
	return ret;
}

void TextFrame::parseWindowParams(TabViewPtr parent, const WindowParams& params) {
	auto path = params.find("Path");
	if(path != params.end()) {
		openWindow(parent, path->second, parseActivateParam(params));
	}
}

TextFrame::TextFrame(TabViewPtr parent, const string& path, bool temporary) :
BaseType(parent, Text::toT(Util::getFileName(path)), IDH_TEXT_VIEWER),
grid(0),
pad(0),
path(path),
temporary(temporary)
{
	setIcon(WinUtil::fileImages->getIcon(WinUtil::getFileIcon(path)));

	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		TextBox::Seed cs = WinUtil::Seeds::textBox;
		cs.style |= WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY;
		if(!SettingsManager::getInstance()->isDefault(SettingsManager::TEXT_VIEWER_FONT)) {
			// the user has configured a different font for text files; use it.
			LOGFONT lf;
			WinUtil::decodeFont(Text::toT(SETTING(TEXT_VIEWER_FONT)), lf);
			cs.font = dwt::FontPtr(new dwt::Font(lf));
		}
		pad = grid->addChild(cs);
		addWidget(pad);
		WinUtil::handleDblClicks(pad);

		pad->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleMenu(sc); });
	}

	{
		Button::Seed seed = WinUtil::Seeds::button;
		seed.caption = T_("Change the text viewer font");
		auto changeFont = grid->addChild(Grid::Seed(1, 1))->addChild(seed);
		changeFont->setHelpId(IDH_TEXT_FONT);
		changeFont->onClicked([this] { handleFontChange(); });
		addWidget(changeFont);
	}

	initStatus();
	status->setText(STATUS_STATUS, str(TF_("Viewing text file: %1%") % Text::toT(path)));

	pad->setTextLimit(0);

	try {
		pad->setText(Text::toT(Text::toDOS(File(path, File::READ, File::OPEN).read(MAX_TEXT_LEN))));
	} catch(const FileException& e) {
		pad->setText(Text::toT(e.getError()));
	}

	layout();
}

void TextFrame::layout() {
	dwt::Rectangle r { getClientSize() };

	r.size.y -= status->refresh();

	r.size.y -= grid->getSpacing(); // add a bottom margin not to be too stuck to the status bar.
	grid->resize(r);
}

void TextFrame::postClosing() {
	if(temporary && !WinUtil::mainWindow->closing()) {
		File::deleteFile(path);
	}
}

void TextFrame::handleFontChange() {
	LOGFONT logFont;
	WinUtil::decodeFont(Text::toT(
		SettingsManager::getInstance()->isDefault(SettingsManager::TEXT_VIEWER_FONT) ?
		SETTING(MAIN_FONT) : SETTING(TEXT_VIEWER_FONT)), logFont);
	DWORD color = 0;
	FontDialog::Options options;
	options.strikeout = false;
	options.underline = false;
	options.color = false;
	if(FontDialog(this).open(logFont, color, &options)) {
		SettingsManager::getInstance()->set(SettingsManager::TEXT_VIEWER_FONT, Text::fromT(WinUtil::encodeFont(logFont)));
		pad->setFont(new dwt::Font(logFont));
	}
}

bool TextFrame::handleMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = pad->getContextMenuPos();
	}

	makeMenu(pt)->open(pt);

	return true;
}

MenuPtr TextFrame::makeMenu(dwt::ScreenCoordinate pt) {
	auto menu = pad->getMenu();

	tstring searchText;
	WinUtil::getChatSelText(pad, searchText, pt);
	WinUtil::addSearchMenu(menu.get(), searchText);

	return menu;
}
