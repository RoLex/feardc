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

#include "NotificationsPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/LoadDialog.h>

#include "resource.h"
#include "MainWindow.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::LoadDialog;

static const ColumnInfo columns[] = {
	{ "", 0, false },
	{ N_("Event"), 100, false },
	{ N_("Sound"), 100, false },
	{ N_("Balloon"), 100, false },
};

NotificationsPage::NotificationsPage(dwt::Widget* parent) :
PropPage(parent, 6, 1),
table(0),
sound(0),
balloon(0),
soundGroup(0),
soundFile(0),
balloonGroup(0),
balloonBg(0)
{
	setHelpId(IDH_NOTIFICATIONSPAGE);

	options[WinUtil::NOTIFICATION_FINISHED_DL].helpId = IDH_SETTINGS_NOTIFICATIONS_FINISHED_DL;
	options[WinUtil::NOTIFICATION_FINISHED_FL].helpId = IDH_SETTINGS_NOTIFICATIONS_FINISHED_FL;
	options[WinUtil::NOTIFICATION_MAIN_CHAT].helpId = IDH_SETTINGS_NOTIFICATIONS_MAIN_CHAT;
	options[WinUtil::NOTIFICATION_PM].helpId = IDH_SETTINGS_NOTIFICATIONS_PM;
	options[WinUtil::NOTIFICATION_PM_WINDOW].helpId = IDH_SETTINGS_NOTIFICATIONS_PM_WINDOW;

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto cur = grid->addChild(GroupBox::Seed(T_("Notifications")))->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(grid->getSpacing());

		auto seed = WinUtil::Seeds::Dialog::table;
		seed.style |= LVS_SINGLESEL;
		seed.lvStyle |= LVS_EX_SUBITEMIMAGES;
		table = cur->addChild(seed);

		const dwt::Point size(16, 16);
		dwt::ImageListPtr images(new dwt::ImageList(size));
		images->add(dwt::Icon(IDI_CANCEL, size));
		images->add(dwt::Icon(IDI_SOUND, size));
		images->add(dwt::Icon(IDI_BALLOON, size));
		table->setSmallImageList(images);

		cur = cur->addChild(Grid::Seed(1, 2));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(grid->getSpacing());

		sound = cur->addChild(CheckBox::Seed(T_("Play a sound")));
		sound->setHelpId(IDH_SETTINGS_NOTIFICATIONS_SOUND);
		sound->onClicked([this] { handleSoundClicked(); });

		balloon = cur->addChild(CheckBox::Seed(T_("Display a balloon popup")));
		balloon->setHelpId(IDH_SETTINGS_NOTIFICATIONS_BALLOON);
		balloon->onClicked([this] { handleBalloonClicked(); });
	}

	{
		soundGroup = grid->addChild(GroupBox::Seed(T_("Sound settings")));
		soundGroup->setHelpId(IDH_SETTINGS_NOTIFICATIONS_SOUND_FILE);

		auto cur = soundGroup->addChild(Grid::Seed(1, 3));
		cur->column(1).mode = GridInfo::FILL;

		auto button = cur->addChild(Button::Seed(T_("Play")));
		button->setImage(WinUtil::buttonIcon(IDI_SOUND));
		button->onClicked([this] { handlePlayClicked(); });

		soundFile = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		soundFile->onUpdated([this] { handleSoundChanged(); });

		cur->addChild(Button::Seed(T_("&Browse...")))->onClicked([this] { handleBrowseClicked(); });
	}

	{
		balloonGroup = grid->addChild(GroupBox::Seed(T_("Balloon settings")));

		balloonBg = balloonGroup->addChild(CheckBox::Seed(T_("Display the balloon only when FearDC is in the background")));
		balloonBg->setHelpId(IDH_SETTINGS_NOTIFICATIONS_BALLOON_BG);
		balloonBg->onClicked([this] { handleBalloonBgClicked(); });
	}

	{
		auto button = grid->addChild(Grid::Seed(1, 1))->addChild(Button::Seed(T_("Fire a balloon popup example")));
		button->setHelpId(IDH_SETTINGS_NOTIFICATIONS_BALLOON_EXAMPLE);
		button->setImage(WinUtil::buttonIcon(IDI_BALLOON));
		button->onClicked([this] { handleExampleClicked(); });
	}

	WinUtil::makeColumns(table, columns, COLUMN_LAST);

	for(size_t i = 0; i < WinUtil::NOTIFICATION_LAST; ++i) {
		options[i].sound = Text::toT(SettingsManager::getInstance()->get((SettingsManager::StrSetting)WinUtil::notifications[i].sound));
		options[i].balloon = SettingsManager::getInstance()->get((SettingsManager::IntSetting)WinUtil::notifications[i].balloon);

		TStringList row(COLUMN_LAST);
		row[COLUMN_TEXT] = T_(WinUtil::notifications[i].title);
		table->insert(row);

		updateSound(i);
		updateBalloon(i);
	}

	handleSelectionChanged();

	table->onSelectionChanged([this] { handleSelectionChanged(); });
	table->onDblClicked([this] { handleDblClicked(); });

	table->setHelpId([this](unsigned& id) { handleTableHelpId(id); });
}

NotificationsPage::~NotificationsPage() {
}

void NotificationsPage::layout() {
	PropPage::layout();

	table->setColumnWidth(COLUMN_TEXT, table->getWindowSize().x - 220);
}

void NotificationsPage::write() {
	SettingsManager* settings = SettingsManager::getInstance();
	for(size_t i = 0, n = sizeof(options) / sizeof(Option); i < n; ++i) {
		settings->set((SettingsManager::StrSetting)WinUtil::notifications[i].sound, Text::fromT(options[i].sound));
		settings->set((SettingsManager::IntSetting)WinUtil::notifications[i].balloon, options[i].balloon);
	}
}

void NotificationsPage::handleTableHelpId(unsigned& id) {
	// same as PropPage::handleListHelpId
	int item = isAnyKeyPressed() ? table->getSelected() :
		table->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos()))).first;
	if(item >= 0 && options[item].helpId)
		id = options[item].helpId;
}

void NotificationsPage::handleSelectionChanged() {
	int sel = table->getSelected();
	if(sel >= 0) {
		auto& s = options[sel].sound;
		soundGroup->setEnabled(!s.empty());
		soundFile->setText((s == _T("beep")) ? Util::emptyStringT : s);

		auto& b = options[sel].balloon;
		balloonGroup->setEnabled(b != SettingsManager::BALLOON_DISABLED);
		balloonBg->setChecked(b == SettingsManager::BALLOON_BACKGROUND);

		sound->setChecked(!s.empty());
		sound->setEnabled(true);

		balloon->setChecked(options[sel].balloon);
		balloon->setEnabled(true);

	} else {
		soundGroup->setEnabled(false);
		soundFile->setText(Util::emptyStringT);

		balloonGroup->setEnabled(false);
		balloonBg->setChecked(false);

		sound->setChecked(false);
		sound->setEnabled(false);

		balloon->setChecked(false);
		balloon->setEnabled(false);
	}
}

void NotificationsPage::handleDblClicked() {
	auto item = table->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos())));
	if(item.first == -1 || item.second == -1) {
		return;
	}

	switch(item.second) {
	case COLUMN_SOUND: handleSoundClicked(); break;
	case COLUMN_BALLOON: handleBalloonClicked(); break;
	}
}

void NotificationsPage::handleSoundClicked() {
	auto sel = table->getSelected();
	if(sel >= 0) {
		auto& s = options[sel].sound;
		if(s.empty())
			s = _T("beep");
		else
			s.clear();
		updateSound(sel);
		handleSelectionChanged();
	}
}

void NotificationsPage::handleBalloonClicked() {
	auto sel = table->getSelected();
	if(sel >= 0) {
		auto& b = options[sel].balloon;
		b = (b == SettingsManager::BALLOON_DISABLED) ? SettingsManager::BALLOON_ALWAYS : SettingsManager::BALLOON_DISABLED;
		updateBalloon(sel);
		handleSelectionChanged();
	}
}

void NotificationsPage::handlePlayClicked() {
	WinUtil::playSound(options[table->getSelected()].sound);
}

void NotificationsPage::handleSoundChanged() {
	if(!soundFile->getEnabled())
		return;

	auto sel = table->getSelected();
	auto& s = options[sel].sound;
	s = soundFile->getText();
	if(s.empty())
		s = _T("beep");
	updateSound(sel);
}

void NotificationsPage::handleBrowseClicked() {
	tstring x = soundFile->getText();
	if(LoadDialog(this).open(x)) {
		soundFile->setText(x);
	}
}

void NotificationsPage::handleBalloonBgClicked() {
	auto sel = table->getSelected();
	auto& b = options[sel].balloon;
	b = balloonBg->getChecked() ? SettingsManager::BALLOON_BACKGROUND : SettingsManager::BALLOON_ALWAYS;
	updateBalloon(sel);
}

void NotificationsPage::handleExampleClicked() {
	WinUtil::mainWindow->notify(T_("Balloon popup example"), T_("This is an example of a balloon popup notification!"));
}

void NotificationsPage::updateSound(size_t row) {
	auto& s = options[row].sound;
	table->setIcon(row, COLUMN_SOUND, s.empty() ? IMAGE_DISABLED : IMAGE_SOUND);
	table->setText(row, COLUMN_SOUND, s.empty() ? T_("No") : (s == _T("beep")) ? T_("Beep") : T_("Custom"));
}

void NotificationsPage::updateBalloon(size_t row) {
	auto& b = options[row].balloon;
	table->setIcon(row, COLUMN_BALLOON, (b == SettingsManager::BALLOON_DISABLED) ? IMAGE_DISABLED : IMAGE_BALLOON);
	table->setText(row, COLUMN_BALLOON, (b == SettingsManager::BALLOON_DISABLED) ? T_("No") :
		(b == SettingsManager::BALLOON_ALWAYS) ? T_("Yes") : T_("Yes (bg)"));
}
