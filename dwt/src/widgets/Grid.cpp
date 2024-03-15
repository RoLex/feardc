/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <dwt/widgets/Grid.h>

#include <dwt/util/check.h>
#include <dwt/util/GDI.h>
#include <dwt/util/HoldResize.h>

#include <boost/range/distance.hpp>

#include <algorithm>
#include <numeric>

namespace dwt {

Grid::Seed::Seed(size_t rows_, size_t cols_) :
BaseType::Seed(0, WS_EX_CONTROLPARENT),
rows(rows_),
cols(cols_)
{
}

void Grid::create( const Seed & cs )
{
	BaseType::create(cs);
	rows.resize(cs.rows);
	for(size_t i = 0; i < rows.size(); ++i) {
		rows[i].align = GridInfo::CENTER;	// Default to center for vertical alignment
	}
	columns.resize(cs.cols);
	for(size_t i = 0; i < columns.size(); ++i) {
		columns[i].align = GridInfo::STRETCH;	// Default to stretch for horizontal alignment
	}

	onEnabled([this](bool b) { handleEnabled(b); });
}

Point Grid::getPreferredSize() {
	// Make sure we have WidgetInfo's for every child...
	auto children = getChildren<Control>();
	std::for_each(children.first, children.second, [this](Control* w) { getWidgetInfo(w); });

	std::vector<size_t> rowSize = calcSizes(rows, columns, 0, true);
	std::vector<size_t> colSize = calcSizes(columns, rows, 0, false);
	Point p(
		std::accumulate(colSize.begin(), colSize.end(), 0),
		std::accumulate(rowSize.begin(), rowSize.end(), 0));

	return p + actualSpacing();
}

Point Grid::getPreferredSize(size_t row, size_t column) const {
	Point ret(0, 0);

	for(auto& i: widgetInfo) {
		if(i.inCell(row, column) && i.w->hasStyle(WS_VISIBLE)) {
			ret = i.w->getPreferredSize();
			// TODO consider fractions...
			if(i.colSpan > 1) {
				if(ret.x > i.colSpan*spacing) {
					ret.x = (ret.x - i.colSpan*spacing) / i.colSpan;
				} else {
					ret.x = 0;
				}
			}
			if(i.rowSpan > 1) {
				if(ret.y > i.rowSpan*spacing) {
					ret.y = (ret.y - i.rowSpan*spacing) / i.rowSpan;
				} else {
					ret.y = 0;
				}
			}
		}
	}
	return ret;
}

bool Grid::WidgetInfo::inCell(size_t r, size_t c) const {
	return r >= row && r < (row + rowSpan) && c >= column && c < (column + colSpan);
}

GridInfo& Grid::row(size_t i) {
	return rows[i];
}

GridInfo& Grid::column(size_t i) {
	return columns[i];
}

std::vector<size_t> Grid::calcSizes(const GridInfoList& x, const GridInfoList& y, size_t cur, bool isRow) const {
	std::vector<size_t> ret(x.size());

	size_t total = 0;
	size_t fills = 0;
	for(size_t i = 0; i < ret.size(); ++i) {
		ret[i] = x[i].size;

		switch(x[i].mode) {
		case GridInfo::STATIC:
			ret[i] *= util::dpiFactor();
			break;
		case GridInfo::FILL:
			fills++;
			break;
		case GridInfo::AUTO:
			for(size_t j = 0; j < y.size(); ++j) {
				ret[i] = std::max(ret[i], static_cast<size_t>(isRow ? getPreferredSize(i, j).y : getPreferredSize(j, i).x));
			}
			break;
		}
		total += ret[i];
	}

	if(total < cur && fills > 0) {
		size_t left = cur - total;
		for(size_t i = 0; i < ret.size(); ++i) {
			if(x[i].mode != GridInfo::FILL)
				continue;
			size_t take = left / fills;
			ret[i] += take;
			left -= take;
			fills--;
		}
	}
	return ret;
}

Point Grid::actualSpacing() const {
	return Point(columns.empty() ? 0 : (columns.size()-1) * spacing, rows.empty() ? 0 : (rows.size()-1)*spacing);
}

void Grid::layout() {
	auto size = getClientSize();
	auto children = getChildren<Control>();

	// Make sure we have WidgetInfo's for every child...
	std::for_each(children.first, children.second, [this](Control* w) { getWidgetInfo(w); });

	Point as = actualSpacing();

	if(as.x < size.x && as.y < size.y)
		size -= as;

	std::vector<size_t> rowSize = calcSizes(rows, columns, size.y, true);
	std::vector<size_t> colSize = calcSizes(columns, rows, size.x, false);

	util::HoldResize hr(this, boost::distance(children));
	for(auto i = children.first; i != children.second; ++i) {
		WidgetInfo* wi = getWidgetInfo(*i);
		if(!wi || wi->noResize) {
			continue;
		}

		size_t r = wi->row;
		size_t rs = wi->rowSpan;

		size_t c = wi->column;
		size_t cs = wi->colSpan;

		if(r + rs > rowSize.size() || c + cs > colSize.size()) {
			continue;
		}

		size_t x = std::accumulate(colSize.begin(), colSize.begin() + c, 0);
		x += c * spacing;

		size_t y = std::accumulate(rowSize.begin(), rowSize.begin() + r, 0);
		y += r * spacing;

		size_t w = std::accumulate(colSize.begin() + c, colSize.begin() + c + cs, 0);
		w += (cs-1)*spacing;

		size_t h = std::accumulate(rowSize.begin() + r, rowSize.begin() + r + rs, 0);
		h += (rs-1)*spacing;

		Point ps = wi->w->getPreferredSize();

		switch(columns[wi->column].align) {
		case GridInfo::TOP_LEFT: w = ps.x; break;
		case GridInfo::BOTTOM_RIGHT: x += w - ps.x; w = ps.x; break;
		case GridInfo::CENTER: x += (w - ps.x) / 2; w = ps.x; break;
		case GridInfo::STRETCH: break; // Do nothing
		}

		switch(rows[wi->row].align) {
		case GridInfo::TOP_LEFT: h = ps.y; break;
		case GridInfo::BOTTOM_RIGHT: y += h - ps.y; h = ps.y; break;
		case GridInfo::CENTER: y += (h - ps.y) / 2; h = ps.y; break;
		case GridInfo::STRETCH: break; // Do nothing
		}

		hr.resize(wi->w, Rectangle(x, y, w, h));
	}
}

Grid::WidgetInfo* Grid::getWidgetInfo(Control* w) {
	for(auto& i: widgetInfo) {
		if(i.w == w) {
			return &i;
		}
	}

	bool taken = true;
	size_t pos = 0;

	while(taken) {
		taken = false;
		for(auto& i: widgetInfo) {
			size_t r = pos / columns.size();
			size_t c = pos % columns.size();
			if(i.inCell(r, c)) {
				pos++;
				taken = true;
				break;
			}
		}
	}

	size_t r = pos / columns.size();
	size_t c = pos % columns.size();

	if(r >= rows.size()) {
		return 0;
	}

	widgetInfo.emplace_back(w, r, c, 1, 1);
	return &widgetInfo.back();
}

size_t Grid::addRow(const GridInfo& gp) {
	rows.push_back(gp);
	return rows.size() - 1;
}

size_t Grid::addColumn(const GridInfo& gp) {
	columns.push_back(gp);
	return columns.size() - 1;
}

void Grid::removeRow(Control* w) {
	for(auto& i: widgetInfo) {
		if(i.w == w) {
			removeRow(i.row);
			break;
		}
	}
}

void Grid::removeColumn(Control* w) {
	for(auto& i: widgetInfo) {
		if(i.w == w) {
			removeColumn(i.column);
			break;
		}
	}
}

void Grid::removeRow(size_t row) {
	rows.erase(rows.begin() + row);

	for(auto i = widgetInfo.begin(); i != widgetInfo.end();) {

		if(i->row == row) {
			auto w = i->w;
			i = widgetInfo.erase(i);
			w->close();

		} else {
			if(i->row > row) {
				--i->row;
			}

			++i;
		}
	}
}

void Grid::removeColumn(size_t column) {
	columns.erase(columns.begin() + column);

	for(auto i = widgetInfo.begin(); i != widgetInfo.end();) {

		if(i->column == column) {
			auto w = i->w;
			i = widgetInfo.erase(i);
			w->close();

		} else {
			if(i->column > column) {
				--i->column;
			}

			++i;
		}
	}
}

void Grid::clearRows() {
	rows.clear();
}

void Grid::clearColumns() {
	columns.clear();
}

void Grid::setWidget(Control* w, size_t row, size_t column, size_t rowSpan, size_t colSpan) {
	dwtassert(w->getParent() == this, "Control must be a child of the grid");

	for(auto& i: widgetInfo) {
		if(i.w == w) {
			i.row = row;
			i.column = column;
			i.rowSpan = rowSpan;
			i.colSpan = colSpan;
			return;
		}
	}

	widgetInfo.push_back(WidgetInfo(w, row, column, rowSpan, colSpan));
}

void Grid::setWidget(Control* w) {
	for(auto& i: widgetInfo) {
		if(i.w == w) {
			i.noResize = true;
			return;
		}
	}

	widgetInfo.push_back(WidgetInfo(w));
}

void Grid::handleEnabled(bool enabled) {
	// TODO find better way of keeping track of children
	for(HWND wnd = ::FindWindowEx(handle(), NULL, NULL, NULL); wnd; wnd = ::FindWindowEx(handle(), wnd, NULL, NULL)) {
		::EnableWindow(wnd, enabled ? TRUE : FALSE);
	}
}

bool Grid::handleMessage(const MSG &msg, LRESULT &retVal) {
	if(msg.message == WM_PARENTNOTIFY) {
		if(LOWORD(msg.wParam) == WM_DESTROY) {
			auto wnd = (HWND)msg.lParam;
			if(::GetParent(wnd) == handle()) {
				for(auto i = widgetInfo.begin(); i != widgetInfo.end(); ++i) {
					if(i->w->handle() == wnd) {
						widgetInfo.erase(i);
						break;
					}
				}
			}
		}
	}

	return BaseType::handleMessage(msg, retVal);
}

}
