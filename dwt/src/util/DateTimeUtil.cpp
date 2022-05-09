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

#include <dwt/util/DateTimeUtil.h>
#include <dwt/util/TimeSpan.h>

namespace dwt { namespace util {

DateTime::DateTime()
{
	itsSysTime.wYear = 1601;
	itsSysTime.wMonth = 1;
	itsSysTime.wDayOfWeek = 0;
	itsSysTime.wDay = 1;
	itsSysTime.wHour = 0;
	itsSysTime.wMinute = 0;
	itsSysTime.wSecond = 0;
	itsSysTime.wMilliseconds = 0;
}

DateTime::DateTime( const SYSTEMTIME & sysTime )
	: itsSysTime( sysTime )
{}

DateTime::DateTime( unsigned year, unsigned month, unsigned day, unsigned hour, unsigned minute, unsigned seconds, unsigned milliseconds )
{
	itsSysTime.wDayOfWeek = 0;
	itsSysTime.wYear = ( WORD ) year;
	itsSysTime.wMonth = ( WORD ) month;
	itsSysTime.wDay = ( WORD ) day;
	itsSysTime.wHour = ( WORD ) hour;
	itsSysTime.wMinute = ( WORD ) minute;
	itsSysTime.wSecond = ( WORD ) seconds;
	itsSysTime.wMilliseconds = ( WORD ) milliseconds;
}

DateTime::DateTime( time_t unixtimestamp )
{
	struct tm* t = localtime(&unixtimestamp);

	memset(&itsSysTime,0,sizeof(SYSTEMTIME));
	itsSysTime.wYear = t->tm_year + 1900;
	itsSysTime.wMonth = t->tm_mon + 1;
	itsSysTime.wDay = t->tm_mday;
	itsSysTime.wHour = t->tm_hour;
	itsSysTime.wMinute = t->tm_min;
	itsSysTime.wSecond = t->tm_sec;
}

DateTime::DateTime( const DateTime & rhs )
	: itsSysTime( rhs.itsSysTime )
{}

DateTime & DateTime::operator = ( const DateTime & rhs )
{
	itsSysTime = rhs.itsSysTime;
	return * this;
}

DateTime & DateTime::operator = ( const SYSTEMTIME & rhs )
{
	itsSysTime = rhs;
	return * this;
}

const SYSTEMTIME & DateTime::getSystemTime()
{
	return itsSysTime;
}

DateTime DateTime::date()
{
	DateTime retVal( itsSysTime );
	retVal.itsSysTime.wHour = 0;
	retVal.itsSysTime.wMinute = 0;
	retVal.itsSysTime.wSecond = 0;
	retVal.itsSysTime.wMilliseconds = 0;
	return retVal;
}

DateTime DateTime::now()
{
	SYSTEMTIME time;
	::GetSystemTime( & time );
	DateTime retVal( time );
	return retVal;
}

DateTime DateTime::minValue()
{
	DateTime retVal( 1, 1, 1 );
	return retVal;
}

tstring DateTime::toString() const
{
	// "2005-12-26T24:59:59" sample format
	tstring time = toTimeString( _T( "HH':'mm':'ss" ) );
	tstring date = toDateString( _T( "yyyy'-'MM'-'dd" ) );
	return date + _T( "T" ) + time;
}

tstring DateTime::toTimeString( const tstring & format ) const
{
	TCHAR tmp[100];
	int success = ::GetTimeFormat( LOCALE_SYSTEM_DEFAULT, 0, & itsSysTime, format.c_str(), tmp, 100 );
	if ( success == 0 )
		return _T( "" );
	tstring retVal = tmp;
	return retVal;
}

tstring DateTime::toDateString( const tstring & format ) const
{
	TCHAR tmp[100];
	int success = ::GetDateFormat( LOCALE_SYSTEM_DEFAULT, 0, & itsSysTime, format.c_str(), tmp, 100 );
	if ( success == 0 )
		return _T( "" );
	return tmp;
}

time_t DateTime::toUnixTimestamp() const
{
	struct tm tm;
	memset(&tm, 0, sizeof(tm));

	tm.tm_year = itsSysTime.wYear - 1900;
	tm.tm_mon = itsSysTime.wMonth - 1;
	tm.tm_mday = itsSysTime.wDay;
	tm.tm_hour = itsSysTime.wHour;
	tm.tm_min = itsSysTime.wMinute;
	tm.tm_sec = itsSysTime.wSecond;

	return mktime(&tm);
}

DateTime operator +( const DateTime & date, const TimeSpan & time )
{
	FILETIME file;
	::SystemTimeToFileTime( & date.itsSysTime, & file );
	ULONGLONG & tmpVal = reinterpret_cast< ULONGLONG & >( file );
#ifndef __GNUC__
	tmpVal += time.itsNumberOfMilliseconds * 10000i64;
#else
	tmpVal += time.itsNumberOfMilliseconds * 10000;
#endif
	DateTime retVal;
	::FileTimeToSystemTime( & file, & retVal.itsSysTime );
	return retVal;
}

DateTime operator -( const DateTime & date, const TimeSpan & time )
{
	FILETIME file;
	::SystemTimeToFileTime( & date.itsSysTime, & file );
	ULONGLONG & tmpVal = reinterpret_cast< ULONGLONG & >( file );
#ifndef __GNUC__
	tmpVal -= time.itsNumberOfMilliseconds * 10000i64;
#else
	tmpVal -= time.itsNumberOfMilliseconds * 10000;
#endif
	DateTime retVal;
	::FileTimeToSystemTime( & file, & retVal.itsSysTime );
	return retVal;
}

// In all the equality functions we depend on the fact that the default toString function returns the DateTime in a ISO DateTime format
// which means that a normal stringcompare will always be a correct way to check for equality and less then etc...
bool operator >( const DateTime & lhs, const DateTime & rhs )
{
	tstring tmpLeft = lhs.toString();
	tstring tmpRight = rhs.toString();
	return tmpLeft > tmpRight;
}

bool operator <( const DateTime & lhs, const DateTime & rhs )
{
	tstring tmpLeft = lhs.toString();
	tstring tmpRight = rhs.toString();
	return tmpLeft < tmpRight;
}

bool operator >=( const DateTime & lhs, const DateTime & rhs )
{
	return lhs > rhs || lhs == rhs;
}

bool operator <=( const DateTime & lhs, const DateTime & rhs )
{
	return lhs < rhs || lhs == rhs;
}

bool operator == ( const DateTime & lhs, const DateTime & rhs )
{
	tstring tmpLeft = lhs.toString();
	tstring tmpRight = rhs.toString();
	return tmpLeft == tmpRight;
}

bool operator != ( const DateTime & lhs, const DateTime & rhs )
{
	return !(lhs == rhs);
}

} }
