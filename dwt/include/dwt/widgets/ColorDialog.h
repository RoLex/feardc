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

#ifndef DWT_ColorDialog_h
#define DWT_ColorDialog_h

#include "../Widget.h"

namespace dwt {

/// ChooseColorDialog class
/** \ingroup WidgetControls
  * \image html choosecolor.png
  * Class for showing a common ChooseColorDialog box. <br>
  * Either derive from it or call WidgetFactory::createColorDialog. <br>
  * Note! <br>
  * If you wish to use this class with EventHandlerClass classes other than those from
  * SmartWin you need to expose a public function called "parent" taking no arguments
  * returning an HWND. <br>
  * the complete signature of the function will then be "HWND parent()" <br>
  * This is one of few Widgets in SmartWin++ which can be used without linking in the
  * actual library!
  */
class ColorDialog
{
public:
	/// Class type
	typedef ColorDialog ThisType;

	/// Object type
	/** Note, not a pointer!!!!
	  */
	typedef ThisType ObjectType;

	/// Helper class for ColorDialog
	/** Is used as the parameter to the ColorDialog to set the custom colors
	  * and to set the initially default color
	  */
	class ColorParams
	{
		friend class ColorDialog;
		COLORREF itsColor;
		COLORREF itsCustomColors[16];

	public:
		/// Default constructor
		/** Initializes custom colors to "usable colors" and default color to yellow.
		  */
		ColorParams();

		/// Overloaded constructor taking only default color
		/** Initializes custom colors to "usable colors" and default color to the
		  * given color.
		  */
		ColorParams( COLORREF defaultColor );

		/// Overloaded constructor taking default color and custom color
		/** Initializes custom colors and default color to whatever is passed in
		  */
		ColorParams( COLORREF defaultColor, COLORREF customColors[16] );

		/// Returns the currently selected color
		COLORREF getColor() const;
	};

	/// Shows the Choose Color Dialog
	/** Returns a ColorParams object, if user presses Ok the userPressedOk will be
	  * true, else if user pressed Cancel the userPressedOk obviously will be false.
	  * <br>
	  * This dialog "remembers" its state from call to call. If you manipulate the
	  * Custom Colors, the next time it displays, it will have the same Custom
	  * Colors. <br>
	  * Note! <br>
	  * This is true even across DIFFERENT instances of the dialog!! <br>
	  * If basic is true dialog will be showed with only basic functionality, if
	  * allowFullOpen is true dialog will allow the user to show "more info".
	  */
	bool open( ColorParams& params, bool basic = false, bool allowFullOpen = true );

	/// Expicit constructor taking pointer to parent
	explicit ColorDialog( Widget * parent = 0 );

	~ColorDialog() { }

private:
	ColorParams itsColorParams;
	Widget* itsParent;

	HWND getParentHandle() { return itsParent ? itsParent->handle() : NULL; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline COLORREF ColorDialog::ColorParams::getColor() const
{
	return itsColor;
}

inline ColorDialog::ColorParams::ColorParams( COLORREF defaultColor, COLORREF customColors[16] )
	: itsColor( defaultColor )
{
	memcpy(itsCustomColors, customColors, sizeof(itsCustomColors));
}

inline ColorDialog::ColorDialog( Widget * parent )
	: itsParent( parent )
{
}

}

#endif
