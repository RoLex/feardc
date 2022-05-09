/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

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

#ifndef DWT_TABLE_H
#define DWT_TABLE_H

#include "../Rectangle.h"
#include "../resources/ImageList.h"
#include "../aspects/Clickable.h"
#include "../aspects/Collection.h"
#include "../aspects/Columns.h"
#include "../aspects/CustomDraw.h"
#include "../aspects/Data.h"
#include "../aspects/Scrollable.h"
#include "../aspects/Selection.h"
#include "Control.h"
#include <dwt/Theme.h>

#include <utility>
#include <vector>

namespace dwt {

/// List View Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html list.PNG
  * Class for creating a List View or a "DataGrid" control Widget. <br>
  * A List View is a "datagrid" with one or more rows and one or more columns where
  * users can edit text inside each cell and respond to events. <br>
  * Note to get to actual rows there are two often interchanged ways of getting
  * there, one is the LPARAM value and the other is the physical rownumber of your
  * wanted row. Have this in mind when using the DataGrid since this is often a
  * source of error when you get unwanted behaviour. This means you often will have
  * to "map" an LPARAM value to a physical rownumber and vice versa.
  */
class Table :
	public CommonControl,
	public aspects::Clickable<Table>,
	public aspects::Collection<Table, int>,
	public aspects::Columns<Table>,
	public aspects::CustomDraw<Table, NMLVCUSTOMDRAW>,
	public aspects::Data<Table, int>,
	public aspects::Scrollable<Table>,
	public aspects::Selection<Table, int>
{
	typedef CommonControl BaseType;

	friend class WidgetCreator<Table>;
	friend class aspects::Collection<Table, int>;
	friend class aspects::Columns<Table>;
	friend class aspects::Data<Table, int>;
	friend class aspects::Selection<Table, int>;
	friend class aspects::Clickable<Table>;

	typedef std::function<int (LPARAM a, LPARAM b)> SortFunction;
	typedef std::function<void (int)> HeaderF;
	typedef std::function<tstring (int)> TooltipF;

public:
	/// Class type
	typedef Table ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		FontPtr font;

		/// List view extended styles (LVS_EX_*)
		DWORD lvStyle;

		/// Fills with default parameters
		Seed();
	};

	enum SortType {
		SORT_CALLBACK, /// Call a custom callback function to sort items.
 		SORT_STRING, /// Sort alphabetically, respecting international characters in the user's locale.
		SORT_STRING_SIMPLE, /// Sort alphabetically; ignore the user's locale.
		SORT_INT,
		SORT_FLOAT
	};

	/// \ingroup EventHandlersTable
	/// Event handler for the SortItems event
	/** When you sort a Table you need to supply a callback function for
	  * comparing items. <br>
	  * Otherwise the grid won't know how it should sort items. <br>
	  * Some items might be compared after their integer values while other will
	  * maybe be compared after their order in the alphabet etc... <br>
	  * The event handler you supply for the SortItems event should return - 1 if the
	  * first row is supposed to be before the second and 1 if the second is supposed
	  * to be before the first, if items are equal it should return 0 <br>
	  * The two first arguments in your event handler are the LPARAM arguments
	  * supplied when inserting items for the first and second items. <br>
	  * This function will be called MANY times when you sort a grid so you probably
	  * will NOT want to run a costly operation within this event handler.
	  */
	void onSortItems(SortFunction f);

	/// \ingroup EventHandlersTable
	/// Event Handler for the Column Header Click event
	/** This Event is raised whenever one of the headers is clicked, it is useful to
	  * e.g. sort the Table according to which header is being clicked. <br>
	  * Parameters passed is int which defines which header from left to right ( zero
	  * indexed ) is being clicked!
	  */
	void onColumnClick(HeaderF f);

	/// Sorts the list
	/** Call this function to sort the list, it's IMPERATIVE that you before calling
	  * this function defines an event handler for the SortItems event. <br>
	  * Otherwise you will get an exception when calling this function since it
	  * expects to have a compare function to compare items with which you can define
	  * in the onSortItems Event Handler setter
	  */
	void resort();

	void setSort(int aColumn, SortType aType, bool aAscending = true);

	bool isAscending() const;
	int getSortColumn() const;
	SortType getSortType() const;

	/// Returns the text of the given cell
	/** The column is which column you wish to retrieve the text for. <br>
	  * The row is which row in that column you wish to retrieve the text for. <br>
	  * Note this one returns the text of the "logical" column which means that if
	  * you have moved a column ( e.g. column no 3 ) and you're trying to retrieve
	  * the text of column no. 3 it will still return the text of the OLD column no.
	  * 3 which now might be moved to the beginning of the grid, NOT the NEW column
	  * no. 3 <br>
	  * The above does NOT apply for ROWS, meaning it will get the text of the NEW
	  * row if row is moved due to a sort e.g.
	  */
	tstring getText( unsigned int row, unsigned int column );

	/// Sets the text of the given cell
	/** Sets a new string value for a given cell.
	  */
	void setText( unsigned row, unsigned column, const tstring & newVal );

	/// Returns a vector containing all the selected rows of the grid.
	/** The return vector contains unsigned integer values, each value defines a row
	  * in the grid that is selected. <br>
	  * If the grid is in "single selection mode" you should rather use the
	  * getSelectedRow function. <br>
	  * If grid does NOT have any selected items the return vector is empty.
	  */
	std::vector< unsigned > getSelection() const;

	/// Clears the selection of the grid.
	void clearSelection();

	/// Change the current icon of an item
	/** Sets a new icon for a given item
	  */
	void setIcon(unsigned row, unsigned column, int newIconIndex);

	/// Returns a boolean indicating if the Grid is in "read only" mode or not
	/** If the return value is true the Grid is in "read only" mode and cannot be
	  * updated ( except programmatically ) <br>
	  * If the return value is false the Grid is "updateable" through double clicking
	  * a cell. <br>
	  * < b >Note! <br>
	  * The "read only" property is pointless if you have defined your own validation
	  * function through calling "beenValidate"< /b >
	  */
	bool getReadOnly();

	/// Sets the "read only" property of the Grid
	/** If passed true the Grid becomes "Read Only" and you cannot change its values
	  * ( except programmatically ) <br>
	  * If passed false the Grid becomes "updateable". <br>
	  * < b >Note! <br>
	  * The "read only" property is pointless if you have defined your own validation
	  * event handler by calling "beenValidate"< /b >
	  */
	void setReadOnly( bool value = true );

	/** Enable group support, and insert each given group. The group id used as the "index" param
	of the insert function will be the position of the group in the vector given here.
	Once a table has been set into grouped mode, it cannot be switched back to non-grouped mode. */
	void setGroups(const std::vector<tstring>& groups);

	bool isGrouped() const { return grouped; }

	/** Get the rectangle of the specified group's header. Only available on >= Vista. */
	bool getGroupRect(unsigned groupId, Rectangle& rect) const;

	/// Returns the checked state of the given row
	/** A list view can have checkboxes in each row, if the checkbox for the given
	  * row is CHECKED this funtion returns true.
	  */
	bool isChecked( unsigned row );

	/// Sets the checked state of the given row
	/** Every row in a List View can have its own checkbox column.< br >
	  * If this checkbox is selected in the queried row this function will return true.
	  */
	void setChecked( unsigned row, bool value = true );

	/// Sets (or removes) full row select.
	/** Full row select means that when a user press any place in a row the whole row
	  * will be selected instead of the activated cell. <br>
	  * value defines if we're supposed to set the fullrow select property of the
	  * grid or not. <br>
	  * If omitted, parameter defaults to true.
	  */
	void setFullRowSelect( bool value = true );

	/// Sets (or removes) the checkbox style.
	/** If you add this style your List View will get checkboxes in the leftmost
	  * column. <br>
	  * This can be useful for retrieving "selected" properties on items within the
	  * List View. <br>
	  * Note! <br>
	  * You can't set a column to be a "pure" checkbox column, but you CAN avoid to
	  * put text into the column by inserting an empty ( "" ) string.
	  */
	void setCheckBoxes( bool value = true );

	/// Sets (or removes) single row select.
	/** Single row select means that only ONE row can be selected at a time. <br>
	  * Value is parameter value defines if we're supposed to set the single row
	  * select property of the grid or not. <br>
	  * If omitted, parameter defaults to true. <br>
	  * Related functions are getSelectedRow ( single row selection mode ) or
	  * getSelected ( multiple row selection mode )
	  */
	void setSingleRowSelection( bool value = true );

	void setTooltips(TooltipF f);

	/// Adds (or removes) grid lines.
	/** A grid with grid lines will have lines surrounding every cell in it. <br>
	  * value defines if we're supposed to add grid lines or remove them. <br>
	  * If omitted, parameter defaults to true.
	  */
	void setGridLines( bool value = true );

#ifndef WINCE
	/// Adds (or removes) the hoover style.
	/** A grid with hoover style will "flash" the text color in the line the cursor
	  * is above. <br>
	  * Value is parameter value defines if we're supposed to add hoover support or
	  * remove it. <br>
	  * If omitted, parameter defaults to true.
	  */
	void setHover( bool value = true );
#endif

	/// Adds (or removes) the header drag drop style.
	/** A grid with header drag drop style will have the possibility for a user to
	  * actually click and hold a column header and "drag" that column to another
	  * place in the column hierarchy. <br>
	  * Value is parameter value defines if we're supposed to add the header drag
	  * drop style or remove it. <br>
	  * If omitted, parameter defaults to true.
	  */
	void setHeaderDragDrop( bool value = true );

	/// Adds (or removes) the always show selection style.
	/** A grid with this style set will always show its selection, even if
	  * it does not have the focus.
	  * If omitted, parameter defaults to true.
	  */
	void setAlwaysShowSelection( bool value = true );

	/**
	* Inserts a row into the grid. Call createColumns before inserting items.
	* @param row vector containing all the cells of the row. This vector must (of course) be the
	* same size as the number of columns in the grid.
	* @param index defines at which index the new row will be inserted at, or the group identifier
	* if in grouped mode (see setGroups and isGrouped). -1 means "at the bottom". must be >= 0 when
	* in grouped mode.
	* @param iconIndex defines the index of the icon on the image list that will be shown on the row.
	*/
	int insert(const std::vector<tstring>& row, LPARAM lPar = 0, int index = - 1, int iconIndex = - 1);

	/// Reserves a number of items to the list
	/** To be used in combination with the "onGetItem" event <br>
	  * This function reserves a number of items to the list so that the control
	  * knows how many items <br>
	  * it need to give room for. <br>
	  * Has absolutely no meaning unless used in combination with the
	  * insertCallbackRow and the "onGetItem" event in which the application is
	  * queried for items when the control needs to get data to display.
	  */
	void resize( unsigned size );
	using BaseType::resize;
	/// Set the normal image list for the Data Grid.
	/** normalImageList is the image list that contains the images
	  * for the data grid icons in Icon View (big icons).
	  */
	void setNormalImageList(ImageListPtr imageList);

	/// Set the small image list for the Data Grid.
	/** smallImageList is the image list that contains the images
	  * for the data grid icons in Report, List & Small Icon Views.
	  */
	void setSmallImageList(ImageListPtr imageList);

	/// Set the state image list for the Data Grid.
	/** stateImageList is the image list that contains the images
	  * for the data grid icons states.
	  */
	void setStateImageList(ImageListPtr imageList);

	/** Set the image list to find icons from when adding groups. Only available on >= Vista. */
	void setGroupImageList(ImageListPtr imageList);

	/// Change the view for the Data Grid.
	/** The view parameter can be one of LVS_ICON, LVS_SMALLICON, LVS_LIST or
	  * LVS_REPORT. <br>
	  * The Data Grid uses the report view for default.
	  */
	void setView( int view );

	/// Force redraw of a range of items.
	/** You may want to call invalidateWidget after the this call to force repaint.
	  */
	void redraw( int firstRow = 0, int lastRow = -1 );

	void setTableStyle(int style);

	int insert(unsigned mask, int i, LPCTSTR text, unsigned state, unsigned stateMask, int image, LPARAM lparam);

	int getNext(int i, int type) const;

    int find(const tstring& b, int start = -1, bool aPartial = false);

	void scroll(int x, int y);

	/// obsolete
	inline void select(int i) { setSelected(i); }

	void selectAll();
	void checkSel();

	ScreenCoordinate getContextMenuPos();

	void ensureVisible(int i, bool partial = false);

	std::pair<int, int> hitTest(const ScreenCoordinate& pt);

	/// Returns the rect for the item per code (wraps ListView_GetItemRect)
	Rectangle getRect(int row, int code);

	/// Returns the rect for the subitem item per code (wraps ListView_GetSubItemRect)
	Rectangle getRect(int row, int col, int code);

	/// Actually creates the Data Grid Control
	/** You should call WidgetFactory::createTable if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create( const Seed & cs = Seed() );

	// Constructor Taking pointer to parent
	explicit Table(dwt::Widget* parent);

protected:
	/// Adds or Removes extended list view styles from the list view
	void addRemoveTableExtendedStyle( DWORD addStyle, bool add );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Table() {
	}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	bool grouped;

	Theme theme;

	// Edit row index and Edit column index, only used when grid is in "edit mode"
	int itsEditRow;
	int itsEditColumn;

	unsigned itsXMousePosition;
	unsigned itsYMousePosition;

	ImageListPtr itsNormalImageList;
	ImageListPtr itsSmallImageList;
	ImageListPtr itsStateImageList;
	ImageListPtr groupImageList;

	// If true the grid is in "read only mode" meaning that cell values cannot be edited.
	// A simpler version of defining a beenValidate always returning false
	bool isReadOnly;
	bool itsEditingCurrently; // Inbetween BEGIN and END EDIT

	int sortColumn;
	SortType sortType;
	bool ascending;
	SortFunction fun;

	static int CALLBACK compareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );
	static int CALLBACK compareFuncCallback( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

	void setIndex(LVITEM& item, int index) const;
	void initGroupSupport();
	void updateArrow();

	// aspects::Data
	int findDataImpl(LPARAM data, int start = -1);
	LPARAM getDataImpl(int idx);
	void setDataImpl(int i, LPARAM data);

	// aspects::Collection
	void eraseImpl( int row );
	void clearImpl();
	size_t sizeImpl() const;

	// aspects::Colorable
	void setColorImpl(COLORREF text, COLORREF background);

	// aspects::Selection
	int getSelectedImpl() const;
	void setSelectedImpl( int idx );
	size_t countSelectedImpl() const;
	static Message getSelectionChangedMessage();

	// aspects::Clickable
	static Message getClickMessage();
	static Message getRightClickMessage();
	static Message getDblClickMessage();

	// aspects::Columns
	int insertColumnImpl(const Column& column, int after);
	void eraseColumnImpl(unsigned column);
	unsigned getColumnCountImpl() const;
	std::vector<Column> getColumnsImpl() const;
	Column getColumnImpl(unsigned column) const;
	std::vector<int> getColumnOrderImpl() const;
	void setColumnOrderImpl(const std::vector<int>& columns);
	std::vector<int> getColumnWidthsImpl() const;
	void setColumnWidthImpl( unsigned column, int width );
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline Message Table::getSelectionChangedMessage() {
	return Message(WM_NOTIFY, LVN_ITEMCHANGED);
}

inline Message Table::getClickMessage() {
	return Message(WM_NOTIFY, NM_CLICK);
}

inline Message Table::getRightClickMessage() {
	return Message(WM_NOTIFY, NM_RCLICK);
}

inline Message Table::getDblClickMessage() {
	return Message(WM_NOTIFY, NM_DBLCLK);
}

inline void Table::resort() {
	if(sortColumn != -1) {
		if(sortType == SORT_CALLBACK) {
			ListView_SortItems(handle(), &Table::compareFuncCallback, reinterpret_cast<LPARAM>(this));
		} else {
			// Wine 0.9.48 doesn't support this
			ListView_SortItemsEx(handle(), &Table::compareFunc, reinterpret_cast< LPARAM >(this));
		}
	}
}

inline int Table::getSelectedImpl() const {
	return getNext(-1, LVNI_SELECTED);
}

inline size_t Table::countSelectedImpl() const {
	return static_cast<size_t>(ListView_GetSelectedCount( handle() ));
}

inline void Table::setText( unsigned row, unsigned column, const tstring & newVal ) {
	ListView_SetItemText( handle(), row, column, const_cast < TCHAR * >( newVal.c_str() ) );
}

inline bool Table::getReadOnly() {
	return isReadOnly;
}

inline void Table::setReadOnly( bool value ) {
	isReadOnly = value;
	this->Widget::addRemoveStyle( LVS_EDITLABELS, !value );
}

inline bool Table::isChecked( unsigned row ) {
	return ListView_GetCheckState( handle(), row ) == TRUE;
}

inline void Table::setChecked( unsigned row, bool value ) {
	ListView_SetCheckState( handle(), row, value );
}

inline void Table::setFullRowSelect( bool value ) {
	addRemoveTableExtendedStyle( LVS_EX_FULLROWSELECT, value );
}

inline void Table::resize( unsigned size ) {
	ListView_SetItemCount( handle(), size );
}

inline void Table::setCheckBoxes( bool value ) {
	addRemoveTableExtendedStyle( LVS_EX_CHECKBOXES, value );
}

inline void Table::setSingleRowSelection( bool value ) {
	this->Widget::addRemoveStyle( LVS_SINGLESEL, value );
}

inline void Table::setGridLines( bool value ) {
	addRemoveTableExtendedStyle( LVS_EX_GRIDLINES, value );
}

inline void Table::onSortItems(SortFunction f) {
	fun = f;
}

#ifndef WINCE

inline void Table::setHover( bool value ) {
	addRemoveTableExtendedStyle( LVS_EX_TWOCLICKACTIVATE, value );
}
#endif

inline void Table::setHeaderDragDrop( bool value ) {
	addRemoveTableExtendedStyle( LVS_EX_HEADERDRAGDROP, value );
}

inline void Table::setAlwaysShowSelection( bool value ) {
	this->Widget::addRemoveStyle( LVS_SHOWSELALWAYS, value );
}

inline void Table::eraseImpl( int row ) {
	ListView_DeleteItem( handle(), row );
}

inline size_t Table::sizeImpl() const {
	return ListView_GetItemCount( handle() );
}

inline bool Table::isAscending() const {
	return ascending;
}

inline int Table::getSortColumn() const {
	return sortColumn;
}

inline Table::SortType Table::getSortType() const {
	return sortType;
}

inline void Table::setColumnOrderImpl(const std::vector<int>& columns) {
	sendMessage(LVM_SETCOLUMNORDERARRAY, static_cast<WPARAM>(columns.size()), reinterpret_cast<LPARAM>(&columns[0]));
}

inline void Table::setTableStyle(int style) {
	ListView_SetExtendedListViewStyle(handle(), style);
}

inline int Table::getNext(int i, int type) const {
	return ListView_GetNextItem(handle(), i, type);
}

inline int Table::find(const tstring& b, int start, bool aPartial) {
    LVFINDINFO fi = { static_cast<UINT>(aPartial ? LVFI_PARTIAL : LVFI_STRING), b.c_str() };
    return ListView_FindItem(handle(), start, &fi);
}

inline int Table::findDataImpl(LPARAM data, int start) {
    LVFINDINFO fi = { LVFI_PARAM, NULL, data };
    return ListView_FindItem(handle(), start, &fi);
}

inline void Table::ensureVisible(int i, bool partial) {
	ListView_EnsureVisible(handle(), i, false);
}

inline void Table::setColorImpl(COLORREF text, COLORREF background) {
	ListView_SetTextColor(handle(), text);
	ListView_SetTextBkColor(handle(), background);
	ListView_SetBkColor(handle(), background);
}

}

#endif
