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

#include <dwt/widgets/SplitterContainer.h>

#include <dwt/widgets/Splitter.h>
#include <dwt/util/HoldResize.h>

#include <boost/next_prior.hpp>
#include <boost/range/distance.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/for_each.hpp>

namespace dwt {

namespace {
	bool isSplitter(Widget *w) { return dynamic_cast<Splitter*>(w); }
	bool isNotSplitter(Widget *w) { return !isSplitter(w); }
}

using boost::distance;
using boost::for_each;
using boost::next;
using boost::adaptors::filtered;

SplitterContainer::Seed::Seed(double startPos, bool horizontal) :
BaseType::Seed(0, WS_EX_CONTROLPARENT),
startPos(startPos),
horizontal(horizontal)
{
}

void SplitterContainer::create(const Seed& cs) {
	horizontal = cs.horizontal;
	startPos = cs.startPos;
	BaseType::create(cs);
}

void SplitterContainer::setSplitter(size_t n, double pos) {
	auto ns = ensureSplitters();
	if(n >= ns) {
		return;
	}

	auto splitter = *next(getChildren<Splitter>().first, n);
	splitter->setRelativePos(pos);
}

double SplitterContainer::getSplitterPos(size_t n) {
	auto ns = ensureSplitters();
	if(n >= ns) {
		return startPos;
	}

	auto splitter = *next(getChildren<Splitter>().first, n);
	return splitter->getRelativePos();
}

void SplitterContainer::layout() {
	ensureSplitters();

	auto children = getChildren<Widget>();
	auto splitters = getChildren<Splitter>();

	auto rc = Rectangle(getClientSize());
	util::HoldResize hr(this, distance(children));

	if(maximized) {
		for_each(children, [&](Widget *w) {
			if(w == maximized) {
				hr.resize(w, rc);
			} else {
				hr.resize(w, Rectangle());
			}
		});

		return;
	}

	auto &pos = horizontal ? rc.pos.y : rc.pos.x;
	auto &size = horizontal ? rc.size.y : rc.size.x;

	auto avail = size;

	for_each(splitters, [&](Splitter *w) {
		avail -= horizontal ? w->getPreferredSize().y : w->getPreferredSize().x;
	});

	auto splitter_iter = splitters.first;

	auto nc = distance(children | filtered(&isNotSplitter));
	auto i = 0;
	for_each(children | filtered(&isNotSplitter), [&](Widget *w) {
		if(i++ == nc-1) {
			// Last child - give it any remaining space
			size = std::max(avail - pos, 0l);
			hr.resize(w, rc);
		} else {
			auto splitter = *splitter_iter;
			auto ss = horizontal ? splitter->getPreferredSize().y : splitter->getPreferredSize().x;
			size = std::max(avail * splitter->getRelativePos() - ss / 2. - pos, 0.);
			hr.resize(w, rc);

			pos += size;

			size = ss;
			hr.resize(splitter, rc);
			pos += size;
			splitter_iter++;
		}
	});

	for(auto s = splitter_iter; s != splitters.second; ++s) {
		hr.resize(*s, Rectangle());
	}
}

size_t SplitterContainer::ensureSplitters() {
	auto children = getChildren<Widget>();
	auto nc = 0, ns = 0;
	for_each(children, [&](Widget *w) { isSplitter(w) ? ns++ : nc++; });

	while(ns < nc - 1) {
		addChild(Splitter::Seed(startPos, horizontal));
		ns++;
	}

	return ns;
}

void SplitterContainer::checkSplitterPos(SplitterPtr splitter) {
	/* ideally we'd just use find & iter-- to get the previous splitter, but the child enumeration
	doesn't support going backwards. */
	SplitterPtr last = nullptr, prev = nullptr, next = nullptr;
	for_each(getChildren<Splitter>(), [&](Splitter* w) {
		if(w == splitter) {
			prev = last;
		} else if(last == splitter) {
			next = w;
		}
		last = w;
	});

	auto& pos = splitter->pos;
	pos = std::max(pos, prev ? prev->pos : 0.);
	pos = std::min(pos, next ? next->pos : 1.);
}

void SplitterContainer::onMove() {
	layout();
	redraw(true);
}

void SplitterContainer::maximize(Widget* w) {
	if(w != maximized) {
		maximized = w;
		layout();
	}
}

}
