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
#include "UserMatchDlg.h"

#include <boost/range/algorithm/for_each.hpp>

#include <dcpp/version.h>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/MessageBox.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Button;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

UserMatchDlg::UserMatchDlg(dwt::Widget* parent, const UserMatch* initialMatcher) :
GridDialog(parent, 700, DS_CONTEXTHELP),
name(0),
favs(0),
ops(0),
bots(0),
//avdb(0),
rules(0),
forceChat(0),
ignoreChat(0)
{
	onInitDialog([this, initialMatcher] { return handleInitDialog(initialMatcher); });
	onHelp(&WinUtil::help);
}

UserMatchDlg::~UserMatchDlg() {
}

bool UserMatchDlg::handleInitDialog(const UserMatch* initialMatcher) {
	setHelpId(IDH_USER_MATCH);

	grid = addChild(Grid::Seed(5, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->setSpacing(8);

	name = grid->addChild(GroupBox::Seed(T_("Name")))->addChild(WinUtil::Seeds::Dialog::textBox);
	name->setHelpId(IDH_USER_MATCH_NAME);

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(1, 3));
		cur->setSpacing(grid->getSpacing());

		favs = cur->addChild(CheckBox::Seed(T_("Match favorite users")));
		favs->setHelpId(IDH_USER_MATCH_FAVS);

		ops = cur->addChild(CheckBox::Seed(T_("Match operators")));
		ops->setHelpId(IDH_USER_MATCH_OPS);

		bots = cur->addChild(CheckBox::Seed(T_("Match bots")));
		bots->setHelpId(IDH_USER_MATCH_BOTS);

		/*
		avdb = cur->addChild(CheckBox::Seed(T_("Match infected")));
		avdb->setHelpId(IDH_USER_MATCH_AVDB);
		*/
	}

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		rules = cur->addChild(Grid::Seed(0, 4));
		rules->column(1).mode = GridInfo::FILL;

		auto button = cur->addChild(Grid::Seed(1, 1))->addChild(Button::Seed(T_("Add a rule")));
		button->setHelpId(IDH_USER_MATCH_ADD_RULE);
		button->onClicked([this] { addRow(); layoutRules(); });
	}

	{
		auto cur = grid->addChild(GroupBox::Seed())->addChild(Grid::Seed(1, 2));
		cur->setSpacing(grid->getSpacing());

		forceChat = cur->addChild(CheckBox::Seed(T_("Always show chat messages")));
		forceChat->setHelpId(IDH_USER_MATCH_FORCE_CHAT);
		forceChat->onClicked([this] { if(forceChat->getChecked()) ignoreChat->setChecked(false); });

		ignoreChat = cur->addChild(CheckBox::Seed(T_("Ignore chat messages")));
		ignoreChat->setHelpId(IDH_USER_MATCH_IGNORE_CHAT);
		ignoreChat->onClicked([this] { if(ignoreChat->getChecked()) forceChat->setChecked(false); });
	}

	{
		auto cur = grid->addChild(Grid::Seed(1, 3));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->setSpacing(grid->getSpacing());

		WinUtil::addDlgButtons(cur,
			[this] { handleOKClicked(); },
			[this] { endDialog(IDCANCEL); });

		WinUtil::addHelpButton(cur)->onClicked([this] { WinUtil::help(this); });
	}

	if(initialMatcher) {
		name->setText(Text::toT(initialMatcher->name));

		if(initialMatcher->isSet(UserMatch::FAVS)) { favs->setChecked(true); }
		if(initialMatcher->isSet(UserMatch::OPS)) { ops->setChecked(true); }
		if(initialMatcher->isSet(UserMatch::BOTS)) { bots->setChecked(true); }

		/*
		if (initialMatcher->isSet(UserMatch::AVDB))
			avdb->setChecked(true);
		*/

		for(auto& i: initialMatcher->rules) {
			addRow(&i);
		}

		if(initialMatcher->isSet(UserMatch::FORCE_CHAT)) { forceChat->setChecked(true); }
		else if(initialMatcher->isSet(UserMatch::IGNORE_CHAT)) { ignoreChat->setChecked(true); }

		result.style = initialMatcher->style;
	}

	setText(T_("User matching definition"));

	layout();
	centerWindow();

	return false;
}

void UserMatchDlg::handleOKClicked() {
	result.name = Text::fromT(name->getText());

	if(result.name.empty()) {
		dwt::MessageBox(this).show(T_("Specify a name for this user matching definition"),
			_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONEXCLAMATION);
		return;
	}

	if(favs->getChecked()) { result.setFlag(UserMatch::FAVS); }
	if(ops->getChecked()) { result.setFlag(UserMatch::OPS); }
	if(bots->getChecked()) { result.setFlag(UserMatch::BOTS); }

	/*
	if (avdb->getChecked())
		result.setFlag(UserMatch::AVDB);
	*/

	auto controls = rules->getChildren<Control>();
	int8_t counter = -1;
	UserMatch::Rule rule;
	boost::for_each(controls, [this, &counter, &rule](Control* control) {
		enum { RuleField, RuleSearch, RuleMethod, RuleRemove };
		switch(++counter) {
		case RuleField:
			{
				rule = UserMatch::Rule();
				rule.field = static_cast<decltype(rule.field)>(static_cast<ComboBoxPtr>(control)->getSelected());
				break;
			}
		case RuleSearch:
			{
				rule.pattern = Text::fromT(static_cast<TextBoxPtr>(control)->getText());
				break;
			}
		case RuleMethod:
			{
				rule.setMethod(static_cast<UserMatch::Rule::Method>(static_cast<ComboBoxPtr>(control)->getSelected()));
				break;
			}
		case RuleRemove:
			{
				this->result.addRule(std::move(rule));
				counter = -1;
				break;
			}
		}
	});

	if(forceChat->getChecked()) { result.setFlag(UserMatch::FORCE_CHAT); }
	else if(ignoreChat->getChecked()) { result.setFlag(UserMatch::IGNORE_CHAT); }

	endDialog(IDOK);
}

void UserMatchDlg::layoutRules() {
	rules->layout();
	layout();
	centerWindow();
}

void UserMatchDlg::addRow(const UserMatch::Rule* rule) {
	rules->addRow();

	auto field = rules->addChild(WinUtil::Seeds::Dialog::comboBox);
	field->setHelpId(IDH_USER_MATCH_RULE_FIELD);

	auto search = rules->addChild(WinUtil::Seeds::Dialog::textBox);
	search->setHelpId(IDH_USER_MATCH_RULE_PATTERN);

	auto method = rules->addChild(WinUtil::Seeds::Dialog::comboBox);
	method->setHelpId(IDH_USER_MATCH_RULE_METHOD);

	{
		auto seed = Button::Seed(_T("X"));
		seed.padding.x = 10;
		auto remove = rules->addChild(seed);
		remove->setHelpId(IDH_USER_MATCH_RULE_REMOVE);
		remove->onClicked([this, remove] { callAsync([=, this] {
			rules->removeRow(remove);
			layoutRules();
		}); });
	}

	tstring fields[UserMatch::Rule::FIELD_LAST] = { T_("Nick"), T_("CID"), T_("IP"), T_("Hub address") };
	std::for_each(fields, fields + UserMatch::Rule::FIELD_LAST, [field](const tstring& str) { field->addValue(str); });
	field->setSelected(rule ? rule->field : 0);

	WinUtil::addFilterMethods(method);
	method->setSelected(rule ? rule->getMethod() : 0);

	if(rule) {
		search->setText(Text::toT(rule->pattern));
	}
}
