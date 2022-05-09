/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

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

#ifndef DWT_Spinner_h
#define DWT_Spinner_h

#include "../aspects/Scrollable.h"
#include "Control.h"

namespace dwt {

/** sideeffect=\par Side Effects:
  */
/// Spinner Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html spinner.PNG
  * Class for creating a Spinner Control Widget. <br>
  * A Spinner is a Widget used to manipulate an integer value. A good example is the
  * volume control of a music  player which has two buttons, one for louder and the
  * other for softer.
  */
class Spinner :
	public CommonControl,
	public aspects::Scrollable<Spinner>
{
	typedef CommonControl BaseType;
	friend class WidgetCreator<Spinner>;

public:
	/// Class type
	typedef Spinner ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		int minValue;
		int maxValue;

		Control* buddy;

		/// Fills with default parameters
		Seed(int minValue_ = UD_MINVAL, int maxValue_ = UD_MAXVAL, Control* buddy_ = 0);
	};

	/// Sets the range of the Spinner
	/** The range is the unique values of the control, use this function to set the
	  * range of the control. Maximum values are 65000 and minimum are -65000
	  */
	void setRange( int minimum, int maximum );

	/// Assigns a buddy control
	/** A "buddy control" can theoretically be any type of control, but the most
	  * usually type of control to assign is probably either a TextBox or a
	  * Label. <br>
	  * A buddy control is a control which (can) receive automatically the text value
	  * (setText) automatically when the value of the spinner changes. <br>
	  * And if you change the value of the buddy control the Spinner Control will
	  * automatically also change its value.
	  */
	void assignBuddy(Control* buddy);

	Control* getBuddy() const;

	/// Returns the value of the control
	/** The value can be any value between the minimum and maximum range defined in
	  * the setRange function
	  */
	int getValue();

	/// Sets the value of the control
	/** The value can be any value between the minimum and maximum range defined in
	  * the setRange function
	  */
	int setValue( int v );

	typedef std::function<bool (int, int)> UpdateF;
	void onUpdate(UpdateF f);

	/// Actually creates the Spinner Control
	/** You should call WidgetFactory::createSpinner if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create(const Seed &cs = Seed());

protected:
	// Constructor Taking pointer to parent
	explicit Spinner(Widget* parent);

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Spinner() { }

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	void handleSized();
	void assignBuddy_(Control* buddy);
};

}

#endif
