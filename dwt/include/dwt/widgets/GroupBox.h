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

#ifndef DWT_GroupBox_h
#define DWT_GroupBox_h

/// Button Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html radiogroup.PNG
  * Class for creating a Group Box control Widget. <br>
  * A Group Box Widget is a Widget which can contain other Widgets, normally you would
  * add up your RadioButtons into an object of this type
  */
#include "../aspects/Caption.h"
#include "../aspects/Child.h"
#include "Control.h"

namespace dwt {

/** Common stuff for all buttons */
class GroupBox :
	public CommonControl,
	public aspects::Caption<GroupBox>,
	public aspects::Child<GroupBox>
{
	typedef CommonControl BaseType;

	friend class WidgetCreator< GroupBox >;
public:
	/// Class type
	typedef GroupBox ThisType;

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

		/// Amount of space between child and border
		Point padding;

		/// Fills with default parameters
		Seed(const tstring& caption_ = tstring());
	};

	/// Actually creates the GroupBox
	void create( const Seed & cs = Seed() );

	virtual Point getPreferredSize();

	virtual void layout();

protected:
	/// Constructor Taking pointer to parent
	explicit GroupBox( Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~GroupBox()
	{}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	Point padding;

	Rectangle shrink(const Rectangle& client);
	Point expand(const Point& child);

	void handleEnabled(bool enabled);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline GroupBox::GroupBox( Widget * parent )
	: BaseType(parent, ChainingDispatcher::superClass<GroupBox>())
{
}

}

#endif
