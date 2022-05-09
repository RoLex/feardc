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

#ifndef DWT_SPLITTER_H
#define DWT_SPLITTER_H

#include <boost/optional.hpp>

#include <dwt/forward.h>
#include "Control.h"
#include <dwt/Theme.h>

namespace dwt {

class Splitter :
	public Control
{
	typedef Control BaseType;

	friend class WidgetCreator<Splitter>;
	friend class SplitterContainer;

public:
	/// Class type
	typedef Splitter ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		double pos;
		bool horizontal;

		explicit Seed(double pos_ = 0.5, bool horizontal = false);
	};

	bool isHorizontal() const {
		return horizontal;
	}

	double getRelativePos() const {
		return pos;
	}

	void setRelativePos(double pos_) {
		pos = pos_;
	}

	void create(const Seed& cs = Seed());

	SplitterContainerPtr getParent() const;

	virtual Point getPreferredSize() { return horizontal ? Point(0, thickness()) : Point(thickness(), 0); }

protected:
	explicit Splitter(Widget* parent);

	virtual ~Splitter() { }

private:
	Theme theme;

	double pos;
	bool horizontal;

	bool hovering;
	boost::optional<Point> moving; // store the last point to only handle actual moves

	void handlePainting(PaintCanvas& canvas);

	bool handleLButtonDown(const MouseEvent& mouseEvent);
	bool handleLButtonUp();
	bool handleMouseMove(const MouseEvent& mouseEvent);

	int thickness() { return ::GetSystemMetrics(horizontal ? SM_CYEDGE : SM_CXEDGE) + 2; }
};

inline Splitter::Seed::Seed(double pos_, bool horizontal) :
BaseType::Seed(WS_CHILD, 0),
pos(pos_), horizontal(horizontal)
{
}

inline Splitter::Splitter(Widget* parent) :
BaseType(parent, NormalDispatcher::newClass<ThisType>(0, 0, NULL)),
pos(0.5),
horizontal(false),
hovering(false)
{
}

}

#endif
