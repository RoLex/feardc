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

#ifndef DWT_Slider_h
#define DWT_Slider_h

#include "../aspects/Scrollable.h"
#include "Control.h"

namespace dwt {

/// Slider Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html slider.PNG
  * Class for creating a Slider control Widget. <br>
  * A Slider is a Widget which can be used to give e.g. percentage input. For
  * instance a volume control of a stereo system often has several of these ones, one
  * each for the volume, bass and tone. It contains of a thumbtrack and an axis which
  * you can move the thumbtrack up and down, it is quite similar in functionality to
  * the Spinner control, but have another visual appearance.
  */
class Slider :
	public CommonControl,
	// aspects::s
	public aspects::Scrollable< Slider >
{
	typedef CommonControl BaseType;
	friend class WidgetCreator< Slider >;

public:
	/// Class type
	typedef Slider ThisType;

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
		Seed();
	};

	/// Sets the Auto Ticks property of the control
	/** Auto ticks means that the control will have a tick note for each increment in
	  * its range of values.
	  */
	void setAutoTicks( bool value = true );

	/// Sets the horizontal property of the control
	/** If you want the slider to be horizontally aligned (default) then call this
	  * function with true, if you call it with false the slider will show up
	  * vertically instead.
	  */
	void setHorizontal( bool value = true );

	/// Sets the placement of the ticks
	/** If true is passed then the ticks of the slider will be displayed to the left
	  * of the slider, else ticks will show up to the right. Only call this function
	  * if you have called the setHorizontal with false.
	  */
	void setShowTicksLeft( bool value = true );

	/// Sets the placement of the ticks
	/** Sets the placement of the ticks to either top (true) or bottom (false). Only
	  * call this function if you have called the setHorizontal with true
	  */
	void setShowTicksTop( bool value = true );

	/// Sets the placement of the ticks to be BOTH (above/bottom or right/left)
	/** If you call this function with true it will show ticks on BOTH sides of the
	  * control depending on if the slider is aligned horizontally or vertically the
	  * ticks will show up either above and below or to the right and the left of the
	  * slider.
	  */
	void setShowTicksBoth( bool value = true );

	/// Removes ticks
	/** If you call this function with true the ticks will be REMOVED from the
	  * control. If you call it with false the ticks will appear again in its former
	  * positions
	  */
	void setShowTicks( bool value = true );

	/// Sets the range of the slider
	/** The range is the unique values of the control, use this function to set the
	  * range of the control. Maximum values are 65000 and minimum are -65000
	  */
	void setRange( short minimum, short maximum );

	/// Retrieves the maximum position of the Slider
	/** The return value from this function is the maximum value of the Slider
	  */
	int getMaxValue();

	/// Retrieves the minimum position of the Slider
	/** The return value from this function is the minimum value of the Slider
	  */
	int getMinValue();

	/// Sets the position of the thumb
	/** This is the "value" of the control, pass in the new position/value and the
	  * control will set the thumb to that position.
	  */
	void setPosition( int newPosition );

	/// Sets tick frequency
	/** Sets the frequency of the ticks, e.g. if five is given every fifth value of
	  * the slider will have a tick, default value is one.
	  */
	void setTickFrequency( unsigned frequency );

	/// Returns the "value" of the Slider
	/** Returns the "value" of the Slider or the position of the thumb. If you for
	  * instance have defined the minimum/maximum value to be -10 and 10 and the
	  * thumb is in the 3/4 of the max position this function will return 5.
	  */
	int getPosition();

	/// Assigns a buddy control
	/** A "buddy control" may be of any type but most often a TextBox or a
	  * Label is used. <br>
	  * It is normally used for displaying "maximum" and "minimum" text labels. <br>
	  * The buddy control will be positioned either to the left or right of a
	  * vertical Slider or the top or bottom of a horizontal Slider depending on the
	  * boolean value of the first argument. If the first argument is true, the buddy
	  * control will be placed to the left of a vertical Slider or above a horizontal
	  * Slider. <br>
	  * The buddy will be repositioned according to the position of the associated
	  * slider, BUT you MUST set the size of it. Don't burn braincells over the
	  * position setting. Only the SIZE matters. <br>
	  * A Slider may have TWO buddy controls, one with first argument set to true and
	  * one set to false. <br>
	  * If you assign another buddy control with the same first argument, the earlier
	  * instance will no longer be a buddy control, but it will still be there, it
	  * will not cease to exist or be destroyed. See the Slider sample
	  * application for an example of the buddy control in use.
	  */
	void assignBuddy( bool beginning, Widget * buddy );

protected:
	// Constructor Taking pointer to parent
	explicit Slider( Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Slider() {}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void Slider::setAutoTicks( bool value )
{
	this->addRemoveStyle( TBS_AUTOTICKS, value );
}

inline void Slider::setHorizontal( bool value )
{
	this->addRemoveStyle( TBS_HORZ, value );
	this->addRemoveStyle( TBS_VERT, !value );
}

inline void Slider::setShowTicksLeft( bool value )
{
	// TODO: Add assertion that the TBS_VERT is set!
	this->addRemoveStyle( TBS_LEFT, value );
	this->addRemoveStyle( TBS_RIGHT, !value );
}

inline void Slider::setShowTicksTop( bool value )
{
	// TODO: Add assertion that the TBS_HORZ is set!
	this->addRemoveStyle( TBS_TOP, value );
	this->addRemoveStyle( TBS_BOTTOM, !value );
}

inline void Slider::setShowTicksBoth( bool value )
{
	this->addRemoveStyle( TBS_BOTH, value );
}

inline void Slider::setShowTicks( bool value )
{
	this->addRemoveStyle( TBS_NOTICKS, !value );
}

inline void Slider::setRange( short minimum, short maximum )
{
	this->sendMessage( TBM_SETRANGE, static_cast< WPARAM >( TRUE ), MAKELONG( minimum, maximum ) );
}

inline int Slider::getMaxValue()
{
	return ( int )this->sendMessage(TBM_GETRANGEMAX);
}

inline int Slider::getMinValue()
{
	return ( int )this->sendMessage(TBM_GETRANGEMIN);
}

inline void Slider::setPosition( int newPosition )
{
	this->sendMessage( TBM_SETPOS, static_cast< WPARAM >( TRUE ), static_cast< LPARAM >( newPosition ) );
}

inline void Slider::setTickFrequency( unsigned frequency )
{
	this->sendMessage( TBM_SETTICFREQ, static_cast< WPARAM >( frequency ));
}

inline int Slider::getPosition()
{
	return this->sendMessage( TBM_GETPOS );
}

inline void Slider::assignBuddy( bool beginning, Widget * buddy )
{
	assert( buddy && buddy->handle() );
	this->sendMessage( TBM_SETBUDDY, static_cast< WPARAM >( beginning ? TRUE : FALSE ),
		reinterpret_cast< LPARAM >( buddy->handle() ) );
}

inline Slider::Slider( dwt::Widget * parent )
	: BaseType(parent, ChainingDispatcher::superClass<Slider>())
{
}

}

#endif
