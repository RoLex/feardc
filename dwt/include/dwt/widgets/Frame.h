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

#ifndef DWT_Frame_h
#define DWT_Frame_h

#include "../resources/Icon.h"
#include "../aspects/Activate.h"
#include "../aspects/MinMax.h"
#include "Composite.h"

namespace dwt {

/** Base class for a top-level window (either a main app window or a dialog). */
class Frame :
	public Composite,
	public aspects::Activate<Frame>,
	public aspects::MinMax<Frame>
{
	typedef Composite BaseType;
public:
	/// Class type
	typedef Frame ThisType;

	/// Object type
	typedef ThisType * ObjectType;

	/// Animates a window
	/** Slides the window into view from either right or left depending on the
	  * parameter "left". If "left" is true, then from the left,  otherwise from the
	  * right. <br>
	  * Show defines if the window shall come INTO view or go OUT of view. <br>
	  * The "time" parameter is the total duration of the function in milliseconds.
	  */
	void animateSlide( bool show, bool left, unsigned int msTime );

	/// Animates a window
	/** Blends the window INTO view or OUT of view. <br>
	  * Show defines if the window shall come INTO view or go OUT of view. <br>
	  * The "time" parameter is the total duration of the function in milliseconds.
	  */
	void animateBlend( bool show, int msTime );

	/// Animates a window
	/** Collapses the window INTO view or OUT of view. The collapse can be thought of
	  * as either an "explosion" or an "implosion". <br>
	  * Show defines if the window shall come INTO view or go OUT of view. <br>
	  * The "time" parameter is the total duration of the function in milliseconds.
	  */
	void animateCollapse( bool show, int msTime );

	/// Adds or removes the minimize box from the Widget
	void setMinimizeBox( bool value = true );

	/// Adds or removes the maximize box from the Widget
	void setMaximizeBox( bool value = true );

	/// Sets the small icon for the Widget (the small icon appears typically in the top left corner of the Widget)
	void setSmallIcon(const IconPtr& icon);

	/// Sets the large icon for the Widget (the large icon appears e.g. when you press ALT+Tab)
	void setLargeIcon(const IconPtr& icon);

protected:
	struct Seed : public BaseType::Seed {
		Seed(const tstring& caption, DWORD style, DWORD exStyle);
	};

	// Protected since this Widget we HAVE to inherit from
	Frame(Widget *parent, Dispatcher& dispatcher);

	// Protected to avoid direct instantiation, you can inherit but NOT instantiate
	// directly
	virtual ~Frame()
	{}

private:
	IconPtr smallIcon;
	IconPtr largeIcon;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline Frame::Seed::Seed(const tstring& caption, DWORD style, DWORD exStyle) :
	BaseType::Seed(caption, style, exStyle)
{

}

inline void Frame::animateSlide( bool show, bool left, unsigned int time )
{
	::AnimateWindow( this->handle(), static_cast< DWORD >( time ),
		show ?
			left ? AW_SLIDE | AW_HOR_NEGATIVE :
				AW_SLIDE | AW_HOR_POSITIVE
			:
			left ? AW_HIDE | AW_SLIDE | AW_HOR_NEGATIVE :
				AW_HIDE | AW_SLIDE | AW_HOR_POSITIVE
			);
}

inline void Frame::animateBlend( bool show, int msTime )
{
	::AnimateWindow( this->handle(), static_cast< DWORD >( msTime ), show ? AW_BLEND : AW_HIDE | AW_BLEND );
}

inline void Frame::animateCollapse( bool show, int msTime )
{
	::AnimateWindow( this->handle(), static_cast< DWORD >( msTime ), show ? AW_CENTER : AW_HIDE | AW_CENTER );
}

inline void Frame::setMinimizeBox( bool value )
{
	Widget::addRemoveStyle( WS_MINIMIZEBOX, value );
}

inline void Frame::setMaximizeBox( bool value )
{
	Widget::addRemoveStyle( WS_MAXIMIZEBOX, value );
}

inline void Frame::setSmallIcon(const IconPtr& icon) {
	smallIcon = icon;
	sendMessage(WM_SETICON, ICON_SMALL, smallIcon ? reinterpret_cast<LPARAM>(smallIcon->handle()) : 0);
}

inline void Frame::setLargeIcon(const IconPtr& icon) {
	largeIcon = icon;
	sendMessage(WM_SETICON, ICON_BIG, largeIcon ? reinterpret_cast<LPARAM>(largeIcon->handle()) : 0);
}

inline Frame::Frame(Widget * parent, Dispatcher& dispatcher) :
Composite(parent, dispatcher),
smallIcon(0),
largeIcon(0)
{
}

}

#endif
