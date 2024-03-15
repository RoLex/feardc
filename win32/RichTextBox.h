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

#ifndef DCPLUSPLUS_WIN32_RichTextBox_H_
#define DCPLUSPLUS_WIN32_RichTextBox_H_

#include <dcpp/typedefs.h>

#include <dwt/widgets/RichTextBox.h>

#include "forward.h"

/// our rich text boxes that provide find functions and handle links
class RichTextBox : public dwt::RichTextBox {
	typedef dwt::RichTextBox BaseType;
	friend class dwt::WidgetCreator<RichTextBox>;

	typedef std::function<bool (const tstring&)> LinkF;

public:
	typedef RichTextBox ThisType;
	
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed();
	};

	explicit RichTextBox(dwt::Widget* parent);
	void create(const Seed& seed);

	bool handleMessage(const MSG& msg, LRESULT& retVal);

	MenuPtr getMenu();

	tstring findTextPopup();
	void findTextNew();
	void findTextNext();

	/// provides a chance to handle links differently
	void onLink(LinkF f);

private:
	bool handleKeyDown(int c);
	LRESULT handleLink(ENLINK& link);
	void handleLinkTip(tstring& text);

	tstring getLinkText(const ENLINK& link);
	void openLink(const tstring& text);

	ToolTipPtr linkTip;
	DWORD linkTipPos;
	tstring currentLink;
	LinkF linkF;
};

typedef RichTextBox::ObjectType RichTextBoxPtr;

#endif /*RichTextBox_H_*/
