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

#include <dwt/util/GDI.h>

#include <dwt/CanvasClasses.h>
#include <dwt/resources/Icon.h>
#include <dwt/resources/ImageList.h>
#include <dwt/util/check.h>

namespace dwt { namespace util {

IconPtr merge(const ImageList& icons) {
	const size_t n = icons.size();
	dwtassert(n > 0, "No icons to merge");

	// only 1 icon, just return it back.
	if(n == 1)
		return icons.getIcon(0);

	// merge the 2 first icons.
	ImageListPtr temp(new ImageList(ImageList_Merge(icons.handle(), 0, icons.handle(), 1, 0, 0)));

	// merge the rest.
	for(size_t i = 2; i < n; ++i) {
		temp.reset(new ImageList(ImageList_Merge(temp->handle(), 0, icons.handle(), i, 0, 0)));
	}

	return temp->getIcon(0);
}

const float& dpiFactor() {
	static const float factor = static_cast<float>(UpdateCanvas(reinterpret_cast<HWND>(0)).getDeviceCaps(LOGPIXELSX)) / 96.0;
	return factor;
}

} }
