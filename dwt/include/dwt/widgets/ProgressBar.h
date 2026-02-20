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

#ifndef DWT_ProgressBar_h
#define DWT_ProgressBar_h

#include "Control.h"

namespace dwt {

/// ProgressBar Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html progressbar.png
  * Class for creating a ProgressBar control Widget. <br>
  * A ProgressBar is a Widget which can be used to show e.g. percentage of lengthy
  * jobs, often used when downloading from internet or installing applications etc.
  */
class ProgressBar :
	public CommonControl
{
	typedef CommonControl BaseType;

	friend class WidgetCreator< ProgressBar >;

public:
	/// Class type
	typedef ProgressBar ThisType;

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

	/// Sets the Vertical property of the control
	/** Progress is shown from left to right or from bottom to top
	  */
	void setHorizontal( bool value = true );

	/// Sets the range of a progress bar control to a 32 - bit value.
	/** The range is the unique values of the control, use this function to set the
	  * range of the control. The progress bar high limit, low limit, and position
	  * parameters are signed integers. To make full use of the 32 - bit range, set
	  * the range to -0x7FFFFFFF to 0x7FFFFFFF and treat the position as a signed
	  * integer.
	  */
	void setRange( int minimum, int maximum );

	/// Sets the position of the thumb
	/** This is the "value" of the control, pass in the new position/value and the
	  * control will set the thumb to that position.
	  */
	void setPosition( int newPosition );

	/// Sets the Step size
	/** Specifies the step increment for a progress bar. The step increment is the
	  * amount by which the progress bar increases its current position whenever
	  * AddStep is called. By default, the step increment is set to 10.
	  */
	void setStep( unsigned stepsize );

	/// returns the Step size
	unsigned getStep( void );

	/// Returns the "value" of the ProgressBar
	/** Retrieves the current position of the progress bar
	  */
	int getPosition( void );

	/// Adds a Value to the current ProgressBar Position
	/** *Advances the current position of a progress bar by a specified increment and
	  * redraws the bar to reflect the new position.
	  */
	void addToPosition( int positiondelta );

	/// Adds the Value of StepSize to the current Position of the Progressbar
	/** Advances the current position for a progress bar by the step increment and
	  * redraws the bar to reflect the new position. An application sets the step
	  * increment by calling the setStep function.
	  */
	void addStep( void );

	/// Retrieves the maximum value of the ProgressBar.
	/** The maximum value is the value the ProgressBar have if it is "full"
	  */
	int getMaxValue();

	/// Retrieves the minimum value of the ProgressBar.
	/** The minimum value is the value the ProgressBar have if it is "empty"
	  */
	int getMinValue();

protected:
	/// CTOR Taking pointer to parent
	explicit ProgressBar( Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~ProgressBar()
	{}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ProgressBar::setHorizontal( bool value )
{
	this->Widget::addRemoveStyle( PBS_VERTICAL, !value );
}

inline void ProgressBar::addToPosition( int positiondelta )
{
	this->sendMessage(PBM_DELTAPOS, static_cast< WPARAM >( positiondelta ) );
}

inline void ProgressBar::addStep( void )
{
	this->sendMessage(PBM_STEPIT);
}

inline void ProgressBar::setRange( int minimum, int maximum )
{
	this->sendMessage(PBM_SETRANGE32, static_cast< WPARAM >( minimum ), static_cast< LPARAM >( maximum ) );
}

inline int ProgressBar::getMaxValue()
{
	return ( int )this->sendMessage(PBM_GETRANGE, static_cast< WPARAM >( FALSE ) );
}

inline int ProgressBar::getMinValue()
{
	return ( int )this->sendMessage(PBM_GETRANGE, static_cast< WPARAM >( TRUE ) );
}

inline void ProgressBar::setPosition( int newPosition )
{
	this->sendMessage(PBM_SETPOS, static_cast< WPARAM >( newPosition ) );
}

inline void ProgressBar::setStep( unsigned stepsize )
{
	this->sendMessage(PBM_SETSTEP, static_cast< WPARAM >( stepsize ) );
}

inline unsigned int ProgressBar::getStep( void )
{
	unsigned int stepsize = this->sendMessage(PBM_SETSTEP, static_cast< WPARAM >( 1 ) );
	this->sendMessage(PBM_SETSTEP, static_cast< WPARAM >( stepsize ) );
	return stepsize;
}

inline int ProgressBar::getPosition()
{
	return this->sendMessage(PBM_GETPOS );
}

inline ProgressBar::ProgressBar( dwt::Widget * parent ) :
BaseType(parent, ChainingDispatcher::superClass<ProgressBar>())
{
}

}

#endif
