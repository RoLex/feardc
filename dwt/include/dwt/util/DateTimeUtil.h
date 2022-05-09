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

#ifndef DWT_DateTime_H
#define DWT_DateTime_H

#include "../WindowsHeaders.h"
#include "../tstring.h"
#include <time.h>

namespace dwt { namespace util {

class DateTime;
class TimeSpan;

DateTime operator +( const DateTime & date, const TimeSpan & time );
DateTime operator -( const DateTime & date, const TimeSpan & time );

/// Date and Time class
/** Class is inspired from the .Net Framework class System.DateTime and usable areas would be e.g. to parse DateTime objects transfered across SOAP callings
  */
class DateTime
{
	friend DateTime operator +( const DateTime & date, const TimeSpan & time );
	friend DateTime operator -( const DateTime & date, const TimeSpan & time );
private:
	SYSTEMTIME itsSysTime;

public:
	/// Constructs by default the smallest possible date
	DateTime();

	/// Construct a new DateTime from the given SYSTEMTIME
	explicit DateTime( const SYSTEMTIME & sysTime );

	/// Constructs a date according to the values given
	DateTime( unsigned year, unsigned month, unsigned day, unsigned hour = 0, unsigned minute = 0, unsigned seconds = 0, unsigned milliseconds = 0 );

	/// Constructs a date according to a unix timestamp value
	DateTime( time_t unixtimestamp );

	/// Copy constructor
	DateTime( const DateTime & rhs );

	/// Assignment operator which makes a perfect copy of the DateTime object on the right hand side of the assignment operator
	DateTime & operator =( const DateTime & rhs );

	/// Assignment operator which makes a copy of the SYSTEMTIME object on the right hand side of the assignment operator
	DateTime & operator =( const SYSTEMTIME & rhs );

	/// Explicit conversion to SYSTEMTIME
	const SYSTEMTIME & getSystemTime();

	/// Stamps the time part of the DateTime and returns an object containing only DATE information (or the TIME information is all zeros)
	DateTime date();

	/// Static constructor creating a DateTime which will hold the Date and Time of the creation of the object
	static DateTime now();

	/// Static constructor creating the smallest possible value
	static DateTime minValue();

	/// Converts the date to a string in the format "yyyy.MM.ddThh:mm:ss"
	/** Example: "2005.12.29T23:11:52"
	  */
	tstring toString() const;

	/// Returns a string containing only the Time of the object
	/** The format must be in format of e.g. "hh.mm.ss" or "mm-ss/hh" etc where "hh" is hours, "mm" is minutes and "ss" is seconds.
	  */
	tstring toTimeString( const tstring & format ) const;

	/// Returns only the date part of the date time structure in a string format
	/** The format must be in format of e.g. "dd:MM-yyyy" or "yyyy:MM:dd" where "yyyy" is four digits year, "MM" is two digits months and "dd" is two digits day.
	  */
	tstring toDateString( const tstring & format ) const;

	/// Converts the date to a unix timestamp
	time_t toUnixTimestamp() const;
};

// TODO: Comments....
DateTime operator +( const DateTime & date, const TimeSpan & time );

bool operator >( const DateTime & lhs, const DateTime & rhs );

bool operator <( const DateTime & lhs, const DateTime & rhs );

bool operator >=( const DateTime & lhs, const DateTime & rhs );

bool operator <=( const DateTime & lhs, const DateTime & rhs );

bool operator == ( const DateTime & lhs, const DateTime & rhs );

bool operator != ( const DateTime & lhs, const DateTime & rhs );

} }

#endif
