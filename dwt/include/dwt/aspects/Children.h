/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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


#ifndef DWT_ASPECTS_CHILDREN_H
#define DWT_ASPECTS_CHILDREN_H

#include <utility>

#include "../forward.h"
#include "../WidgetCreator.h"

#include <boost/iterator/iterator_facade.hpp>

namespace dwt { namespace aspects {

template<typename WidgetType>
class Children {
	WidgetType& W() { return *static_cast<WidgetType*>(this); }
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }

public:
	template<typename ChildWidget>
	class ChildIterator : public boost::iterator_facade<ChildIterator<ChildWidget>, ChildWidget*, boost::forward_traversal_tag, ChildWidget*> {
	public:
		typedef ChildWidget* value_type;

		ChildIterator() : cur(0) { }
		explicit ChildIterator(Widget* start) : cur(start) { }
		static ChildIterator first(Widget *parent) { return ChildIterator(next(parent, 0)); }

	private:
		friend class boost::iterator_core_access;

		void increment() { cur = next(cur->getParent(), cur); }

	    bool equal(const ChildIterator& other) const {
	        return this->cur == other.cur;
	    }

	    value_type dereference() const { return static_cast<value_type>(cur); }

	    static Widget* next(Widget *parent, Widget *child) {
	    	do {
				child = hwnd_cast<Widget*>(::FindWindowEx(parent->handle(), child ? child->handle() : NULL, NULL, NULL));
			} while(child && !dynamic_cast<ChildWidget*>(child));

	    	return child;
	    }

	    Widget* cur;
	};

	template<typename SeedType>
	typename SeedType::WidgetType::ObjectType addChild(const SeedType& seed) {
		return WidgetCreator<typename SeedType::WidgetType>::create(static_cast<WidgetType*>(this), seed);
	}

	template<typename T>
	std::pair<ChildIterator<T>, ChildIterator<T> > getChildren() {
		return std::make_pair(ChildIterator<T>::first(&W()), ChildIterator<T>());
	}

	void removeChild(Widget *w) {
		::DestroyWindow(w->handle());
	}
};

} }

#endif
