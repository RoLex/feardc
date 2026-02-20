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

#ifndef DCPLUSPLUS_WIN32_SPLASHWINDOW_H
#define DCPLUSPLUS_WIN32_SPLASHWINDOW_H

#include <dcpp/typedefs.h>

#include <dwt/widgets/Window.h>

class SplashWindow : public dwt::Window  {
public:
	SplashWindow();
	virtual ~SplashWindow();

	void operator()(const string& status);
	void operator()(float progress);

private:
	void draw();

	long iconSize;
	dwt::IconPtr icon;
	uint32_t logoRot;

	tstring status;
	float progress;
};

#endif
