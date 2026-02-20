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

#ifndef DWT_Icon_h
#define DWT_Icon_h

#include "../WindowsHeaders.h"
#include "../forward.h"
#include "../Point.h"
#include "../tstring.h"
#include "Handle.h"

namespace dwt {

struct IconPolicy : public NullPolicy<HICON> {
	void release(HICON h) { ::DestroyIcon(h); }
};

/// Class encapsulating an HICON and ensuring that the contained HICON is freed
/// upon destruction of this object
/** Use this class if you need RAII semantics encapsulating an HICON
  */
class Icon : public Handle<IconPolicy>
{
public:
	Icon(HICON icon, bool own = true);

	/** Construct an icon from a resource id.
	 * @param size desired size, useful to pick up the correct image when the icon contains
	 * multiple images. if 0, the system will figure out the size of the 1st image by itself.
	 */
	Icon(const unsigned resourceId, const Point& size = Point(0, 0));

	/** Construct an icon from a file.
	 * @param size desired size, useful to pick up the correct image when the icon contains
	 * multiple images. if 0, the system will figure out the size of the 1st image by itself.
	 */
	Icon(const tstring& filePath, const Point& size = Point(0, 0));

	/**
	* get the size of the icon, in pixels. note: icons can contain multiple images with different
	* sizes; this will only give you the size of the first image! hence this function is useful
	* when working with dynamically created icons, but you should be very careful when using it
	* with icons loaded from resource.
	*/
	Point getSize() const;

	bool operator==(const Icon& rhs) const;

private:
	friend class Handle<IconPolicy>;
	typedef Handle<IconPolicy> ResourceType;

	const unsigned resId; // store the resource id to facilitate the comparison in operator==
};

}

#endif
