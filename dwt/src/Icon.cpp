/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2025, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

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

#include <dwt/resources/Icon.h>
#include <dwt/resources/Bitmap.h>
#include <dwt/DWTException.h>

namespace dwt {

Icon::Icon(HICON icon, bool own) :
ResourceType(icon, own),
resId(0)
{
}

/*
* we use ::LoadImage instead of ::LoadIcon in order to be able to pick up the correct image,
* depending on the "size" argument. also, our call to ::LoadImage should use LR_SHARED to match
* ::LoadIcon more closely, but we don't pass that flag since all our icons are managed and
* destroyed by DWT.
*/

Icon::Icon(const unsigned resourceId, const Point& size) :
ResourceType((HICON)::LoadImage(::GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), IMAGE_ICON, size.x, size.y, LR_DEFAULTCOLOR)),
resId(resourceId)
{
	if(!handle()) {
		throw Win32Exception("Failed to create an icon from a resource");
	}
}

Icon::Icon(const tstring& filePath, const Point& size) :
ResourceType((HICON)::LoadImage(0, filePath.c_str(), IMAGE_ICON, size.x, size.y, LR_DEFAULTCOLOR | LR_LOADFROMFILE)),
resId(0)
{
	if(!handle()) {
		throw Win32Exception("Failed to create an icon from a file");
	}
}

Point Icon::getSize() const {
	ICONINFO ii;
	if(!::GetIconInfo(handle(), &ii))
		throw Win32Exception("GetIconInfo in Icon::getSize failed");

	// wrap these in Bitmap so they get destroyed properly
	Bitmap mask(ii.hbmMask);
	Bitmap color(ii.hbmColor);

	return color.getSize();
}

bool Icon::operator==(const Icon& rhs) const {
	if(resId && rhs.resId)
		return resId == rhs.resId;

	return HandleType() == HandleType(rhs);
}

}
