/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_Rebar_h
#define DWT_Rebar_h

#include "Control.h"
#include "../aspects/CustomDraw.h"

namespace dwt {

/// Rebar Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html Rebar.PNG
  * A Rebar is a strip of buttons normally associated with menu commands, like  for
  * instance Internet Explorer has ( unless you have made them invisible ) a toolbar
  * of buttons, one for going "home", one to stop rendering of the  current page, one
  * to view the log of URL's you have been to etc... In addition to serving like a
  * dockable toolbar ( see ToolBar ) a Rebar  Widget can also contain more
  * complex Widgets lke for instance a ComboBox, a TextBox and so on...
  */
class Rebar :
	public Control,
	public aspects::CustomDraw<Rebar, NMCUSTOMDRAW>
{
	typedef Control BaseType;

	friend class WidgetCreator<Rebar>;

public:
	/// Class type
	typedef Rebar ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed();
	};

	/// Actually creates the Rebar
	/** You should call WidgetFactory::createRebar if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create(const Seed& cs = Seed());

	/// Refreshes the Rebar
	/** Call this one after the container widget has been resized to make sure the
	  * Rebar is having the right size...
	  */
	int refresh();

	/** @param style see the REBARBANDINFO doc for possible values (fStyle section) */
	void add(Widget* w, unsigned style = 0, const tstring& text = tstring());
	void remove(Widget* w);

	bool empty() const;
	unsigned size() const;

protected:
	// Protected to avoid direct instantiation
	explicit Rebar(Widget* parent);
	virtual ~Rebar() { }

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];
};

}

#endif
