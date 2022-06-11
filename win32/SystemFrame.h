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

#ifndef DCPLUSPLUS_WIN32_SYSTEM_FRAME_H
#define DCPLUSPLUS_WIN32_SYSTEM_FRAME_H

#include <dcpp/LogManagerListener.h>

#include "StaticFrame.h"

class SystemFrame : public StaticFrame<SystemFrame>,
	private LogManagerListener
{
	typedef StaticFrame<SystemFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

private:
	friend class StaticFrame<SystemFrame>;
	friend class MDIChildFrame<SystemFrame>;

	TextBoxPtr log;

	SystemFrame(TabViewPtr parent);
	virtual ~SystemFrame();

	void layout();
	bool preClosing();

	void addLine(time_t t, const tstring& msg);
	void openFile(const string& path) const;

	bool handleContextMenu(const dwt::ScreenCoordinate& pt);
	bool handleDoubleClick(const dwt::MouseEvent& mouseEvent);

	// LogManagerListener
	virtual void on(Message, time_t t, const string& message) noexcept;
};

#endif
