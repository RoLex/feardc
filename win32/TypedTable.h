/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_TYPED_TABLE_H
#define DCPLUSPLUS_WIN32_TYPED_TABLE_H

#include "forward.h"
#include "WinUtil.h"

/** Table with an object associated to each item.

@tparam ContentType Type of the objects associated to each item.

@tparam managed Whether this class should handle deleting associated objects.

@note Support for texts:
The ContentType class must provide a const tstring& getText(int col) [const] function.

@note Support for images:
The ContentType class must provide a int getImage(int col) [const] function.

@note Support for item sorting:
The ContentType class must provide a
static int compareItems([const] ContentType* a, [const] ContentType* b, int col) function.

@note Support for custom styles per item (whole row) or per sub-item (each cell):
The ContentType class must provide a
int getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int col) [const] function. It is
called a first time with col=-1 to set the style of the whole item. It can return:
- CDRF_DODEFAULT to keep the default style for the item.
- CDRF_NEWFONT to change the style of the item.
- CDRF_NOTIFYSUBITEMDRAW to request custom styles for each sub-item (getStyle will then be called
for each sub-item).

@note Support for tooltips:
The ContentType class must provide a tstring getTooltip() [const] function. Note that tooltips are
only for the first column. */
template<typename ContentType, bool managed, typename TableType>
class TypedTable : public TableType
{
	typedef TableType BaseType;
	typedef TypedTable<ContentType, managed, TableType> ThisType;

public:
	typedef ThisType* ObjectType;

	explicit TypedTable( dwt::Widget * parent ) : BaseType(parent) {

	}

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(const typename BaseType::Seed& seed) : BaseType::Seed(seed) {
		}
	};

	virtual ~TypedTable() {
	}

	void create(const Seed& seed) {
#ifdef _DEBUG
		writeDebugInfo();
#endif
		BaseType::create(seed);

		addTextEvent();
		addImageEvent();
		addSortEvent();
		addStyleEvent();
		addTooltipEvent();

		if(managed) {
			this->onDestroy([this] { this->clear(); });
		}
	}

	int insert(ContentType* item) {
		return insert(getSortPos(item), item);
	}

	int insert(int i, ContentType* item) {
		return BaseType::insert(LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, i,
			LPSTR_TEXTCALLBACK, 0, 0, I_IMAGECALLBACK, reinterpret_cast<LPARAM>(item));
	}

	ContentType* getData(int iItem) {
		return reinterpret_cast<ContentType*>(BaseType::getData(iItem));
	}

	void setData(int iItem, ContentType* lparam) {
		if(managed) {
			delete getData(iItem);
		}
		BaseType::setData(iItem, reinterpret_cast<LPARAM>(lparam));
	}

	ContentType* getSelectedData() {
		int item = this->getSelected();
		return item == -1 ? 0 : getData(item);
	}

	using BaseType::find;

	int find(ContentType* item) {
		return this->findData(reinterpret_cast<LPARAM>(item));
	}

	struct CompFirst {
		CompFirst() { }
		bool operator()(ContentType& a, const tstring& b) {
			return Util::stricmp(a.getText(0), b) < 0;
		}
	};

	void forEach(void (ContentType::*func)()) {
		for(size_t i = 0, n = this->size(); i < n; ++i)
			(getData(i)->*func)();
	}
	void forEachSelected(void (ContentType::*func)(), bool removing = false) {
		int i = -1;
		while((i = ListView_GetNextItem(this->handle(), removing ? -1 : i, LVNI_SELECTED)) != -1)
			(getData(i)->*func)();
	}
	template<class _Function>
	_Function forEachT(_Function pred) {
		for(size_t i = 0, n = this->size(); i < n; ++i)
			pred(getData(i));
		return pred;
	}
	template<class _Function>
	_Function forEachSelectedT(_Function pred, bool removing = false) {
		int i = -1;
		while((i = ListView_GetNextItem(this->handle(), removing ? -1 : i, LVNI_SELECTED)) != -1) {
			pred(getData(i));
		}
		return pred;
	}

	void update(int i) {
		this->redraw(i, i);
	}

	void update(ContentType* item) { int i = find(item); if(i != -1) update(i); }

	void clear() { if(managed) this->forEachT(DeleteFunction()); this->BaseType::clear(); }
	void erase(int i) { if(managed) delete getData(i); this->BaseType::erase(i); }

	void erase(ContentType* item) { int i = find(item); if(i != -1) this->erase(i); }

	void setSort(int col = -1, bool ascending = true) {
		BaseType::setSort(col, BaseType::SORT_CALLBACK, ascending);
	}

private:
	/// @todo simplify all these if/when the next C++ standard has static if or concepts...

#define define_if(tester, retType) template<typename DeducedType = ContentType> \
	typename std::enable_if<tester<DeducedType>::value, retType>::type

	HAS_FUNC(HasText, const tstring&, getText(0));
	HAS_FUNC(HasImage, int, getImage(0));
	HAS_FUNC(HasSort, int, compareItems(nullptr, nullptr, 0));
	// over-complicated to test for lvalue refs...
	HAS_FUNC(HasStyle, int, getStyle(std::function<HFONT&()>()(), std::function<COLORREF&()>()(), std::function<COLORREF&()>()(), 0));
	HAS_FUNC(HasTooltip, tstring, getTooltip());

	define_if(HasText, void) addTextEvent() {
		this->onRaw([this](WPARAM, LPARAM lParam) -> LRESULT {
			auto& data = *reinterpret_cast<NMLVDISPINFO*>(lParam);
			if(data.item.mask & LVIF_TEXT) {
				this->handleText(data);
			}
			return 0;
		}, dwt::Message(WM_NOTIFY, LVN_GETDISPINFO));
	}
	define_if(!HasText, void) addTextEvent() { }

	define_if(HasImage, void) addImageEvent() {
		this->onRaw([this](WPARAM, LPARAM lParam) -> LRESULT {
			auto& data = *reinterpret_cast<NMLVDISPINFO*>(lParam);
			if(data.item.mask & LVIF_IMAGE) {
				this->handleImage(data);
			}
			return 0;
		}, dwt::Message(WM_NOTIFY, LVN_GETDISPINFO));
	}
	define_if(!HasImage, void) addImageEvent() { }

	define_if(HasSort, void) addSortEvent() {
		this->onSortItems([this](LPARAM lhs, LPARAM rhs) { return this->handleSort(lhs, rhs); });
		this->onColumnClick([this](int column) { this->handleColumnClick(column); });
	}
	define_if(!HasSort, void) addSortEvent() { }

	define_if(HasStyle, void) addStyleEvent() {
		this->onCustomDraw([this](NMLVCUSTOMDRAW& data) { return this->handleCustomDraw(data); });
	}
	define_if(!HasStyle, void) addStyleEvent() { }

	define_if(HasTooltip, void) addTooltipEvent() {
		this->setTooltips([this](int i) -> tstring { return this->getData(i)->getTooltip(); });
	}
	define_if(!HasTooltip, void) addTooltipEvent() { }

#ifdef _DEBUG
	void writeDebugInfo() {
		typedef ContentType T;
		printf("Creating a TypedTable<%s>; text: %d, images: %d, sorting: %d, custom styles: %d, tooltips: %d\n",
			typeid(T).name(), HasText<T>::value, HasImage<T>::value, HasSort<T>::value, HasStyle<T>::value, HasTooltip<T>::value);
	}
#endif

	define_if(HasSort, int) getSortPos(ContentType* a) {
		int high = this->size();
		if((this->getSortColumn() == -1) || (high == 0))
			return high;

		high--;

		int low = 0;
		int mid = 0;
		ContentType* b = NULL;
		int comp = 0;
		while( low <= high ) {
			mid = (low + high) / 2;
			b = getData(mid);
			comp = ContentType::compareItems(a, b, this->getSortColumn());

			if(!this->isAscending())
				comp = -comp;

			if(comp == 0) {
				return mid;
			} else if(comp < 0) {
				high = mid - 1;
			} else if(comp > 0) {
				low = mid + 1;
			}
		}

		comp = ContentType::compareItems(a, b, this->getSortColumn());
		if(!this->isAscending())
			comp = -comp;
		if(comp > 0)
			mid++;

		return mid;
	}
	define_if(!HasSort, int) getSortPos(ContentType* a) {
		return this->size();
	}

	define_if(HasText, void) handleText(NMLVDISPINFO& data) {
		ContentType* content = reinterpret_cast<ContentType*>(data.item.lParam);
		const tstring& text = content->getText(data.item.iSubItem);
		_tcsncpy(data.item.pszText, text.data(), std::min(text.size(), static_cast<size_t>(data.item.cchTextMax)));
		if(text.size() < static_cast<size_t>(data.item.cchTextMax)) {
			data.item.pszText[text.size()] = 0;
		}
	}

	define_if(HasImage, void) handleImage(NMLVDISPINFO& data) {
		ContentType* content = reinterpret_cast<ContentType*>(data.item.lParam);
		data.item.iImage = content->getImage(data.item.iSubItem);
	}

	define_if(HasSort, void) handleColumnClick(int column) {
		if(column != this->getSortColumn()) {
			this->setSort(column, true);
		} else if(this->isAscending()) {
			this->setSort(this->getSortColumn(), false);
		} else {
			this->setSort(-1, true);
		}
	}

	define_if(HasSort, int) handleSort(LPARAM lhs, LPARAM rhs) {
		return ContentType::compareItems(reinterpret_cast<ContentType*>(lhs), reinterpret_cast<ContentType*>(rhs), this->getSortColumn());
	}

	define_if(HasStyle, LRESULT) handleCustomDraw(NMLVCUSTOMDRAW& data) {
		if(data.nmcd.dwDrawStage == CDDS_PREPAINT) {
			return CDRF_NOTIFYITEMDRAW;
		}

		if((data.nmcd.dwDrawStage & CDDS_ITEMPREPAINT) == CDDS_ITEMPREPAINT && data.dwItemType == LVCDI_ITEM && data.nmcd.lItemlParam) {
			HFONT font = nullptr;
			auto ret = reinterpret_cast<ContentType*>(data.nmcd.lItemlParam)->getStyle(font, data.clrText, data.clrTextBk,
				((data.nmcd.dwDrawStage & CDDS_SUBITEM) == CDDS_SUBITEM) ? data.iSubItem : -1);
			if(ret == CDRF_NEWFONT && font) {
				::SelectObject(data.nmcd.hdc, font);
			}
			return ret;
		}

		return CDRF_DODEFAULT;
	}
};

#endif
