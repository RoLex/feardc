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
#include "PluginInfoDlg.h"

#include <dcpp/PluginManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Link.h>
#include <dwt/widgets/MessageBox.h>

#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Link;

PluginInfoDlg::PluginInfoDlg(dwt::Widget* parent, const string& path) :
dwt::ModalDialog(parent),
grid(0)
{
	onInitDialog([this, path] { return handleInitDialog(path); });
}

PluginInfoDlg::~PluginInfoDlg() {
}

int PluginInfoDlg::run() {
	create(dwt::Point(400, 300));
	return show();
}

bool PluginInfoDlg::handleInitDialog(const string& path) {
	grid = addChild(Grid::Seed(0, 2));
	grid->column(1).mode = GridInfo::FILL;
	grid->setSpacing(6);

	DcextInfo info;
	try {
		info = PluginManager::getInstance()->extract(path);

	} catch(const Exception& e) {
		resize(dwt::Rectangle());
		auto err = Text::toT(e.getError());
		callAsync([this, err, path] {
			error(err, Text::toT(Util::getFileName(path)));
			endDialog(IDCANCEL);
		});
		return true;
	}

	// similar to PluginPage.cpp

	enum Type { Name, Version, Description, Author, Website };

	auto addInfo = [this](tstring name, const string& value, Type type) {
		if(type == Description) {
			grid->addRow(GridInfo(0, GridInfo::FILL, GridInfo::STRETCH));
		} else {
			grid->addRow();
		}
		grid->addChild(Label::Seed(name));
		if(type == Website && !value.empty()) {
			grid->addChild(Link::Seed(Text::toT(value), true, WinUtil::parseLink));
		} else {
			grid->addChild(Label::Seed(value.empty() ?
				T_("<Information unavailable>") : Text::toT(value)));
		}
	};

	addInfo(T_("Name: "), info.name, Name);
	addInfo(T_("Version: "), Util::toString(info.version), Version);
	addInfo(T_("Description: "), info.description, Description);
	addInfo(T_("Author: "), info.author, Author);
	addInfo(T_("Website: "), info.website, Website);

	{
		grid->addRow();
		auto cur = grid->addChild(Grid::Seed(1, 2));
		grid->setWidget(cur, grid->rowCount() - 1, 0, 1, 2);
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(grid->getSpacing());
		WinUtil::addDlgButtons(cur,
			[this, info] { handleOK(info); },
			[this] { endDialog(IDCANCEL); })
			.first->setText(info.updating ? T_("Update the plugin") : T_("Install the plugin"));
	}

	setText(info.updating ? T_("Updating a plugin") : T_("Adding a plugin"));

	layout();
	centerWindow();

	return false;
}

void PluginInfoDlg::handleOK(const DcextInfo& info) {
	try {
		PluginManager::getInstance()->install(info);
		endDialog(IDOK);

	} catch(const Exception& e) {
		error(Text::toT(e.getError()), Text::toT(info.name));
		endDialog(IDCANCEL);
	}
}

void PluginInfoDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}

void PluginInfoDlg::error(const tstring& message, const tstring& title) {
	dwt::MessageBox(this).show(tstring(T_("Cannot install the plugin:")) + _T("\r\n\r\n") + message, title,
		dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
}
