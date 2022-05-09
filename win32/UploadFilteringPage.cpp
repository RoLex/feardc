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

#include "resource.h"

#include "UploadFilteringPage.h"

#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/Spinner.h>

#include <dcpp/format.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/version.h>
#include "ParamDlg.h"
#include "HashProgressDlg.h"
#include "WinUtil.h"
#include "ItemsEditDlg.h"

using dwt::FolderDialog;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Spinner;

UploadFilteringPage::UploadFilteringPage(dwt::Widget* parent) :
PropPage(parent, 6, 1),
minSizeControl(0),
maxSizeControl(0),
modifyRegExButton(0),
modifyExtensionsButton(0),
modifyPathsButton(0)
{
	setHelpId(IDH_UPLOADFILTERINGPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		auto optionsGroup = grid->addChild(GroupBox::Seed(T_("Options")));

		// dummy grid so that the check-box doesn't fill the whole row.
		shareHiddenCheckBox = optionsGroup->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Share hidden files")));
		items.emplace_back(shareHiddenCheckBox, SettingsManager::SHARE_HIDDEN, PropPage::T_BOOL);
		shareHiddenCheckBox->setHelpId(IDH_SETTINGS_UPLOAD_SHAREHIDDEN);
	}

	{
		auto fileSizeGroup = grid->addChild(GroupBox::Seed(T_("Only share files whose file size is above/below")));

		auto fileSizeGrid = fileSizeGroup->addChild(Grid::Seed(1, 2));
		fileSizeGrid->column(0).mode = GridInfo::FILL;
		fileSizeGrid->column(1).mode = GridInfo::FILL;

		{
			dwt::Control* ctrl;
			addItem(fileSizeGrid, ctrl, T_("Above (minimum size)"), SettingsManager::SHARING_SKIPLIST_MINSIZE, PropPage::T_INT64, IDH_SETTINGS_UPLOAD_SKIPLIST_MINSIZE, T_("B"));
			minSizeControl = dynamic_cast<TextBoxPtr>(ctrl);
		}
		{
			dwt::Control* ctrl;
			addItem(fileSizeGrid, ctrl, T_("Below (maximum size)"), SettingsManager::SHARING_SKIPLIST_MAXSIZE, PropPage::T_INT64, IDH_SETTINGS_UPLOAD_SKIPLIST_MAXSIZE, T_("B"));
			maxSizeControl = dynamic_cast<TextBoxPtr>(ctrl);
		}
	}

	{
		auto regexGroup = grid->addChild(GroupBox::Seed(T_("File name filtering with regular expressions")));

		auto regexGrid = regexGroup->addChild(Grid::Seed(2, 2));
		regexGrid->column(0).mode = GridInfo::FILL;

		regexTextBox = regexGrid->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(regexTextBox, SettingsManager::SHARING_SKIPLIST_REGEX, PropPage::T_STR);
		regexTextBox->setHelpId(IDH_SETTINGS_UPLOAD_SKIPLIST_REGEX);

		modifyRegExButton = regexGrid->addChild(Button::Seed(T_("M&odify")));
		modifyRegExButton->onClicked([this] { handleModButtonClicked(T_("Regular expressions"), regexTextBox); });

		regexGrid->addChild(Label::Seed(T_("Use semicolon to separate multiple regular expressions.")));
	}

	{
		auto extensionsGroup = grid->addChild(GroupBox::Seed(T_("File extension filtering")));

		auto extensionsGrid = extensionsGroup->addChild(Grid::Seed(2, 2));
		extensionsGrid->column(0).mode = GridInfo::FILL;

		extensionsTextBox = extensionsGrid->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(extensionsTextBox, SettingsManager::SHARING_SKIPLIST_EXTENSIONS, PropPage::T_STR);
		extensionsTextBox->setHelpId(IDH_SETTINGS_UPLOAD_SKIPLIST_EXTENSIONS);

		modifyExtensionsButton = extensionsGrid->addChild(Button::Seed(T_("M&odify")));
		modifyExtensionsButton->onClicked([this] { handleModButtonClicked(T_("File extensions"), extensionsTextBox); });

		extensionsGrid->addChild(Label::Seed(T_("Use semicolon to separate multiple extensions.")));
	}

	{
		auto pathsGroup = grid->addChild(GroupBox::Seed(T_("Path filtering")));

		auto pathsGrid = pathsGroup->addChild(Grid::Seed(2, 2));
		pathsGrid->column(0).mode = GridInfo::FILL;

		pathsTextBox = pathsGrid->addChild(WinUtil::Seeds::Dialog::textBox);
		items.emplace_back(pathsTextBox, SettingsManager::SHARING_SKIPLIST_PATHS, PropPage::T_STR);
		pathsTextBox->setHelpId(IDH_SETTINGS_UPLOAD_SKIPLIST_PATHS);

		modifyPathsButton = pathsGrid->addChild(Button::Seed(T_("M&odify")));
		modifyPathsButton->onClicked([this] { handleModButtonClicked(T_("Paths"), pathsTextBox); });

		pathsGrid->addChild(Label::Seed(T_("Use semicolon to separate multiple paths.")));
	}

	PropPage::read(items);
}

UploadFilteringPage::~UploadFilteringPage() {
}

void UploadFilteringPage::layout() {
	PropPage::layout();
}

void UploadFilteringPage::write()
{
	if(isModified())
	{
		PropPage::write(items);

		ShareManager::getInstance()->updateFilterCache();

		ShareManager::getInstance()->setDirty();
		ShareManager::getInstance()->refresh(true, false, true);
	}
}

void UploadFilteringPage::addItem(dwt::Grid* parent, dwt::Control*& control, const tstring& text, int setting, PropPage::Type t, unsigned helpId, const tstring& text2) {
	auto group = parent->addChild(GroupBox::Seed(text));
	group->setHelpId(helpId);

	auto cur = group->addChild(Grid::Seed(1, 2));
	if(!text.empty() && !text2.empty())
	{
		cur->column(0).mode = GridInfo::FILL;
	}

	if((t == PropPage::T_INT) || (t == PropPage::T_INT64))
	{
		control = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
	}
	else if(t == PropPage::T_STR)
	{
		control = cur->addChild(WinUtil::Seeds::Dialog::textBox);
	}
	else if(t == PropPage::T_BOOL)
	{
		control = cur->addChild(WinUtil::Seeds::Dialog::checkBox);
	}

	items.emplace_back(control, setting, t);

	if(text2.empty())
		cur->setWidget(control, 0, 0, 1, 2);
	else
		cur->addChild(Label::Seed(text2));
}

void UploadFilteringPage::handleModButtonClicked(const tstring& strName, TextBoxPtr textBox )
{
	handleModButtonClicked(strName, strName, strName, strName, textBox);
}

void UploadFilteringPage::handleModButtonClicked(const tstring& strDialogName, const tstring& strTitle, const tstring& strDescription, const tstring& strEditTitle, TextBoxPtr textBox )
{
	TStringList lst = StringTokenizer<tstring>(textBox->getText(), ';').getTokens();

	// "ensureUniqueness" set to "false" as we don't want case-insensitive uniqueness for paths.
	ItemsEditDlg dlg(this, strDialogName, lst, false);
	dlg.setTitle(strTitle);
	dlg.setDescription(strDescription);
	dlg.setEditTitle(strEditTitle);

	if(dlg.run() == IDOK)
	{
		StringList lstValue;
		Text::fromT(dlg.getValues(), lstValue);

		textBox->setText(Text::toT(Util::toString(";", lstValue)));
	}
}

bool UploadFilteringPage::isModified()
{
	bool isModified = 
		(shareHiddenCheckBox->getChecked() != SETTING(SHARE_HIDDEN)) ||
		(Util::toInt64(Text::fromT(minSizeControl->getText())) != SETTING(SHARING_SKIPLIST_MINSIZE)) ||
		(Util::toInt64(Text::fromT(maxSizeControl->getText())) != SETTING(SHARING_SKIPLIST_MAXSIZE)) ||
		(Text::fromT(regexTextBox->getText()) != SETTING(SHARING_SKIPLIST_REGEX)) ||
		(Text::fromT(extensionsTextBox->getText()) != SETTING(SHARING_SKIPLIST_EXTENSIONS)) ||
		(Text::fromT(pathsTextBox->getText()) != SETTING(SHARING_SKIPLIST_PATHS));
	return isModified;
}
