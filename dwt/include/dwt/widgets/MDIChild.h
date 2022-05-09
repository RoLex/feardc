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

#ifndef DWT_MDIChild_h
#define DWT_MDIChild_h

#ifndef WINCE

#include "../tstring.h"
#include "../resources/Icon.h"
#include "MDIParent.h"
#include "Frame.h"

namespace dwt {

/// MDI Child Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html mdi.PNG
  * Class for creating a MDI Child Widget. <br>
  * An MDI Child is a Widget which is kind of like a special case of Window, it
  * exists only for two purposes which is 1. Contained in the MDIParent class
  * and 2. To serve as a container widget for your control widgets. <br>
  * Use either the MDIParent::createMDIChild or inherit from this class and roll
  * your own logic. <br>
  * Related classes: <br>
  * MDIParent
  */
class MDIChild :
	public Composite,

	public aspects::MinMax<MDIChild>

{
	typedef Composite BaseType;
public:

	/// Class type
	typedef MDIChild ThisType;

	/// Object type
	typedef ThisType * ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		bool activate;

		/// Fills with default parameters
		Seed(const tstring& caption = tstring());

	};

	/// Creates a MDIChild Window
	/** This version creates a MessageMapMDIChildWidget to plug into MDIParent
	  * container window.
	  */
	void createMDIChild( const Seed& cs = Seed() );

	void activate();

	MDIParent* getParent() { return static_cast<MDIParent*>(BaseType::getParent()); }
protected:
	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

	// Protected since this Widget we HAVE to inherit from
	explicit MDIChild( Widget * parent );

	virtual ~MDIChild();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline MDIChild::~MDIChild()
{
}

inline MDIChild::MDIChild( Widget * parent )
	: BaseType(parent, NormalDispatcher::getDefault())
{}

}

#endif //! WINCE

#endif
