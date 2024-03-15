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

#include "stdafx.h"

#include "StatsFrame.h"

#include <dcpp/Socket.h>
#include <dcpp/TimerManager.h>

#include <dwt/resources/Pen.h>

const string StatsFrame::id = "Stats";
const string& StatsFrame::getId() const { return id; }

StatsFrame::StatsFrame(TabViewPtr parent) :
	BaseType(parent, T_("Network Statistics"), IDH_NET_STATS, IDI_NET_STATS),
	pen(new dwt::Pen(WinUtil::textColor)),
	upPen(new dwt::Pen(SETTING(UPLOAD_BG_COLOR))),
	downPen(new dwt::Pen(SETTING(DOWNLOAD_BG_COLOR))),
	width(0),
	height(0),
	twidth(0),
	lastTick(GET_TICK()),
	scrollTick(0),
	lastUp(Socket::getTotalUp()),
	lastDown(Socket::getTotalDown()),
	max(0)
{
	setFont(WinUtil::font);

	onPainting([this](dwt::PaintCanvas& canvas) {
		dwt::Rectangle rect { canvas.getPaintRect() };
		if(rect.width() == 0 || rect.height() == 0)
			return;
		draw(canvas, rect);
	});
	onPrinting([this](dwt::Canvas& canvas) { draw(canvas, dwt::Rectangle(getClientSize())); });

	initStatus();

	layout();

	setTimer([this] { return eachSecond(); }, 1000);
}

StatsFrame::~StatsFrame() {
}

void StatsFrame::draw(dwt::Canvas& canvas, const dwt::Rectangle& rect) {
	{
		auto select(canvas.select(*WinUtil::bgBrush));
		::BitBlt(canvas.handle(), rect.x(), rect.y(), rect.width(), rect.height(), NULL, 0, 0, PATCOPY);
	}

	canvas.setTextColor(WinUtil::textColor);
	canvas.setBkColor(WinUtil::bgColor);

	auto selectFont(canvas.select(*WinUtil::font));

	long fontHeight = getTextSize(_T("A")).y;
	int lines = height / (fontHeight * LINE_HEIGHT);
	int lheight = height / (lines+1);

	{
		auto select(canvas.select(*pen));
		for(int i = 0; i < lines; ++i) {
			int ypos = lheight * (i+1);
			if(ypos > fontHeight + 2) {
				canvas.moveTo(rect.left(), ypos);
				canvas.lineTo(rect.right(), ypos);
			}

			if(rect.x() <= twidth) {
				ypos -= fontHeight + 2;
				if(ypos < 0)
					ypos = 0;
				if(height == 0)
					height = 1;
				tstring txt = Text::toT(Util::formatBytes(max * (height-ypos) / height) + "/s");
				dwt::Point txtSize = getTextSize(txt);
				long tw = txtSize.x;
				if(tw + 2 > twidth)
					twidth = tw + 2;
				dwt::Rectangle rect { dwt::Point(1, ypos), txtSize };
				canvas.drawText(txt, rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
			}
		}

		if(rect.x() < twidth) {
			tstring txt = Text::toT(Util::formatBytes(max) + "/s");
			dwt::Point txtSize = getTextSize(txt);
			long tw = txtSize.x;
			if(tw + 2 > twidth)
				twidth = tw + 2;
			dwt::Rectangle rect { dwt::Point(1, 1), txtSize };
			canvas.drawText(txt, rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
		}

	}

	long clientRight = getClientSize().x;

	{
		auto select(canvas.select(*upPen));
		drawLine(canvas, up.begin(), up.end(), rect, clientRight);
	}

	{
		auto select(canvas.select(*downPen));
		drawLine(canvas, down.begin(), down.end(), rect, clientRight);
	}
}

void StatsFrame::layout() {
	dwt::Rectangle r { getClientSize() };

	r.size.y -= status->refresh();

	width = r.width();
	height = r.size.y - 1;

	redraw();
}

bool StatsFrame::eachSecond() {
	uint64_t tick = GET_TICK();
	uint64_t tdiff = tick - lastTick;
	if(tdiff == 0)
		return true;

	uint64_t scrollms = (tdiff + scrollTick)*PIX_PER_SEC;
	uint64_t scroll = scrollms / 1000;

	if(scroll == 0)
		return true;

	scrollTick = scrollms - (scroll * 1000);

	dwt::Point clientSize = getClientSize();
	RECT rect = { twidth, 0, clientSize.x, clientSize.y };
	::ScrollWindow(handle(), -((int)scroll), 0, &rect, &rect);

	int64_t d = Socket::getTotalDown();
	int64_t ddiff = d - lastDown;
	int64_t u = Socket::getTotalUp();
	int64_t udiff = u - lastUp;

	addTick(ddiff, tdiff, down, downAvg, scroll);
	addTick(udiff, tdiff, up, upAvg, scroll);

	int64_t mspeed = 0;
	auto i = down.begin();
	for(; i != down.end(); ++i) {
		if(mspeed < i->speed)
			mspeed = i->speed;
	}
	for(i = up.begin(); i != up.end(); ++i) {
		if(mspeed < i->speed)
			mspeed = i->speed;
	}
	if(mspeed > max || ((max * 3 / 4) > mspeed) ) {
		max = mspeed;
		redraw();
	}

	lastTick = tick;
	lastUp = u;
	lastDown = d;
	return true;
}

void StatsFrame::drawLine(dwt::Canvas& canvas, StatIter begin, StatIter end, const dwt::Rectangle& rect, long clientRight) {
	auto i = begin;
	for(; i != end; ++i) {
		if((clientRight - (long)i->scroll) < rect.right())
			break;
		clientRight -= i->scroll;
	}
	if(i != end) {
		int y = (max == 0) ? 0 : (int)((i->speed * height) / max);
		canvas.moveTo(clientRight, height - y);
		clientRight -= i->scroll;
		++i;

		for(; i != end && clientRight > twidth; ++i) {
			y = (max == 0) ? 0 : (int)((i->speed * height) / max);
			canvas.lineTo(clientRight, height - y);
			if(clientRight < rect.left())
				break;
			clientRight -= i->scroll;
		}
	}
}

void StatsFrame::addTick(int64_t bdiff, int64_t tdiff, StatList& lst, AvgList& avg, int scroll) {
	while((int)lst.size() > ((width / PIX_PER_SEC) + 1) ) {
		lst.pop_back();
	}
	while(avg.size() > AVG_SIZE ) {
		avg.pop_back();
	}
	int64_t bspeed = bdiff * (int64_t)1000 / tdiff;
	avg.push_front(bspeed);

	bspeed = 0;

	for(auto& ai: avg) {
		bspeed += ai;
	}

	bspeed /= avg.size();
	lst.emplace_front(scroll, bspeed);
}
