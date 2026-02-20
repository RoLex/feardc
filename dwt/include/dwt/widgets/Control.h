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

#ifndef DWT_CONTROL_H_
#define DWT_CONTROL_H_

#include "../Widget.h"

#include "../aspects/Border.h"
#include "../aspects/Closeable.h"
#include "../aspects/Colorable.h"
#include "../aspects/Command.h"
#include "../aspects/ContextMenu.h"
#include "../aspects/DragDrop.h"
#include "../aspects/Enabled.h"
#include "../aspects/Fonts.h"
#include "../aspects/Help.h"
#include "../aspects/Keyboard.h"
#include "../aspects/Mouse.h"
#include "../aspects/Painting.h"
#include "../aspects/Raw.h"
#include "../aspects/Sizable.h"
#include "../aspects/Timer.h"
#include "../aspects/Visible.h"

namespace dwt {

/** Base class for all windows */
class Control:
	public Widget,

	public aspects::Border<Control>,
	public aspects::Closeable<Control>,
	public aspects::Colorable<Control>,
	public aspects::Command<Control>,
	public aspects::ContextMenu<Control>,
	public aspects::DragDrop<Control>,
	public aspects::Enabled<Control>,
	public aspects::Fonts<Control>,
	public aspects::Help<Control>,
	public aspects::Keyboard<Control>,
	public aspects::Mouse<Control>,
	public aspects::Painting<Control>,
	public aspects::Raw<Control>,
	public aspects::Sizable<Control>,
	public aspects::Timer<Control>,
	public aspects::Visible<Control>
{
	typedef Widget BaseType;

public:
	/**
	* add a combination of keys that will launch a function when they are hit. see the ACCEL
	* structure doc for information about the "fVirt" and "key" arguments.
	* FVIRTKEY is always added to fVirt.
	*
	* may be called before calling create, in which case accels will be automatically initiated;
	* or they can be initialized later on using the initAccels function.
	*/
	void addAccel(BYTE fVirt, WORD key, const CommandDispatcher::F& f);
	void initAccels();

	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

protected:
	struct Seed : public BaseType::Seed {
		Seed(DWORD style, DWORD exStyle = 0, const tstring& caption = tstring());
	};

	typedef Control ControlType;

	Control(Widget* parent, Dispatcher& dispatcher);
	void create(const Seed& seed);

	virtual ~Control();

private:
	friend class Application;

	std::vector<ACCEL> accels;
	HACCEL accel;
	static const unsigned id_offset = 100; // better play safe and avoid 0-based ids...

	bool filter(MSG& msg);
};

typedef Control CommonControl;

}

#endif /*CONTROL_H_*/
