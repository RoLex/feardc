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

#include "stdafx.h"

#include "resource.h"

#include "PropPage.h"

#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/LoadDialog.h>

#include <dcpp/SettingsManager.h>
#include "WinUtil.h"

using dwt::FolderDialog;
using dwt::Grid;
using dwt::GridInfo;
using dwt::LoadDialog;

namespace { ::RECT padding = { 7, 4, 7, 8 }; }

PropPage::PropPage(dwt::Widget* parent, int rows, int cols) : dwt::Container(parent), grid(0) {
	{
		Seed seed(0, WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT);
		seed.style &= ~WS_VISIBLE;
		create(seed);
	}

	grid = addChild(Grid::Seed(rows, cols));
	grid->setSpacing(10);
}

PropPage::~PropPage() {
}

void PropPage::layout() {
	auto clientSize = getClientSize();
	grid->resize(dwt::Rectangle(padding.left, padding.top, clientSize.x - padding.left - padding.right, clientSize.y - padding.top - padding.bottom));
}

void PropPage::read(const ItemList& items) {
	SettingsManager* settings = SettingsManager::getInstance();

	for(auto& i: items) {
		switch(i.type) {
		case T_STR:
			{
				auto setting = static_cast<SettingsManager::StrSetting>(i.setting);
				if(!settings->isDefault(setting)) {
					static_cast<TextBoxPtr>(i.widget)->setText(Text::toT(settings->get(setting)));
				}
				break;
			}
		case T_INT:
			{
				auto setting = static_cast<SettingsManager::IntSetting>(i.setting);
				if(!settings->isDefault(setting)) {
					static_cast<TextBoxPtr>(i.widget)->setText(Text::toT(std::to_string(settings->get(setting))));
				}
				break;
			}
		case T_INT_WITH_SPIN:
			{
				auto setting = static_cast<SettingsManager::IntSetting>(i.setting);
				static_cast<TextBoxPtr>(i.widget)->setText(Text::toT(std::to_string(settings->get(setting))));
				break;
			}
		case T_INT64:
			{
				auto setting = static_cast<SettingsManager::Int64Setting>(i.setting);
				if(!settings->isDefault(setting)) {
					static_cast<TextBoxPtr>(i.widget)->setText(Text::toT(std::to_string(settings->get(setting))));
				}
				break;
			}
		case T_INT64_WITH_SPIN:
			{
				auto setting = static_cast<SettingsManager::Int64Setting>(i.setting);
				static_cast<TextBoxPtr>(i.widget)->setText(Text::toT(std::to_string(settings->get(setting))));
				break;
			}
		case T_BOOL:
			{
				auto setting = static_cast<SettingsManager::BoolSetting>(i.setting);
				static_cast<CheckBoxPtr>(i.widget)->setChecked(settings->get(setting));
				break;
			}
		}
	}
}

void PropPage::read(const ListItem* listItems, TablePtr list) {
	dcassert(listItems && list);
	lists[list] = listItems;

	list->addColumn();

	SettingsManager* settings = SettingsManager::getInstance();
	for(size_t i = 0; listItems[i].setting != 0; ++i) {
		list->setChecked(list->insert({ T_(listItems[i].desc) }),
			listItems[i].readF ? listItems[i].readF() :
			settings->get(static_cast<SettingsManager::BoolSetting>(listItems[i].setting), true));
	}

	list->setColumnWidth(0, LVSCW_AUTOSIZE);

	list->setHelpId([this, list](unsigned& id) { handleListHelpId(list, id); });
}

void PropPage::write(const ItemList& items) {
	SettingsManager* settings = SettingsManager::getInstance();

	for(auto& i: items) {
		switch(i.type) {
		case T_STR:
			{
				auto setting = static_cast<SettingsManager::StrSetting>(i.setting);
				settings->set(setting, Text::fromT(static_cast<TextBoxPtr>(i.widget)->getText()));
				break;
			}
		case T_INT:
		case T_INT_WITH_SPIN:
			{
				auto setting = static_cast<SettingsManager::IntSetting>(i.setting);
				settings->set(setting, Text::fromT(static_cast<TextBoxPtr>(i.widget)->getText()));
				break;
			}
		case T_INT64:
		case T_INT64_WITH_SPIN:
			{
				auto setting = static_cast<SettingsManager::Int64Setting>(i.setting);
				settings->set(setting, Text::fromT(static_cast<TextBoxPtr>(i.widget)->getText()));
				break;
			}
		case T_BOOL:
			{
				auto setting = static_cast<SettingsManager::BoolSetting>(i.setting);
				settings->set(setting, static_cast<CheckBoxPtr>(i.widget)->getChecked());
				break;
			}
		}
	}
}

void PropPage::write(TablePtr list) {
	dcassert(list);
	const ListItem* listItems = lists[list];
	SettingsManager* settings = SettingsManager::getInstance();
	for(size_t i = 0; listItems[i].setting != 0; ++i) {
		auto checked = list->isChecked(i);
		if(listItems[i].writeF) {
			listItems[i].writeF(checked);
		} else {
			settings->set(static_cast<SettingsManager::BoolSetting>(listItems[i].setting), checked);
		}
	}
}

void PropPage::handleBrowseDir(TextBoxPtr box, int setting) {
	tstring dir = box->getText();
	if(dir.empty())
		dir = Text::toT(SettingsManager::getInstance()->get((SettingsManager::StrSetting)setting));
	if(FolderDialog(this).open(dir))
		box->setText(dir);
}

void PropPage::handleBrowseFile(TextBoxPtr box, int setting) {
	tstring target = box->getText();
	if(target.empty())
		target = Text::toT(SettingsManager::getInstance()->get((SettingsManager::StrSetting)setting));
	if(LoadDialog(this).setInitialDirectory(target).open(target))
		box->setText(target);
}

dwt::Point PropPage::getPreferredSize() {
	return grid->getPreferredSize() + dwt::Point(padding.left + padding.right, padding.top + padding.bottom);
}

void PropPage::handleListHelpId(TablePtr list, unsigned& id) {
	// we have the help id of the whole list-view; convert to the one of the specific option the user wants help for
	int item = isAnyKeyPressed() ? list->getSelected() :
		list->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos()))).first;
	const ListItem* listItems = lists[list];
	if(item >= 0 && listItems[item].helpId)
		id = listItems[item].helpId;
}
