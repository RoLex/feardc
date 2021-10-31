/*

  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_FORWARD_H_
#define DWT_FORWARD_H_

#include "WindowsHeaders.h"

namespace boost {
template<class T> class intrusive_ptr;
}

namespace dwt {

template< class WidgetType >
class WidgetCreator;

class Bitmap;
typedef boost::intrusive_ptr<Bitmap> BitmapPtr;

class Brush;
typedef boost::intrusive_ptr<Brush> BrushPtr;

class Button;
typedef Button* ButtonPtr;

class CheckBox;
typedef CheckBox* CheckBoxPtr;

class ColorDialog;

class ComboBox;
typedef ComboBox* ComboBoxPtr;

class Container;
typedef Container* ContainerPtr;

class Control;
typedef Control* ControlPtr;

class CoolBar;
typedef CoolBar* CoolBarPtr;

class DateTime;
typedef DateTime* DateTimePtr;

class FolderDialog;

class Font;
typedef boost::intrusive_ptr<Font> FontPtr;

class FontDialog;

class Frame;
typedef Frame* FramePtr;

class Grid;
typedef Grid* GridPtr;
class GridInfo;

class GroupBox;
typedef GroupBox* GroupBoxPtr;

class Header;
typedef Header* HeaderPtr;

class Icon;
typedef boost::intrusive_ptr<Icon> IconPtr;

class ImageList;
typedef boost::intrusive_ptr<ImageList> ImageListPtr;

class Label;
typedef Label* LabelPtr;

class Link;
typedef Link* LinkPtr;

class LoadDialog;

class Menu;
typedef std::unique_ptr<Menu> MenuPtr;

class MDIChild;
typedef MDIChild* MDIChildPtr;

class MDIFrame;
typedef MDIFrame* MDIFramePtr;

class MDIParent;
typedef MDIParent* MDIParentPtr;

// Not a Color - corresponds to 1 + 0xFFFFFF (the max value of a COLORREF)
static const COLORREF NaC = 0x1000000;

class Notification;
typedef std::unique_ptr<Notification> NotificationPtr;

class Pen;
typedef boost::intrusive_ptr<Pen> PenPtr;

struct Point;

class ProgressBar;
typedef ProgressBar* ProgressBarPtr;

class RadioButton;
typedef RadioButton* RadioButtonPtr;

class Rebar;
typedef Rebar* RebarPtr;

struct Rectangle;

class Region;
typedef boost::intrusive_ptr<Region> RegionPtr;

class RichTextBox;
typedef RichTextBox* RichTextBoxPtr;

class SaveDialog;

class ScrolledContainer;
typedef ScrolledContainer* ScrolledContainerPtr;

class Slider;
typedef Slider* SliderPtr;

class Spinner;
typedef Spinner* SpinnerPtr;

class Splitter;
typedef Splitter* SplitterPtr;

class SplitterContainer;
typedef SplitterContainer* SplitterContainerPtr;

class StatusBar;
typedef StatusBar* StatusBarPtr;

class Table;
typedef Table* TablePtr;

class TableTree;
typedef TableTree* TableTreePtr;

class TabView;
typedef TabView* TabViewPtr;

class TextBox;
typedef TextBox* TextBoxPtr;

class ToolBar;
typedef ToolBar* ToolBarPtr;

class ToolTip;
typedef ToolTip* ToolTipPtr;

class Tree;
typedef Tree* TreePtr;

class VirtualTree;
typedef VirtualTree* VirtualTreePtr;

class Window;
typedef Window* WindowPtr;

}

#endif /*FORWARD_H_*/
