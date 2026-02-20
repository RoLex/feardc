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

#ifndef DWT_Label_h
#define DWT_Label_h

#include "../aspects/Caption.h"
#include "../aspects/Clickable.h"
#include "Control.h"

namespace dwt {

/// Static Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html static.PNG
  * Class for creating a Static control. <br>
  * A static control is kind of like a TextBox which cannot be written into, it's
  * basically just a control to put text into though you can also put other things
  * into it, e.g. Icons or Bitmaps. <br>
  * But it does have some unique properties : <br>
  * It can be clicked and double clicked which a TextBox cannot! <br>
  * It can load a bitmap.
  */
class Label :
	public CommonControl,
	public aspects::Caption<Label>,
	private aspects::Clickable<Label>
{
	typedef CommonControl BaseType;
	friend class WidgetCreator< Label >;
	friend class aspects::Clickable<Label>;

public:
	/// Class type
	typedef Label ThisType;

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

		IconPtr icon;

		/// Make the control display text.
		Seed(const tstring& caption_ = tstring());

		/// Make the control display an icon.
		Seed(IconPtr icon);
	};

	/// Actually creates the Static Control
	/** You should call WidgetFactory::createStatic if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create(const Seed& seed = Seed());

	virtual Point getPreferredSize();

	using aspects::Clickable<ThisType>::onClicked;
	using aspects::Clickable<ThisType>::onDblClicked;

protected:
	// Constructor Taking pointer to parent
	explicit Label( dwt::Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Label()
	{}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	IconPtr icon; // keep a reference

	// aspects::Clickable
	static Message getClickMessage();
	static Message getDblClickMessage();
};

inline Message Label::getClickMessage() {
	return Message(WM_COMMAND, MAKEWPARAM(0, STN_CLICKED));
}

inline Message Label::getDblClickMessage() {
	return Message(WM_COMMAND, MAKEWPARAM(0, STN_DBLCLK));
}

inline Label::Label( Widget * parent )
	: BaseType(parent, ChainingDispatcher::superClass<Label>())
{
}

}

#endif
