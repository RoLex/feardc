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

#include "stdafx.h"
#include "SplashWindow.h"

#include <dcpp/format.h>
#include <dcpp/Text.h>
#include <dcpp/version.h>

#include <dwt/DWTException.h>
#include <dwt/widgets/ProgressBar.h>

#include "resource.h"
#include "WinUtil.h"

SplashWindow::SplashWindow() :
	dwt::Window(0),
	logoRot(0),
	progress(0)
{
	// 256x icons only work on >= Vista. on failure, try loading a 48x image.
	try {
		iconSize = 128;
		icon = WinUtil::createIcon(IDI_DCPP, iconSize);
	} catch(const dwt::DWTException&) {
		iconSize = 48;
		icon = WinUtil::createIcon(IDI_DCPP, iconSize);
	}

	{
		// create a layered window (WS_EX_LAYERED); it will be shown later with UpdateLayeredWindow.
		Seed seed(_T(APPNAME) _T(" ") _T(VERSIONSTRING));
		seed.style = WS_POPUP | WS_VISIBLE;
		seed.exStyle = WS_EX_LAYERED;
		create(seed);
	}

	setFont(0); // default font since settings haven't been loaded yet

	// start showing when loading settings. language info isn't loaded at this point...
	operator()(Text::fromT(_T(APPNAME)));
}

SplashWindow::~SplashWindow() {
}

void SplashWindow::operator()(const string& status) {
	this->status = str(TF_("Loading FearDC, please wait... (%1%)") % Text::toT(status));
	progress = 0;
	callAsync([this] { draw(); });
}

void SplashWindow::operator()(float progress) {
	this->progress = progress;
	callAsync([this] { draw(); });
}

void SplashWindow::draw() {
	auto text = status;
	if(progress) { text += _T(" [") + Text::toT(Util::toString(progress * 100.)) + _T("%]"); }
	logoRot = (logoRot + 1) % 4;

	// set up sizes.
	const long spacing { 6 }; // space between the icon and the text
	const dwt::Point padding { 4, 4 }; // padding borders around the text
	const auto textSize = getTextSize(text) + padding + padding;
	SIZE size { std::max(iconSize, textSize.x), iconSize + spacing + textSize.y };
	dwt::Rectangle textRect { std::max(iconSize - textSize.x, 0L) / 2, size.cy - textSize.y, textSize.x, textSize.y };
	dwt::Rectangle iconRect { std::max(textSize.x - iconSize, 0L) / 2, 0, iconSize, iconSize };

	dwt::UpdateCanvas windowCanvas { this };
	dwt::CompatibleCanvas canvas { windowCanvas.handle() };

	// create the bitmap with CreateDIBSection to have access to its bits (used when drawing text).
	BITMAPINFO info { { sizeof(BITMAPINFOHEADER), size.cx, -size.cy, 1, 32, BI_RGB } };
	RGBQUAD* bits;
	dwt::Bitmap bitmap { ::CreateDIBSection(windowCanvas.handle(), &info, DIB_RGB_COLORS, &reinterpret_cast<void*&>(bits), 0, 0) };
	auto select(canvas.select(bitmap));

	auto bit = [&](long x, long y) -> RGBQUAD& { return bits[x + y * size.cx]; };

	// draw the icon.
	canvas.drawIcon(icon, iconRect);

	// rotate the icon by swapping its bits, quarter by quarter (just because I can).
	auto iconBit = [&](long x, long y) -> RGBQUAD& { return bit(iconRect.left() + x, iconRect.top() + y); };
	for(long y = 0; y < iconSize / 2; ++y) {
		for(long x = 0; x < iconSize / 2; ++x) {
			auto &bit1 = iconBit(x, y), &bit3 = iconBit(iconSize - 1 - x, iconSize - 1 - y),
				&bit2 = iconBit(y, iconSize - 1 - x), &bit4 = iconBit(iconSize - 1 - y, x);
			switch(logoRot) {
			case 0: break; // identity
			case 1: // 90
				{
					auto prev = bit1;
					bit1 = bit2;
					bit2 = bit3;
					bit3 = bit4;
					bit4 = prev;
					break;
				}
			case 2: // 180
				{
					std::swap(bit1, bit3);
					std::swap(bit2, bit4);
					break;
				}
			case 3: // 270
				{
					auto prev = bit1;
					bit1 = bit4;
					bit4 = bit3;
					bit3 = bit2;
					bit2 = prev;
					break;
				}
			}
		}
	}

	// draw text borders and fill the text background.
	::RECT rc = textRect;
	::DrawEdge(canvas.handle(), &rc, EDGE_BUMP, BF_RECT | BF_MIDDLE);

	// draw the text.
	auto bkMode(canvas.setBkMode(true));
	canvas.setTextColor(dwt::Color::predefined(COLOR_WINDOWTEXT));
	auto selectFont(canvas.select(*getFont()));
	canvas.drawText(text.c_str(), textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// set bits within the text rectangle to not be transparent. rgbReserved is the alpha value.
	// to simulate a progress bar, some bits are made to be semi-opaque.
	long pos = textRect.left() + progress * static_cast<float>(textRect.width());
	for(long y = textRect.top(), yn = textRect.bottom(); y < yn; ++y) {
		for(long x = textRect.left(); x < pos; ++x) {
			bit(x, y).rgbReserved = 191;
		}
		for(long x = pos, xn = textRect.right(); x < xn; ++x) {
			bit(x, y).rgbReserved = 255;
		}
	}

	auto desktop = getPrimaryDesktopSize();
	POINT pt { std::max(desktop.x - size.cx, 0L) / 2, std::max(desktop.y - size.cy - iconSize, 0L) / 2 };
	POINT canvasPt { 0 };
	BLENDFUNCTION blend { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	::UpdateLayeredWindow(handle(), windowCanvas.handle(), &pt, &size, canvas.handle(), &canvasPt, 0, &blend, ULW_ALPHA);
}
