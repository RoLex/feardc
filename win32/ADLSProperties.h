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

#ifndef DCPLUSPLUS_WIN32_A_D_L_S_PROPERTIES_H
#define DCPLUSPLUS_WIN32_A_D_L_S_PROPERTIES_H

#include <dcpp/forward.h>

#include "GridDialog.h"

class ADLSProperties : public GridDialog
{
public:
	ADLSProperties(dwt::Widget* parent, ADLSearch& search_);
	virtual ~ADLSProperties();

private:
	TextBoxPtr searchString;
	CheckBoxPtr reg;
	ComboBoxPtr searchType;
	TextBoxPtr minSize;
	TextBoxPtr maxSize;
	ComboBoxPtr sizeType;
	TextBoxPtr destDir;
	CheckBoxPtr active;
	CheckBoxPtr autoQueue;

	ADLSearch& search;

	bool handleInitDialog();
	void handleOKClicked();
};

#endif // !defined(DCPLUSPLUS_WIN32_A_D_L_S_PROPERTIES_H)
