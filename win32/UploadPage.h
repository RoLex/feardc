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

#ifndef DCPLUSPLUS_WIN32_UPLOAD_PAGE_H
#define DCPLUSPLUS_WIN32_UPLOAD_PAGE_H

#include <dcpp/typedefs.h>

#include "PropPage.h"

class UploadPage : public PropPage
{
public:
	UploadPage(dwt::Widget* parent);
	virtual ~UploadPage();

	virtual void layout();
	virtual void write();

private:
	ItemList items;

	TablePtr directories;
	LabelPtr total;

	ButtonPtr rename;
	ButtonPtr remove;

	void handleDoubleClick();
	bool handleKeyDown(int c);
	void handleSelectionChanged();
	void handleDragDrop(const TStringList& files);
	void handleAddClicked();
	void handleRenameClicked();
	void handleRemoveClicked();

	void addRow(const string& virtualPath, const string& realPath);
	void fillList();
	void refreshTotalSize();
	void addDirectory(const tstring& aPath);
};

#endif // !defined(DCPLUSPLUS_WIN32_UPLOAD_PAGE_H)
