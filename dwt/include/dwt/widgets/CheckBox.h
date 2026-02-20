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

#ifndef DWT_CheckBox_h
#define DWT_CheckBox_h

#include "Button.h"

namespace dwt {

/// Check Box Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html checkbox.PNG
  * Class for creating a checkbox control Widget. <br>
  * A checkbox is a Widget which can be pressed to "pop in" and pressed again to "pop
  * out". <br>
  * It can contain descriptive text etc.
  */
class CheckBox :
	// Aspect classes
	public Button
{
	typedef Button BaseType;
	friend class WidgetCreator< CheckBox >;

public:
	/// Class type
	typedef CheckBox ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		/// Fills with default parameters
		Seed(const tstring& caption_ = tstring());
	protected:
		// For checkboxes
		Seed(const tstring& caption_, DWORD style);
	};

	/// Returns the checked state of the Check Box
	/** Return value is true if Check Box is checked, otherwise false.
	  */
	bool getChecked();

	/// Sets the checked state of the Check Box
	/** Call this one to programmaticially check a Check Box.
	  */
	void setChecked( bool value = true );

	virtual Point getPreferredSize();

protected:
	// Constructor Taking pointer to parent
	explicit CheckBox( dwt::Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~CheckBox()
	{}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void CheckBox::setChecked( bool value )
{
	this->sendMessage(BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED);
}

inline bool CheckBox::getChecked()
{
	return this->sendMessage(BM_GETCHECK) == BST_CHECKED;
}

inline CheckBox::CheckBox( dwt::Widget * parent )
	: BaseType( parent )
{
}

}

#endif
