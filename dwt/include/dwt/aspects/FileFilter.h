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

#ifndef DWT_aspects_FileFilter_h
#define DWT_aspects_FileFilter_h

#include "../WindowsHeaders.h"
#include "../tstring.h"
#include "../util/check.h"

namespace dwt { namespace aspects {

/// Class for adding a filter to e.g. a LoadDialog dialog.
/** \ingroup aspects::Classes
  * Class is an Aspect class which should be realized into classes that needs it.
  * <br>
  * Help the LoadDialog and the SaveDialog to add up filters on which file
  * types to look for!
  */
template<typename WidgetType>
class FileFilter
{
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

public:
	/// Adds a filter to the object.
	/** filterName is the friendly name of the filter, normally this would be e.g.
	  * "HTML Files" etc. filter is the actual filter to filter in files to show
	  * normally this would be e.g. "*.html".
	  */
	WidgetType& addFilter( const tstring & filterName, const tstring & filter )
	{
		itsFilter.insert( itsFilter.end(), filterName.begin(), filterName.end() );
		itsFilter.push_back( '\0' );
		itsFilter.insert( itsFilter.end(), filter.begin(), filter.end() );
		itsFilter.push_back( '\0' );
		return W();
	}

	/// Sets the active filter to the specified index
	/** If you have added five filters and set the active filter to 3 then the fourth
	  * filter you added will be the active filter. Active filter means the default
	  * filter used when first showing the dialog.
	  */
	WidgetType& setActiveFilter( unsigned filterNo ) {
		if ( filterNo >= itsFilter.size() ) {
			dwtDebugFail( "Tried to set active filter to more than number of filters in filter..." );
			return W();
		}
		itsActiveFilter = filterNo;
		return W();
	}

	WidgetType& setDefaultExtension(const tstring& defExt) {
		itsDefExt = defExt;
		return W();
	}

	/// Returns the active filter of the object
	/** The active filter is the "currently selected" filter of the filter class
	  */
	unsigned getActiveFilter() const {
		// Filter index is NOT a zero indexed array...
		return itsActiveFilter + 1;
	}

	/// Sets the starting directory of the LoadDialog or SaveDialog Widget
	/** If given your dialog will try to start in the given directory, otherwise it
	  * will use the working directory of the process.
	  */
	WidgetType& setInitialDirectory( const tstring& initialDir ) {
		itsInitialDir = initialDir;
		return W();
	}

	bool open(tstring& file, unsigned flags = 0) {
		// get the current directory and restore it later to avoid directory locks
		TCHAR buf[MAX_PATH];
		::GetCurrentDirectory(MAX_PATH, buf);

		OPENFILENAME ofn = { 0 };
		getOFN(ofn);
		file.resize(PATH_BUFFER_SIZE);
		ofn.lpstrFile = const_cast<LPTSTR>(file.c_str());
		ofn.Flags = flags;

		bool ret = W().openImpl(ofn);
		if(ret)
			file = ofn.lpstrFile;

		::SetCurrentDirectory(buf);
		return ret;
	}

protected:
	FileFilter(Widget* parent)
		: itsParent(parent), itsActiveFilter( 0 )
	{}

	void getOFN(OPENFILENAME& ofn) {
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = itsParent ? itsParent->handle() : NULL;
		ofn.lpstrFilter = ifNotEmpty(itsFilter);
		ofn.nFilterIndex = this->getActiveFilter();
		ofn.nMaxFile = PATH_BUFFER_SIZE;
		ofn.lpstrInitialDir = ifNotEmpty(itsInitialDir);
		ofn.lpstrDefExt = ifNotEmpty(itsDefExt);
	}

private:
	static const int PATH_BUFFER_SIZE = 32768;

	Widget* itsParent;

	unsigned int itsActiveFilter;
	tstring itsInitialDir;
	tstring itsFilter;
	tstring itsDefExt;

	static const TCHAR* ifNotEmpty(const tstring& str) {
		return str.empty() ? NULL : str.c_str();
	}
};

} }

#endif
