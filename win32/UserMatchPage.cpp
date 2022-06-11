/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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
#include "UserMatchPage.h"

#include <boost/range/adaptor/reversed.hpp>

#include <dcpp/format.h>
#include <dcpp/UserMatchManager.h>
#include <dcpp/version.h>

#include <dwt/util/StringUtils.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"

#include "HoldRedraw.h"
#include "SettingsDialog.h"
#include "StylesPage.h"
#include "UserMatchDlg.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;

static const ColumnInfo columns[] = {
	{ "", 100, false }
};

UserMatchPage::UserMatchPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
table(0),
edit(0),
up(0),
down(0),
remove(0),
dirty(false)
{
	setHelpId(IDH_USERMATCHPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto cur = grid->addChild(GroupBox::Seed(T_("User matching definitions")))->addChild(Grid::Seed(3, 1));
		cur->setHelpId(IDH_SETTINGS_USER_MATCH_LIST);
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(grid->getSpacing());

		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.style |= LVS_NOCOLUMNHEADER;
		table = cur->addChild(seed);

		auto row = cur->addChild(Grid::Seed(1, 5));
		row->column(0).mode = GridInfo::FILL;
		row->column(1).mode = GridInfo::FILL;
		row->column(2).mode = GridInfo::FILL;
		row->column(3).mode = GridInfo::FILL;
		row->column(4).mode = GridInfo::FILL;
		row->setSpacing(cur->getSpacing());

		auto button = row->addChild(Button::Seed(T_("&Add")));
		button->setHelpId(IDH_SETTINGS_USER_MATCH_ADD);
		button->onClicked([this] { handleAddClicked(); });

		edit = row->addChild(Button::Seed(T_("&Edit")));
		edit->setHelpId(IDH_SETTINGS_USER_MATCH_EDIT);
		edit->onClicked([this] { handleEditClicked(); });

		up = row->addChild(Button::Seed(T_("Move &Up")));
		up->setHelpId(IDH_SETTINGS_USER_MATCH_MOVE_UP);
		up->onClicked([this] { handleMoveUpClicked(); });

		down = row->addChild(Button::Seed(T_("Move &Down")));
		down->setHelpId(IDH_SETTINGS_USER_MATCH_MOVE_DOWN);
		down->onClicked([this] { handleMoveDownClicked(); });

		remove = row->addChild(Button::Seed(T_("&Remove")));
		remove->setHelpId(IDH_SETTINGS_USER_MATCH_REMOVE);
		remove->onClicked([this] { handleRemoveClicked(); });

		button = cur->addChild(Grid::Seed(1, 1))->addChild(
			Button::Seed(T_("Configure styles (fonts / colors) for these user matching definitions")));
		button->setHelpId(IDH_SETTINGS_USER_MATCH_STYLES);
		button->setImage(WinUtil::buttonIcon(IDI_STYLES));
		button->onClicked([this] { static_cast<SettingsDialog*>(getRoot())->activatePage<StylesPage>(); });
	}

	WinUtil::makeColumns(table, columns, 1);

	list = UserMatchManager::getInstance()->getList();
	for(auto& i: list) {
		addEntry(i);
	}

	handleSelectionChanged();

	table->onDblClicked([this] { handleDoubleClick(); });
	table->onKeyDown([this](int c) { return handleKeyDown(c); });
	table->onSelectionChanged([this] { handleSelectionChanged(); });
	table->setTooltips([this](int i) { return handleTooltip(i); });

	// async to avoid problems if one page gets init'd before the other.
	callAsync([this] { updateStyles(); });
}

UserMatchPage::~UserMatchPage() {
}

void UserMatchPage::layout() {
	PropPage::layout();

	table->setColumnWidth(0, table->getWindowSize().x - 20);
}

void UserMatchPage::write() {
	if(dirty) {
		UserMatchManager::getInstance()->setList(std::move(list));
		WinUtil::updateUserMatchFonts();
	}
}

void UserMatchPage::setDirty() {
	dirty = true;
}

void UserMatchPage::updateStyles() {
	static_cast<SettingsDialog*>(getRoot())->getPage<StylesPage>()->updateUserMatches(list);
}

void UserMatchPage::handleDoubleClick() {
	if(table->hasSelected()) {
		handleEditClicked();
	} else {
		handleAddClicked();
	}
}

bool UserMatchPage::handleKeyDown(int c) {
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

void UserMatchPage::handleSelectionChanged() {
	auto sel = table->getSelection();

	bool predef = false;
	for(auto i: sel) {
		if(list[i].isSet(UserMatch::PREDEFINED)) {
			predef = true;
			break;
		}
	}

	edit->setEnabled(sel.size() == 1 && !predef);
	up->setEnabled(!sel.empty());
	down->setEnabled(!sel.empty());
	remove->setEnabled(!sel.empty() && !predef);
}

tstring UserMatchPage::handleTooltip(int i) {
	auto& matcher = list[i];

	static const size_t maxChars = 100; // max chars per tooltip line

	tstring ret = str(TF_("Name: %1%") % Text::toT(matcher.name));
	dwt::util::cutStr(ret, maxChars);

	auto addLine = [&ret](tstring line) {
		dwt::util::cutStr(line, maxChars);
		ret += _T("\r\n") + line;
	};

	if(matcher.isSet(UserMatch::PREDEFINED))
		addLine(T_("Predefined"));
	if(matcher.isSet(UserMatch::GENERATED))
		addLine(str(TF_("Generated by %1%") % APPNAME));

	if(matcher.empty())
		addLine(T_("Match every user"));

	if(matcher.isSet(UserMatch::FAVS))
		addLine(T_("Match favorite users"));
	if(matcher.isSet(UserMatch::OPS))
		addLine(T_("Match operators"));
	if(matcher.isSet(UserMatch::BOTS))
		addLine(T_("Match bots"));

	tstring fields[UserMatch::Rule::FIELD_LAST] = { T_("Nick"), T_("CID"), T_("IP"), T_("Hub address") };
	tstring methods[UserMatch::Rule::METHOD_LAST] = { T_("Partial match"), T_("Exact match"), T_("Regular Expression") };
	for(auto& rule: matcher.rules) {
		addLine(str(TF_("%1% %2%: %3%") % fields[rule.field] % methods[rule.getMethod()] % Text::toT(rule.pattern)));
	}

	if(matcher.isSet(UserMatch::FORCE_CHAT))
		addLine(T_("Always show chat messages"));
	else if(matcher.isSet(UserMatch::IGNORE_CHAT))
		addLine(T_("Ignore chat messages"));

	if(!matcher.style.font.empty())
		addLine(T_("Custom font"));
	if(matcher.style.textColor >= 0)
		addLine(T_("Custom text color"));
	if(matcher.style.bgColor >= 0)
		addLine(T_("Custom background color"));

	return ret;
}

void UserMatchPage::handleAddClicked() {
	UserMatchDlg dlg(this);
	if(dlg.run() == IDOK) {
		list.push_back(dlg.getResult());

		addEntry(dlg.getResult());

		setDirty();
		updateStyles();
	}
}

void UserMatchPage::handleEditClicked() {
	auto sel = table->getSelected();
	if(sel == -1)
		return;

	auto matcher = &list[sel];
	if(matcher->isSet(UserMatch::PREDEFINED))
		return;

	UserMatchDlg dlg(this, matcher);
	if(dlg.run() == IDOK) {
		list[sel] = dlg.getResult();

		table->erase(sel);
		addEntry(dlg.getResult(), sel);

		setDirty();
		updateStyles();
	}
}

void UserMatchPage::handleMoveUpClicked() {
	HoldRedraw hold { table };
	auto sel = table->getSelection();
	for(auto i: sel) {
		if(i > 0) {
			std::swap(list[i], list[i - 1]);

			table->erase(i);
			addEntry(list[i - 1], i - 1);
			table->select(i - 1);
		}
	}

	setDirty();
	updateStyles();
}

void UserMatchPage::handleMoveDownClicked() {
	HoldRedraw hold { table };
	auto sel = table->getSelection();
	for(auto i: sel | boost::adaptors::reversed) {
		if(i < table->size() - 1) {
			std::swap(list[i], list[i + 1]);

			table->erase(i);
			addEntry(list[i + 1], i + 1);
			table->select(i + 1);
		}
	}

	setDirty();
	updateStyles();
}

void UserMatchPage::handleRemoveClicked() {
	if(!table->hasSelected())
		return;

	if(dwt::MessageBox(this).show(T_("Do you really want to delete selected user matching definitions?"),
		_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDYES)
	{
		int i, j = -1;
		while((i = table->getNext(j, LVNI_SELECTED)) != -1) {
			if(list[i].isSet(UserMatch::PREDEFINED)) {
				j = i + 1;
				continue;
			}

			list.erase(list.begin() + i);
			table->erase(i);
			j = -1;
		}
	}

	setDirty();
	updateStyles();
}

void UserMatchPage::addEntry(const UserMatch& matcher, int index) {
	table->insert(TStringList(1, Text::toT(matcher.name)), 0, index);
}
