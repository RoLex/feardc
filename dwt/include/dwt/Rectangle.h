/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
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

#ifndef DWT_SMARTWIN_RECTANGLE_H_
#define DWT_SMARTWIN_RECTANGLE_H_

#include "Point.h"

namespace dwt {
/// Data structure for defining a rectangle
/** \ingroup WidgetLayout
  * The member functions are helpful in dividing large rectangles into smaller ones,
  * which is exactly what is needed to layout widgets in windows.
  */
struct Rectangle {
	Point pos;

	Point size;

	/// Constructor initializing the rectangle to (0, 0, 0, 0).
	/** Default constructor initializing everything to zero (0)
	  */
	Rectangle() { };

	Rectangle(const ::RECT& rc) : pos(rc.left, rc.top), size(rc.right - rc.left, rc.bottom - rc.top) { }

	/// Constructor initializing the rectangle with a position and size.
	/** Note that the pSize is actually a size and NOT the lower right Point.
	  */
	Rectangle( const Point & pPos, const Point & pSize ) : pos(pPos), size(pSize) { }

	/// Constructor initializing the rectangle with a size.
	/** Top-left becomes (0, 0), while bottom-right is set to pSize.
	  */
	Rectangle( const Point & pSize ) : pos(0, 0), size(pSize) { }

	::RECT toRECT() const;
	operator ::RECT() const;

	/// Constructor initializing the rectangle with longs instead of Points.
	/** ( x,y ) defines the upper right corner, ( x+width, y+height ) defines the
	  * lower left corner.
	  */
	Rectangle( long x, long y, long width, long height );

	long left() const { return pos.x; }
	long x() const { return left(); }

	long top() const { return pos.y; }
	long y() const {return top(); }

	long right() const { return left() + width(); }

	long bottom() const { return top() + height(); }

	long width() const { return size.x; }

	long height() const { return size.y; }

	/** Check whether the point is inside the rectangle. */
	bool contains(const Point& pt) const;

	/** Adjust the rectangle to make sure it is not outside of the desktop of the monitor closest
	to the given base widget. */
	Rectangle& ensureVisibility(Widget* base);
};

bool operator==(const Rectangle& lhs, const Rectangle& rhs);

/// \ingroup GlobalStuff
/// "Default" Rectangle for window creation
/**  The system selects the default position/size for the window.
  */
extern const Rectangle letTheSystemDecide;

}

#endif /*RECTANGLE_H_*/
