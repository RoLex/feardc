/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#ifndef DWT_MDIParent_h
#define DWT_MDIParent_h

#include "Control.h"
#include "../util/check.h"
#include "MDIFrame.h"

namespace dwt {

// Forward declaring friends
template< class WidgetType >
class WidgetCreator;

/** sideeffect= \par Side Effects :
  */
/// MDI Parent Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html mdi.PNG
  * Class for creating a MDI Parent Widget. <br>
  * An MDI Parent is a Widget which can contain several "inner Widgets" which must be
  * of type MDIChild, think of it as a special "container" Widget for only
  * MDIChild widgets. <br>
  * Note! <br>
  * You can either inherit your own MDIChild Widgets or use the factory method
  * createMDIChild to create MDI Children Widgets. <br>
  * Related class : <br>
  * MDIChild
  */
class MDIParent :
	public Control

{
	typedef Control BaseType;
	friend class WidgetCreator< MDIParent >;
public:
	/// Class type
	typedef MDIParent ThisType;

	/// Object type
	typedef ThisType * ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		/**
		 * First child id for mdi menu, must be different from any other main menu id.
		 * Also, the menuHandle parameter of cs should point to the menu that will receive
		 *
		 **/
		UINT idFirstChild;

		HMENU windowMenu;

		Seed();
	};

	/// Actually creates the MDI EventHandlerClass Control
	/** You should call WidgetFactory::createMDIParent if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create( const Seed & cs = Seed() );

	void cascade() {
		this->sendMessage(WM_MDICASCADE);
	}

	void tile(bool horizontal) {
		this->sendMessage(WM_MDITILE, horizontal ? MDITILE_HORIZONTAL : MDITILE_VERTICAL);
	}

	void arrange() {
		this->sendMessage(WM_MDIICONARRANGE);
	}

	Widget* getActive() {
		return hwnd_cast<Widget*>(reinterpret_cast<HWND>(this->sendMessage(WM_MDIGETACTIVE)));
	}

	bool isActiveMaximized() {
		BOOL max = FALSE;
		this->sendMessage(WM_MDIGETACTIVE, 0, reinterpret_cast<LPARAM>(&max));
		return (max > 0);
	}

	void setActive(dwt::Widget* widget) {
		// TODO check that this is an instance of mdichild
		this->sendMessage(WM_MDIACTIVATE, reinterpret_cast<WPARAM>(widget->handle()));
	}

	void next() {
		getParent()->sendMessage(WM_SYSCOMMAND, SC_NEXTWINDOW, MAKELPARAM(0, -1));
	}

	MDIFrame* getParent() { return static_cast<MDIFrame*>(BaseType::getParent()); }
protected:
	/// Constructor Taking pointer to parent
	explicit MDIParent( Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use WidgetFactory class which is friend
	virtual ~MDIParent()
	{}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline MDIParent::MDIParent( Widget * parent )
	: BaseType(parent, NormalDispatcher::getDefault())
{
	dwtassert(parent, "Can't have a MDIParent without a parent...");
}

// end namespace dwt
}

#endif
