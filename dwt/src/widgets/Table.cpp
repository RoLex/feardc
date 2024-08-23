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

#include <dwt/widgets/Table.h>

#include <cmath>

#include <boost/scoped_array.hpp>
#include <boost/range/algorithm/for_each.hpp>

#include <vsstyle.h>
#include <vssym32.h>

#include <dwt/CanvasClasses.h>
#include <dwt/util/check.h>
#include <dwt/util/StringUtils.h>
#include <dwt/DWTException.h>
#include <dwt/WidgetCreator.h>
#include <dwt/widgets/ToolTip.h>

namespace dwt {

using boost::range::for_each;

const TCHAR Table::windowClass[] = WC_LISTVIEW;

Table::Seed::Seed() :
	BaseType::Seed(WS_CHILD | WS_TABSTOP | LVS_REPORT),
	font(0),
	lvStyle(LVS_EX_DOUBLEBUFFER)
{
}

void Table::create( const Seed & cs )
{
	/* handle Ctrl+A to select every item. */
	if((cs.style & LVS_SINGLESEL) != LVS_SINGLESEL) {
		addAccel(FCONTROL, 'A', [this] { selectAll(); });
	}

	/* handle the space bar on multi-selection tables with check-boxes (single-sel ones already handle
	it). */
	if((cs.lvStyle & LVS_EX_CHECKBOXES) == LVS_EX_CHECKBOXES && (cs.style & LVS_SINGLESEL) != LVS_SINGLESEL) {
		addAccel(0, VK_SPACE, [this] { checkSel(); });
	}

	BaseType::create(cs);
	setFont(cs.font);
	if(cs.lvStyle != 0)
		setTableStyle(cs.lvStyle);
}

Table::Table(dwt::Widget* parent) :
BaseType(parent, ChainingDispatcher::superClass<Table>()),
grouped(false),
itsEditRow(0),
itsEditColumn(0),
itsXMousePosition(0),
itsYMousePosition(0),
isReadOnly(false),
itsEditingCurrently(false),
sortColumn(-1),
sortType(SORT_CALLBACK),
ascending(true)
{
}

void Table::setSort(int aColumn, SortType aType, bool aAscending) {
	bool doUpdateArrow = (aColumn != sortColumn || aAscending != ascending);

	sortColumn = aColumn;
	sortType = aType;
	ascending = aAscending;

	resort();
	if (doUpdateArrow)
		updateArrow();
}

void Table::updateArrow() {
	int flag = isAscending() ? HDF_SORTUP : HDF_SORTDOWN;
	HWND header = ListView_GetHeader(handle());
	int count = Header_GetItemCount(header);
	for (int i=0; i < count; ++i)
	{
		HDITEM item;
		item.mask = HDI_FORMAT;
		Header_GetItem(header, i, &item);
		item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
		if (i == getSortColumn())
			item.fmt |= flag;
		Header_SetItem(header, i, &item);
	}
}

void Table::scroll(int x, int y) {
	ListView_Scroll(handle(), x, y);
}

void Table::setSelectedImpl(int item) {
	ListView_SetItemState(handle(), item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

void Table::selectAll() {
	for(size_t i = 0, n = size(); i < n; ++i)
		setSelected(i);
}

void Table::checkSel() {
	/* check every row, unless they are all already checked; in that case, uncheck them. */
	auto allChecked = true;
	auto sel = getSelection();
	for(auto i: sel) {
		if(!isChecked(i)) {
			allChecked = false;
			break;
		}
	}
	for(auto i: sel) {
		setChecked(i, !allChecked);
	}
}

void Table::clearSelection() {
	int i = -1;
	while((i = getNext(i, LVNI_SELECTED)) != -1) {
		ListView_SetItemState(handle(), i, 0, LVIS_SELECTED | LVIS_FOCUSED);
	}
	while((i = getNext(i, LVNI_FOCUSED)) != -1) {
		ListView_SetItemState(handle(), i, 0, LVIS_FOCUSED);
	}
}

int Table::insertColumnImpl(const Column& column, int n) {
	LVCOLUMN lvColumn = { LVCF_FMT };

	lvColumn.fmt |= column.alignment == Column::LEFT ? LVCFMT_LEFT : column.alignment == Column::CENTER ? LVCFMT_CENTER : LVCFMT_RIGHT;

	if(!column.header.empty()) {
		lvColumn.mask |= LVCF_TEXT,
		lvColumn.pszText = const_cast < TCHAR * >( column.header.c_str() );
	}

	if(column.width >= 0) {
		lvColumn.mask |= LVCF_WIDTH;
		lvColumn.cx = column.width;
	}

	auto ret = ListView_InsertColumn( handle(), n, &lvColumn);
	if ( ret == - 1 ) {
		throw Win32Exception("Error while trying to create column in list view" );
	}

	return ret;
}

int Table::insert(const std::vector<tstring>& row, LPARAM lPar, int index, int iconIndex) {
	LVITEM lvi = { LVIF_TEXT | LVIF_PARAM };

	lvi.pszText = const_cast<LPTSTR>(row[0].c_str());

	if(itsNormalImageList || itsSmallImageList) {
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = (iconIndex == -1 ? I_IMAGECALLBACK : iconIndex);
	}

	lvi.lParam = lPar;

	setIndex(lvi, index);

	int ret = ListView_InsertItem(handle(), &lvi);
	if(ret == - 1) {
		throw Win32Exception("Error while trying to insert row in Table");
	}

	// now insert sub-items (for columns)
	lvi.mask = LVIF_TEXT | LVIF_IMAGE;
	lvi.iSubItem = 1;
	std::for_each(row.cbegin() + 1, row.cend(), [this, &lvi](const tstring& text) {
		lvi.pszText = const_cast<LPTSTR>(text.c_str());
		lvi.cchTextMax = static_cast<int>(text.size());
		if(!ListView_SetItem(handle(), &lvi)) {
			throw Win32Exception("Error while trying to insert row sub-item in Table");
		}
		lvi.iSubItem++;
	});

	return ret;
}

int Table::insert(unsigned mask, int i, LPCTSTR text, unsigned state, unsigned stateMask, int image, LPARAM lparam) {
	LVITEM item = { mask };
	item.state = state;
	item.stateMask = stateMask;
	item.pszText = const_cast<LPTSTR>(text);
	item.iImage = image;
	item.lParam = lparam;
	setIndex(item, i);
	return ListView_InsertItem(handle(), &item);
}

void Table::setIndex(LVITEM& item, int index) const {
	if(grouped) {
		dwtassert(index >= 0, "Table::insert in grouped mode: index must be >= 0 since it is a group id");
		item.mask |= LVIF_GROUPID;
		item.iGroupId = index;

	} else {
		item.iItem = (index == -1) ? size() : index;
	}
}

void Table::onColumnClick(HeaderF f) {
	addCallback(Message(WM_NOTIFY, LVN_COLUMNCLICK), [f](const MSG& msg, LRESULT&) -> bool {
		f(reinterpret_cast<LPNMLISTVIEW>(msg.lParam)->iSubItem);
		return true;
	});
}

ScreenCoordinate Table::getContextMenuPos() {
	int pos = getNext(-1, LVNI_SELECTED | LVNI_FOCUSED);
	POINT pt = { 0 };
	if(pos >= 0) {
		RECT lrc = getRect(pos, LVIR_LABEL);
		pt.x = lrc.left;
		pt.y = lrc.top + ((lrc.bottom - lrc.top) / 2);
	}
	return ClientCoordinate(pt, this);
}

tstring Table::getText( unsigned int row, unsigned int column )
{
	// TODO: Get string length first?
	const int BUFFER_MAX = 2048;
	TCHAR buffer[BUFFER_MAX + 1];
	buffer[0] = '\0';
	ListView_GetItemText(handle(), row, column, buffer, BUFFER_MAX);
	return buffer;
}

std::vector< unsigned > Table::getSelection() const
{
	std::vector< unsigned > retVal;
	int tmpIdx = - 1;
	while ( true )
	{
		tmpIdx = ListView_GetNextItem( handle(), tmpIdx, LVNI_SELECTED );
		if ( tmpIdx == - 1 )
			break;
		retVal.push_back( static_cast< unsigned >( tmpIdx ) );
	}
	return retVal;
}

unsigned Table::getColumnCountImpl() const {
	HWND header = ListView_GetHeader(handle());
	return Header_GetItemCount(header);
}

void Table::addRemoveTableExtendedStyle( DWORD addStyle, bool add ) {
	DWORD newStyle = ListView_GetExtendedListViewStyle( handle() );
	if ( add && ( newStyle & addStyle ) != addStyle )
	{
		newStyle |= addStyle;
	}
	else if ( !add && ( newStyle & addStyle ) == addStyle )
	{
		newStyle ^= addStyle;
	}
	ListView_SetExtendedListViewStyle( handle(), newStyle );
}

std::vector<int> Table::getColumnOrderImpl() const {
	std::vector<int> ret(getColumnCount());
	if(!::SendMessage(handle(), LVM_GETCOLUMNORDERARRAY, static_cast<WPARAM>(ret.size()), reinterpret_cast<LPARAM>(&ret[0]))) {
		ret.clear();
	}
	return ret;
}

std::vector<int> Table::getColumnWidthsImpl() const {
	std::vector<int> ret(getColumnCount());
	for(size_t i = 0; i < ret.size(); ++i) {
		ret[i] = ::SendMessage(handle(), LVM_GETCOLUMNWIDTH, static_cast<WPARAM>(i), 0);
	}
	return ret;
}

void Table::setGroups(const std::vector<tstring>& groups) {
	bool wasGrouped = grouped;

	// must be called every time on XP
	grouped = ListView_EnableGroupView(handle(), TRUE) >= 0;
	if(!grouped)
		return;

	if(!wasGrouped) {
		initGroupSupport();
	}

	LVGROUP group = { sizeof(LVGROUP) };
	for(auto& i: groups) {
		if(i.empty()) {
			group.mask = LVGF_GROUPID;
			group.pszHeader = 0;
		} else {
			group.mask = LVGF_GROUPID | LVGF_HEADER;
			group.pszHeader = const_cast<LPWSTR>(i.c_str());
		}
		if(groupImageList) {
			group.mask |= LVGF_TITLEIMAGE;
			group.iTitleImage = group.iGroupId;
		}
		if(ListView_InsertGroup(handle(), -1, &group) == -1) {
			throw DWTException("Group insertion failed in Table::setGroups");
		}
		group.iGroupId++;
	}

	grouped = true;
}

bool Table::getGroupRect(unsigned groupId, Rectangle& rect) const {
	::RECT rc;
	if(ListView_GetGroupRect(handle(), groupId, LVGGR_HEADER, &rc)) {
		rect = Rectangle(rc);
		return true;
	}
	return false;
}

void Table::initGroupSupport() {
	// add some spacing between groups.
	LVGROUPMETRICS metrics = { sizeof(LVGROUPMETRICS), LVGMF_BORDERSIZE };
	ListView_GetGroupMetrics(handle(), &metrics);
	metrics.Bottom += std::max(metrics.Top, 12u);
	ListView_SetGroupMetrics(handle(), &metrics);

	/* fiddle with the painting of group headers to allow custom colors that match the background (the
	theme will be respected). */

	theme.load(VSCLASS_LISTVIEW, this);

	onCustomDraw([this](NMLVCUSTOMDRAW& data) -> LRESULT {
		if(data.dwItemType != LVCDI_GROUP)
			return CDRF_DODEFAULT;

		switch(data.nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			{
				// got a group! get the current theme text color and compare it to the bg.
				auto color = theme ? theme.getColor(LVP_GROUPHEADER, LVGH_OPEN, TMT_HEADING1TEXTCOLOR) : NaC;
				if(color == NaC)
					color = 0; // assume black
				auto bgColor = ListView_GetBkColor(handle());
				if(std::abs(GetRValue(color) + GetGValue(color) + GetBValue(color)
					- GetRValue(bgColor) - GetGValue(bgColor) - GetBValue(bgColor)) < 300)
				{
					/* the theme color and the bg color are too close to each other; start by
					filling the canvas with an invert of the bg, then invert the whole canvas after
					everything has been drawn (after CDDS_POSTPAINT). */
					FreeCanvas canvas { data.nmcd.hdc };
					Brush brush { 0xFFFFFF - bgColor };

					Rectangle rect;
					if(!getGroupRect(data.nmcd.dwItemSpec, rect))
						rect = Rectangle(data.rcText);

					LONG iconPos = 0;

					if(groupImageList) {
						// don't invert the icon. let's find out where it is placed...
						if(theme) {
							auto temp = rect;
							theme.formatRect(canvas, LVP_GROUPHEADER, LVGH_OPEN, temp);
							iconPos = temp.left() - rect.left();
						}
						if(iconPos <= 0)
							iconPos = 10; // assume a 10px margin for unthemed visual styles

						rect.size.x = iconPos;
						canvas.fill(rect, brush);

						rect.pos.x = rect.right() + 16;
						rect.size.x = data.rcText.right - rect.pos.x;
					}

					canvas.fill(rect, brush);

					// set a flag so we don't have to re-compare colors on CDDS_POSTPAINT.
					data.nmcd.lItemlParam = iconPos + 1;
				}
				break;
			}

		case CDDS_POSTPAINT:
			{
				if(data.nmcd.lItemlParam) {
					LONG iconPos = data.nmcd.lItemlParam - 1;

					FreeCanvas canvas { data.nmcd.hdc };

					Rectangle rect;
					if(!getGroupRect(data.nmcd.dwItemSpec, rect))
						rect = Rectangle(data.rcText);

					if(iconPos > 0) {
						rect.size.x = iconPos;
						canvas.invert(Region(rect));

						rect.pos.x = rect.right() + 16;
						rect.size.x = data.rcText.right - rect.pos.x;
					}

					canvas.invert(Region(rect));
				}
				break;
			}
		}
		return CDRF_DODEFAULT;
	});
}

LPARAM Table::getDataImpl(int idx) {
	LVITEM item = { LVIF_PARAM };
	item.iItem = idx;

	if(!ListView_GetItem(handle(), &item)) {
		return 0;
	}
	return item.lParam;
}

void Table::setDataImpl(int idx, LPARAM data) {
	LVITEM item = { LVIF_PARAM };
	item.iItem = idx;
	item.lParam = data;

	ListView_SetItem(handle(), &item);
}

void Table::clearImpl() {
	ListView_DeleteAllItems(handle());

	if(grouped) {
		ListView_RemoveAllGroups(handle());
	}
}

void Table::setIcon(unsigned row, unsigned column, int newIconIndex) {
	LVITEM item = { LVIF_IMAGE, static_cast<int>(row), static_cast<int>(column) };
	item.iImage = newIconIndex;
	ListView_SetItem(handle(), &item);
}

void Table::setNormalImageList( ImageListPtr imageList ) {
	  itsNormalImageList = imageList;
	  ListView_SetImageList( handle(), imageList->getImageList(), LVSIL_NORMAL );
}

void Table::setSmallImageList( ImageListPtr imageList ) {
	  itsSmallImageList = imageList;
	  ListView_SetImageList( handle(), imageList->getImageList(), LVSIL_SMALL );
}

void Table::setStateImageList( ImageListPtr imageList ) {
	  itsStateImageList = imageList;
	  ListView_SetImageList( handle(), imageList->getImageList(), LVSIL_STATE );
}

void Table::setGroupImageList(ImageListPtr imageList) {
	groupImageList = imageList;
	ListView_SetGroupHeaderImageList(handle(), groupImageList->handle());
}

void Table::setView( int view ) {
	if ( ( view & LVS_TYPEMASK ) != view )
	{
		dwtWin32DebugFail("Invalid View type");
	}
	//little hack because there is no way to do this with Widget::addRemoveStyle
	int newStyle = GetWindowLongPtr( handle(), GWL_STYLE );
	if ( ( newStyle & LVS_TYPEMASK ) != view )
	{
		SetWindowLongPtr( handle(), GWL_STYLE, ( newStyle & ~LVS_TYPEMASK ) | view );
	}
}

void Table::eraseColumnImpl( unsigned columnNo ) {
	dwtassert(columnNo != 0, "Can't delete the leftmost column");
	ListView_DeleteColumn( handle(), columnNo );
}

void Table::setColumnWidthImpl( unsigned columnNo, int width ) {
	if ( ListView_SetColumnWidth( handle(), columnNo, width ) == FALSE ) {
		dwtWin32DebugFail("Couldn't resize columns of Table");
	}
}

std::vector<Column> Table::getColumnsImpl() const {
	std::vector<Column> ret(getColumnCount());
	for(size_t i = 0, n = ret.size(); i < n; ++i) {
		ret[i] = getColumn(i);
	}
	return ret;
}

Column Table::getColumnImpl(unsigned column) const {
	LVCOLUMN col { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT };

	TCHAR buf[1024];
	col.pszText = buf;
	col.cchTextMax = sizeof(buf) / sizeof(TCHAR);

	if(!ListView_GetColumn(handle(), column, &col)) {
		dwtWin32DebugFail("Failed getting column information in Table");
		return Column();
	}

	return Column(col.pszText, col.cx,
		(col.fmt & LVCFMT_LEFT) == LVCFMT_LEFT ? Column::LEFT : (col.fmt & LVCFMT_RIGHT) == LVCFMT_RIGHT ? Column::RIGHT : Column::CENTER);
}

void Table::redraw( int firstRow, int lastRow ) {
	if(lastRow == -1) {
		lastRow = size();
	}
	if( ListView_RedrawItems( handle(), firstRow, lastRow ) == FALSE )
	{
		dwtWin32DebugFail("Error while redrawing items in Table");
	}
}

template<typename T>
static int compare(T a, T b) {
	return (a < b) ? -1 : ((a == b) ? 0 : 1);
}

int CALLBACK Table::compareFuncCallback(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	Table* p = reinterpret_cast<Table*>(lParamSort);
	int result = p->fun(lParam1, lParam2);
	if(!p->isAscending())
		result = -result;
	return result;
}

int CALLBACK Table::compareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	Table* p = reinterpret_cast<Table*>(lParamSort);

	const int BUF_SIZE = 128;
	TCHAR buf[BUF_SIZE];
	TCHAR buf2[BUF_SIZE];

	int na = (int)lParam1;
	int nb = (int)lParam2;

	int result = 0;

	SortType type = p->sortType;
	if(type == SORT_CALLBACK) {
		result = p->fun(p->getData(na), p->getData(nb));
	} else {
		ListView_GetItemText(p->handle(), na, p->sortColumn, buf, BUF_SIZE);
		ListView_GetItemText(p->handle(), nb, p->sortColumn, buf2, BUF_SIZE);

		if(type == SORT_STRING) {
			result = wcscoll(buf, buf2);
		} else if(type == SORT_STRING_SIMPLE) {
			result = wcscmp(buf, buf2);
		} else if(type == SORT_INT) {
			result = compare(_ttoi(buf), _ttoi(buf2));
		} else if(type == SORT_FLOAT) {
			double b1, b2;
			_stscanf(buf, _T("%lf"), &b1);
			_stscanf(buf2, _T("%lf"), &b2);
			result = compare(b1, b2);
		}
	}
	if(!p->ascending)
		result = -result;
	return result;
}

std::pair<int, int> Table::hitTest(const ScreenCoordinate& pt) {
	LVHITTESTINFO lvi = { ClientCoordinate(pt, this).getPoint() };
	return ListView_SubItemHitTest(handle(), &lvi) == -1 ? std::make_pair(-1, -1) : std::make_pair(lvi.iItem, lvi.iSubItem);
}

void Table::setTooltips(TooltipF f) {
	addRemoveTableExtendedStyle(LVS_EX_INFOTIP, true);
	HWND tipH = ListView_GetToolTips(handle());
	if(tipH) {
		// make tooltips last longer
		auto tip = WidgetCreator<ToolTip>::attach(this, tipH);
		tip->setDelay(TTDT_AUTOPOP, tip->getDelay(TTDT_AUTOPOP) * 3);
	}
	addCallback(Message(WM_NOTIFY, LVN_GETINFOTIP), [f](const MSG& msg, LRESULT&) -> bool {
		auto& tip = *reinterpret_cast<LPNMLVGETINFOTIP>(msg.lParam);
		if(tip.cchTextMax <= 2)
			return true;
		tstring text(f(tip.iItem));
		util::cutStr(text, tip.cchTextMax - 1);
		_tcscpy(tip.pszText, text.c_str());
		return true;
	});
}

Rectangle Table::getRect(int row, int code) {
	::RECT r;
	ListView_GetItemRect(handle(), row, &r, code);
	return Rectangle(r);
}

Rectangle Table::getRect(int row, int col, int code) {
	/* avoid ListView_GetSubItemRect which returns more or less garbage (depending on the Windows
	version) when asked for the first column. */

	dwtassert(ListView_GetHeader(handle()), "Table::getRect: attempt to get a sub rect but there's no header");

	auto rect = getRect(row, code);

	::RECT colRect { };
	Header_GetItemRect(ListView_GetHeader(handle()), col, &colRect);
	rect.pos.x += colRect.left;
	rect.size.x = colRect.right - colRect.left;

	return rect;
}

}
