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
#include "PluginPage.h"

#include "HoldRedraw.h"
#include "PluginApiWin.h"
#include "resource.h"
#include "WinUtil.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Link.h>
#include <dwt/widgets/MessageBox.h>

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/format.h>
#include <dcpp/PluginManager.h>
#include <dcpp/ScopedFunctor.h>
#include <dcpp/version.h>

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Link;

enum { COLUMN_PLUGIN, COLUMN_GUID, COLUMN_COUNT };
static const ColumnInfo columns[COLUMN_COUNT] = {
	{ "", 100, false },
	{ "", 0, false } // hidden column to store the GUID
};

PluginPage::PluginPage(dwt::Widget* parent) :
PropPage(parent, 3, 1),
plugins(0),
add(0),
configure(0),
enable(0),
disable(0),
moveUp(0),
moveDown(0),
remove(0),
pluginInfo(0)
{
	setHelpId(IDH_PLUGINPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;
	grid->setSpacing(6);

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Installed Plugins")));

		auto cur = group->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setHelpId(IDH_SETTINGS_PLUGINS_LIST);
		cur->setSpacing(6);

		auto seed = WinUtil::Seeds::Dialog::table;
		seed.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
		plugins = cur->addChild(seed);

		plugins->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleContextMenu(sc); });	

		{
			auto buttons = cur->addChild(Grid::Seed(2, 4));
			for(size_t i = 0; i < buttons->columnCount(); ++i) {
				buttons->column(i).mode = GridInfo::FILL;
			}
			buttons->setSpacing(cur->getSpacing());

			add = buttons->addChild(Button::Seed(T_("&Add")));
			add->onClicked([this] { handleAddPlugin(); });
			add->setHelpId(IDH_SETTINGS_PLUGINS_ADD);

			configure = buttons->addChild(Button::Seed(T_("&Configure")));
			configure->onClicked([this] { handleConfigurePlugin(); });
			configure->setHelpId(IDH_SETTINGS_PLUGINS_CONFIGURE);

			enable = buttons->addChild(Button::Seed(T_("Enable")));
			enable->onClicked([this] { handleEnable(); });
			enable->setHelpId(IDH_SETTINGS_PLUGINS_ENABLE);

			disable = buttons->addChild(Button::Seed(T_("Disable")));
			disable->onClicked([this] { handleDisable(); });
			disable->setHelpId(IDH_SETTINGS_PLUGINS_DISABLE);

			moveUp = buttons->addChild(Button::Seed(T_("Move &Up")));
			moveUp->onClicked([this] { handleMovePluginUp(); });
			moveUp->setHelpId(IDH_SETTINGS_PLUGINS_MOVE_UP);
			buttons->setWidget(moveUp, 1, 1);

			moveDown = buttons->addChild(Button::Seed(T_("Move &Down")));
			moveDown->onClicked([this] { handleMovePluginDown(); });
			moveDown->setHelpId(IDH_SETTINGS_PLUGINS_MOVE_DOWN);
			buttons->setWidget(moveDown, 1, 2);

			remove = buttons->addChild(Button::Seed(T_("&Remove")));
			remove->onClicked([this] { handleRemovePlugin(); });
			remove->setHelpId(IDH_SETTINGS_PLUGINS_REMOVE);
			buttons->setWidget(remove, 1, 3);
		}
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Plugin information")));
		group->setHelpId(IDH_SETTINGS_PLUGINS_INFO);
		pluginInfo = group->addChild(Grid::Seed(1, 1));
		pluginInfo->column(0).mode = GridInfo::FILL;
		pluginInfo->setSpacing(grid->getSpacing());
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;
		cur->addChild(Label::Seed(str(TF_("Some plugins may require you to restart %1%") % Text::toT(APPNAME))));
		cur->addChild(Link::Seed(_T("<a href=\"https://dcplusplus.sourceforge.io/plugins/\">") + T_("Visit the DC plugin repository") + _T("</a>")));
	}

	WinUtil::makeColumns(plugins, columns, COLUMN_COUNT);

	refreshList();

	handleSelectionChanged();

	plugins->onDblClicked([this] { handleDoubleClick(); });
	plugins->onKeyDown([this] (int c) { return handleKeyDown(c); });
	plugins->onSelectionChanged([this] { handleSelectionChanged(); });
}

PluginPage::~PluginPage() {
}

void PluginPage::layout() {
	PropPage::layout();

	plugins->setColumnWidth(COLUMN_PLUGIN, plugins->getWindowSize().x - 20);
}

void PluginPage::handleDoubleClick() {
	if(plugins->hasSelected()) {
		handleConfigurePlugin();
	}
}

bool PluginPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddPlugin();
		return true;
	case VK_DELETE:
		handleRemovePlugin();
		return true;
	}
	return false;
}

void PluginPage::handleSelectionChanged() {
	auto selected = plugins->hasSelected();
	auto enabled = selected ? PluginManager::getInstance()->isLoaded(sel()) : false;
	configure->setEnabled(selected);
	enable->setEnabled(selected && !enabled);
	disable->setEnabled(selected && enabled);
	moveUp->setEnabled(selected);
	moveDown->setEnabled(selected);
	remove->setEnabled(selected);

	ScopedFunctor([&] { pluginInfo->layout(); pluginInfo->redraw(); });

	HoldRedraw hold { pluginInfo };

	pluginInfo->clearRows();

	/* destroy previous children. store them in a vector beforehand or the enumeration will fail
	(since they're getting destroyed)... */
	auto children = pluginInfo->getChildren<Control>();
	boost::for_each(std::vector<Control*>(children.first, children.second), [](Control* w) { w->close(); });

	pluginInfo->addRow(GridInfo(0, GridInfo::FILL, GridInfo::STRETCH));

	if(plugins->countSelected() != 1) {
		pluginInfo->addChild(Label::Seed(T_("No plugin has been selected")));
		return;
	}

	auto infoGrid = pluginInfo->addChild(Grid::Seed(0, 2));
	infoGrid->column(1).mode = GridInfo::FILL;
	infoGrid->setSpacing(pluginInfo->getSpacing());

	// similar to PluginInfoDlg.cpp

	enum Type { Name, Version, Description, Author, Website, FileName, Path };

	auto addInfo = [this, infoGrid](tstring name, const string& value, Type type) {
		if(type == Description || type == Path) {
			infoGrid->addRow(GridInfo(0, GridInfo::FILL, GridInfo::STRETCH));
		} else {
			infoGrid->addRow();
		}
		infoGrid->addChild(Label::Seed(name));
		if((type == Website || type == Path) && !value.empty()) {
			infoGrid->addChild(Link::Seed(Text::toT(value), true, WinUtil::parseLink));
		} else {
			infoGrid->addChild(Label::Seed(value.empty() ?
				T_("<Information unavailable>") : Text::toT(value)));
		}
	};

	auto plugin = PluginManager::getInstance()->getPlugin(sel());

	addInfo(T_("Name: "), plugin.name, Name);
	addInfo(T_("Version: "), Util::toString(plugin.version), Version);
	addInfo(T_("Description: "), plugin.description, Description);
	addInfo(T_("Author: "), plugin.author, Author);
	addInfo(T_("Website: "), plugin.website, Website);
	addInfo(T_("File name: "), Util::getFileName(plugin.path), FileName);
	addInfo(T_("Installation path: "), Util::getFilePath(plugin.path), Path);
}

bool PluginPage::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = plugins->getContextMenuPos();
	}

	auto selected = plugins->hasSelected();
	auto enabled = selected ? PluginManager::getInstance()->isLoaded(sel()) : false;

	auto menu = addChild(WinUtil::Seeds::menu);

	menu->setTitle(T_("Plugins"), WinUtil::menuIcon(IDI_PLUGINS));
	menu->appendItem(T_("&Add"), [this] { handleAddPlugin(); });
	menu->appendSeparator();
	menu->appendItem(T_("&Configure"), [this] { handleConfigurePlugin(); }, nullptr, selected);
	menu->appendItem(T_("Enable"), [this] { handleEnable(); }, nullptr, selected && !enabled);
	menu->appendItem(T_("Disable"), [this] { handleDisable(); }, nullptr, selected && enabled);
	menu->appendSeparator();
	menu->appendItem(T_("Move &Up"), [this] { handleMovePluginUp(); }, nullptr, selected);
	menu->appendItem(T_("Move &Down"), [this] { handleMovePluginDown(); }, nullptr, selected);
	menu->appendSeparator();
	menu->appendItem(T_("&Remove"), [this] { handleRemovePlugin(); }, nullptr, selected);

	menu->open(pt);
	return true;
}

void PluginPage::handleAddPlugin() {
	PluginUtils::addPlugin(this);

	refreshList();
}

void PluginPage::handleConfigurePlugin() {
	if(!plugins->hasSelected())
		return;

	PluginUtils::configPlugin(sel(), this);
}

void PluginPage::handleEnable() {
	if(!plugins->hasSelected())
		return;

	PluginUtils::enablePlugin(sel(), this);

	handleSelectionChanged();
}

void PluginPage::handleDisable() {
	if(!plugins->hasSelected())
		return;

	PluginUtils::disablePlugin(sel(), this);

	handleSelectionChanged();
}

void PluginPage::handleMovePluginUp() {
	if(!plugins->hasSelected())
		return;

	auto idx = plugins->getSelected();
	if(idx == 0)
		return;
	HoldRedraw hold { plugins };
	const auto guid = sel();
	plugins->erase(idx);
	PluginManager::getInstance()->movePlugin(guid, -1);
	--idx;
	addEntry(idx, guid);
	plugins->setSelected(idx);
	plugins->ensureVisible(idx);
}

void PluginPage::handleMovePluginDown() {
	if(!plugins->hasSelected())
		return;

	auto idx = plugins->getSelected();
	if(idx == static_cast<int>(plugins->size()) - 1)
		return;
	HoldRedraw hold { plugins };
	const auto guid = sel();
	plugins->erase(idx);
	PluginManager::getInstance()->movePlugin(guid, 1);
	++idx;
	addEntry(idx, guid);
	plugins->setSelected(idx);
	plugins->ensureVisible(idx);
}

void PluginPage::handleRemovePlugin() {
	if(!plugins->hasSelected())
		return;

	if(dwt::MessageBox(this).show(T_("Really remove?"), T_("Plugins"), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES) {
		int i;
		while((i = plugins->getNext(-1, LVNI_SELECTED)) != -1) {
			PluginManager::getInstance()->removePlugin(Text::fromT(plugins->getText(i, COLUMN_GUID)));
			plugins->erase(i);
		}
	}
}

void PluginPage::refreshList() {
	plugins->clear();
	for(auto& guid: PluginManager::getInstance()->getPluginList()) {
		addEntry(plugins->size(), guid);
	}
}

void PluginPage::addEntry(size_t idx, const string& guid) {
	TStringList row(COLUMN_COUNT);
	row[COLUMN_PLUGIN] = Text::toT(PluginManager::getInstance()->getPlugin(guid).name);
	row[COLUMN_GUID] = Text::toT(guid);
	plugins->insert(row, 0, idx);
}

string PluginPage::sel() const {
	return Text::fromT(plugins->getText(plugins->getSelected(), COLUMN_GUID));
}
