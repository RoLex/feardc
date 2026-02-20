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

#ifndef DWT_Bitmap_h
#define DWT_Bitmap_h

#include "../WindowsHeaders.h"
#include "../forward.h"
#include "../tstring.h"
#include "Handle.h"

namespace dwt {

/// Class encapsulating an HBITMAP and ensuring that the contained HBITMAP is freed
/// upon destruction of this object
/** Use this class if you need RAII semantics encapsulating an HBITMAP
  */
class Bitmap : public Handle<GdiPolicy<HBITMAP> >
{
public:
	/// RAII Constructor taking a HBITMAP
	/** Note! <br>
	  * Class takes "control" of HBITMAP meaning it will automatically free the
	  * contained HBITMAP upon destruction
	  */
	Bitmap( HBITMAP bitmap, bool own = true);

	/// RAII Constructor loading a bitmap from a resource ID
	/** Note! <br>
	  * Class takes "control" of HBITMAP meaning it will automatically free the
	  * contained HBITMAP upon destruction
	  */
	Bitmap( unsigned resourceId, unsigned flags = LR_CREATEDIBSECTION );

	/// RAII Constructor loading a bitmap from a file on disc
	/** Note! <br>
	  * Class takes "control" of HBITMAP meaning it will automatically free the
	  * contained HBITMAP upon destruction <br>
	  * Note! <br>
	  * The Pocket PC (SmartPhone, WindowsMobile, Windows CE) version of this
	  * function support loading of several different types of images:
	  * <ul>
	  * <li>Bitmap (BMP)</li>
	  * <li>GIF</li>
	  * <li>PNG</li>
	  * <li>JPG (also known as JPEG)</li>
	  * <li>Icons (ICO)</li>
	  * </ul>
	  */
	Bitmap( const tstring & filePath, unsigned flags = LR_CREATEDIBSECTION );

	/// @deprecated, use handle
	HBITMAP getBitmap() const;

	/// Returns the Bitmaps size
	/** Returns the size (width = x, height = y) in pixels of the Bitmap
	  */
	Point getSize() const;

	/// Returns the given HBITMAP size
	/** Returns the size (width = x, height = y) in pixels of the HBITMAP
	  */
	static Point getSize( HBITMAP Bitmap );

	/// Resize the given Bitmap
	/** This function is IMMUTABLE meaning it will not modify the given bitmap at all
	  * but rather return a new Bitmap containing the resized Bitmap.
	  */
	BitmapPtr resize(const Point& newSize) const;

private:
	friend class Handle<GdiPolicy<HBITMAP> >;
	typedef Handle<GdiPolicy<HBITMAP> > ResourceType;
};

}

#endif
