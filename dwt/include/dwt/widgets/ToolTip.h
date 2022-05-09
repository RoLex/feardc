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

#ifndef DWT_ToolTip_H_
#define DWT_ToolTip_H_

#include "../Widget.h"
#include "../aspects/Closeable.h"
#include "../aspects/CustomDraw.h"
#include "../aspects/Enabled.h"
#include "../aspects/Fonts.h"
#include "../aspects/Raw.h"
#include "../aspects/Visible.h"
#include "../util/check.h"

namespace dwt {

class ToolTip :
	public Widget,
	public aspects::Closeable<ToolTip>,
	public aspects::CustomDraw<ToolTip, NMTTCUSTOMDRAW>,
	public aspects::Enabled<ToolTip>,
	public aspects::Fonts<ToolTip>,
	public aspects::Raw<ToolTip>,
	public aspects::Visible<ToolTip>
{
	typedef Widget BaseType;
	friend class WidgetCreator<ToolTip>;

	typedef std::function<void (tstring&)> F;

public:
	/// Class type
	typedef ToolTip ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		/// Fills with default parameters
		Seed();
	};

	void relayEvent(const MSG& msg);

	void setText(const tstring& text_);
	void setText(Widget* widget, const tstring& text);
	void addTool(Widget* widget, LPTSTR text = LPSTR_TEXTCALLBACK);
	void setTool(Widget* widget, F callback);

	void setMaxTipWidth(int width);
	/** get a delay value. see the TTM_GETDELAYTIME doc for possible params. */
	int getDelay(int param);
	/** set a delay value. see the TTM_GETDELAYTIME doc for possible params. use -1 for default. */
	void setDelay(int param, int delay);

	void setActive(bool b);
	void refresh();

	void onGetTip(F f);

	/** Create the ToolTip control.
	@note Be careful about tooltip destruction! Unlike other widgets, tooltips aren't regular
	children; they are instead "owned" by their parent (Windows terminology). Therefore, unless the
	owner doesn't have any non-owner parent, Windows won't destroy the tooltip control when the
	owner is destroyed; so make sure you explicitly destroy it by calling the "close" method. */
	void create(const Seed& cs = Seed());

protected:
	// Constructor Taking pointer to parent
	explicit ToolTip( Widget * parent );

	// To assure nobody accidentally deletes any heaped object of this type, parent
	// is supposed to do so when parent is killed...
	virtual ~ToolTip()
	{}

	tstring text;

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];
};

inline ToolTip::ToolTip(Widget *parent)
	: BaseType(parent, ChainingDispatcher::superClass<ToolTip>())
{
	dwtassert(parent, "Can't have a ToolTip without a parent...");
}

inline void ToolTip::setMaxTipWidth(int width) {
	sendMessage(TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(width));
}

}

#endif /*ToolTip_H_*/
