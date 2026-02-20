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

#ifndef DWT_Menu_h
#define DWT_Menu_h

#include "../CanvasClasses.h"
#include "../Dispatchers.h"
#include <dwt/Theme.h>

namespace dwt {

/// Menu class
/** \ingroup WidgetControls
* \WidgetUsageInfo
* \image html menu.png
* Class for creating a Menu Control which then can be attached to e.g. a
* Window. <br>
* Note for Desktop version only! <br>
*/
class Menu : private boost::noncopyable
{
	friend class WidgetCreator<Menu>;

	typedef Dispatchers::VoidVoid<> Dispatcher;

	static struct Colors {
		Colors();
		void reset();

		COLORREF text;
		COLORREF gray;
		COLORREF background;
		COLORREF stripBar;
		COLORREF highlightBackground;
		COLORREF highlightText;
		COLORREF titleText;
	} colors;

public:
	typedef Menu ThisType;
	typedef MenuPtr ObjectType;

	struct Seed {
		typedef ThisType WidgetType;

		Seed(bool ownerDrawn_ = true,
			const Point& iconSize_ = Point(16, 16),
			FontPtr font_ = 0);
		bool popup;
		bool ownerDrawn;
		Point iconSize;
		FontPtr font;
	};

	static const int borderGap; /// Gap between the border and item
	static const int pointerGap; /// Gap between item text and sub - menu pointer
	static const int textIconGap; /// Gap between text and icon
	static const int textBorderGap; /// Gap between text and rectangel border
	static const unsigned minWidth; /// Minimum width of a menu item

	HMENU handle() const {
		return itsHandle;
	}

	Widget* getParent() const {
		return parent;
	}

	/// Actually creates the menu
	/** Creates the menu, the menu will be created initially empty!
	*/
	void create(const Seed& seed);

	void setMenu();

	/// Appends a popup to the menu
	/** Everything you "append" to a menu is added sequentially to the menu <br>
	* This specific "append" function appends a "popup" menu which is a menu
	* containing other menus. <br>
	* With other words a menu which is not an "option" but rather a new "subgroup".
	* <br>
	* The "File" menu of most application is for instance a "popup" menu while the
	* File/Print is often NOT a popup. <br>
	* To append items to the popup created call one of the appendItem overloaded
	* functions on the returned value of this function. <br>
	* Also, although references to all menu objects must be kept ( since they're
	* not collected automatically like other Widgets ) <br>
	* you don't have to keep a reference to the return value of this function since
	* it's being added as a reference to the children list of the "this" object.
	* <br>
	* A popup is basically another branch in the menu hierarchy <br>
	* See the Menu project for a demonstration.
	*/
	Menu* appendPopup(const tstring& text, const IconPtr& icon = IconPtr(), bool subTitle = true);

	/// Appends a separator item to the menu
	/** A menu separator is basically just "air" between menu items.< br >
	* A separator cannot be "clicked" or "chosen".
	*/
	void appendSeparator();

	/// Appends a Menu Item
	/// @return index of the newly appended item
	unsigned appendItem(const tstring& text, const Dispatcher::F& f = Dispatcher::F(), const IconPtr& icon = IconPtr(), bool enabled = true, bool defaultItem = false);

	/// Removes specified item from this menu
	/** Call this function to actually DELETE a menu item from the menu hierarchy.
	* Note that you have to specify the item position; and whenever you remove an item,
	* all subsequent items change positions. To remove a range of items, remove from
	* end to start.
	*/
	void remove(unsigned index);

	/// Remove all items from the menu
	/** Will also delete any submenus.
	*/
	void clear();

	/// Return the number of items in the menu
	unsigned size() const;

	/** Display the menu at the last known mouse position and handle the selected command, if any.
	@param flags refer to the TrackPopupMenu doc for possible values. */
	void open(unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON);
	/** Display the menu and handle the selected command, if any.
	@param sc Where the menu should be displayed.
	@param flags Refer to the TrackPopupMenu doc for possible values. */
	void open(const ScreenCoordinate& sc, unsigned flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON);

	/// Sets menu title
	/** A Menu can have a title, this function sets that title
	*/
	void setTitle(const tstring& title, const IconPtr& icon = IconPtr(), bool drawSidebar = false);

	void setFont(FontPtr font);

	/// Sets title font
	/** Create a font through e.g. createFont in WidgetFactory or similar and set the
	* title font to the menu title through using this function
	*/
	void setTitleFont(FontPtr font);

	/// Removes menu title
	/** If clearSidebar is true, sidebar is removed
	*/
	void clearTitle( bool clearSidebar = false );

	/// Checks (or uncheck) a specific menu item
	/** Which menu item you wish to check ( or uncheck ) is passed in as the "id"
	  * parameter. <br>
	  * If the "value" parameter is true the item will be checked, otherwise it will
	  * be unchecked
	  */
	void checkItem( unsigned index, bool value = true );

	/// Enables (or disables) a specific menu item
	/** Which menu item you wish to enable ( or disable ) is passed in as the "id"
	  * parameter. <br>
	  * If the "value" parameter is true the item becomes enabled, otherwise disabled
	  */
	void setItemEnabled( unsigned index, bool value = true );

	UINT getMenuState(unsigned index);

	/// Return true if the item is a separator (by position)
	bool isSeparator(unsigned index);
	/// Return true if the menu item is checked
	bool isChecked(unsigned index);
	/// Return true if the menu item is a popup menu
	bool isPopup(unsigned index);
	/// Return true if the menu item is enabled (not grey and not disabled)
	bool isEnabled(unsigned index);

	void setDefaultItem(unsigned index);

	/// Returns the text of a specific menu item
	tstring getText(unsigned index) const;

	/// Sets the text of a specific menu item
	void setText(unsigned index, const tstring& text);

	Menu* getChild(unsigned position);

	virtual ~Menu();

	template<typename T>
	static bool handlePainting(T& t) {
		ItemDataWrapper* wrapper = reinterpret_cast<ItemDataWrapper*>(t.itemData);
		return wrapper->menu->handlePainting(t, *wrapper);
	}

protected:
	/// Constructor Taking pointer to parent
	explicit Menu(Widget* parent);

private:
	// ////////////////////////////////////////////////////////////////////////
	// Menu item data wrapper, used internally
	// MENUITEMINFO's dwItemData *should* point to it
	// ////////////////////////////////////////////////////////////////////////
	struct ItemDataWrapper
	{
		// The menu item belongs to
		// For some messages (e.g. WM_MEASUREITEM),
		// Windows doesn't specify it, so
		// we need to keep this
		Menu* menu;

		// Item index in the menu
		// This is needed, because ID's for items
		// are not unique (although Windows claims)
		// e.g. we can have an item with ID 0,
		// that is either separator or popup menu
		unsigned index;

		/// Default menu item; draw with a bold font
		bool isDefault;

		// Specifies if item is menu title
		bool isTitle;

		/// Menu item icon
		const IconPtr icon;

		// Wrapper Constructor
		ItemDataWrapper(
			Menu* menu_,
			unsigned index_,
			bool isTitle_ = false,
			const IconPtr& icon_ = IconPtr()
			) :
		menu(menu_),
			index(index_),
			isDefault(false),
			isTitle(isTitle_),
			icon(icon_)
		{}
	};

	bool handlePainting(DRAWITEMSTRUCT& drawInfo, ItemDataWrapper& wrapper);
	bool handlePainting(MEASUREITEMSTRUCT& measureInfo, ItemDataWrapper& wrapper);

	LRESULT handleNCPaint(UINT message, WPARAM wParam, long menuWidth);

	Menu* getRootMenu() { return parentMenu ? parentMenu->getRootMenu() : this; }

	Menu* parentMenu; /// only defined for sub-menus; this is a link to their container menu
	HMENU itsHandle;
	Widget* parent;

	bool ownerDrawn;
	bool popup;

	// its sub menus
	std::vector<std::unique_ptr<Menu>> itsChildren;
	// its item data
	std::vector<std::unique_ptr<ItemDataWrapper>> itsItemData;

	static const unsigned id_offset = 100;
	typedef std::unique_ptr<std::vector<Dispatcher::F> > commands_type;
	commands_type commands; // just a pointer because sub-menus don't need this

	Theme theme;

	Point iconSize;

	FontPtr font;
	FontPtr boldFont; // for default items
	FontPtr titleFont; // Menu title font

	// Menu title
	tstring itsTitle;

	// if true title is drawn as sidebar
	bool drawSidebar;

	Point getTextSize(const tstring& text, const FontPtr& font_) const;
};

}

#endif
