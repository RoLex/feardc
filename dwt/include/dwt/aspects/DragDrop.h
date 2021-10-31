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

#ifndef DWT_aspects_DragDrop_h
#define DWT_aspects_DragDrop_h

#include "../Message.h"
#include "../Point.h"
#include "../tstring.h"

namespace dwt { namespace aspects {

/// Aspect class used by dialog Widgets that have the possibility of trapping "drop files events".
/** \ingroup aspects::Classes
  * E.g. the ModalDialog can trap "drop files events" therefore they realize the aspects::DragDrop through inheritance.
  */
template< class WidgetType >
class DragDrop
{
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

public:
	/// \ingroup EventHandlersaspects::AspectDragDrop
	/// Setting the event handler for the "drop files" event
	/** If supplied event handler is called when a file is dropped over the widget.
	  * The function would receive a vector with all file paths and the coordinats where the files were dropped <br>
	  *
	  * Example:
	  *
	  * void DropFile(const std::vector<tstring>& files, Point droppoint) {
	  * 	tstring path = files.at(0);
	  * 	setText(path);
	  * 	int x = droppoint.x;
	  * 	int y = droppoint.y;
	  * }
	  */
	void onDragDrop(std::function<void (const std::vector<tstring>&, Point)> f) {
		W().addCallback(Message(WM_DROPFILES), [f](const MSG& msg, LRESULT& ret) -> bool {
			std::vector<tstring> files;
			POINT pt = { 0 };

			HDROP handle = reinterpret_cast<HDROP>(msg.wParam);
			if(handle) {
				UINT iFiles = ::DragQueryFile(handle, 0xFFFFFFFF, 0, 0);
				TCHAR pFilename[MAX_PATH];
				for(UINT i = 0; i < iFiles; ++i) {
					memset(pFilename, 0, MAX_PATH * sizeof(TCHAR));
					::DragQueryFile(handle, i, pFilename, MAX_PATH);
					files.push_back(pFilename);
				}
				::DragQueryPoint(handle, &pt);
				::DragFinish(handle);
			}

			f(files, pt);
			return false;
		});
	}

	/// Setup Drag & Drop for this dialog
	/** This sets up the ability to receive an WM_DROPFILES msg if you drop a file on the dialog
	*/
	void setDragAcceptFiles(bool accept = true) {
		::DragAcceptFiles(H(), accept);
	}
};

} }

#endif
