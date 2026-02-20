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

#include "resource.h"

#include "FavoriteDirsPage.h"

#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/MessageBox.h>

#include <dcpp/SettingsManager.h>
#include <dcpp/FavoriteManager.h>
#include <dcpp/version.h>
#include "WinUtil.h"
#include "ParamDlg.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::FolderDialog;

static const ColumnInfo columns[] = {
	{ N_("Favorite name"), 100, false },
	{ N_("Directory"), 100, false }
};

FavoriteDirsPage::FavoriteDirsPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
directories(0),
rename(0),
remove(0)
{
	setHelpId(IDH_FAVORITE_DIRSPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	auto group = grid->addChild(GroupBox::Seed(T_("Favorite download directories")));
	group->setHelpId(IDH_SETTINGS_FAVORITE_DIRS_FAVORITE_DIRECTORIES);
	GridPtr grid = group->addChild(Grid::Seed(2, 3));
	grid->column(0).mode = dwt::GridInfo::FILL;
	grid->column(1).mode = dwt::GridInfo::FILL;
	grid->column(2).mode = dwt::GridInfo::FILL;
	grid->row(0).mode = dwt::GridInfo::FILL;
	grid->row(0).align = dwt::GridInfo::STRETCH;

	{
		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.exStyle |= WS_EX_ACCEPTFILES;
		directories = grid->addChild(seed);
		grid->setWidget(directories, 0, 0, 1, 3);
	}

	ButtonPtr add = grid->addChild(Button::Seed(T_("&Add folder")));
	add->onClicked([this] { handleAddClicked(); });
	add->setHelpId(IDH_SETTINGS_FAVORITE_DIRS_ADD);
	rename = grid->addChild(Button::Seed(T_("Re&name")));
	rename->onClicked([this] { handleRenameClicked(); });
	rename->setHelpId(IDH_SETTINGS_FAVORITE_DIRS_RENAME);
	remove = grid->addChild(Button::Seed(T_("&Remove")));
	remove->onClicked([this] { handleRemoveClicked(); });
	remove->setHelpId(IDH_SETTINGS_FAVORITE_DIRS_REMOVE);

	WinUtil::makeColumns(directories, columns, 2);

	for(auto& j: FavoriteManager::getInstance()->getFavoriteDirs())
		addRow(Text::toT(j.second), Text::toT(j.first));

	handleSelectionChanged();

	directories->onDblClicked([this] { handleDoubleClick(); });
	directories->onKeyDown([this](int c) { return handleKeyDown(c); });
	directories->onSelectionChanged([this] { handleSelectionChanged(); });
	directories->onDragDrop([this] (const TStringList &files, dwt::Point) { handleDragDrop(files); });
}

FavoriteDirsPage::~FavoriteDirsPage() {
}

void FavoriteDirsPage::layout() {
	PropPage::layout();

	directories->setColumnWidth(1, directories->getWindowSize().x - 120);
}

void FavoriteDirsPage::handleDoubleClick() {
	if(directories->hasSelected()) {
		handleRenameClicked();
	} else {
		handleAddClicked();
	}
}

bool FavoriteDirsPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddClicked();
		return true;
	case VK_DELETE:
		handleRemoveClicked();
		return true;
	}
	return false;
}

void FavoriteDirsPage::handleSelectionChanged() {
	bool enable = directories->hasSelected();
	rename->setEnabled(enable);
	remove->setEnabled(enable);
}

void FavoriteDirsPage::handleDragDrop(const TStringList& files) {
	for(auto& i: files)
		if(PathIsDirectory(i.c_str()))
			addDirectory(i);
}

void FavoriteDirsPage::handleAddClicked() {
	tstring target;
	if(FolderDialog(this).open(target)) {
		addDirectory(target);
	}
}

void FavoriteDirsPage::handleRenameClicked() {
	int i = -1;
	while((i = directories->getNext(i, LVNI_SELECTED)) != -1) {
		tstring old = directories->getText(i, 0);
		ParamDlg dlg(this, T_("Favorite name"), T_("Under what name you see the directory"), old);
		if(dlg.run() == IDOK) {
			tstring line = dlg.getValue();
			if (FavoriteManager::getInstance()->renameFavoriteDir(Text::fromT(old), Text::fromT(line))) {
				directories->setText(i, 0, line);
			} else {
				dwt::MessageBox(this).show(T_("Directory or directory name already exists"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
					dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
			}
		}
	}
}

void FavoriteDirsPage::handleRemoveClicked() {
	int i;
	while((i = directories->getNext(-1, LVNI_SELECTED)) != -1)
		if(FavoriteManager::getInstance()->removeFavoriteDir(Text::fromT(directories->getText(i, 1))))
			directories->erase(i);
}

void FavoriteDirsPage::addRow(const tstring& name, const tstring& path) {
	directories->insert({ name, path });
}

void FavoriteDirsPage::addDirectory(const tstring& aPath) {
	tstring path = aPath;
	if( path[ path.length() -1 ] != PATH_SEPARATOR )
		path += PATH_SEPARATOR;

	ParamDlg dlg(this, T_("Favorite name"), T_("Under what name you see the directory"), Util::getLastDir(path));
	if(dlg.run() == IDOK) {
		const tstring& line = dlg.getValue();
		if(FavoriteManager::getInstance()->addFavoriteDir(Text::fromT(path), Text::fromT(line))) {
			addRow(line, path);
		} else {
			dwt::MessageBox(this).show(T_("Directory or directory name already exists"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
				dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
		}
	}
}
