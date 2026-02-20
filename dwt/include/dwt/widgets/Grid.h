/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
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

#ifndef DWT_GRID_H
#define DWT_GRID_H

#include "Container.h"

#include <vector>

namespace dwt {

class GridInfo {
public:
	enum Modes {
		STATIC,
		FILL,
		AUTO
	};

	enum Align {
		TOP_LEFT,
		BOTTOM_RIGHT,
		CENTER,
		STRETCH
	};

	GridInfo(int size_ = 0, Modes mode_ = AUTO, Align align_ = TOP_LEFT) : size(size_), mode(mode_), align(align_) { }

	/** Width or height */
	size_t size;

	Modes mode;

	Align align;
};

/**
 * Lays out child items in a grid
 */
class Grid :
	public Container
{
	typedef Container BaseType;
public:
	/// Class type
	typedef Grid ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		size_t rows;
		size_t cols;

		/// Fills with default parameters
		Seed(size_t rows_ = 1, size_t cols_ = 1);
	};

	virtual void layout();

	size_t addRow(const GridInfo& gp = GridInfo(0, GridInfo::AUTO, GridInfo::CENTER));
	size_t addColumn(const GridInfo& gp = GridInfo(0, GridInfo::AUTO, GridInfo::STRETCH));

	/// Remove the row that contains the specified widget.
	void removeRow(Control* w);
	/// Remove the column that contains the specified widget.
	void removeColumn(Control* w);

	void removeRow(size_t row);
	void removeColumn(size_t column);

	void clearRows();
	void clearColumns();

	void setWidget(Control* w, size_t row, size_t column, size_t rowSpan = 1, size_t colSpan = 1);
	void setWidget(Control* w);

	size_t getSpacing() const { return spacing; }
	void setSpacing(size_t spacing_) { spacing = spacing_; }

	GridInfo& row(size_t i);
	GridInfo& column(size_t i);

	size_t rowCount() const { return rows.size(); }
	size_t columnCount() const { return columns.size(); }

	void create( const Seed & cs = Seed() );

	Point getPreferredSize();

	/// Returns true if handled, else false
	virtual bool handleMessage(const MSG &msg, LRESULT &retVal);

protected:
	friend class WidgetCreator<Grid>;

	explicit Grid( Widget * parent ) : BaseType(parent), spacing(3) { }

private:

	struct WidgetInfo {
		WidgetInfo(Control* w_, size_t row_, size_t column_, size_t rowSpan_, size_t colSpan_) :
			w(w_), row(row_), column(column_), rowSpan(rowSpan_), colSpan(colSpan_), noResize(false) { }
		WidgetInfo(Control* w_) :
			w(w_), noResize(true) { }

		Control* w;

		size_t row;
		size_t column;

		size_t rowSpan;
		size_t colSpan;

		bool noResize;

		/** Does the widget appear in a certain row/column? */
		bool inCell(size_t r, size_t c) const;
	};

	size_t spacing;

	typedef std::vector<GridInfo> GridInfoList;
	GridInfoList rows;
	GridInfoList columns;

	typedef std::vector<WidgetInfo> WidgetInfoList;
	WidgetInfoList widgetInfo;

	Point getPreferredSize(size_t row, size_t column) const;

	std::vector<size_t> calcSizes(const GridInfoList& x, const GridInfoList& y, size_t cur, bool row) const;

	Point actualSpacing() const;

	WidgetInfo* getWidgetInfo(Control* w);

	void handleEnabled(bool enabled);
};

}

#endif
