/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

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

#ifndef DWT_COMPOSITE_H_
#define DWT_COMPOSITE_H_

#include "../aspects/Caption.h"
#include "../aspects/Children.h"
#include "../aspects/EraseBackground.h"
#include "../resources/Icon.h"
#include "../WidgetCreator.h"
#include "Control.h"

namespace dwt {

class Composite :
	public Control,
	public aspects::Caption<Composite>,
	public aspects::Children<Composite>,
	public aspects::EraseBackground<Composite>
{
	typedef Control BaseType;

public:
	typedef Composite ThisType;

	typedef ThisType* ObjectType;

protected:
	friend class WidgetCreator<Composite>;

	struct Seed : public BaseType::Seed {
		Seed(const tstring& caption, DWORD style, DWORD exStyle);
	};

	Composite(Widget* parent, Dispatcher& dispatcher) : BaseType(parent, dispatcher) { };
};

inline Composite::Seed::Seed(const tstring& caption, DWORD style, DWORD exStyle) :
BaseType::Seed(style | WS_CLIPCHILDREN, exStyle, caption)
{
}

}

#endif /*COMPOSITE_H_*/
