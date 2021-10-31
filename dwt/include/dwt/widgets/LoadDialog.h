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

#ifndef DWT_LoadDialog_h
#define DWT_LoadDialog_h

#include "../Widget.h"
#include "../tstring.h"
#include "../aspects/FileFilter.h"
#include <vector>

namespace dwt {

/// LoadFileDialog class
/** \ingroup WidgetControls
  * \image html loadfile.PNG
  * Class for showing a LoadFileDialog box. <br>
  * \sa SaveDialog
  * \sa aspects::FileFilter
  */
class LoadDialog
	: public aspects::FileFilter<LoadDialog>
{
	typedef aspects::FileFilter<LoadDialog> BaseType;
	friend class aspects::FileFilter<LoadDialog>;

public:
	/// Class type
	typedef LoadDialog ThisType;

	/// Object type
	/** Note, not a pointer!!!!
	  */
	typedef ThisType ObjectType;

	/// Shows the dialog
	/** Returns an empty vector if user press cancel. <br>
	  * Returns a vector of "file paths" if user presses ok. <br>
	  * Use the inherited functions aspects::FileFilter::addFilter and
	  * aspects::FileFilter::activeFilter <br>
	  * before calling this function, if you wish the dialog to show only certain
	  * types of files.
	  */
	bool openMultiple(std::vector<tstring>& files, unsigned flags = 0);

	// Constructor Taking pointer to parent
	explicit LoadDialog( Widget * parent = 0 );

private:
	// aspects::FileFilter
	bool openImpl(OPENFILENAME& ofn);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline LoadDialog::LoadDialog( Widget * parent )
	: BaseType( parent )
{}

}

#endif
