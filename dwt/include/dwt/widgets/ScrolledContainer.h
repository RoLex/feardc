/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
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

#ifndef DWT_SCROLLEDCONTAINER_H_
#define DWT_SCROLLEDCONTAINER_H_

#include "../aspects/Child.h"

#include "Control.h"

namespace dwt {

class ScrolledContainer :
	public Control,
	// aspects::s
	public aspects::Child<ScrolledContainer>
{
	typedef Control BaseType;

public:
	typedef ScrolledContainer ThisType;

	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(DWORD style = 0, DWORD exStyle = 0);
	};

	// Use our seed type
	void create( const Seed& cs = Seed() );

	virtual Point getPreferredSize();

	/** Layout the widget in the specified rectangle (in client coordinates) */
	virtual void layout();

	/// Returns true if handled, else false
	virtual bool handleMessage(const MSG &msg, LRESULT &retVal);

protected:
	friend class WidgetCreator<ScrolledContainer>;

	ScrolledContainer(Widget* parent, Dispatcher& dispatcher = NormalDispatcher::getDefault()) : BaseType(parent, dispatcher) { };

private:

	void setScrollInfo(int type, int page, int max, int pos = 0);
};

inline ScrolledContainer::Seed::Seed(DWORD style, DWORD exStyle) :
BaseType::Seed(style | WS_CHILD | WS_HSCROLL | WS_VSCROLL, exStyle | WS_EX_CONTROLPARENT)
{
}

}

#endif /* DWT_SCROLLEDCONTAINER_H_ */
