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
#include "ParamDlg.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Spinner.h>
#include <dwt/widgets/ToolTip.h>

#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Spinner;

ParamDlg::ParamDlg(dwt::Widget* parent, const tstring& title) :
GridDialog(parent, width),
left(0)
{
	onInitDialog([=] { return initDialog(title); });
}

ParamDlg::ParamDlg(dwt::Widget* parent, const tstring& title, const tstring& name, const tstring& value, bool password) :
GridDialog(parent, width),
left(0)
{
	onInitDialog([=] { return initDialog(title); });
	addTextBox(name, value, password);
}

ParamDlg::ParamDlg(dwt::Widget* parent, const tstring& title, const tstring& name, const TStringList& choices, size_t sel, bool edit) :
GridDialog(parent, width),
left(0)
{
	onInitDialog([=] { return initDialog(title); });
	addComboBox(name, choices, sel, edit);
}

void ParamDlg::addTextBox(const tstring& name, const tstring& value, bool password) {
	initFs.push_back([=] { initTextBox(name, value, password); });
}

void ParamDlg::addIntTextBox(const tstring& name, const tstring& value, const int min, const int max) {
	initFs.push_back([=] { initIntTextBox(name, value, min, max); });
}

void ParamDlg::addComboBox(const tstring& name, const TStringList& choices, size_t sel, bool edit) {
	initFs.push_back([=] { initComboBox(name, choices, sel, edit); });
}

void ParamDlg::initTextBox(const tstring& name, const tstring& value, bool password) {
	auto box = left->addChild(GroupBox::Seed(name))->addChild(WinUtil::Seeds::Dialog::textBox);
	if(password) {
		box->setPassword();
	}
	box->setText(value);
	valueFs.push_back([box] { return box->getText(); });

	addTooltip(name);
}

void ParamDlg::initIntTextBox(const tstring& name, const tstring& value, const int min, const int max) {
	auto cur = left->addChild(GroupBox::Seed(name))->addChild(Grid::Seed(1, 1));
	cur->column(0).mode = GridInfo::FILL;

	auto box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
	box->setText(value);

	cur->setWidget(cur->addChild(Spinner::Seed(min, max, box)));

	valueFs.push_back([box] { return box->getText(); });

	addTooltip(name);
}

void ParamDlg::initComboBox(const tstring& name, const TStringList& choices, size_t sel, bool edit) {
	auto box = left->addChild(GroupBox::Seed(name))->addChild(edit ? WinUtil::Seeds::Dialog::comboBoxEdit : WinUtil::Seeds::Dialog::comboBox);
	for(auto& i: choices)
		box->addValue(i);
	box->setSelected(sel);
	valueFs.push_back([box] { return box->getText(); });

	addTooltip(name);
}

bool ParamDlg::initDialog(const tstring& title) {
	grid = addChild(Grid::Seed(1, 2));
	grid->column(0).mode = GridInfo::FILL;
	grid->setSpacing(6);

	const size_t n = initFs.size();
	left = grid->addChild(Grid::Seed(n, 1));
	left->column(0).mode = GridInfo::FILL;

	for(size_t i = 0; i < n; ++i)
		initFs[i]();

	WinUtil::addDlgButtons(grid->addChild(Grid::Seed(2, 1)),
		[this] { okClicked(); },
		[this] { endDialog(IDCANCEL); });

	setText(title);

	layout();
	centerWindow();

	return false;
}

void ParamDlg::okClicked() {
	const size_t n = valueFs.size();
	values.resize(n);
	for(size_t i = 0; i < n; ++i)
		values[i] = valueFs[i]();
	endDialog(IDOK);
}

void ParamDlg::addTooltip(const tstring& name) {
	auto tip = left->addChild(dwt::ToolTip::Seed());
	tip->setText(name);
	onDestroy([tip] { tip->close(); });
}
