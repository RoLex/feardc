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

#ifndef DCPLUSPLUS_WIN32_ABOUT_DLG_H
#define DCPLUSPLUS_WIN32_ABOUT_DLG_H

#include <memory>
#include <string>

#include <dcpp/forward.h>
#include <dcpp/HttpManagerListener.h>

#include <dwt/widgets/ModalDialog.h>

#include "forward.h"

using std::string;
using std::unique_ptr;

class AboutDlg : public dwt::ModalDialog, private HttpManagerListener
{
public:
	AboutDlg(dwt::Widget* parent);
	virtual ~AboutDlg();

	int run();

private:
	GridPtr grid;
	LabelPtr version;

	HttpConnection* c;

	bool handleInitDialog();

	void layout();

	void completeDownload(bool success, const string& result);

	// HttpManagerListener
	void on(HttpManagerListener::Failed, HttpConnection*, const string&) noexcept;
	void on(HttpManagerListener::Complete, HttpConnection*, OutputStream*) noexcept;
};

#endif // !defined(DCPLUSPLUS_WIN32_ABOUT_DLG_H)
