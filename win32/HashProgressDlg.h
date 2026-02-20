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

#ifndef DCPLUSPLUS_WIN32_HASH_PROGESS_DLG_H
#define DCPLUSPLUS_WIN32_HASH_PROGESS_DLG_H

#include "GridDialog.h"

class HashProgressDlg : public GridDialog
{
public:
	HashProgressDlg(dwt::Widget* parent, bool aAutoClose);
	virtual ~HashProgressDlg();

private:
	LabelPtr file;
	LabelPtr stat;
	LabelPtr speed;
	LabelPtr left;
	ProgressBarPtr progress;
	ButtonPtr pauseResume;

	bool autoClose;
	uint64_t startBytes;
	size_t startFiles;
	uint64_t startTime;

	bool handleInitDialog();

	bool updateStats();

	void handlePauseResume();
	void setButtonState();
};

#endif // !defined(DCPLUSPLUS_WIN32_HASH_PROGESS_DLG_H)
