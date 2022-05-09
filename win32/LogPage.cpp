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
#include "LogPage.h"

#include <dcpp/SettingsManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/File.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

PropPage::ListItem LogPage::listItems[] = {
	{ SettingsManager::LOG_MAIN_CHAT, N_("Log main chat"), IDH_SETTINGS_LOG_MAIN_CHAT },
	{ SettingsManager::LOG_PRIVATE_CHAT, N_("Log private chat"), IDH_SETTINGS_LOG_PRIVATE_CHAT },
	{ SettingsManager::LOG_DOWNLOADS, N_("Log downloaded segments"), IDH_SETTINGS_LOG_DOWNLOADS },
	{ SettingsManager::LOG_FINISHED_DOWNLOADS, N_("Log finished downloads"), IDH_SETTINGS_LOG_FINISHED_DOWNLOADS },
	{ SettingsManager::LOG_UPLOADS, N_("Log uploaded segments"), IDH_SETTINGS_LOG_UPLOADS },
	{ SettingsManager::LOG_SYSTEM, N_("Log system messages"), IDH_SETTINGS_LOG_SYSTEM },
	{ SettingsManager::LOG_STATUS_MESSAGES, N_("Log status messages"), IDH_SETTINGS_LOG_STATUS_MESSAGES },
	{ SettingsManager::LOG_FILELIST_TRANSFERS, N_("Log filelist transfers"), IDH_SETTINGS_LOG_FILELIST_TRANSFERS },
	{ 0, 0 }
};

LogPage::LogPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
dir(0),
options(0),
logFormat(0),
logFile(0),
dontLogCCPMCheckBox(0),
oldSelection(-1)
{
	setHelpId(IDH_LOGPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	auto group = grid->addChild(GroupBox::Seed(T_("Logging")));

	GridPtr grid = group->addChild(Grid::Seed(4, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 3));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Directory")))->setHelpId(IDH_SETTINGS_LOG_DIRECTORY);
		dir = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(dir, SettingsManager::LOG_DIRECTORY, PropPage::T_STR);
		dir->setHelpId(IDH_SETTINGS_LOG_DIRECTORY);

		ButtonPtr browse = cur->addChild(Button::Seed(T_("&Browse...")));
		browse->onClicked([this] { handleBrowseDir(dir, SettingsManager::LOG_DIRECTORY); });
		browse->setHelpId(IDH_SETTINGS_LOG_DIRECTORY);
	}

	options = grid->addChild(WinUtil::Seeds::Dialog::optionsTable);

	{
		GridPtr cur = grid->addChild(Grid::Seed(2, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;

		cur->addChild(Label::Seed(T_("Format")));
		logFormat = cur->addChild(WinUtil::Seeds::Dialog::textBox);

		cur->addChild(Label::Seed(T_("Filename")));
		logFile = cur->addChild(WinUtil::Seeds::Dialog::textBox);
	}

	{
		dontLogCCPMCheckBox = grid->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Do not log the direct encrypted secure private chat")));
		items.emplace_back(dontLogCCPMCheckBox, SettingsManager::DONT_LOG_CCPM, PropPage::T_BOOL);
		dontLogCCPMCheckBox->setHelpId(IDH_SETTINGS_DONT_LOG_CCPM_CHAT); 
	}

	PropPage::read(items);
	PropPage::read(listItems, options);

	for(int i = 0; i < LogManager::LAST; ++i) {
		TStringPair pair;
		pair.first = Text::toT(LogManager::getInstance()->getSetting(i, LogManager::FILE));
		pair.second = Text::toT(LogManager::getInstance()->getSetting(i, LogManager::FORMAT));
		logOptions.push_back(pair);
	}

	logFormat->setEnabled(false);
	logFile->setEnabled(false);

	options->onSelectionChanged([this] { handleSelectionChanged(); });
}

LogPage::~LogPage() {
}

void LogPage::write() {
	PropPage::write(items);
	PropPage::write(options);

	const string& s = SETTING(LOG_DIRECTORY);
	if(s.length() > 0 && s[s.length() - 1] != '\\') {
		SettingsManager::getInstance()->set(SettingsManager::LOG_DIRECTORY, s + '\\');
	}
	File::ensureDirectory(SETTING(LOG_DIRECTORY));

	//make sure we save the last edit too, the user
	//might not have changed the selection
	getValues();

	for(int i = 0; i < LogManager::LAST; ++i) {
		string tmp = Text::fromT(logOptions[i].first);
		if(Util::stricmp(Util::getFileExt(tmp), ".log") != 0)
			tmp += ".log";

		LogManager::getInstance()->saveSetting(i, LogManager::FILE, tmp);
		LogManager::getInstance()->saveSetting(i, LogManager::FORMAT, Text::fromT(logOptions[i].second));
	}
}

void LogPage::handleSelectionChanged() {
	getValues();

	int sel = options->getSelected();
	if(sel >= 0 && sel < LogManager::LAST) {
		bool checkState = options->isChecked(sel);
		logFormat->setEnabled(checkState);
		logFile->setEnabled(checkState);

		logFile->setText(logOptions[sel].first);
		logFormat->setText(logOptions[sel].second);
	} else {
		logFormat->setEnabled(false);
		logFile->setEnabled(false);

		logFile->setText(Util::emptyStringT);
		logFormat->setText(Util::emptyStringT);
	}
	if(sel < LogManager::LAST)
		oldSelection = sel;
}

void LogPage::getValues() {
	if(oldSelection >= 0) {
		logOptions[oldSelection].first = logFile->getText();
		logOptions[oldSelection].second = logFormat->getText();
	}
}
