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
#include "ListFilter.h"

#include <dwt/widgets/Grid.h>

ListFilter::ListFilter(const ColumnInfo* columns, size_t colCount, UpdateF updateF) :
columns(columns),
colCount(colCount),
updateF(updateF)
{
}

void ListFilter::createTextBox(GridPtr grid) {
	auto seed = WinUtil::Seeds::textBox;
	seed.style |= ES_AUTOHSCROLL | WS_CLIPCHILDREN;
	text = grid->addChild(seed);

	text->onUpdated([this] { textUpdated(); });

	WinUtil::addSearchIcon(text);
}

void ListFilter::createColumnBox(GridPtr grid) {
	column = grid->addChild(WinUtil::Seeds::comboBox);

	for(size_t i = 0; i < colCount; ++i) {
		column->addValue(T_(columns[i].name));
	}
	column->addValue(T_("Any"));
	column->setSelected(colCount);

	column->onSelectionChanged([this] { columnChanged(); });
}

void ListFilter::createMethodBox(GridPtr grid) {
	method = grid->addChild(WinUtil::Seeds::comboBox);

	WinUtil::addFilterMethods(method);
	method->setSelected(StringMatch::PARTIAL);

	method->onSelectionChanged([this] { updateF(); });
}

ListFilter::Preparation ListFilter::prepare() {
	Preparation prep;
	if(empty())
		return prep;

	prep.column = column->getSelected();
	prep.method = method->getSelected();

	if(prep.method < StringMatch::METHOD_LAST) {
		matcher.setMethod(static_cast<StringMatch::Method>(prep.method));
		matcher.prepare();

	} else {
		prep.size = prepareSize();
	}

	return prep;
}

bool ListFilter::match(const Preparation& prep, InfoF infoF) const {
	if(empty())
		return true;

	if(prep.method < StringMatch::METHOD_LAST) {
		if(prep.column >= colCount) {
			for(size_t i = 0; i < colCount; ++i) {
				if(matcher.match(infoF(i))) {
					return true;
				}
			}
		} else {
			return matcher.match(infoF(prep.column));
		}

	} else {
		auto size = Util::toDouble(infoF(prep.column));
		switch(prep.method - StringMatch::METHOD_LAST) {
		case EQUAL: return size == prep.size;
		case GREATER_EQUAL: return size >= prep.size;
		case LESS_EQUAL: return size <= prep.size;
		case GREATER: return size > prep.size;
		case LESS: return size < prep.size;
		case NOT_EQUAL: return size != prep.size;
		}
	}

	return false;
}

bool ListFilter::empty() const {
	return matcher.pattern.empty();
}

void ListFilter::textUpdated() {
	auto newText = Text::fromT(text->getText());
	if(newText != matcher.pattern) {
		matcher.pattern = newText;
		updateF();
	}
}

void ListFilter::columnChanged() {
	auto n = method->size();
	size_t col = column->getSelected();

	if(col < colCount && columns[col].numerical) {
		if(n <= StringMatch::METHOD_LAST) {
			method->addValue(_T("="));
			method->addValue(_T(">="));
			method->addValue(_T("<="));
			method->addValue(_T(">"));
			method->addValue(_T("<"));
			method->addValue(_T("!="));
		}

	} else if(n > StringMatch::METHOD_LAST) {
		for(size_t i = StringMatch::METHOD_LAST; i < n; ++i) {
			method->erase(StringMatch::METHOD_LAST);
		}
		if(method->getSelected() == -1) {
			method->setSelected(StringMatch::PARTIAL);
		}
	}

	updateF();
}

double ListFilter::prepareSize() const {
	size_t end;
	int64_t multiplier;

	if((end = Util::findSubString(matcher.pattern, _("TiB"))) != string::npos) {
		multiplier = 1024LL * 1024LL * 1024LL * 1024LL;
	} else if((end = Util::findSubString(matcher.pattern, _("GiB"))) != string::npos) {
		multiplier = 1024 * 1024 * 1024;
	} else if((end = Util::findSubString(matcher.pattern, _("MiB"))) != string::npos) {
		multiplier = 1024 * 1024;
	} else if((end = Util::findSubString(matcher.pattern, _("KiB"))) != string::npos) {
		multiplier = 1024;
	} else if((end = Util::findSubString(matcher.pattern, _("TB"))) != string::npos) {
		multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
	} else if((end = Util::findSubString(matcher.pattern, _("GB"))) != string::npos) {
		multiplier = 1000 * 1000 * 1000;
	} else if((end = Util::findSubString(matcher.pattern, _("MB"))) != string::npos) {
		multiplier = 1000 * 1000;
	} else if((end = Util::findSubString(matcher.pattern, _("KB"))) != string::npos) {
		multiplier = 1000;
	} else {
		multiplier = 1;
	}

	return Util::toDouble(matcher.pattern.substr(0, end)) * multiplier;
}
