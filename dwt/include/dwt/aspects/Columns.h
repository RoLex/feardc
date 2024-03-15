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

#ifndef DWT_ASPECTCOLUMNS_H_
#define DWT_ASPECTCOLUMNS_H_

#include <vector>

#include <boost/range/algorithm/for_each.hpp>

#include "../forward.h"
#include "../widgets/Column.h"

namespace dwt { namespace aspects {

/** Aspect for widgets that display data in columns.
 *
 * Columns are zero-indexed.
 */
template<typename WidgetType>
class Columns {
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

public:
	/** Add a column at the end of the current list of columns */
	void addColumn(const tstring& header = tstring(), int width = -1, Column::Alignment alignement = Column::LEFT);

	/** Add a column at the end of the current list of columns */
	void addColumn(const Column& column);

	/** Insert a column after the specified column.
	 *
	 * @return The index of the inserted column
	 */
	int insertColumn(const Column& column, int after);

	/** Reset the column list to the givent columns */
	void setColumns(const std::vector<Column>& columns);

	/** Remove the specified column.
	 * Any columns after the specified columns will be shifted back.
	 */
	void eraseColumn(unsigned column);

	/** Returns the current number of columns  */
	unsigned getColumnCount() const;

	/** Returns information about all columns */
	std::vector<Column> getColumns() const;

	/** Return column information about the given column */
	Column getColumn(unsigned column) const;

	/** Return the current column order.
	 * The returned vector will have the same number of elements as there are currently columns.
	 */
	std::vector<int> getColumnOrder() const;

	/** Update the column order.
	 *
	 * @param columns The new column order - the vector must have as many elements as there are columns
	 */
	void setColumnOrder(const std::vector<int>& columns);

	/** Return all column widths */
	std::vector<int> getColumnWidths() const;

	/** Update all column widths.
	 *
	 * @param widths The new column widths - the vector must have as many elements as there are columns
	 */
	void setColumnWidths(const std::vector<int>& widths);

	/** Set the width of the specified column
	 *
	 * @param column Column index
	 * @param width Width in pixels, or one of the constants in Column::AutoWidth
	 */
	void setColumnWidth( unsigned column, int width );
};

template<typename WidgetType>
inline void Columns<WidgetType>::addColumn(const tstring& header, int width, Column::Alignment alignment) {
	addColumn(Column(header, width, alignment));
}

template<typename WidgetType>
inline void Columns<WidgetType>::addColumn(const Column& column) {
	insertColumn(column, getColumnCount());
}

template<typename WidgetType>
inline int Columns<WidgetType>::insertColumn(const Column& column, int after) {
	return W().insertColumnImpl(column, after);
}

template<typename WidgetType>
inline void Columns<WidgetType>::setColumns(const std::vector<Column>& columns) {
	/** @todo "auto" doesn't pass here; strange error on GCC 6.2:
	 * "inconsistent deduction for 'auto': 'unsigned int' and then 'auto'" */
	for(decltype(getColumnCount()) i = 0, iend = getColumnCount(); i < iend; ++i) eraseColumn(i);
	for(decltype(columns.size()) i = 0, iend = columns.size(); i < iend; ++i) insertColumn(columns[i], i);
}

template<typename WidgetType>
inline void Columns<WidgetType>::eraseColumn(unsigned column) {
	W().eraseColumnImpl(column);
}

template<typename WidgetType>
inline unsigned Columns<WidgetType>::getColumnCount() const  {
	return W().getColumnCountImpl();
}

template<typename WidgetType>
inline std::vector<Column> Columns<WidgetType>::getColumns() const {
	return W().getColumnsImpl();
}

template<typename WidgetType>
inline Column Columns<WidgetType>::getColumn(unsigned column) const {
	return W().getColumnImpl(column);
}

template<typename WidgetType>
inline std::vector<int> Columns<WidgetType>::getColumnOrder() const {
	return W().getColumnOrderImpl();
}

template<typename WidgetType>
inline void Columns<WidgetType>::setColumnOrder(const std::vector<int>& columns) {
	W().setColumnOrderImpl(columns);
}

template<typename WidgetType>
inline std::vector<int> Columns<WidgetType>::getColumnWidths() const {
	return W().getColumnWidthsImpl();
}

template<typename WidgetType>
inline void Columns<WidgetType>::setColumnWidths(const std::vector<int>& widths) {
	for(size_t i = 0, iend = widths.size(); i < iend; ++i) setColumnWidth(i, widths[i]);
}

template<typename WidgetType>
inline void Columns<WidgetType>::setColumnWidth( unsigned column, int width ) {
	W().setColumnWidthImpl(column, width);
}

} }

#endif /* DWT_ASPECTCHILD_H_ */
