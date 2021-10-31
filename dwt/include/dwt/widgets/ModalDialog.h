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

#ifndef DWT_ModalDialog_h
#define DWT_ModalDialog_h

#include "../aspects/Dialog.h"
#include "Frame.h"
#include "../Application.h"

namespace dwt {

/// Modal Dialog class
/** \ingroup WidgetControls
  * \image html dialog.PNG
  * Class for creating a Modal Dialog based optionally on an embedded resource. <br>
  * Use create( unsigned resourceId ) if you define the dialog in a .rc file,
  * and use create() if you define the dialog completly in C++ source. <br>
  * Use the create function to actually create a dialog. <br>
  * Class is a public superclass of Frame and therefore can use all
  * features of Frame. <br>
  * Note! <br>
  * Usually you create a ModalDialog on the stack. <br>
  * This Widget does NOT have selfdestructive semantics and should normally be
  * constructed on the stack! <br>
  * The create function does NOT return before the Widget is destroyed! <br>
  * Thus, you must declare the "onInitDialog" event handler before calling the
  * "create()", either in the contructor, or in some intialization routine
  * called before create();
  */

class ModalDialog :
	public Frame,
	public aspects::Dialog<ModalDialog>
{
	typedef Frame BaseType;

public:
	/// Class type
	typedef ModalDialog ThisType;

	/// Object type
	/** Note, not a pointer!!!!
	  */
	typedef ThisType ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(const Point& size, DWORD styles_ = 0);
	};

	void create(const Seed& cs);

	/// Display the dialog and return only when the dialog has been dismissed
	int show();

	/// Ends the Modal Dialog Window started with create().
	/** Pass a return value for create() and close the dialog. <br>
	  * To be called by the dialog class when it should close. <br>
	  * Note that the member variables of the ModalDialog class still exist,
	  * but not any subwindows or Control Widgets.
	  */
	void endDialog( int returnValue );

	/// Dialog Init Event Handler setter
	/** This would normally be the event handler where you attach your Widget
	  * controls and do all the initializing etc... <br>
	  * It's important that you declare this event handler BEFORE calling the
	  * create function since that function doesn't actually return before the
	  * dialog is destroyed! <br>
	  * Method signature must be bool foo(); <br>
	  * If you return true from your Event Handler the system will NOT mess up the
	  * initial focus you have chosen, if you return false the system will decide
	  * which control to initially have focus according to the tab order of the
	  * controls!
	  */
	void onInitDialog(std::function<bool ()> f) {
		addCallback(Message(WM_INITDIALOG), [f](const MSG&, LRESULT& ret) -> bool {
			ret = f() ? TRUE : FALSE;
			return true;
		});
	}

protected:
	// Protected since this Widget we HAVE to inherit from
	explicit ModalDialog( Widget * parent = 0 );

	virtual ~ModalDialog();

	/// Called by default when WM_CLOSE is posted to the dialog
	bool defaultClosing() {
		endDialog(IDCANCEL);
		return true;
	}

	virtual void kill();
private:
	friend class ChainingDispatcher;
	static const TCHAR* windowClass;

	bool quit;
	int ret;

	Application::FilterIter filterIter;
	bool filter(MSG& msg);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ModalDialog::endDialog(int retv) {
	quit = true;
	ret = retv;
}

}

#endif
