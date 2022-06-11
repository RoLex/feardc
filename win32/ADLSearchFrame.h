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

#ifndef DCPLUSPLUS_WIN32_ADL_SEARCH_FRAME_H
#define DCPLUSPLUS_WIN32_ADL_SEARCH_FRAME_H

#include <dcpp/ADLSearch.h>

#include "StaticFrame.h"

class ADLSearchFrame :
	public StaticFrame<ADLSearchFrame>
{
	typedef StaticFrame<ADLSearchFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	friend class StaticFrame<ADLSearchFrame>;
	friend class MDIChildFrame<ADLSearchFrame>;

	ADLSearchFrame(TabViewPtr parent);
	virtual ~ADLSearchFrame();

	void layout();

	bool preClosing();
	void postClosing();

private:
	enum {
		COLUMN_FIRST,
		COLUMN_ACTIVE_SEARCH_STRING = COLUMN_FIRST,
		COLUMN_REGEX,
		COLUMN_SOURCE_TYPE,
		COLUMN_DEST_DIR,
		COLUMN_MIN_FILE_SIZE,
		COLUMN_MAX_FILE_SIZE,
		COLUMN_LAST
	};

	GridPtr grid;
	TablePtr items;

	void handleAdd();
	void handleProperties();
	void handleUp();
	void handleDown();
	void handleRemove();
	void handleDoubleClick();
	bool handleKeyDown(int c);
	LRESULT handleItemChanged(LPARAM lParam);
	bool handleContextMenu(dwt::ScreenCoordinate sc);

	void addEntry(ADLSearch& search, int index = -1, bool scroll = true);
};

#endif // !defined(DCPLUSPLUS_WIN32_ADL_SEARCH_FRAME_H)
