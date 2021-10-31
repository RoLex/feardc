/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_NOTEPAD_FRAME_H
#define DCPLUSPLUS_WIN32_NOTEPAD_FRAME_H

#include "StaticFrame.h"

class NotepadFrame :
	public StaticFrame<NotepadFrame>,
	private SettingsManagerListener
{
	typedef StaticFrame<NotepadFrame> BaseType;
public:
	enum Status {
		STATUS_STATUS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	friend class StaticFrame<NotepadFrame>;
	friend class MDIChildFrame<NotepadFrame>;

	NotepadFrame(TabViewPtr parent);
	virtual ~NotepadFrame();

	void layout();
	bool preClosing();

private:
	TextBoxPtr pad;

	void save();
	virtual void on(SettingsManagerListener::Save, SimpleXML&) noexcept;

	bool handleMenu(dwt::ScreenCoordinate pt);
	MenuPtr makeMenu(dwt::ScreenCoordinate pt);
};

#endif

