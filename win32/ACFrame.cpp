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

#include "ACFrame.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/SaveDialog.h>

#include "HoldRedraw.h"
#include "ParamDlg.h"
#include "TypedTable.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::SaveDialog;

const string ACFrame::id = "AboutConfig";
const string& ACFrame::getId() const { return id; }

static const ColumnInfo settingColumns[] = {
	{ N_("Setting name"), 150, false },
	{ N_("Status"), 50, false },
	{ N_("Type"), 40, false },
	{ N_("Value"), 1000, false },
};

const string ACFrame::SettingInfo::typeStrings[] = { "String", "Integer", "Boolean", "Int64", "Float" };

ACFrame::SettingInfo::SettingInfo(const int index_) : index(index_) {
	auto sm = SettingsManager::getInstance();

	if(sm->getType(index, type)) {
		columns[COLUMN_NAME] = Text::toT(sm->getSettingTags()[index]);
		columns[COLUMN_TYPE] = Text::toT(typeStrings[static_cast<int>(type)]);
		read();
	} else {
		dcassert(0);
	}
}

ACFrame::ACFrame(TabViewPtr parent) :
	BaseType(parent, T_("About:config"), IDH_ABOUTCONFIG, IDI_DCPP, false),
	grid(0),
	disclaimer(0),
	settings(0),
	filterGroup(0),
	filter(settingColumns, COLUMN_LAST, [this] { fillList(); }),
	optionsGroup(0),
	options(0),
	orgSize(0),
	visibleSettings(0)
{
	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		upper = grid->addChild(Grid::Seed(2, 1));
		upper->setSpacing(0);
		upper->column(0).mode = GridInfo::FILL;
		upper->row(0).size = 0;
		upper->row(0).mode = GridInfo::STATIC;
		upper->row(1).mode = GridInfo::FILL;
		upper->row(1).align = GridInfo::STRETCH;

		{
			Label::Seed seed = WinUtil::Seeds::label;
			seed.style &= ~WS_VISIBLE;
			seed.style |= WS_DISABLED | SS_CENTER;
			disclaimer = upper->addChild(seed);
			addWidget(disclaimer, NO_FOCUS);
			disclaimer->onClicked([this] { dismissDisclaim(); });
		}

		{
			WidgetSettings::Seed seed = WinUtil::Seeds::table;
			seed.style |= LVS_SINGLESEL;
			settings = upper->addChild(seed);
			addWidget(settings);
			settings->onContextMenu([this](const dwt::ScreenCoordinate& sc) { return handleSettingsContextMenu(sc); });
		}

		WinUtil::makeColumns(settings, settingColumns, COLUMN_LAST, SETTING(ACFRAME_ORDER), SETTING(ACFRAME_WIDTHS));
		settings->setSort(COLUMN_NAME, true);
		settings->onDblClicked([this] { modify(); });
		WinUtil::setColor(settings);
	}

	{
		auto lower = grid->addChild(Grid::Seed(2, 4));
		// columns 0 and 3, and row 1, are just here to have proper margins.
		lower->column(0).size = 0;
		lower->column(0).mode = GridInfo::STATIC;
		lower->column(1).mode = GridInfo::FILL;
		lower->column(2).mode = GridInfo::FILL;
		lower->column(3).size = 0;
		lower->column(3).mode = GridInfo::STATIC;
		lower->row(1).size = 0;
		lower->row(1).mode = GridInfo::STATIC;
		lower->setSpacing(6);

		GroupBox::Seed gs = WinUtil::Seeds::group;

		gs.caption = T_("F&ilter");
		filterGroup = lower->addChild(gs);
		lower->setWidget(filterGroup, 0, 1);
		filterGroup->setHelpId(IDH_ABOUT_CONFIG_FILTER); 

		auto filGrid = filterGroup->addChild(Grid::Seed(1, 3));
		filGrid->column(0).mode = GridInfo::FILL;

		filter.createTextBox(filGrid);
		filter.text->setHelpId(IDH_ABOUT_CONFIG_FILTER); 
		filter.text->setCue(T_("Filter settings"));
		addWidget(filter.text);

		filter.createColumnBox(filGrid);
		filter.column->setHelpId(IDH_ABOUT_CONFIG_FILTER); 
		addWidget(filter.column);

		filter.createMethodBox(filGrid);
		filter.method->setHelpId(IDH_ABOUT_CONFIG_FILTER); 
		addWidget(filter.method);

		gs.caption = T_("Options");
		optionsGroup = lower->addChild(gs);
		lower->setWidget(optionsGroup, 0, 2);

		options = optionsGroup->addChild(Grid::Seed(1, 5));
		options->column(0).mode = GridInfo::FILL;
		options->column(1).mode = GridInfo::FILL;
		options->column(2).mode = GridInfo::FILL;
		options->column(3).mode = GridInfo::FILL;
		options->column(4).mode = GridInfo::FILL;

		ButtonPtr modify, setDefault, saveFull, saveChanged, refresh;
		Button::Seed cs = WinUtil::Seeds::button;

		cs.caption = T_("Modify value");
		modify = options->addChild(cs);
		modify->setHelpId(IDH_ABOUT_CONFIG_MODIFY); 
		modify->onClicked([this] { this->modify(); });
		addWidget(modify);

		cs.caption = T_("Set default value");
		setDefault = options->addChild(cs);
		setDefault->setHelpId(IDH_ABOUT_CONFIG_SET_DEFAULT); 
		setDefault->onClicked([this] { setDefaultValue(); });
		addWidget(setDefault);

		cs.caption = T_("Save settings now");
		saveFull = options->addChild(cs);
		saveFull->setHelpId(IDH_ABOUT_CONFIG_SAVE); 
		saveFull->onClicked([this] { SettingsManager::getInstance()->save(); updateStatus(T_("Settings saved")); });
		addWidget(saveFull);

		cs.caption = T_("Export settings");
		saveChanged = options->addChild(cs);
		saveChanged->setHelpId(IDH_ABOUT_CONFIG_EXPORT); 
		saveChanged->onClicked([this] { exportSettings(); });
		addWidget(saveChanged);

		cs.caption = T_("Refresh settings");
		refresh = options->addChild(cs);
		refresh->setHelpId(IDH_ABOUT_CONFIG_REFRESH); 
		refresh->onClicked([this] { fillList(); });
		addWidget(refresh);
	}

	initStatus();

	addAccel(0, VK_RETURN, [this] { modify(); });
	addAccel(FALT, 'I', [this] { filter.text->setFocus(); }); 
	initAccels();

	layout();

	fillList();

	activate();
	settings->setFocus();

	orgSize = static_cast<int>(settings->size());
	status->setText(STATUS_STATUS, Text::toT(std::to_string(orgSize)) + T_(" settings loaded"));
}

ACFrame::~ACFrame() {
}

void ACFrame::layout() {
	dwt::Rectangle r(this->getClientSize());

	r.size.y -= status->refresh();

	grid->resize(r);
}

bool ACFrame::preClosing() {
	return true;
}

void ACFrame::postClosing() {
	SettingsManager::getInstance()->set(SettingsManager::ACFRAME_ORDER, WinUtil::toString(settings->getColumnOrder()));
	SettingsManager::getInstance()->set(SettingsManager::ACFRAME_WIDTHS, WinUtil::toString(settings->getColumnWidths()));
}

void ACFrame::updateStatus(tstring text) {
	if(orgSize > visibleSettings) {
		status->setText(STATUS_SETTINGS, Text::toT(std::to_string(visibleSettings)) + T_(" Matched setting(s) found"));
	} else {
		status->setText(STATUS_SETTINGS, T_("No filtering terms specified"));
	}

	if(!text.empty())
		status->setText(STATUS_STATUS, text);
}

void ACFrame::modify() {
	if(!settings->hasSelected())
		return;

	auto setting = settings->getSelectedData();
	auto& value = setting->getValue();

	//determine whether to call the ParamDlg or not in the event of a bool value
	if(setting->type == SettingsManager::TYPE_BOOL) {
		auto iValue = Util::toInt(Text::fromT(value));
		setting->update(Text::toT(std::to_string(abs(iValue - 1))));
		settings->update(setting);
		return;
	}

	ParamDlg dlg(this, T_("Enter a new value"), setting->getName(), value);
	if(dlg.run() == IDOK) {
		setting->update(dlg.getValue());
		settings->update(setting);
	}

	settings->setFocus();
}

void ACFrame::fillList() {
	dcdebug("ACFrame::fillList\n");

	if(SETTING(AC_DISCLAIM)) {
		if(disclaimer->getEnabled()) {
			disclaimer->setText(Util::emptyStringT);
			disclaimer->setEnabled(false);
			disclaimer->setVisible(false);
			upper->row(0).mode = GridInfo::STATIC;
			upper->layout();
		} else {
			disclaimer->setText(T_("Warning!\n"
				"Changing these settings can be harmful to the stability, security and performance of this application.\n"
				"You should only continue if you know what you're doing!\n"
				"Click here to proceed."
			));
			disclaimer->setEnabled(true);
			disclaimer->setVisible(true);
			settings->setEnabled(false);
			filterGroup->setEnabled(false);
			optionsGroup->setEnabled(false);
			upper->row(0).mode = GridInfo::AUTO;
			upper->layout();
		}
	}

	auto filterPrep = filter.prepare();

	HoldRedraw hold(settings);
	settings->clear();

	auto sm = SettingsManager::getInstance();
	visibleSettings = 0;

	for(auto i = 0; i < SettingsManager::SETTINGS_LAST; ++i) {
		if (sm->getSettingTags()[i] == "SENTRY")
			continue;

		auto si = new SettingInfo(i);
		auto filterInfoF = [this, &si](int column) { return Text::fromT(si->getText(column)); };

		if(filter.empty() || filter.match(filterPrep, filterInfoF)) {
			settings->insert(static_cast<int>(settings->size()), si);
			visibleSettings++;
		}
	}
	settings->resort();

	if (settings->size() == 1) settings->setSelected(0);

	updateStatus();
}

void ACFrame::exportSettings() {
	auto target = Text::toT("DCPlusPlus.xml");
	if(SaveDialog(this).open(target)) {
		SettingsManager::getInstance()->save(Text::fromT(target));
		updateStatus(T_("Settings were exported to: ") + target);
	}
}

void ACFrame::setDefaultValue() {
	if(!settings->hasSelected())
		return;

	auto setting = settings->getSelectedData();
	SettingsManager::getInstance()->unset(setting->index);
	setting->read();
	settings->update(setting);
	settings->setFocus();
}

int ACFrame::SettingInfo::getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const {

	if(!isDefault) {
		LOGFONT lf;
		WinUtil::decodeFont(Text::toT(SETTING(MAIN_FONT)), lf);
		//set FW_BOLD while we already have a LOGFONT rather than firing off dwt::Font::makeBold() which just calls getLogFont() anyways...
		lf.lfWeight = FW_BOLD;
		auto boldFont = new dwt::Font(lf);
		font = boldFont->handle();
	}

	return CDRF_NEWFONT;
}

void ACFrame::dismissDisclaim() {
	auto sm = SettingsManager::getInstance();
	sm->set(static_cast<SettingsManager::BoolSetting>(SettingsManager::AC_DISCLAIM), 0);

	if(disclaimer->getEnabled()) {
		disclaimer->setText(Util::emptyStringT);
		disclaimer->setEnabled(false);
		disclaimer->setVisible(false);
		settings->setEnabled(true);
		filterGroup->setEnabled(true);
		optionsGroup->setEnabled(true);
		upper->row(0).mode = GridInfo::STATIC;
		upper->layout();
	}

	fillList();
}

bool ACFrame::handleSettingsContextMenu(dwt::ScreenCoordinate pt) {
	if(!settings->hasSelected())
		return false;

	auto setting = settings->getSelectedData();
	auto& name = setting->getName();
	auto menu = addChild(WinUtil::Seeds::menu);
	menu->setTitle(name, getParent()->getIcon(this));

	if(setting->type == SettingsManager::TYPE_BOOL) {
		menu->appendItem(T_("&Toggle"), [this] { modify(); });
	} else {
		menu->appendItem(T_("&Modify"), [this] { modify(); });
	}

	menu->appendSeparator();
	menu->appendItem(T_("Copy &Name"), [this] { WinUtil::setClipboard(settings->getSelectedData()->getName()); });
	menu->appendItem(T_("Copy &Value"), [this] { WinUtil::setClipboard(settings->getSelectedData()->getValue()); });
	menu->appendItem(T_("&Set Default Value"), [this] { setDefaultValue(); });

	menu->open(pt);
	return true;
}
