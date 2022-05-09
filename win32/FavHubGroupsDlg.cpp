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
#include "FavHubGroupsDlg.h"

#include <dcpp/FavoriteManager.h>
#include <dcpp/format.h>
#include <dcpp/version.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "HoldRedraw.h"
#include "TypedTable.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

static const ColumnInfo columns[] = {
	{ N_("Group name"), 100, false }
};

FavHubGroupsDlg::FavHubGroupsDlg(dwt::Widget* parent, FavoriteHubEntry* parentEntry_) :
dwt::ModalDialog(parent),
grid(0),
groups(0),
update(0),
remove(0),
properties(0),
edit(0),
nick(0),
description(0),
email(0),
userIp(0),
userIp6(0),
showJoins(0),
favShowJoins(0),
logMainChat(0),
parentEntry(parentEntry_)
{
	onInitDialog([this] { return handleInitDialog(); });
	onHelp(&WinUtil::help);
}

FavHubGroupsDlg::~FavHubGroupsDlg() {
}

int FavHubGroupsDlg::run() {
	create(Seed(dwt::Point(450, 450), DS_CONTEXTHELP));
	return show();
}

FavHubGroupsDlg::GroupInfo::GroupInfo(const FavHubGroup& group_) : group(group_) {
	columns[COLUMN_NAME] = Text::toT(group.first);
}

const tstring& FavHubGroupsDlg::GroupInfo::getText(int col) const {
	return columns[col];
}

int FavHubGroupsDlg::GroupInfo::compareItems(const GroupInfo* a, const GroupInfo* b, int col) {
	return compare(a->columns[col], b->columns[col]);
}

bool FavHubGroupsDlg::handleInitDialog() {
	setHelpId(IDH_FAV_HUB_GROUPS);

	grid = addChild(Grid::Seed(4, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	groups = grid->addChild(Groups::Seed(WinUtil::Seeds::Dialog::table));
	groups->setHelpId(IDH_FAV_HUB_GROUPS_LIST);

	{
		auto cur = grid->addChild(Grid::Seed(1, 3));

		Button::Seed seed(T_("&Add"));
		auto add = cur->addChild(seed);
		add->setHelpId(IDH_FAV_HUB_GROUPS_ADD);
		add->onClicked([this] { handleAdd(); });

		seed.caption = T_("&Update");
		update = cur->addChild(seed);
		update->setHelpId(IDH_FAV_HUB_GROUPS_UPDATE);
		update->onClicked([this] { handleUpdate(); });

		seed.caption = T_("&Remove");
		remove = cur->addChild(seed);
		remove->setHelpId(IDH_FAV_HUB_GROUPS_REMOVE);
		remove->onClicked([this] { handleRemove(); });
	}

	{
		properties = grid->addChild(GroupBox::Seed(T_("Group properties")));
		auto cur = properties->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;

		{
			auto cur2 = cur->addChild(Grid::Seed(1, 2));
			cur2->column(1).mode = GridInfo::FILL;
			cur2->setHelpId(IDH_FAV_HUB_GROUPS_NAME);

			cur2->addChild(Label::Seed(T_("Name")));

			edit = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
		}

		{
			auto group = cur->addChild(GroupBox::Seed(T_("Identification (leave blank for defaults)")));

			auto cur2 = group->addChild(Grid::Seed(4, 2));
			cur2->column(0).align = GridInfo::BOTTOM_RIGHT;
			cur2->column(1).mode = GridInfo::FILL;

			cur2->addChild(Label::Seed(T_("Nick")))->setHelpId(IDH_FAVORITE_HUB_NICK);
			nick = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
			nick->setHelpId(IDH_FAVORITE_HUB_NICK);
			WinUtil::preventSpaces(nick);

			cur2->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_FAVORITE_HUB_USER_DESC);
			description = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
			description->setHelpId(IDH_FAVORITE_HUB_USER_DESC);

			cur2->addChild(Label::Seed(T_("Email")))->setHelpId(IDH_FAVORITE_HUB_EMAIL);
			email = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
			email->setHelpId(IDH_FAVORITE_HUB_EMAIL);

			cur2->addChild(Label::Seed(T_("IPv4")))->setHelpId(IDH_FAVORITE_HUB_USER_IP);
			userIp = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
			userIp->setHelpId(IDH_FAVORITE_HUB_USER_IP);
			WinUtil::preventSpaces(userIp);

			cur2->addChild(Label::Seed(T_("IPv6")))->setHelpId(IDH_FAVORITE_HUB_USER_IP6);
			userIp6 = cur2->addChild(WinUtil::Seeds::Dialog::textBox);
			userIp6->setHelpId(IDH_FAVORITE_HUB_USER_IP6);
			WinUtil::preventSpaces(userIp6);
		}

		{
			auto cur2 = cur->addChild(Grid::Seed(3, 2));
			cur2->column(0).mode = GridInfo::FILL;
			cur2->column(0).align = GridInfo::BOTTOM_RIGHT;

			cur2->addChild(Label::Seed(T_("Show joins / parts in chat by default")));
			showJoins = cur2->addChild(WinUtil::Seeds::Dialog::comboBox);
			WinUtil::fillTriboolCombo(showJoins);
			showJoins->setSelected(0);

			cur2->addChild(Label::Seed(T_("Only show joins / parts for favorite users")));
			favShowJoins = cur2->addChild(WinUtil::Seeds::Dialog::comboBox);
			WinUtil::fillTriboolCombo(favShowJoins);
			favShowJoins->setSelected(0);

			cur2->addChild(Label::Seed(T_("Log main chat")));
			logMainChat = cur2->addChild(WinUtil::Seeds::Dialog::comboBox);
			WinUtil::fillTriboolCombo(logMainChat);
			logMainChat->setSelected(0);
		}
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;

		pair<ButtonPtr, ButtonPtr> buttons = WinUtil::addDlgButtons(cur,
			[this] { handleClose(); },
			[this] { handleClose(); });
		buttons.first->setText(T_("&Close"));
		buttons.second->setVisible(false);
	}

	WinUtil::makeColumns(groups, columns, COLUMN_LAST);

	{
		HoldRedraw hold { groups };

		for(auto& i: FavoriteManager::getInstance()->getFavHubGroups())
			add(i, false);

		groups->setSort(COLUMN_NAME);
	}

	handleSelectionChanged();

	groups->onKeyDown([this](int c) { return handleKeyDown(c); });
	groups->onSelectionChanged([this] { handleSelectionChanged(); });

	setText(T_("Favorite hub groups"));

	layout();
	centerWindow();

	return false;
}

bool FavHubGroupsDlg::handleKeyDown(int c) {
	switch(c) {
	case VK_DELETE:
		handleRemove();
		return true;
	}
	return false;
}

void FavHubGroupsDlg::handleSelectionChanged() {
	size_t selected = groups->countSelected();
	update->setEnabled(selected == 1);
	remove->setEnabled(selected > 0);

	tstring text;
	HubSettings settings;
	if(selected == 1) {
		const FavHubGroup& group = groups->getSelectedData()->group;
		text = Text::toT(group.first);
		settings = group.second;
	}
	edit->setText(text);
	nick->setText(Text::toT(settings.get(HubSettings::Nick)));
	description->setText(Text::toT(settings.get(HubSettings::Description)));
	email->setText(Text::toT(settings.get(HubSettings::Email)));
	userIp->setText(Text::toT(settings.get(HubSettings::UserIp)));
	userIp6->setText(Text::toT(settings.get(HubSettings::UserIp6)));
	showJoins->setSelected(toInt(settings.get(HubSettings::ShowJoins)));
	favShowJoins->setSelected(toInt(settings.get(HubSettings::FavShowJoins)));
	logMainChat->setSelected(toInt(settings.get(HubSettings::LogMainChat)));
}

void FavHubGroupsDlg::handleAdd() {
	tstring name = edit->getText();
	if(addable(name)) {
		HoldRedraw hold { groups };
		add(name, getSettings());
	}
}

void FavHubGroupsDlg::handleUpdate() {
	if(groups->countSelected() != 1)
		return;

	const tstring name = edit->getText();
	const size_t selected = groups->getSelected();
	if(!addable(name, selected))
		return;

	string oldName = groups->getData(selected)->group.first;
	if(name != Text::toT(oldName)) {
		// name of the group changed; propagate the change to hubs that are in this group
		for(auto& i: FavoriteManager::getInstance()->getFavoriteHubs(oldName))
			i->setGroup(Text::fromT(name));
	}

	HoldRedraw hold { groups };
	auto settings = getSettings();
	groups->erase(selected);
	add(name, std::move(settings));
}

void FavHubGroupsDlg::handleRemove() {
	groups->forEachSelectedT([this](const GroupInfo *group) { removeGroup(group); }, true);
}

void FavHubGroupsDlg::handleClose() {
	FavoriteManager::getInstance()->setFavHubGroups(groups->forEachT(GroupCollector()).groups);
	FavoriteManager::getInstance()->save();

	endDialog(IDOK);
}

void FavHubGroupsDlg::add(const FavHubGroup& group, bool ensureVisible) {
	int pos = groups->insert(new GroupInfo(group));
	if(ensureVisible)
		groups->ensureVisible(pos);
}

void FavHubGroupsDlg::add(const tstring& name, HubSettings&& settings) {
	add(FavHubGroup(Text::fromT(name), move(settings)));
}

HubSettings FavHubGroupsDlg::getSettings() const {
	HubSettings settings;
	settings.get(HubSettings::Nick) = Text::fromT(nick->getText());
	settings.get(HubSettings::Description) = Text::fromT(description->getText());
	settings.get(HubSettings::Email) = Text::fromT(email->getText());
	settings.get(HubSettings::UserIp) = Text::fromT(userIp->getText());
	settings.get(HubSettings::UserIp6) = Text::fromT(userIp6->getText());
	settings.get(HubSettings::ShowJoins) = to3bool(showJoins->getSelected());
	settings.get(HubSettings::FavShowJoins) = to3bool(favShowJoins->getSelected());
	settings.get(HubSettings::LogMainChat) = to3bool(logMainChat->getSelected());
	return settings;
}

bool FavHubGroupsDlg::addable(const tstring& name, int ignore) {
	if(name.empty()) {
		dwt::MessageBox(this).show(T_("You must enter a name"), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return false;
	}

	for(size_t i = 0, size = groups->size(); i < size; ++i) {
		if(ignore >= 0 && i == static_cast<size_t>(ignore))
			continue;
		if(Text::toT(groups->getData(i)->group.first) == name) {
			dwt::MessageBox(this).show(str(TF_("Another group with the name \"%1%\" already exists") % name),
				_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
			return false;
		}
	}

	return true;
}

void FavHubGroupsDlg::removeGroup(const GroupInfo* group) {
	auto hubs = FavoriteManager::getInstance()->getFavoriteHubs(group->group.first);
	size_t count = hubs.size();
	if(count > 0) {
		bool removeChildren = dwt::MessageBox(this).show(str(TF_(
			"The group \"%1%\" contains %2% hubs; remove all of the hubs as well?\n\n"
			"If you select 'Yes', all of these hubs are going to be deleted!\n\n"
			"If you select 'No', these hubs will simply be moved to the main default group.")
			% Text::toT(group->group.first) % count), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == dwt::MessageBox::RETBOX_YES;
		for(auto& j: hubs) {
			if(removeChildren && j != parentEntry)
				FavoriteManager::getInstance()->removeFavorite(j);
			else
				j->setGroup("");
		}
	}
	groups->erase(group);
}

void FavHubGroupsDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));

	groups->setColumnWidth(0, groups->getWindowSize().x - 20);
}
