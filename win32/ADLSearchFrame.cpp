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
#include "ADLSearchFrame.h"

#include <boost/range/adaptor/reversed.hpp>

#include <dcpp/Client.h>
#include <dcpp/version.h>
#include <dcpp/format.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"

#include "HoldRedraw.h"
#include "ADLSProperties.h"

using std::swap;
using dwt::Grid;
using dwt::GridInfo;

const string ADLSearchFrame::id = "ADLSearch";
const string& ADLSearchFrame::getId() const { return id; }

static const ColumnInfo itemsColumns[] = {
	{ N_("Enabled / Search String"), 200, false },
	{ N_("Regular Expression"), 100, false },
	{ N_("Source Type"), 100, false },
	{ N_("Destination Directory"), 100, false },
	{ N_("Min Size"), 100, true },
	{ N_("Max Size"), 100, true }
};

ADLSearchFrame::ADLSearchFrame(TabViewPtr parent) :
BaseType(parent, T_("Automatic Directory Listing Search"), IDH_ADL_SEARCH, IDI_ADLSEARCH),
grid(0),
items(0)
{
	grid = addChild(Grid::Seed(2, 6));
	grid->column(0).mode = GridInfo::FILL;
	grid->column(1).mode = GridInfo::FILL;
	grid->column(2).mode = GridInfo::FILL;
	grid->column(3).mode = GridInfo::FILL;
	grid->column(4).mode = GridInfo::FILL;
	grid->column(5).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto cs = WinUtil::Seeds::table;
		cs.style |= LVS_NOSORTHEADER;
		cs.lvStyle |= LVS_EX_CHECKBOXES;
		items = grid->addChild(cs);
		grid->setWidget(items, 0, 0, 1, 6);
		addWidget(items);

		WinUtil::makeColumns(items, itemsColumns, COLUMN_LAST, SETTING(ADLSEARCHFRAME_ORDER), SETTING(ADLSEARCHFRAME_WIDTHS));

		items->onDblClicked([this] { handleDoubleClick(); });
		items->onKeyDown([this](int c) { return handleKeyDown(c); });
		items->onRaw([this](WPARAM, LPARAM l) { return handleItemChanged(l); }, dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));
		items->onContextMenu([this](dwt::ScreenCoordinate sc) { return handleContextMenu(sc); });
	}

	{
		auto cs = WinUtil::Seeds::button;

		cs.caption = T_("&New...");
		auto button = grid->addChild(cs);
		button->setHelpId(IDH_ADLS_NEW);
		button->onClicked([this] { handleAdd(); });
		addWidget(button);

		cs.caption = T_("&Properties");
		button = grid->addChild(cs);
		button->setHelpId(IDH_ADLS_PROPERTIES);
		button->onClicked([this] { handleProperties(); });
		addWidget(button);

		cs.caption = T_("Move &Up");
		button = grid->addChild(cs);
		button->setHelpId(IDH_ADLS_MOVE_UP);
		button->onClicked([this] { handleUp(); });
		addWidget(button);

		cs.caption = T_("Move &Down");
		button = grid->addChild(cs);
		button->setHelpId(IDH_ADLS_MOVE_DOWN);
		button->onClicked([this] { handleDown(); });
		addWidget(button);

		cs.caption = T_("&Remove");
		button = grid->addChild(cs);
		button->setHelpId(IDH_ADLS_REMOVE);
		button->onClicked([this] { handleRemove(); });
		addWidget(button);

		button = WinUtil::addHelpButton(grid);
		button->onClicked([this] { WinUtil::help(this); });
		addWidget(button);
	}

	initStatus();

	layout();

	// Load all searches
	for(auto& i: ADLSearchManager::getInstance()->collection)
		addEntry(i, /*itemCount*/ -1, /*scroll*/ false);
}

ADLSearchFrame::~ADLSearchFrame() {
}

void ADLSearchFrame::layout() {
	dwt::Rectangle r { getClientSize() };

	r.size.y -= status->refresh();

	grid->resize(r);
}

bool ADLSearchFrame::preClosing() {
	ADLSearchManager::getInstance()->save();
	return true;
}

void ADLSearchFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::ADLSEARCHFRAME_ORDER, WinUtil::toString(items->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::ADLSEARCHFRAME_WIDTHS, WinUtil::toString(items->getColumnWidths()));
}

void ADLSearchFrame::handleAdd() {
	ADLSearch search;
	ADLSProperties dlg(this, search);
	if(dlg.run() == IDOK) {
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;

		int index;

		// Add new search to the end or if selected, just before
		if(items->countSelected() == 1) {
			index = items->getSelected();
			collection.insert(collection.begin() + index, search);
		} else {
			index = -1;
			collection.push_back(search);
		}

		ADLSearchManager::getInstance()->save();

		addEntry(search, index);
	}
}

void ADLSearchFrame::handleProperties() {
	bool save = false;

	// Get selection info
	auto selected = items->getSelection();
	for(auto i: selected) {
		// Edit existing
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		ADLSearch search = collection[i];

		// Invoke dialog with selected search
		ADLSProperties dlg(this, search);
		if(dlg.run() == IDOK) {
			// Update search collection
			collection[i] = search;
			save = true;

			// Update list control
			HoldRedraw hold { items };
			items->erase(i);
			addEntry(search, i);
			items->select(i);
		}
	}

	if(save)
		ADLSearchManager::getInstance()->save();
}

void ADLSearchFrame::handleUp() {
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	bool save = false;

	HoldRedraw hold { items };
	auto selected = items->getSelection();
	for(auto i: selected) {
		if(i > 0) {
			ADLSearch search = collection[i];
			swap(collection[i], collection[i - 1]);
			save = true;

			items->erase(i);
			addEntry(search, i - 1);
			items->select(i - 1);
		}
	}

	if(save)
		ADLSearchManager::getInstance()->save();
}

void ADLSearchFrame::handleDown() {
	ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
	bool save = false;

	HoldRedraw hold { items };
	auto selected = items->getSelection();
	for(auto i: selected | boost::adaptors::reversed) {
		if(i < items->size() - 1) {
			ADLSearch search = collection[i];
			swap(collection[i], collection[i + 1]);
			save = true;

			items->erase(i);
			addEntry(search, i + 1);
			items->select(i + 1);
		}
	}

	if(save)
		ADLSearchManager::getInstance()->save();
}

void ADLSearchFrame::handleRemove() {
	bool save = false;

	if(!SETTING(CONFIRM_ADLS_REMOVAL) || dwt::MessageBox(this).show(T_("Really remove?"), _T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES) {
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		int i;
		while((i = items->getNext(-1, LVNI_SELECTED)) != -1) {
			collection.erase(collection.begin() + i);
			save = true;

			items->erase(i);
		}
	}

	if(save)
		ADLSearchManager::getInstance()->save();
}

void ADLSearchFrame::handleDoubleClick() {
	if(items->hasSelected()) {
		handleProperties();
	} else {
		handleAdd();
	}
}

bool ADLSearchFrame::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAdd();
		return true;
	case VK_DELETE:
		handleRemove();
		return true;
	case VK_RETURN:
		handleProperties();
		return true;
	}
	return false;
}

LRESULT ADLSearchFrame::handleItemChanged(LPARAM lParam) {
	LPNMITEMACTIVATE item = reinterpret_cast<LPNMITEMACTIVATE>(lParam);

	if((item->uChanged & LVIF_STATE) == 0)
		return 0;
	if((item->uOldState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;
	if((item->uNewState & INDEXTOSTATEIMAGEMASK(0xf)) == 0)
		return 0;

	if(item->iItem >= 0)
	{
		// Set new active status check box
		ADLSearchManager::SearchCollection& collection = ADLSearchManager::getInstance()->collection;
		ADLSearch& search = collection[item->iItem];
		search.isActive = items->isChecked(item->iItem);
		ADLSearchManager::getInstance()->save();
	}
	return 0;
}

bool ADLSearchFrame::handleContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 && pt.y() == -1) {
		pt = items->getContextMenuPos();
	}

	size_t sel = items->countSelected();

	auto menu = addChild(WinUtil::Seeds::menu);
	menu->setTitle((sel == 0) ? getText() : (sel == 1) ? escapeMenu(items->getText(items->getSelected(), COLUMN_ACTIVE_SEARCH_STRING)) :
		str(TF_("%1% items") % sel), getParent()->getIcon(this));
	menu->appendItem(T_("&New..."), [this] { handleAdd(); });
	menu->appendItem(T_("&Properties"), [this] { handleProperties(); }, dwt::IconPtr(), sel);
	menu->appendItem(T_("&Remove"), [this] { handleRemove(); }, dwt::IconPtr(), sel);
	WinUtil::addCopyMenu(menu.get(), items);

	menu->open(pt);
	return true;
}

void ADLSearchFrame::addEntry(ADLSearch& search, int index, bool scroll) {
	int itemCount = items->insert({
		Text::toT(search.searchString),
		search.isRegEx() ? T_("Yes") : T_("No"),
		Text::toT(search.SourceTypeToString(search.sourceType)),
		Text::toT(search.destDir),
		(search.minFileSize >= 0) ? Text::toT(std::to_string(search.minFileSize)) + _T(" ") + Text::toT(search.SizeTypeToString(search.typeFileSize)) : Util::emptyStringT,
		(search.maxFileSize >= 0) ? Text::toT(std::to_string(search.maxFileSize)) + _T(" ") + Text::toT(search.SizeTypeToString(search.typeFileSize)) : Util::emptyStringT
	}, 0, index);
	if(index == -1)
		index = itemCount;
	items->setChecked(index, search.isActive);
	if (scroll)
		items->ensureVisible(index);
}
