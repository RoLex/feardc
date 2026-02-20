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

#ifndef DCPLUSPLUS_WIN32_GRIDDIALOG_H
#define DCPLUSPLUS_WIN32_GRIDDIALOG_H

#include <dwt/widgets/ModalDialog.h>

#include "forward.h"

/// base for a dialog that contains a grid and adapts to the grid's preferred height
class GridDialog : public dwt::ModalDialog
{
public:
	GridDialog(dwt::Widget* parent, const long width_, const DWORD styles_ = 0);

	int run();

protected:
	GridPtr grid;

	void layout();

private:
	const long width;
	const DWORD styles;
};

#endif
