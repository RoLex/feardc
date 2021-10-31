/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_StringConversion_h
#define DWT_StringConversion_h

#include <string>
#include <boost/scoped_array.hpp>
#include "../WindowsHeaders.h"
#include "xCeptionSmartUtilities.h"

namespace dwt { namespace util {

/// Helper for converting between different codepages
struct ConversionCodepage
{
	/// ConversionCodepage::Codepage, used in combination with UnicodeConverter
	/** States what kind of conversion you'd like
	  * E.g. if you have a std::string which you now is in UTF-8 you need to use ConversionCodepage::Codepage UTF8 to convert it to a std::wstring
	  */
	enum Codepage
	{
		UTF8 = CP_UTF8,
		UTF7 = CP_UTF7,
		ANSI = CP_ACP,
		MACINTOSH = CP_MACCP,
		OEM = CP_OEMCP,
		SYMBOL = CP_SYMBOL,
		CURRENT_THREAD = CP_THREAD_ACP
	};
};

// Generic version is NEVER implemented...
template< class From, class To >
class UnicodeConverter
{
private:
	UnicodeConverter(); // Never implemented
};

/// Specialized version of UnicodeConverter class for converting FROM UNICODE or wchar_t TO char (std::string)
/** It can in many scenarios be useful to be able to convert from wchar_t to char, one good example is when you have a
  * a filepath and you want to construct a std::fstream object.<br>
  * Often you would for instance use a LoadDialog or a SaveDialog Widget to retrieve a path from the user.<br>
  * These classes returns their file paths in UNICODE format ifi UNICODE is defined.<br>
  * But the std::fstream constructors cannot take any string types other than char * types for a file path.<br>
  * Then you can use this class to convert from wchar_t string to char strings. <br>
  * Example: <br>
  * std::string charHi = "hello";
  * tstring hi= SmartUtil::UnicodeConverter<char, TCHAR>::doConvert( charHi, SmartUtil::ConversionCodepage::UTF8 ); <br>
  * will convert if needed, and do nothing if TCHAR is char !
  */
template< >
class UnicodeConverter< wchar_t, char >
{
public:
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const std::string & input, ConversionCodepage::Codepage codepage )
	{
		return canConvert( input.c_str(), codepage );
	}

	/// Returns true if conversion is possible
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const char * input, ConversionCodepage::Codepage codepage )
	{
		return true;
	}

	/// Converts from UNICODE format to a "normal" char string...
	/** The input will be converted to a std::string object which will be returned to the caller.
	  */
	static std::string doConvert( const std::wstring & input, ConversionCodepage::Codepage codepage )
	{
		return doConvert( input.c_str(), codepage );
	}

	/// Converts from UNICODE format to a "normal" char string...
	/** The input will be converted to a std::string object which will be returned to the caller.
	  */
	static std::string doConvert( const wchar_t * input, ConversionCodepage::Codepage codepage )
	{
		// Trivial cases...
		if ( 0 == input || L'\0' == input[0] )
			return std::string();

		// First we must check how big buffer we need
		int size = ::WideCharToMultiByte( ( unsigned int ) codepage, 0, input, - 1, 0, 0, 0, 0 );
		boost::scoped_array< char > buffer( new char[ size ] );

		// Then we can do the actual conversion
		::WideCharToMultiByte( ( unsigned int ) codepage, 0, input, - 1, buffer.get(), size, 0, 0 );

		return buffer.get();
	}
};

/// Specialized version of UnicodeConverter class for converting FROM "normal" char strings TO UNICODE string or wchar_t string (std::wstring)
/** Use one of the static members to convert from char strings to wchar_t strings.
  * the return value of both is std::wstring
  */
template< >
class UnicodeConverter< char, wchar_t >
{
public:
	/// Returns true if conversion is possible
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const std::string & input, ConversionCodepage::Codepage codepage )
	{
		return canConvert( input.c_str(), codepage );
	}

	/// Returns true if conversion is possible
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const char * input, ConversionCodepage::Codepage codepage )
	{
		if ( codepage == ConversionCodepage::UTF8 )
		{
			unsigned int skipNext = 0;
			for ( const char * idx = input;
				* idx != 0;
				++idx )
			{
				if ( skipNext != 0 )
				{
					if ( !( * idx & 0x80 ) )
						throw xCeptionSmartUtilities( "Can't convert string since it contains a non-trailing character within a lead sequence" );
					--skipNext;
					continue;
				}
				switch ( * idx & 0xFC )
				{
					case 0xFC : // 111111xx
						++skipNext;
					case 0xF8 : // 111110xx
						++skipNext;
					case 0xF0 : // 111100xx
					case 0xF4 : // 111101xx
						++skipNext;
					case 0xE0 : // 111000xx
					case 0xE4 : // 111001xx
					case 0xE8 : // 111010xx
					case 0xEC : // 111011xx
						++skipNext;
					case 0xC0 : // 110000xx
					case 0xC4 : // 110001xx
					case 0xC8 : // 110010xx
					case 0xCC : // 110011xx
					case 0xD0 : // 110100xx
					case 0xD8 : // 110110xx
					case 0xDC : // 110111xx
						++skipNext;
				}
			}

			// Now if skipNext is not 0 this means we've seen a leading character but no trailing one...
			// This is a BUG and CAN'T be converted since it implies we've seen only "half" a charcter...
			if ( skipNext != 0 )
				return false;
			return true;
		}
		return true;
	}

	/// Converts from UNICODE format to a "normal" char string...
	/** The input will be converted to a std::string object which will be returned to the caller.
	  */
	static std::wstring doConvert( const std::string & input, ConversionCodepage::Codepage codepage )
	{
		return doConvert( input.c_str(), codepage );
	}

	/// Converts from UNICODE format to a "normal" char string...
	/** The input will be converted to a std::wstring object which will be returned to the caller.
	  */
	static std::wstring doConvert( const char * input, ConversionCodepage::Codepage codepage )
	{
		// Trivial cases...
		if ( 0 == input || '\0' == input[0] )
			return std::wstring();

		// We need to check if the last character is a lead character without a trailing one...
		if ( !canConvert( input, codepage ) )
		{
			throw xCeptionSmartUtilities( "Couldn't translate string into codepage, probable cause is that lead byte for 16 bits char was found but no trailing byte was found" );
		}

		// Check to see how big buffer we need, this is probably not nessecary since widechar always will have the same size or
		// LESS then the number of characters before conversion...
		// But we like to play it safe!
		int size = ::MultiByteToWideChar( ( unsigned int ) codepage, MB_ERR_INVALID_CHARS, input, - 1, 0, 0 );
		if ( size == 0 )
		{
			DWORD error = ::GetLastError();
			if ( error == ERROR_NO_UNICODE_TRANSLATION )
			{
				throw xCeptionSmartUtilities( "Couldn't translate string into codepage, probable cause is that lead byte for 16 bits char was found but no trailing byte was found" );
			}
			throw xCeptionSmartUtilities( "Couldn't translate string into codepage" );
		}
		boost::scoped_array< wchar_t > buffer( new wchar_t[ size ] );

		// Now doing the conversion
		size = ::MultiByteToWideChar( ( unsigned int ) codepage, MB_ERR_INVALID_CHARS, input, - 1, buffer.get(), size );

		std::wstring retVal( buffer.get() );
		return retVal;
	}
};

/// Specialized version of UnicodeConverter class for NOT converting AT ALL
/** This probably looks stupid at first sight, but if you wish to TRULY make UNICODE/ASCII support be COMPLETELY transparent you should
  * in fact have a version that does NOTHING.<br>
  * The reason is to avoid having the application do custom "#ifdef" on UNICODE checks to see "if we're supposed to convert or not".<br>
  */
template< >
class UnicodeConverter< char, char >
{
public:
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const std::string & input, ConversionCodepage::Codepage codepage )
	{
		return canConvert( input.c_str(), codepage );
	}

	/// Returns true if conversion is possible
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const char * input, ConversionCodepage::Codepage codepage )
	{
		return true;
	}

	/// Does not convert AT ALL
	/** Read the class info to understand why this is here.<br>
	  * Note the signature is slightly different than the "converting" classes. This is to avoid unnecesary copying of the input string.
	  */
	static const std::string & doConvert( const std::string & input, ConversionCodepage::Codepage codepage )
	{
		return input;
	}

	/// Does not convert AT ALL
	/** Read the class info to understand why this is here.<br>
	  */
	static std::string doConvert( const char * input, ConversionCodepage::Codepage codepage )
	{
		// This one DOES carry a bit overhead, but I think it's worth it to make the UNICODE support completely transparent without bringing in ugly macros...
		return input;
	}
};

/// Specialized version of UnicodeConverter class for NOT converting AT ALL
/** This probably looks stupid at first sight, but if you wish to TRULY make UNICODE/ASCII support be COMPLETELY transparent you should
  * in fact have a version that does NOTHING.<br>
  * The reason is to avoid having the application do custom "#ifdef" on UNICODE checks to see "if we're supposed to convert or not".<br>
  * Fact is if you think about iy you will probably quickly see that this is one of the only ways to completely let UNICODE support be 100% transparent.
  */
template< >
class UnicodeConverter< wchar_t, wchar_t >
{
public:
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const std::string & input, ConversionCodepage::Codepage codepage )
	{
		return canConvert( input.c_str(), codepage );
	}

	/// Returns true if conversion is possible
	/** There may exist many reasons as to why conversion is NOT possible.
	  * E.g. we may have a string which contains a character which has not been fully seen!
	  * You might have had some stream operation which haven't extracted a string containing the whole character, meaning
	  * you may have seen a leading byte for an UTF-8 conversion but no trailing byte etc.
	  */
	static bool canConvert( const char * input, ConversionCodepage::Codepage codepage )
	{
		return true;
	}

	/// Does not convert AT ALL
	/** Read the class info to understand why this is here.<br>
	  * Note the signature is slightly different than the "converting" classes. This is to avoid unnecesary copying of the input string.
	  */
	static const std::wstring & doConvert( const std::wstring & input, ConversionCodepage::Codepage codepage )
	{
		return input;
	}

	/// Does not convert AT ALL
	/** Read the class info to understand why this is here.<br>
	  */
	static std::wstring doConvert( const wchar_t * input, ConversionCodepage::Codepage codepage )
	{
		// This one DOES carry a bit overhead, but I think it's worth it to make the UNICODE support completely transparent without bringing in ugly macros...
		return input;
	}
};

/// \ingroup GlobalStuff
/// Typdef for easy conversion from char to wchar_t strings
/** Be careful with using these ones since it is not generic, you shouldn't use this one unless you ALWAYS expect no matter what build you're
  * in to get the "from" part in a char string and the to part in a wchar_t string.
  */
typedef UnicodeConverter< char, wchar_t > Ascii2Unicode;

/// \ingroup GlobalStuff
// Typdef for easy conversion from wchar_t to char strings
/** Be careful with using these ones since it is not generic, you shouldn't use this one unless you ALWAYS expect no matter what build you're
  * in to get the "from" part in a wchar_t string and the to part in a char string.
  */
typedef UnicodeConverter< wchar_t, char > Unicode2Ascii;

/// \ingroup GlobalStuff
// Typdef for easy conversion from wchar_t to char strings
/** This is the more "safe" version of the typedefs if you wish to GUARANTEE that the OUTPUT is of the current "build version".<br>
  * If UNICODE is defined you will NOT get any conversion at all, if UNICODE is NOT defined you WILL get a conversion to a char string.<br>
  * This and the Ascii2UnicodeOrMaybeNot is the most "safe" ones if you wish to make UNICODE support transparent.
  */
typedef UnicodeConverter< wchar_t, TCHAR > Unicode2CurrentBuild;

/// \ingroup GlobalStuff
// Typdef for easy conversion from char to wchar_t strings
/** This is the more "safe" version of the typedefs if you wish to GUARANTEE that the OUTPUT is of the current "build version".<br>
  * If UNICODE is defined you WILL get a conversion to a wchar_t string, if UNICODE is NOT defined you WILL NOT get a conversion at all.
  * This and the Unicode2AsciiOrMaybeNot is the most "safe" ones if you wish to make UNICODE support transparent.
  */
typedef UnicodeConverter< char, TCHAR > Ascii2CurrentBuild;

/// \ingroup GlobalStuff
// Typdef for easy conversion from wchar_t to char strings
/** This is the more "safe" version of the typedefs if you wish to GUARANTEE that the OUTPUT is of the current "build version".<br>
  * If UNICODE is defined you will NOT get any conversion at all, if UNICODE is NOT defined you WILL get a conversion to a char string.<br>
  * This and the Ascii2UnicodeOrMaybeNot is the most "safe" ones if you wish to make UNICODE support transparent.
  */
typedef UnicodeConverter< TCHAR, char > AsciiGuaranteed;

/// \ingroup GlobalStuff
// Typdef for easy conversion from char to wchar_t strings
/** This is the more "safe" version of the typedefs if you wish to GUARANTEE that the OUTPUT is of the current "build version".<br>
  * If UNICODE is defined you WILL get a conversion to a wchar_t string, if UNICODE is NOT defined you WILL NOT get a conversion at all.
  * This and the Unicode2AsciiOrMaybeNot is the most "safe" ones if you wish to make UNICODE support transparent.
  */
typedef UnicodeConverter< TCHAR, wchar_t > UnicodeGuaranteed;

} }

#endif
