/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

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

#include <dwt/widgets/Menu.h>

#include <algorithm>

#include <boost/scoped_array.hpp>

#include <vsstyle.h>

#include <dwt/resources/Brush.h>
#include <dwt/resources/Pen.h>
#include <dwt/DWTException.h>
#include <dwt/util/check.h>
#include <dwt/util/StringUtils.h>
#include <dwt/widgets/Control.h>

namespace dwt {

Menu::Colors Menu::colors;

const int Menu::borderGap = 3;
const int Menu::pointerGap = 5;
const int Menu::textIconGap = 8;
const int Menu::textBorderGap = 4;
const unsigned Menu::minWidth = 150;

Menu::Colors::Colors() {
	reset();
}

void Menu::Colors::reset() {
	text = Color::predefined(COLOR_MENUTEXT);
	gray = Color::predefined(COLOR_GRAYTEXT);
	background = Color::predefined(COLOR_MENU);
	stripBar = Color::darken(background, 0.06);
	highlightBackground = Color::predefined(COLOR_HIGHLIGHT);
	highlightText = Color::predefined(COLOR_HIGHLIGHTTEXT);
	titleText = Color::predefined(COLOR_MENUTEXT);
}

Menu::Seed::Seed(bool ownerDrawn_, const Point& iconSize_, FontPtr font_) :
popup(true),
ownerDrawn(ownerDrawn_),
iconSize(iconSize_),
font(font_)
{
}

Menu::Menu(Widget* parent) :
parentMenu(0),
parent(parent),
ownerDrawn(true),
popup(true),
drawSidebar(false)
{
	dwtassert(dynamic_cast<Control*>(parent), "A Menu must have a parent derived from dwt::Control");
}

void Menu::create(const Seed& seed) {
	ownerDrawn = seed.ownerDrawn;
	popup = seed.popup;

	itsHandle = popup ? ::CreatePopupMenu() : ::CreateMenu();
	if(!itsHandle) {
		throw Win32Exception("CreateMenu in Menu::create failed");
	}

	if(ownerDrawn) {
		iconSize = seed.iconSize;

		setFont(seed.font);

		if(!popup) {
			getParent()->addCallback(Message(WM_SYSCOLORCHANGE), Dispatchers::VoidVoid<0, false>([] { colors.reset(); }));
		}

		if(!parentMenu) {
			// only the root menu manages the theme.
			theme.load(VSCLASS_MENU, getParent(), !popup);
		}
	}

	if(!popup) {
		// set the MNS_NOTIFYBYPOS style to the whole menu
		MENUINFO mi = { sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS };
		if(!::SetMenuInfo(handle(), &mi))
			throw Win32Exception("SetMenuInfo in Menu::create failed");
	}
}

LRESULT Menu::handleNCPaint(UINT message, WPARAM wParam, long menuWidth) {
	// forward to ::DefWindowProc
	const MSG msg { getParent()->handle(), message, wParam, 0 };
	getParent()->getDispatcher().chain(msg);

	auto& theme = getRootMenu()->theme;
	if(!theme)
		return TRUE;

	MENUBARINFO info { sizeof(MENUBARINFO) };
	if(::GetMenuBarInfo(getParent()->handle(), OBJID_MENU, 0, &info)) {
		Rectangle rect { info.rcBar };
		rect.pos -= getParent()->getWindowRect().pos; // convert to client coords

		rect.pos.x += menuWidth;
		rect.size.x -= menuWidth;

		BufferedCanvas<WindowUpdateCanvas> canvas { getParent(), rect.left(), rect.top() };

		// avoid non-drawn left edge
		Rectangle rect_bg = rect;
		rect_bg.pos.x -= 1;
		rect_bg.size.x += 1;
		theme.drawBackground(canvas, MENU_BARBACKGROUND, MB_ACTIVE, rect_bg, false);

		canvas.blast(rect);
	}

	return TRUE;
}

void Menu::setMenu() {
	dwtassert(!popup,"Only non-popup menus may call setMenu (change the Seed accordingly)");

	if(!::SetMenu(getParent()->handle(), handle()))
		throw Win32Exception("SetMenu in Menu::setMenu failed");

	// get the width that current items will occupy
	long menuWidth = 0;
	for(unsigned i = 0, n = size(); i < n; ++i) {
		::RECT rect;
		::GetMenuItemRect(getParent()->handle(), handle(), i, &rect);
		menuWidth += Rectangle(rect).width();
	}

	Control* control = static_cast<Control*>(getParent());
	control->onRaw([this, menuWidth](WPARAM wParam, LPARAM) { return handleNCPaint(WM_NCPAINT, wParam, menuWidth); }, Message(WM_NCPAINT));
	control->onRaw([this, menuWidth](WPARAM wParam, LPARAM) { return handleNCPaint(WM_NCACTIVATE, wParam, menuWidth); }, Message(WM_NCACTIVATE));
	::DrawMenuBar(control->handle());
}

Menu* Menu::appendPopup(const tstring& text, const IconPtr& icon, bool subTitle) {
	// create the sub-menu
	auto sub = new Menu(getParent());
	sub->create(Seed(ownerDrawn, iconSize, font));
	sub->parentMenu = this;

	if(subTitle && popup) {
		sub->setTitle(text, icon);
	}

	// init structure for new item
	MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_SUBMENU | MIIM_STRING };

	// set item text
	info.dwTypeData = const_cast< LPTSTR >( text.c_str() );

	// set sub menu
	info.hSubMenu = sub->handle();

	// get position to insert
	auto position = size();

	if(ownerDrawn) {
		info.fMask |= MIIM_DATA | MIIM_FTYPE;

		info.fType = MFT_OWNERDRAW;

		// create item data
		auto wrapper = new ItemDataWrapper(this, position, false, icon);
		info.dwItemData = reinterpret_cast<ULONG_PTR>(wrapper);
		itsItemData.emplace_back(wrapper);
	}

	// append to this menu at the end
	if(!::InsertMenuItem(itsHandle, position, TRUE, &info)) {
		throw Win32Exception("Could not add a sub-menu");
	}
	itsChildren.emplace_back(sub);
	return sub;
}

Menu::~Menu() {
	// destroy this menu.
	::DestroyMenu(handle());
}

void Menu::setFont(FontPtr font) {
	this->font = font ? font : new Font(Font::DefaultGui);
	titleFont = boldFont = this->font->makeBold();
	for(auto& i: itsChildren) {
		i->setFont(this->font);
	}
}

void Menu::setTitleFont(FontPtr font) {
	titleFont = font;
}

void Menu::clearTitle(bool clearSidebar) {
	if(!ownerDrawn)
		return;

	if(!clearSidebar && !itsTitle.empty())
		remove(0);

	// clear title text
	itsTitle.clear();
}

void Menu::checkItem(unsigned index, bool value) {
	::CheckMenuItem( handle(), index, MF_BYPOSITION | (value ? MF_CHECKED : MF_UNCHECKED) );
}

void Menu::setItemEnabled(unsigned index, bool value) {
	if ( ::EnableMenuItem( handle(), index, MF_BYPOSITION | (value ? MF_ENABLED : MF_GRAYED) ) == - 1 )
	{
		dwtWin32DebugFail("Couldn't enable/disable the menu item, item doesn't exist" );
	}
}

UINT Menu::getMenuState(unsigned index) {
	return ::GetMenuState(handle(), index, MF_BYPOSITION);
}

bool Menu::isSeparator(unsigned index) {
	return (getMenuState(index) & MF_SEPARATOR) == MF_SEPARATOR;
}

bool Menu::isChecked(unsigned index) {
	return (getMenuState(index) & MF_CHECKED) == MF_CHECKED;
}

bool Menu::isPopup(unsigned index) {
	return (getMenuState(index) & MF_POPUP) == MF_POPUP;
}

bool Menu::isEnabled(unsigned index) {
	return !(getMenuState(index) & (MF_DISABLED | MF_GRAYED));
}

void Menu::setDefaultItem(unsigned index) {
	if(!::SetMenuDefaultItem(handle(), index, TRUE))
		throw Win32Exception("SetMenuDefaultItem in Menu::setDefaultItem fizzled...");
}

tstring Menu::getText(unsigned index) const {
	MENUITEMINFO mi = { sizeof(MENUITEMINFO), MIIM_STRING };
	if ( ::GetMenuItemInfo( itsHandle, index, TRUE, & mi ) == FALSE )
		throw Win32Exception( "Couldn't get item info 1 in Menu::getText" );
	boost::scoped_array< TCHAR > buffer( new TCHAR[++mi.cch] );
	mi.dwTypeData = buffer.get();
	if ( ::GetMenuItemInfo( itsHandle, index, TRUE, & mi ) == FALSE )
		throw Win32Exception( "Couldn't get item info 2 in Menu::getText" );
	return mi.dwTypeData;
}

void Menu::setText(unsigned index, const tstring& text) {
	MENUITEMINFO mi = { sizeof(MENUITEMINFO), MIIM_STRING };
	mi.dwTypeData = (TCHAR*) text.c_str();
	if ( ::SetMenuItemInfo( itsHandle, index, TRUE, & mi ) == FALSE ) {
		dwtWin32DebugFail("Couldn't set item info in Menu::setText");
	}
}

void Menu::setTitle(const tstring& title, const IconPtr& icon, bool drawSidebar /* = false */) {
	if(!ownerDrawn)
		return;

	this->drawSidebar = drawSidebar;

	const bool hasTitle = !itsTitle.empty();
	// set the new title
	itsTitle = title;

	if(!drawSidebar) {
		// init struct for title info
		MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_STATE | MIIM_STRING | MIIM_FTYPE | MIIM_DATA, MFT_OWNERDRAW, MF_DISABLED };

		// set title text
		info.dwTypeData = const_cast<LPTSTR>(itsTitle.c_str());

		// create wrapper for title item
		auto wrapper = new ItemDataWrapper(this, 0, true, icon);
		info.dwItemData = reinterpret_cast<ULONG_PTR>(wrapper);

		// adjust item data wrappers for all existing items
		for(size_t i = 0; i < itsItemData.size(); ++i)
			if(itsItemData[i])
				++itsItemData[i]->index;

		// push back title
		itsItemData.emplace_back(wrapper);

		if(!(!hasTitle ? ::InsertMenuItem(itsHandle, 0, TRUE, &info) : ::SetMenuItemInfo(itsHandle, 0, TRUE, &info))) {
			throw Win32Exception("Could not add a menu title");
		}
	}
}

bool Menu::handlePainting(DRAWITEMSTRUCT& drawInfo, ItemDataWrapper& wrapper) {
	MENUITEMINFO info { sizeof(MENUITEMINFO), MIIM_CHECKMARKS | MIIM_FTYPE | MIIM_DATA | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU };
	if(!::GetMenuItemInfo(handle(), wrapper.index, TRUE, &info))
		throw Win32Exception("Couldn't get menu item info when drawing");

	dwtassert((info.fType & MFT_OWNERDRAW) != 0, "Trying to draw a menu item that is not owner-drawn");

	// get state info
	bool isGrayed = ( drawInfo.itemState & ODS_GRAYED ) == ODS_GRAYED;
	bool isChecked = ( drawInfo.itemState & ODS_CHECKED ) == ODS_CHECKED;
	bool isDisabled = ( drawInfo.itemState & ODS_DISABLED ) == ODS_DISABLED;
	bool isSelected = ( drawInfo.itemState & ODS_SELECTED ) == ODS_SELECTED;
	bool isHighlighted = ( drawInfo.itemState & ODS_HOTLIGHT ) == ODS_HOTLIGHT;

	// find item icon
	/// @todo add support for HBITMAPs embedded in MENUITEMINFOs (hbmpChecked, hbmpUnchecked, hbmpItem)
	const IconPtr& icon = wrapper.icon;

	int stripWidth = iconSize.x + textIconGap;

	Rectangle rect { drawInfo.rcItem };

	auto& theme = getRootMenu()->theme;
	BufferedCanvas<FreeCanvas> canvas { drawInfo.hDC, rect.left(), rect.top() };

	// this will contain adjusted sidebar width
	int sidebarWidth = 0;

	if(drawSidebar) {
		// get logical info for title font
		auto lf = titleFont->getLogFont();

		// 90 degree rotation and bold
		lf.lfOrientation = lf.lfEscapement = 900;

		// create title font from logical info and select it
		Font vertFont { lf };
		auto select(canvas.select(vertFont));

		// set sidebar width to text height
		sidebarWidth = canvas.getTextExtent(itsTitle).y;

		// adjust item rectangle and item background
		rect.pos.x += sidebarWidth;
		rect.size.x -= sidebarWidth;

		if((drawInfo.itemAction & ODA_DRAWENTIRE) && !itsTitle.empty()) {
			// draw sidebar with menu title

			// select title color
			COLORREF oldColor = canvas.setTextColor( colors.titleText );

			// set background mode to transparent
			auto bkMode(canvas.setBkMode(true));

			// get rect for sidebar
			::RECT rc;
			::GetClipBox(drawInfo.hDC, &rc);
			//rc.left -= borderGap;

			// set title rectangle
			Rectangle textRect { 0, 0, sidebarWidth, rc.bottom - rc.top };

			// draw background
			canvas.fill(textRect, Brush(colors.stripBar));

			// draw title
			textRect.pos.y += 10;
			canvas.drawText(itsTitle, textRect, DT_BOTTOM | DT_SINGLELINE);

			// clear
			canvas.setTextColor( oldColor );
		}
	}

	bool highlight = (isSelected || isHighlighted) && !isDisabled;

	int part = 0, state = 0;
	if(theme) {
		int part_bg, state_bg;
		if(popup && !wrapper.isTitle) {
			part = MENU_POPUPITEM;
			state = (isDisabled || isGrayed) ? ((isSelected || isHighlighted) ? MPI_DISABLEDHOT : MPI_DISABLED) :
				highlight ? MPI_HOT : MPI_NORMAL;
			part_bg = MENU_POPUPBACKGROUND;
			state_bg = 0;
		} else {
			part = MENU_BARITEM;
			state = wrapper.isTitle ? MBI_NORMAL : isSelected ? MBI_PUSHED : isHighlighted ? MBI_HOT : MBI_NORMAL;
			part_bg = MENU_BARBACKGROUND;
			state_bg = MB_ACTIVE;
		}

		if(theme.isBackgroundPartiallyTransparent(part, state)) {
			Rectangle bgRect = rect;
			if(part_bg == MENU_BARBACKGROUND) {
				// avoid non-drawn edges
				bgRect.pos.x -= 1;
				bgRect.size.x += 2;
			}
			theme.drawBackground(canvas, part_bg, state_bg, bgRect, false);
		}
		theme.drawBackground(canvas, part, state, rect, false);

	} else {
		COLORREF color;
		if(highlight) {
			color = colors.highlightBackground;
		} else if(!popup) {
			BOOL flat;
			color = (::SystemParametersInfo(SPI_GETFLATMENU, 0, &flat, 0) && flat) ? Color::predefined(COLOR_MENUBAR) : colors.background;
		} else if(wrapper.isTitle) {
			color = colors.stripBar;
		} else {
			color = colors.background;
		}
		canvas.fill(rect, Brush(color));
	}

	Rectangle stripRect { rect };
	stripRect.size.x = stripWidth;

	if(!(theme ? (isSelected || isHighlighted) : highlight) && popup && !wrapper.isTitle) {
		// paint the strip bar (on the left, where icons go)
		canvas.fill(stripRect, Brush(colors.stripBar));
	}

	if(isChecked && theme) {
		/// @todo we should also support bullets
		theme.drawBackground(canvas, MENU_POPUPCHECKBACKGROUND, icon ? MCB_BITMAP : MCB_NORMAL, stripRect, false);
		if(!icon)
			theme.drawBackground(canvas, MENU_POPUPCHECK, MC_CHECKMARKNORMAL, stripRect, false);
	}

	if(popup && info.fType & MFT_SEPARATOR) {
		// separator
		Rectangle sepRect { rect };
		sepRect.pos.x += stripWidth + textIconGap;

		if(theme) {
			Point pt;
			if(theme.getPartSize(canvas, MENU_POPUPSEPARATOR, 0, pt)) {
				sepRect.size.y = std::min(sepRect.size.y, pt.y);
			}
			theme.drawBackground(canvas, MENU_POPUPSEPARATOR, 0, sepRect, false);

		} else {
			// center in the item rectangle
			sepRect.pos.y += sepRect.height() / 2 - 1;

			// draw the separator as a line
			Pen pen { colors.gray };
			auto select(canvas.select(pen));
			canvas.moveTo(sepRect.pos);
			canvas.lineTo(sepRect.width(), sepRect.top());
		}

	} else {
		// not a seperator, then draw item text and icon

		// get item text
		const int length = info.cch + 1;
		std::vector< TCHAR > buffer( length );
		int count = ::GetMenuString(handle(), wrapper.index, &buffer[0], length, MF_BYPOSITION);
		tstring itemText( buffer.begin(), buffer.begin() + count );

		if(!itemText.empty()) {
			// index will contain accelerator position
			size_t index = itemText.find_last_of( _T( '\t' ) );

			// split item text to draw accelerator correctly
			tstring text = itemText.substr( 0, index );

			// get accelerator
			tstring accelerator;

			if ( index != itemText.npos )
				accelerator = itemText.substr( index + 1 );

			// Select item font
			auto select(canvas.select(*(
				wrapper.isTitle ? titleFont :
				wrapper.isDefault ? boldFont :
				font)));

			// compute text rectangle
			Rectangle textRect = rect;
			if(popup) {
				if(!wrapper.isTitle || icon) {
					textRect.pos.x += stripWidth + textIconGap;
					textRect.size.x -= stripWidth + textIconGap;
				}
				textRect.size.x -= borderGap;
			}

			unsigned drawTextFormat = DT_VCENTER | DT_SINGLELINE;
			if((drawInfo.itemState & ODS_NOACCEL) == ODS_NOACCEL)
				drawTextFormat |= DT_HIDEPREFIX;
			unsigned drawAccelFormat = drawTextFormat | DT_RIGHT;
			drawTextFormat |= (popup && !wrapper.isTitle) ? DT_LEFT : DT_CENTER;
			if(getRootMenu()->popup) // menus not in the menu bar may get cropped
				drawTextFormat |= DT_WORD_ELLIPSIS;

			if(theme) {
				theme.drawText(canvas, part, state, text, drawTextFormat, textRect);
				if(!accelerator.empty())
					theme.drawText(canvas, part, state, accelerator, drawAccelFormat, textRect);

			} else {
				auto bkMode(canvas.setBkMode(true));

				canvas.setTextColor(
					isGrayed ? colors.gray :
					wrapper.isTitle ? colors.titleText :
					highlight ? colors.highlightText :
					colors.text);

				canvas.drawText(text, textRect, drawTextFormat);
				if(!accelerator.empty())
					canvas.drawText(accelerator, textRect, drawAccelFormat);
			}
		}

		// set up icon rectangle
		Rectangle iconRect { rect.pos, iconSize };
		iconRect.pos.x += (stripWidth - iconSize.x) / 2;
		iconRect.pos.y += (rect.height() - iconSize.y) / 2;
		if(isSelected && !isDisabled && !isGrayed) {
			// selected and active item; adjust icon position
			iconRect.pos.x--;
			iconRect.pos.y--;
		}

		if(icon) {
			// the item has an icon; draw it
			canvas.drawIcon(icon, iconRect);

		} else if(isChecked && !theme) {
			// the item has no icon set but it is checked; draw the check mark or radio bullet

			// background color
			canvas.fill(iconRect, Brush(highlight ? colors.highlightBackground : colors.stripBar));

			CompatibleCanvas boxCanvas { canvas.handle() };
			Bitmap bitmap { ::CreateCompatibleBitmap(canvas.handle(), iconSize.x, iconSize.y) };
			auto select(boxCanvas.select(bitmap));

			::RECT rc = Rectangle(iconSize);
			::DrawFrameControl(boxCanvas.handle(), &rc, DFC_MENU, (info.fType & MFT_RADIOCHECK) ? DFCS_MENUBULLET : DFCS_MENUCHECK);

			const int adjustment = 2; // adjustment for mark to be in the center

			::BitBlt(canvas.handle(), iconRect.left() + adjustment, iconRect.top(), iconSize.x, iconSize.y,
				boxCanvas.handle(), 0, 0, SRCAND);
		}

		if(isChecked && !theme && popup && !wrapper.isTitle) {
			// draw the surrounding rect after the check-mark
			stripRect.pos.x += 1;
			stripRect.pos.y += 1;
			stripRect.size.x -= 2;
			stripRect.size.y -= 2;

			Pen pen { colors.gray };
			auto select(canvas.select(pen));
			canvas.line(stripRect);
		}

		// draw the sub-menu arrow.
		if(popup && info.hSubMenu) {
			if(theme) {
				int part_arrow = MENU_POPUPSUBMENU, state_arrow = (isDisabled || isGrayed) ? MSM_DISABLED : MSM_NORMAL;
				Point pt { 9, 9 };
				theme.getPartSize(canvas, part_arrow, state_arrow, pt);
				Rectangle arrowRect = rect;
				theme.formatRect(canvas, part_arrow, state_arrow, arrowRect);
				theme.drawBackground(canvas, part_arrow, state_arrow,
					Rectangle(arrowRect.pos + Point(arrowRect.size.x - pt.x, std::max(arrowRect.size.y - pt.y, 0L) / 2), pt), false);

			} else {
				Point pt { 12, 12 };
				Rectangle arrowRect(rect.pos + Point(rect.size.x - pt.x - 3, std::max(rect.size.y - pt.y, 0L) / 2), pt);

				CompatibleCanvas arrowCanvas { canvas.handle() };
				Bitmap bitmap { ::CreateCompatibleBitmap(arrowCanvas.handle(), pt.x, pt.y) };
				auto select(arrowCanvas.select(bitmap));

				::RECT rc = Rectangle(pt);
				::DrawFrameControl(arrowCanvas.handle(), &rc, DFC_MENU, DFCS_MENUARROW);

				// same color as the text color
				Brush brush { isGrayed ? colors.gray :
					wrapper.isTitle ? colors.titleText :
					highlight ? colors.highlightText :
					colors.text };
				auto selectBrush(canvas.select(brush));

				// DrawFrameControl returns a monochrome mask; use it with these ROPs to paint just the arrow.
				::MaskBlt(canvas.handle(), arrowRect.left(), arrowRect.top(), arrowRect.width(), arrowRect.height(),
					arrowCanvas.handle(), 0, 0, bitmap.handle(), 0, 0, MAKEROP4(SRCAND, PATCOPY));
			}
		}
	}

	if(drawSidebar && (drawInfo.itemAction & ODA_DRAWENTIRE)) {
		// adjust for sidebar
		rect.pos.x -= sidebarWidth;
		rect.size.x += sidebarWidth;
	}

	// blast buffer into screen
	canvas.blast(rect);

	// prevent Windows from painting on top of what we have painted (such as the sub-menu arrow).
	::ExcludeClipRect(drawInfo.hDC, rect.left(), rect.top(), rect.right(), rect.bottom());

	return true;
}

bool Menu::handlePainting(MEASUREITEMSTRUCT& measureInfo, ItemDataWrapper& wrapper) {
	// this will contain the calculated size
	auto& itemWidth = measureInfo.itemWidth;
	auto& itemHeight = measureInfo.itemHeight;

	MENUITEMINFO info { sizeof(MENUITEMINFO), MIIM_FTYPE | MIIM_DATA | MIIM_CHECKMARKS | MIIM_STRING };
	if(!::GetMenuItemInfo(handle(), wrapper.index, TRUE, &info))
		throw Win32Exception("Couldn't get menu item info when measuring");

	dwtassert((info.fType & MFT_OWNERDRAW) != 0, "Trying to measure a menu item that is not owner-drawn");

	// check if separator
	if(info.fType & MFT_SEPARATOR) {
		auto& theme = getRootMenu()->theme;
		if(theme) {
			UpdateCanvas canvas { getParent() };
			Point pt;
			if(theme.getPartSize(canvas, MENU_POPUPSEPARATOR, 0, pt)) {
				itemWidth = pt.x;
				itemHeight = pt.y;
				return true;
			}
		}

		itemWidth = 60;
		itemHeight = 8;
		return true;
	}

	Point textSize = getTextSize(getText(wrapper.index),
		wrapper.isTitle ? titleFont :
		wrapper.isDefault ? boldFont :
		font);
	if(!wrapper.isTitle) // the title will adjust its hor size per others and add ellipsis if needed
		itemWidth = textSize.x;
	itemHeight = textSize.y;

	// find item icon
	/// @todo add support for HBITMAPs embedded in MENUITEMINFOs (hbmpChecked, hbmpUnchecked, hbmpItem)
	const IconPtr& icon = wrapper.icon;

	// adjust width
	if(popup || (!popup && icon)) {
		// adjust item width
		itemWidth += iconSize.x + textIconGap + pointerGap;

		// adjust item height
		itemHeight = (std::max)( itemHeight, ( UINT ) iconSize.y + borderGap );
	}

	// adjust width for sidebar
	if(drawSidebar)
		itemWidth += getTextSize(getText(0), titleFont).y; // 0 is the title index

	// make sure the calculated size is acceptable
	if(getRootMenu()->popup) {
		itemWidth = std::min(itemWidth, static_cast<unsigned>(getParent()->getWindowSize().x / 2));
		itemWidth = std::max(itemWidth, minWidth);
	}
	itemHeight = std::max(itemHeight, static_cast<UINT>(::GetSystemMetrics(SM_CYMENU)));
	return true;
}

void Menu::appendSeparator() {
	// init structure for new item
	MENUITEMINFO itemInfo { sizeof(MENUITEMINFO), MIIM_FTYPE, MFT_SEPARATOR };

	// get position to insert
	auto position = size();

	if(ownerDrawn) {
		itemInfo.fMask |= MIIM_DATA;
		itemInfo.fType |= MFT_OWNERDRAW;

		// create item data wrapper
		auto wrapper = new ItemDataWrapper(this, position);
		itemInfo.dwItemData = reinterpret_cast<ULONG_PTR>(wrapper);
		itsItemData.emplace_back(wrapper);
	}

	if(!::InsertMenuItem(itsHandle, position, TRUE, &itemInfo)) {
		throw Win32Exception("Could not add a menu separator");
	}
}

void Menu::remove(unsigned index) {
	auto child = ::GetSubMenu(itsHandle, index);

	if(!::RemoveMenu(itsHandle, index, MF_BYPOSITION)) {
		throw Win32Exception("Couldn't remove item in Menu::remove");
	}

	if(ownerDrawn) {
		for(auto i = itsItemData.begin(); i != itsItemData.end();) {
			if((*i)->index == index) {
				i = itsItemData.erase(i);
			} else {
				if((*i)->index > index) {
					--(*i)->index; // adjust succeeding item indices
				}
				++i;
			}
		}
	}

	if(child) {
		// remove this sub-menu from the child list.
		itsChildren.erase(std::remove_if(itsChildren.begin(), itsChildren.end(),
			[child](std::unique_ptr<Menu>& sub) { return sub->handle() == child; }), itsChildren.end());

	} else if(!getRootMenu()->popup) {
		// in the menu bar: remove the callback.
		getParent()->clearCallbacks(Message(WM_MENUCOMMAND, index * 31 + reinterpret_cast<LPARAM>(handle())));
	}
}

void Menu::clear() {
	// must be backwards, since higher indexes change when removing lower ones
	for(int i = size() - 1, end = itsTitle.empty() ? 0 : 1; i >= end; --i) {
		remove(i);
	}
}

unsigned Menu::size() const {
	int count = ::GetMenuItemCount(itsHandle);
	if(count < 0)
		throw Win32Exception("GetMenuItemCount in Menu::size failed");
	return count;
}

unsigned Menu::appendItem(const tstring& text, const Dispatcher::F& f, const IconPtr& icon, bool enabled, bool defaultItem) {
	// init structure for new item
	MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_STRING };

	if(!enabled) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_DISABLED;
	}
	if(defaultItem) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_DEFAULT;
	}

	const auto index = size();

	if(f) {
		Widget *parent = getParent();
		Dispatcher::F async_f = [this, parent, f] { parent->callAsync(f); };
		if(getRootMenu()->popup) {
			commands_type& commands_ref = getRootMenu()->commands;
			if(!commands_ref.get())
				commands_ref.reset(new commands_type::element_type);
			info.wID = id_offset + commands_ref->size();
			commands_ref->push_back(async_f);
		} else {
			// sub-menus of a menu bar operate via WM_MENUCOMMAND
			getParent()->addCallback(Message(WM_MENUCOMMAND, index * 31 + reinterpret_cast<LPARAM>(handle())),
				Dispatcher(async_f));
		}
	}

	// set text
	info.dwTypeData = const_cast< LPTSTR >( text.c_str() );

	if(ownerDrawn) {
		info.fMask |= MIIM_FTYPE | MIIM_DATA;
		info.fType = MFT_OWNERDRAW;

		// set item data
		auto wrapper = new ItemDataWrapper(this, index, false, icon);
		if(defaultItem)
			wrapper->isDefault = true;
		info.dwItemData = reinterpret_cast<ULONG_PTR>(wrapper);
		itsItemData.emplace_back(wrapper);
	}

	if(!::InsertMenuItem(itsHandle, index, TRUE, &info)) {
		throw Win32Exception("Couldn't insert item in Menu::appendItem");
	}
	return index;
}

void Menu::open(unsigned flags) {
	open(ScreenCoordinate(Point::fromLParam(::GetMessagePos())), flags);
}

void Menu::open(const ScreenCoordinate& sc, unsigned flags) {
	// sub-menus of the menu bar send WM_MENUCOMMAND; however, commands from ephemeral menus are handled right here.
	if(getRootMenu()->popup && (flags & TPM_RETURNCMD) != TPM_RETURNCMD)
		flags |= TPM_RETURNCMD;
	unsigned ret = ::TrackPopupMenu(handle(), flags, sc.getPoint().x, sc.getPoint().y, 0, getParent()->handle(), 0);
	if(ret >= id_offset) {
		commands_type& commands_ref = getRootMenu()->commands;
		if(commands_ref.get() && ret - id_offset < commands_ref->size())
			(*commands_ref)[ret - id_offset]();
	}
}

Menu* Menu::getChild(unsigned position) {
	HMENU h = ::GetSubMenu(handle(), position);
	for(size_t i = 0, n = itsChildren.size(); i < n; ++i) {
		auto menu = itsChildren[i].get();
		if(menu->handle() == h) {
			return menu;
		}
	}
	return nullptr;
}

Point Menu::getTextSize(const tstring& text, const FontPtr& font_) const {
	UpdateCanvas canvas(getParent());
	auto select(canvas.select(*font_));
	return canvas.getTextExtent(text) + Point(borderGap, borderGap);
}

}
