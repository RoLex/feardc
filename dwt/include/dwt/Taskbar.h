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

#ifndef DWT_TASKBAR_H
#define DWT_TASKBAR_H

#include "WindowsHeaders.h"
#include "forward.h"
#include "resources/Bitmap.h"
#include "resources/Icon.h"
#include <unordered_map>

namespace dwt {

/** provides widgets with the ability to play with the taskbar associated with a main window. */
class Taskbar {
public:
	void initTaskbar(WindowPtr window_);

protected:
	Taskbar();
	virtual ~Taskbar();

	void addToTaskbar(ContainerPtr tab);
	void removeFromTaskbar(ContainerPtr tab);
	void moveOnTaskbar(ContainerPtr tab, ContainerPtr rightNeighbor = 0);
	void setActiveOnTaskbar(ContainerPtr tab);
	void setTaskbarIcon(ContainerPtr tab, const IconPtr& icon);

	ITaskbarList3* taskbar;

private:
	BitmapPtr getBitmap(ContainerPtr tab, LPARAM thumbnailSize);

	/// function called when the user activates a tab using the taskbar.
	virtual void setActive(ContainerPtr) = 0;

	WindowPtr window;
	std::unordered_map<ContainerPtr, FramePtr> tabs;
};

}

#endif
