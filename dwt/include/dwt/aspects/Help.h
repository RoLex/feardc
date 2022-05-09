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

#ifndef DWT_ASPECTHELP_H_
#define DWT_ASPECTHELP_H_

#include "../Message.h"
#include <functional>

namespace dwt { namespace aspects {

template<typename WidgetType>
class Help {
	const WidgetType& W() const { return *static_cast<const WidgetType*>(this); }
	WidgetType& W() { return *static_cast<WidgetType*>(this); }

	HWND H() const { return W().handle(); }

	typedef std::function<void (unsigned&)> id_function_type;

public:
	unsigned getHelpId() {
		unsigned ret = ::GetWindowContextHelpId(H());
		if(!ret) {
			WidgetType* parent = dynamic_cast<WidgetType*>(W().getParent());
			if(parent)
				ret = parent->getHelpId();
		}
		helpImpl(ret);
		if(id_function)
			id_function(ret);
		return ret;
	}

	void setHelpId(unsigned id) {
		::SetWindowContextHelpId(H(), id);
	}

	/** set a callback function that can modify the id returned by getHelpId. */
	void setHelpId(id_function_type f) {
		id_function = f;
	}

	void onHelp(std::function<void (WidgetType*)> f) {
		W().addCallback(Message(WM_HELP), [f, this](const MSG& msg, LRESULT& ret) -> bool {
			LPHELPINFO lphi = reinterpret_cast<LPHELPINFO>(msg.lParam);
			if(!lphi || lphi->iContextType != HELPINFO_WINDOW)
				return false;

			HWND hWnd = reinterpret_cast<HWND>(lphi->hItemHandle);
			// make sure this handle is ours
			if(hWnd != this->H() && !::IsChild(this->H(), hWnd))
				return false;

			WidgetType* widget = hwnd_cast<WidgetType*>(hWnd);
			if(!widget)
				return false;

			f(widget);

			ret = TRUE;
			return true;
		});
	}

private:
	id_function_type id_function;

	/** implement this in the derived widget in order to change the help id before it is
	dispatched. */
	virtual void helpImpl(unsigned& id) {
		// empty on purpose.
	}
};

} }

#endif /*ASPECTHELP_H_*/
