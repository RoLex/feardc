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

#ifndef DWT_CONTAINER_H_
#define DWT_CONTAINER_H_

#include "Composite.h"

namespace dwt {

class Container :
	public Composite
{
	typedef Composite BaseType;
	friend class WidgetCreator<Container>;
public:
	typedef Container ThisType;

	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		Seed(DWORD style = 0, DWORD exStyle = 0);
	};

	// Use our seed type
	void create( const Seed& cs = Seed() );

	virtual void layout();
protected:
	Container(Widget* parent, Dispatcher& dispatcher = NormalDispatcher::getDefault()) :
		 BaseType(parent, dispatcher) { }
};

inline Container::Seed::Seed(DWORD style, DWORD exStyle) :
BaseType::Seed(tstring(), style | WS_CHILD, exStyle) {
}

inline void Container::create(const Seed& cs) {
	BaseType::create(cs);
	onWindowPosChanged([this] (const Rectangle &) { this->layout(); });
}

inline void Container::layout() {
	// Default layout is to give the first child the whole client area
	auto c = getChildren<Control>();
	if(c.first != c.second) {
		(*c.first)->resize(Rectangle(getClientSize()));
	}
}

}
#endif /*CONTAINER_H_*/
