/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_PROP_PAGE_H
#define DCPLUSPLUS_WIN32_PROP_PAGE_H

#include <functional>
#include <unordered_map>
#include <dcpp/typedefs.h>
#include <dwt/widgets/Container.h>

#include "forward.h"

using std::function;
using std::unordered_map;

/** This class is meant to serve as a base for setting pages. It provies handy methods to handle
 * global settings, help IDs and the like. */
class PropPage : public dwt::Container
{
public:
	PropPage(dwt::Widget* parent, int rows, int cols);
	virtual ~PropPage();

	virtual void layout();
	virtual void write() { }

	enum Type {
		T_STR,
		T_INT,
		T_INT_WITH_SPIN, // fill even when the current value is the same as the default value (for controls with a spin buddy)
		T_INT64,
		T_INT64_WITH_SPIN, // fill even when the current value is the same as the default value (for controls with a spin buddy)
		T_BOOL
	};

	/** An item to be read from and written to global settings; mapped to a GUI widget. */
	struct Item {
		Item() : widget(0), setting(0), type(T_STR) { }
		Item(Widget* w, int s, Type t) : widget(w), setting(s), type(t) { }
		Widget* widget;
		int setting;
		Type type;
	};

	typedef std::vector<Item> ItemList;

	/* One element in a list of boolean items; to be represented by a check-box list. */
	struct ListItem {
		int setting;
		const char* desc;
		unsigned helpId;
		function<bool ()> readF; // optional function to implement custom reads.
		function<void (bool)> writeF; // optional function to implement custom writes.
	};

	/** Read the specified items from global settings and render the data into the GUI widgets they
	 * map to. When settings haven't been customized by the user (default value), GUI widgets are
	 * left blank. */
	static void read(const ItemList& items);

	/** Read the specified boolean items from global settings and render the data into the
	 * specified GUI list widget. */
	void read(const ListItem* listItems, TablePtr list);

	/** Read the specified items from the GUI widgets they map to and save them into global
	 * settings. */
	static void write(const ItemList& items);
	/** Read the specified boolean items from the GUI list widget they map to and save them into
	 * global settings. */
	void write(TablePtr list);

protected:
	void handleBrowseDir(TextBoxPtr box, int setting);
	void handleBrowseFile(TextBoxPtr box, int setting);

	GridPtr grid;

private:
	virtual dwt::Point getPreferredSize();

	void handleListHelpId(TablePtr list, unsigned& id);

	unordered_map<TablePtr, const ListItem*> lists;
};

#endif // !defined(PROP_PAGE_H)
