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

#ifndef DWT_RESOURCE_H_
#define DWT_RESOURCE_H_

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/core/noncopyable.hpp>

namespace dwt {

/** Policy for all handles that have null as NULL_HANDLE */
template<typename H>
struct NullPolicy {
	typedef H HandleType;
	static const H NULL_HANDLE;
	void release(H) { }
};

template<typename H> const H NullPolicy<H>::NULL_HANDLE = NULL;

/** A policy for GDI objects (bitmap, pen, brush etc) */
template<typename H>
struct GdiPolicy : public NullPolicy<H> {
	void release(H h) { ::DeleteObject(h); }
};

template<typename Policy>
class Handle;

/**
 * Ref-counted base class for resources - nothing stops you from using this one the stack as well
 */
template<typename Policy>
class Handle : protected Policy, boost::noncopyable {
public:
	typedef typename Policy::HandleType HandleType;

	operator HandleType() const { return h; }
	HandleType handle() const { return *this; }

protected:
	Handle() : Handle(Policy::NULL_HANDLE, false) { }
	Handle(HandleType h, bool owned = true) : h(h), owned(owned), ref(0) { }

	void init(HandleType h_, bool owned_) {
		h = h_;
		owned = owned_;
	}

	virtual ~Handle() {
		if(owned && h != Policy::NULL_HANDLE)
			this->release(h);
	}

private:
	friend void intrusive_ptr_add_ref(Handle<Policy>* p) { ++p->ref; }
	friend void intrusive_ptr_release(Handle<Policy>* p) { if(--p->ref == 0) { delete p; } }

	HandleType h;
	bool owned;

	long ref;
};

}

#endif /*RESOURCE_H_*/
