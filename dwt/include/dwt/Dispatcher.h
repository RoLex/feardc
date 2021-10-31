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

#ifndef DWT_DISPATCHER_H
#define DWT_DISPATCHER_H

#include "WindowsHeaders.h"
#include "tstring.h"
#include <dwt/resources/Icon.h>
#include <typeinfo>

namespace dwt {

class WindowProc {
public:
	static LRESULT CALLBACK initProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static Widget* getInitWidget(const MSG& msg);
	static LRESULT returnUnknown(const MSG& msg);
	static HWND getHandler(const MSG& msg);
};

class Dispatcher {
public:
	virtual LRESULT chain(const MSG& msg) = 0;

	LPCTSTR getClassName() const { return reinterpret_cast<LPCTSTR>(atom); }

	static HCURSOR getDefaultCursor();
	static HBRUSH getDefaultBackground();

	static bool isRegistered(LPCTSTR className);

	virtual ~Dispatcher();

protected:
	friend class std::unique_ptr<Dispatcher>;

	Dispatcher(WNDCLASSEX& cls);
	Dispatcher(LPCTSTR name);

	template<typename T>
	static LPCTSTR className() {
		return className(typeid(T).name());
	}

	static WNDCLASSEX makeWndClass(LPCTSTR name);
	static void fillWndClass(WNDCLASSEX& cls, LPCTSTR name);

private:
	static LPCTSTR className(const std::string& name);

	void registerClass(WNDCLASSEX& cls);

	ATOM atom;

	static std::vector<tstring> classNames;
};

class NormalDispatcher : public Dispatcher {
public:
	static Dispatcher& getDefault();

	template<typename T>
	static Dispatcher& newClass(const IconPtr& icon = 0, const IconPtr& smallIcon = 0,
		HCURSOR cursor = getDefaultCursor(), HBRUSH background = getDefaultBackground())
	{
		WNDCLASSEX cls = makeWndClass(className<T>());
		if(icon)
			cls.hIcon = icon->handle();
		if(smallIcon)
			cls.hIconSm = smallIcon->handle();
		cls.hCursor = cursor;
		cls.hbrBackground = background;

		static std::unique_ptr<Dispatcher> dispatcher(new NormalDispatcher(cls));
		return *dispatcher;
	}

	virtual LRESULT chain(const MSG& msg);

private:
	NormalDispatcher(WNDCLASSEX& cls);
	NormalDispatcher(LPCTSTR name);
};

class ChainingDispatcher : public Dispatcher {
public:
	template<typename T>
	static Dispatcher& superClass() {
		static std::unique_ptr<Dispatcher> dispatcher = superClass(T::windowClass, className<T>());
		return *dispatcher;
	}

	virtual LRESULT chain(const MSG& msg);

private:
	static std::unique_ptr<Dispatcher> superClass(LPCTSTR original, LPCTSTR newName);

	ChainingDispatcher(WNDCLASSEX& cls, WNDPROC wndProc_);

	WNDPROC wndProc;
};

}

#endif
