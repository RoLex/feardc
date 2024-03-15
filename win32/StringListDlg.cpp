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
#include "StringListDlg.h"

#include <boost/range/adaptor/reversed.hpp>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/Grid.h>

#include "resource.h"

#include "HoldRedraw.h"
#include "ParamDlg.h"
#include "WinUtil.h"

using dwt::Button;
using dwt::Grid;
using dwt::GridInfo;

StringListDlg::StringListDlg(dwt::Widget* parent, const TStringList& initialValues, bool ensureUniqueness) :
dwt::ModalDialog(parent),
grid(0),
editBox(0),
list(0),
addBtn(0),
up(0),
down(0),
editBtn(0),
remove(0),
unique(ensureUniqueness)
{
	onInitDialog([=, this] { return handleInitDialog(initialValues); });
	onHelp(&WinUtil::help);
}

StringListDlg::~StringListDlg() {
}

int StringListDlg::run() {
	create(Seed(dwt::Point(400, 316), DS_CONTEXTHELP));
	return show();
}

void StringListDlg::insert(const tstring& line, int index) {
	if(!checkUnique(line))
		return;
	int itemCount = list->insert(TStringList(1, line), 0, index);
	if(index == -1)
		index = itemCount;
	list->ensureVisible(index);
}

void StringListDlg::modify(unsigned row, const tstring& text) {
	if(!checkUnique(text))
		return;
	list->setText(row, 0, text);
}

tstring StringListDlg::getTitle() const {
	return T_("List configuration");
}

tstring StringListDlg::getEditTitle() const {
	return T_("Item edition");
}

tstring StringListDlg::getEditDescription() const {
	return T_("Value");
}

unsigned StringListDlg::getHelpId(HelpFields field) const {
	switch(field) {
		case HELP_DIALOG: return IDH_STRING_LIST;
		case HELP_EDIT_BOX: return IDH_STRING_LIST_EDIT_BOX;
		case HELP_LIST: return IDH_STRING_LIST_LIST;
		case HELP_ADD: return IDH_STRING_LIST_ADD;
		case HELP_MOVE_UP: return IDH_STRING_LIST_MOVE_UP;
		case HELP_MOVE_DOWN: return IDH_STRING_LIST_MOVE_DOWN;
		case HELP_EDIT: return IDH_STRING_LIST_EDIT;
		case HELP_REMOVE: return IDH_STRING_LIST_REMOVE;
	}
	return 0;
}

void StringListDlg::add(const tstring& s) {
	insert(s);
}

void StringListDlg::edit(unsigned row, const tstring& s) {
	ParamDlg dlg(this, getEditTitle(), getEditDescription(), s);
	if(dlg.run() == IDOK)
		modify(row, dlg.getValue());
}

bool StringListDlg::handleInitDialog(const TStringList& initialValues) {
	setHelpId(getHelpId(HELP_DIALOG));

	grid = addChild(Grid::Seed(2, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	editBox = grid->addChild(WinUtil::Seeds::Dialog::textBox);
	editBox->setHelpId(getHelpId(HELP_EDIT_BOX));

	{
		Button::Seed seed = Button::Seed(T_("&Add"));
		seed.menuHandle = reinterpret_cast<HMENU>(IDOK);
		addBtn = grid->addChild(seed);
		addBtn->setHelpId(getHelpId(HELP_ADD));
		addBtn->onClicked([this] { handleAddClicked(); });
	}

	{
		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.style |= LVS_NOCOLUMNHEADER;
		list = grid->addChild(seed);
		list->setHelpId(getHelpId(HELP_LIST));
	}

	{
		auto cur = grid->addChild(Grid::Seed(6, 1));
		cur->row(4).mode = GridInfo::FILL;
		cur->row(4).align = GridInfo::BOTTOM_RIGHT;

		Button::Seed seed;

		seed.caption = T_("Move &Up");
		up = cur->addChild(seed);
		up->setHelpId(getHelpId(HELP_MOVE_UP));
		up->onClicked([this] { handleMoveUpClicked(); });

		seed.caption = T_("Move &Down");
		down = cur->addChild(seed);
		down->setHelpId(getHelpId(HELP_MOVE_DOWN));
		down->onClicked([this] { handleMoveDownClicked(); });

		seed.caption = T_("&Edit");
		editBtn = cur->addChild(seed);
		editBtn->setHelpId(getHelpId(HELP_EDIT));
		editBtn->onClicked([this] { handleEditClicked(); });

		seed.caption = T_("&Remove");
		remove = cur->addChild(seed);
		remove->setHelpId(getHelpId(HELP_REMOVE));
		remove->onClicked([this] { handleRemoveClicked(); });

		::SetWindowLongPtr(
			WinUtil::addDlgButtons(cur,
				[this] { handleOKClicked(); },
				[this] { endDialog(IDCANCEL); })
			.first->handle(), GWLP_ID, 0); // the def button is the "Add" button
	}

	list->addColumn();

	for(auto& i: initialValues)
		insert(i);

	handleSelectionChanged();
	handleInputUpdated();

	list->onDblClicked([this] { handleDoubleClick(); });
	list->onKeyDown([this](int c) { return handleKeyDown(c); });
	list->onSelectionChanged([this] { handleSelectionChanged(); });
	editBox->onUpdated([this] { handleInputUpdated(); });

	setText(getTitle());

	layout();
	centerWindow();

	return false;
}

void StringListDlg::handleDoubleClick() {
	if(list->hasSelected()) {
		handleEditClicked();
	}
}

bool StringListDlg::handleKeyDown(int c) {
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

void StringListDlg::handleSelectionChanged() {
	bool enable = list->hasSelected();
	up->setEnabled(enable);
	down->setEnabled(enable);
	editBtn->setEnabled(enable);
	remove->setEnabled(enable);
}

void StringListDlg::handleInputUpdated() {
	bool enable = editBox->length() > 0;
	if(addBtn->getEnabled() != enable) {
		addBtn->setEnabled(enable);
	}
}

void StringListDlg::handleAddClicked() {
	if(editBox->length() <= 0)
		return;

	add(editBox->getText());

	if(!editBox->hasFocus())
		editBox->setFocus();
	editBox->setSelection();
}

void StringListDlg::handleMoveUpClicked() {
	HoldRedraw hold { list };
	auto selected = list->getSelection();
	for(auto i: selected) {
		if(i > 0) {
			tstring selText = list->getText(i, 0);
			list->erase(i);
			insert(selText, i - 1);
			list->select(i - 1);
		}
	}
}

void StringListDlg::handleMoveDownClicked() {
	HoldRedraw hold { list };
	auto selected = list->getSelection();
	for(auto i: selected | boost::adaptors::reversed) {
		if(i < list->size() - 1) {
			tstring selText = list->getText(i, 0);
			list->erase(i);
			insert(selText, i + 1);
			list->select(i + 1);
		}
	}
}

void StringListDlg::handleEditClicked() {
	int i = -1;
	while((i = list->getNext(i, LVNI_SELECTED)) != -1) {
		edit(i, list->getText(i, 0));
	}
}

void StringListDlg::handleRemoveClicked() {
	int i;
	while((i = list->getNext(-1, LVNI_SELECTED)) != -1)
		list->erase(i);
}

void StringListDlg::handleOKClicked() {
	for(int i = 0, iend = list->size(); i < iend; ++i)
		values.push_back(list->getText(i, 0));

	endDialog(IDOK);
}

void StringListDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));

	list->setColumnWidth(0, list->getWindowSize().x - 20);
}

bool StringListDlg::checkUnique(const tstring& text) {
	if(!unique)
		return true;

	int pos = list->find(text);
	if(pos == -1)
		return true;

	list->ensureVisible(pos);
	return false;
}
