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

#ifndef DWT_Window_h
#define DWT_Window_h

#include "Frame.h"

namespace dwt {

/// "Window" class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html window.PNG
  * Class for creating a "normal" window. <br>
  * A normal window is what most applications would use as the basic main structure,
  * in it you can add other Widgets like for instance buttons, comboboxes etc. <br>
  * A Window is basically a container Widget for other Widgets to reside in.
  * <br>
  * Class is a public superclass of Frame and therefore can use all
  * features of Frame.
  */
class Window
	: public Frame
{
public:
	/// Class type
	typedef Window ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	typedef Frame BaseType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		/// Fills with default parameters
		Seed(const tstring& caption = tstring());
	};

	/// Actually creates the window
	/** This one creates the window. <br>
	  * All Windows, and classes derived from them must create the Window
	  * before using it with functions such as setBounds() or setVisible( false ). <br>
      * The simple version "create()" uses a default Seed for the window attributes.
	  * The seed is not taken a constant because the class name will be generated at registration.
	  */
	void create( const Seed& cs = Seed() );

	explicit Window(Widget* parent = 0, Dispatcher& dispatcher = NormalDispatcher::getDefault());

	virtual ~Window();

	/// @return whether this window (or a dialog it owns) is the current foreground window.
	bool onForeground() const;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline Window::Window(Widget* parent, Dispatcher& dispatcher) :
BaseType(parent, dispatcher)
{
}

inline Window::~Window()
{
}

}

#endif
