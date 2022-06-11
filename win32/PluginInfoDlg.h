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

#ifndef DCPLUSPLUS_WIN32_PLUGININFODLG_H
#define DCPLUSPLUS_WIN32_PLUGININFODLG_H

#include <dcpp/typedefs.h>

#include <dwt/widgets/ModalDialog.h>

#include "forward.h"

class PluginInfoDlg : public dwt::ModalDialog
{
public:
	PluginInfoDlg(dwt::Widget* parent, const string& path);
	virtual ~PluginInfoDlg();

	int run();

private:
	bool handleInitDialog(const string& path);
	void handleOK(const DcextInfo& info);

	void layout();

	void error(const tstring& message, const tstring& title);

	GridPtr grid;
};

#endif
