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

#ifndef DWT_LibraryLoader_h
#define DWT_LibraryLoader_h

#include "WindowsHeaders.h"
#include "tstring.h"

#include <boost/core/noncopyable.hpp>

namespace dwt {

/// Class for helping loading of libraries or dlls
/** Class normally meant for internal usage but can also be used publicly in user
  * code. <br>
  * Usage is whenever you need to ensure a dll is loaded instantiate a static object
  * of this type, static to ensure Constructor is only called upon the first
  * roundtrip of that code! <br>
  * Then if that library is not loaded somewhere earlier in some other portions of
  * your code it will load that library on demand the first time you need it! <br>
  * Class is thread safe to ensure safe usage across multiple threads! Though if you
  * need explicitly freeing of library (which you seldom need) class can be used
  * explicitly allocated on the stack or something and freed up in DTOR of class!
  * <br>
  * Class is also reference counting instances of the same library and only actually
  * calls LoadLibrary the first time it's instantiated with the same arguments!
  * Though when LAST object with same arguments goes out of scope library is
  * UNLOADED! Note also that library names are case sensitive in the refcounting
  * part, also "path sensitive". Meaning it will differentiate between e.g.
  * "%SYSTEM%/x.dll" and "x.dll" though there probably IS no real difference, this is
  * a potential "hard to track" bug meaning always instantiate this class with the
  * EXACT SAME arguments! <br>
  * See example usage in RichTextBox.h
  */
class LibraryLoader : private boost::noncopyable {
public:
	/// Constructor loading the given library
	/** Constructor loading the given library unless library is loaded from before.
	  * <br>
	  * Note this class is "reference counting" libraries meaning you can create
	  * several instances of this class with the same library name and it will only
	  * load the library the first time,  though when the LAST object is destroyed it
	  * will unload the given library! <br>
	  * Normally though you don't need to explicitly unload libraries meaning you can
	  * just allocate a static stack object of this type with the given library name
	  * anywhere you need to ensure your library must be loaded!
	  */
	LibraryLoader(const tstring& libraryName, bool allowFailure = false);

	/// Argument free Constructor
	/** Argument free Constructor, does NOTHING call load to actually load library!
	  */
	LibraryLoader();

	/// Actually loads library
	/** Call this one to actually load the given library or use Constructor taking
	  * tstring argument which automatically loads library!
	  */
	void load(const tstring& libraryName, bool allowFailure = false);

	/// Get procedure address from loaded library by name
	/** Allows you get a procedure address from the dll.
	  * Example: <br>
	  * static LibraryLoader richEditLibrary( _T( "riched20.dll" ) ); <br>
	  * FARPROC x1 = richEditLibrary.getProcAddress( _T("CreateTextServices") ); <br>
	  * FARPROC x2 = richEditLibrary.getProcAddress( 4 ); <br>
	  */
	FARPROC getProcAddress( const tstring & procedureName );

	/// Get procedure address from loaded library by ordinal value
	/**
	  */
	FARPROC getProcAddress( long procedureOrdinal );

	bool loaded() const { return itsHMod; }

	/// DTOR freeing up library
	/** Normally there's not much need of explicitly freeing up a library meaning you
	  * can just allocate your library objects as static objects, also it won't
	  * unload library until LAST object with same argument is being destroyed!
	  */
	~LibraryLoader();

private:
	HMODULE itsHMod;
};

}

#endif
