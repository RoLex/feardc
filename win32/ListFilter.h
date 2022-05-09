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

#ifndef DCPLUSPLUS_WIN32_LIST_FILTER_H
#define DCPLUSPLUS_WIN32_LIST_FILTER_H

#include <boost/core/noncopyable.hpp>

#include <dcpp/StringMatch.h>

#include "WinUtil.h"

/** Manages controls to help filter a table. */
class ListFilter : boost::noncopyable {
	typedef std::function<void ()> UpdateF;
	typedef std::function<string (size_t)> InfoF;

	struct Preparation {
		size_t column;
		size_t method;
		double size;
	};

public:
	ListFilter(const ColumnInfo* columns, size_t colCount, UpdateF updateF);

	void createTextBox(GridPtr grid);
	void createColumnBox(GridPtr grid);
	void createMethodBox(GridPtr grid);

	Preparation prepare();
	bool match(const Preparation& prep, InfoF infoF) const;

	bool empty() const;

	TextBoxPtr text;
	ComboBoxPtr column;
	ComboBoxPtr method;

private:
	enum {
		EQUAL,
		GREATER_EQUAL,
		LESS_EQUAL,
		GREATER,
		LESS,
		NOT_EQUAL,
	};

	void textUpdated();
	void columnChanged();

	double prepareSize() const;

	const ColumnInfo* const columns;
	const size_t colCount;
	const UpdateF updateF;

	StringMatch matcher;
};

#endif
