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

#ifndef DWT_LINK_H
#define DWT_LINK_H

#include "../aspects/Caption.h"
#include "Control.h"

namespace dwt {

/** This control can display text and clickable links. */
class Link :
	public Control,
	public aspects::Caption<Link>
{
	typedef Control BaseType;
	friend class WidgetCreator<Link>;

public:
	typedef Link ThisType;
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		FontPtr font;

		/** @param link whether to treat the initial text as a link of its own, or as a mixed
		link/text (with <a> tags). */
		Seed(const tstring& caption = tstring(), bool link = false, std::function<bool (const tstring&, bool)> linkManager = nullptr);

		std::function<bool (const tstring&, bool)> fpLinkManager;
	};

	void create(const Seed& seed = Seed());

	void setLink(const tstring& link, size_t index = 0);

	Point getPreferredSize();

protected:
	// Constructor Taking pointer to parent
	explicit Link(Widget* parent);

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Link() { }

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];
};

}

#endif
