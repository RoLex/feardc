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


#ifndef DCPLUSPLUS_WIN32_FORWARD_H_
#define DCPLUSPLUS_WIN32_FORWARD_H_

#include <dwt/forward.h>

using dwt::ButtonPtr;
using dwt::CheckBoxPtr;
using dwt::ComboBoxPtr;
using dwt::ContainerPtr;
using dwt::GridPtr;
using dwt::GroupBoxPtr;
using dwt::LabelPtr;
using dwt::MenuPtr;
using dwt::ProgressBarPtr;
using dwt::RadioButtonPtr;
using dwt::RebarPtr;
using dwt::SliderPtr;
using dwt::SpinnerPtr;
using dwt::SplitterContainerPtr;
using dwt::TablePtr;
using dwt::TabViewPtr;
using dwt::TextBoxPtr;
using dwt::ToolBarPtr;
using dwt::ToolTipPtr;

class HubFrame;

class ShellMenu;
typedef std::unique_ptr<ShellMenu> ShellMenuPtr;

class RichTextBox;
typedef RichTextBox* RichTextBoxPtr;

class TransferView;

template<typename ContentType, bool managed = true, typename TableType = dwt::Table>
class TypedTable;

template<typename ContentType, bool managed = true, typename TreeType = dwt::Tree>
class TypedTree;

#endif /* FORWARD_H_ */
