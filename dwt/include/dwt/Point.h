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

#ifndef DWT_POINT_H
#define DWT_POINT_H

#include "WindowsHeaders.h"

namespace dwt {

/// POD structure for defining a point
/** Used in e.g. functions that take a mouse position etc...
  */
struct Point
{
	long x;
	long y;

	/// Constructor Initializing the point to (0,0)
	/** Default constructor initializing x and y member to 0 and 0
	  */
	Point();

	/// Constructor initializing the point with the given arguments.
	/** Constructor initializing the structure with the given arguments. Takes x and
	  * y coordinate to be used.
	  */
	Point( long x, long y );

	Point(const POINT& pt);

	Point& operator=(const POINT& pt);

	static Point fromMSG(const MSG& msg);

	static Point fromLParam(LPARAM lParam);

	LPARAM toLParam() const;
	::POINT toPOINT() const;
	operator ::POINT() const;

	/// Sets this Point to the maximum value for each x y dimension.
	/** Each x,y dimension is adjusted by the p Point.
	  */
	void maxOf( const Point & p );

	/// Sets this Point to the minimum value for each x y dimension.
	/** Each x,y dimension is adjusted by the p Point.
	  */
	void minOf( const Point & p );
};

class ScreenCoordinate {
public:
	ScreenCoordinate() { }
	ScreenCoordinate(const Point& pt) : point(pt) { }

	const Point& getPoint() const { return point; }
	Point& getPoint() { return point; }

	long x() const { return getPoint().x; }
	long y() const { return getPoint().y; }

private:
	Point point;
};

class Widget;

class ClientCoordinate {
public:
	explicit ClientCoordinate(const ClientCoordinate& cc, Widget* w_);

	explicit ClientCoordinate(const ScreenCoordinate& sc, Widget* w_);

	explicit ClientCoordinate(const Point& pt, Widget* w_) : point(pt), w(w_) { }

	operator ScreenCoordinate() const;

	const Point& getPoint() const { return point; }
	Point& getPoint() { return point; }

	long x() const { return getPoint().x; }
	long y() const { return getPoint().y; }

private:
	Point point;
	Widget* w;
};

/// \ingroup GlobalStuff
/// Adds a Point to the left hand side point
/** Basically just adds up the x and y coordinates of the two points and puts the
  * result into the left hand side Point
  */
Point & operator += ( Point & lhs, const Point & rhs );

/// \ingroup GlobalStuff
/// Adds two Points and returns the result
/** Basically just adds up the x and y coordinates of the two points and puts the
  * result into a new Point and returns that Point
  */
Point operator +( const Point & lhs, const Point & rhs );

/// \ingroup GlobalStuff
/// Subtracts a Point to the left hand side point
/** Basically just subtracts the x and y coordinates of the two points and puts the
  * result into the left hand side Point
  */
Point & operator -= ( Point & lhs, const Point & rhs );

/// \ingroup GlobalStuff
/// Subtracts two Points and returns the result
/** Basically just subtracts up the x and y coordinates of the two points and puts
  * the result into a new Point and returns that Point
  */
Point operator -( const Point & lhs, const Point & rhs );

/// \ingroup GlobalStuff
/// Returns true if the two Points are equal
/** Operator == for checking for equality
  */
bool operator == ( const Point & lhs, const Point & rhs );

/// \ingroup GlobalStuff
/// Returns false if the two Points are equal
/** Operator != for checking for equality
  */
bool operator != ( const Point & lhs, const Point & rhs );

inline Point::Point( long pX, long pY ) { x = pX; y = pY; }

inline Point::Point() { x = y = 0; }

inline Point::Point(const POINT& pt) : x(pt.x), y(pt.y) { }
inline Point& Point::operator=(const POINT& pt) { x = pt.x; y = pt.y; return *this; }

inline Point Point::fromLParam(LPARAM lParam) { return Point(GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam )); }

inline Point Point::fromMSG(const MSG& msg) { return fromLParam(msg.lParam); }

inline LPARAM Point::toLParam() const { return MAKELPARAM(x, y); }
inline POINT Point::toPOINT() const { POINT ret = { x, y }; return ret; }
inline Point::operator POINT() const { return toPOINT(); }

inline bool operator == ( const Point & lhs, const Point & rhs ) {
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool operator != ( const Point & lhs, const Point & rhs ) { return !( lhs == rhs ); }

}

#endif
