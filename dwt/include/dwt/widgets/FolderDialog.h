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

#ifndef DWT_WidgetChooseFolder_h
#define DWT_WidgetChooseFolder_h

#include "../Widget.h"
#include "../tstring.h"

namespace dwt {

/// ChooseFolderDialog class
/** \ingroup WidgetControls
  * \image html ChooseFolder.PNG
  * Class for showing a ChooseFolderDialog box. <br>
  * Either derive from it or call WidgetFactory::createChooseFolder. <br>
  * Note! <br>
  * If you wish to use this class with Parent classes other than those from SmartWin
  * you need to expose a public function called "parent" taking no arguments returning
  * and HWND in the Parent template parameter. <br>
  * the complete signature of the function will then be "HWND parent()"
  */
class FolderDialog
{
public:
	/// Class type
	typedef FolderDialog ThisType;

	/// Object type
	/** Note, not a pointer!!!!
	  */
	typedef ThisType ObjectType;

	// Constructor Taking pointer to parent
	explicit FolderDialog( Widget * parent = 0 );

	/** Set the root directory of the dialog. Only this directory or directories below it will be
	selectable. If not defined, the default is to use the desktop as root. */
	FolderDialog& setRoot(const int csidl);

	FolderDialog& setTitle(const tstring& title);

	/** Set the initially selected and expanded directory. When both a CSIDL & string are defined,
	the directory defined by string is given priority. May also be set by using the "dir" parameter
	of the open function. */
	FolderDialog& setInitialSelection(const tstring& sel);
	FolderDialog& setInitialSelection(const int csidl);

	/** Display the dialog.
	@param dir On input, may define an initially selected dir (shortcut for setInitialSelection).
	On output, contains the selected directory on success.
	@return Whether a directory was selected and successfully resolved. */
	bool open(tstring& dir);

	~FolderDialog();

private:
	Widget* parent;
	LPITEMIDLIST pidlRoot;
	tstring title;
	tstring initialSel;
	LPITEMIDLIST pidlInitialSel;

	HWND getParentHandle() const { return parent ? parent->handle() : nullptr; }

	static int CALLBACK browseCallbackProc(HWND hwnd, UINT uMsg, LPARAM, LPARAM lpData);
};

}

#endif
