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

#ifndef DWT_DISPATCHERS_H_
#define DWT_DISPATCHERS_H_

#include "WindowsHeaders.h"
#include <functional>

namespace dwt {

namespace Dispatchers {

template<typename T>
struct Base {
	typedef std::function<T> F;

	Base(const F& f_) : f(f_) { }

protected:
	F f;
};

template<typename T>
T convert(const MSG& msg) {
	return T(msg);
}

template<typename P, P (*C)(const MSG&) = convert<P>, LRESULT value = 0, bool handled = true >
class ConvertBase : public Base<void(const P&)> {
	typedef Base<void(const P&)> BaseType;
public:
	ConvertBase(const typename BaseType::F& f_) : BaseType(f_) { }

	bool operator()(const MSG& msg, LRESULT& ret) const {
		this->f((*C)(msg));
		ret = value;
		return handled;
	}
};

template<LRESULT value = 0, bool handled = true>
struct VoidVoid : public Base<void ()> {

	VoidVoid(const F& f_) : Base<void ()>(f_) { }

	bool operator()(const MSG& msg, LRESULT& ret) const {
		this->f();
		ret = value;
		return handled;
	}
};

}

}

#endif /*DISPATCHERS_H_*/
