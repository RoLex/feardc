/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

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

#ifndef DWT_SPLITTERCONTAINER_H_
#define DWT_SPLITTERCONTAINER_H_

#include <dwt/forward.h>
#include "Container.h"

namespace dwt {

class SplitterContainer :
	public Container
{
	typedef Container BaseType;

	friend class WidgetCreator<SplitterContainer>;
	friend class Splitter;

public:
	/// Class type
	typedef SplitterContainer ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;
		Seed(double startPos = 0.5, bool horizontal = false);

		double startPos;
		bool horizontal;
	};

	void create(const Seed& cs = Seed());

	void setSplitter(size_t n, double relativePos);
	double getSplitterPos(size_t n);

	void maximize(Widget *w);

	virtual void layout();

private:
	explicit SplitterContainer( Widget * parent ) : BaseType(parent), maximized(0), horizontal(false), startPos(0.5) { }

	size_t ensureSplitters();

	/** Make sure the splitter's position is within acceptable bounds and doesn't step on other
	splitters. */
	void checkSplitterPos(SplitterPtr splitter);

	void onMove();

	Widget *maximized;
	bool horizontal;
	double startPos;
};

}

#endif /* DWT_SPLITTERCONTAINER_H_ */
