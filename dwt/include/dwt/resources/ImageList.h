/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2022, Jacek Sieka

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

#ifndef DWT_ImageList_h
#define DWT_ImageList_h

#include "../WindowsHeaders.h"
#include "../forward.h"
#include "../tstring.h"
#include "../Point.h"
#include "Bitmap.h"
#include "Icon.h"
#include "Handle.h"

namespace dwt {

/// Class encapsulating an HIMAGELIST and ensuring that the contained HIMAGELIST is
/// freed upon destruction of this object
/** Use this class if you need RAII semantics encapsulating an HIMAGELIST. <br>
  * You can use an ImageList on e.g. a Data Grid.
  */
struct ImageListPolicy : public NullPolicy<HIMAGELIST> {
	void release(HIMAGELIST h) { ImageList_Destroy(h); }
};

class ImageList : public Handle<ImageListPolicy>
{
public:
	/// RAII Constructor taking a HIMAGELIST
	/** Note! <br>
	  * The "own" parameter specifies if the class takes "control" of
	  * HIMAGELIST meaning it will automatically free the
	  * contained HIMAGELIST upon destruction
	  */
	ImageList( HIMAGELIST imageList, bool own = true );

	/// RAII Constructor
	/** Creates a new ImageList <br>
	  * The width and height refer to the image size of the image list elements. <br>
	  * The flags parameter specifies the bit depth and if there is a mask for the
	  * images on the list. e.g. (ILC_COLOR32 | ILC_MASK) [32 bits with mask];
	  * (ILC_COLOR16) [16 bits, no mask].
	  */
	ImageList(const Point& size, unsigned flags = ILC_COLOR32 | ILC_MASK);

	/// Deprecated, use handle()
	HIMAGELIST getImageList() const;

	/// Add one or more images to the list
	void add( const Bitmap & bitmap );

	/// Add one or more masked images to the list
	/** Note, you need to set the flag ILC_MASK on the constructor for this to work.
	  * <br>
	  * The mask is a black and white image; where it's black, the shown image will
	  * be transparent.
	  */
	void add( const Bitmap & bitmap, const Bitmap & mask );

	/// Add one or more masked images to the list, calculating the mask from a specific color
	/** Note, you need to set the flag ILC_MASK on the constructor for this to work.
	  * <br>
	  * The mask is color; where the bitmap has that color, the shown image will
	  * be transparent. <br>
	  * The bitmap parameter specifies a bitmap with several images. The
	  * number of images is calculated by the bitmap width, e.g. if the image list is
	  * 32x32 and you add a 128x32 bitmap, it will add four 32x32 bitmaps (note, the
	  * height must be the same of the image list) Returns the number of images
	  * added.
	  */
	void add(const Bitmap& bitmap, COLORREF mask);

	/// Add an icon to the list
	/** Note, you need to set the flag ILC_MASK on the constructor to support
	  * transparency. <br>
	  * If the icon has multiple sizes, it will choose the size specified in the
	  * constructor.
	  */
	void add( const Icon & icon );

	/// Add a single image form another imagelist
	void add(const ImageList& imageList, int image);

	/// Add all images from another imagelist
	void add( const ImageList& imageList );

	/// Returns the image size.
	/** Returns the size of the images in the Image List (all images in the Image
	  * List have the same size)
	  */
	Point getImageSize() const;

	IconPtr getIcon(unsigned i) const;

	void setBkColor(COLORREF color);

	COLORREF getBkColor() const;

	bool empty() const;
	size_t size() const;

private:
	friend class Handle<ImageListPolicy>;
	typedef Handle<ImageListPolicy> ResourceType;

	// Resize underlying image list, helper method just to reuse code
	void resize( unsigned newSize );

	// Add a bitmap with multiple images, helper method
	unsigned int addMultiple( int count, HBITMAP bitmap, HBITMAP mask );

	unsigned int addMultiple( int count, HBITMAP bitmap, COLORREF mask );
};

inline HIMAGELIST ImageList::getImageList() const
{
	return handle();
}

}

#endif
