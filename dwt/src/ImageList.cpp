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

#include <dwt/resources/ImageList.h>
#include <dwt/DWTException.h>
#include <dwt/util/check.h>

namespace dwt {

ImageList::ImageList( HIMAGELIST imageList, bool own )
	: ResourceType( imageList, own )
{}

ImageList::ImageList(const Point& size, unsigned flags) :
	ImageList(ImageList_Create(size.x, size.y, flags, 0, 1))
{
	if( handle() == NULL ) {
		throw Win32Exception("Couldn't create ImageList");
	}
}

void ImageList::add( const Bitmap & bitmap )
{
	if(ImageList_Add( handle(), bitmap.handle(), NULL ) == -1) {
		dwtWin32DebugFail("Add failed");
	}
}

void ImageList::add( const Bitmap & bitmap, const Bitmap & mask )
{
	if(ImageList_Add( handle(), bitmap.handle(), mask.handle() ) == -1) {
		dwtWin32DebugFail("Add masked failed");
	}
}

void ImageList::add( const Bitmap& bitmap, COLORREF mask) {
	if(ImageList_AddMasked(handle(), bitmap.handle(), mask) == -1) {
		dwtWin32DebugFail("Add colormasked failed");
	}
}

void ImageList::add( const Icon & icon ) {
	if(ImageList_AddIcon( handle(), icon.handle() ) == -1) {
		dwtWin32DebugFail("Add icon failed");
	}
}

void ImageList::add(const ImageList& imageList, int image) {
	add(*imageList.getIcon(image));
}

void ImageList::add( const ImageList& imageList )
{
	int initialSize = size();
	try {
		int images = imageList.size();
		for(int i = 0; i < images; ++i) {
			add(imageList, i);
		}
	} catch(...) {
		ImageList_SetImageCount(handle(), initialSize);
		throw;
	}
}

Point ImageList::getImageSize() const
{
	int x, y;
	BOOL success = ImageList_GetIconSize( handle(), & x, & y );
	if ( !success ) {
		throw Win32Exception("Couldn't get the ImageList image size");
	}
	return Point( x, y );
}

bool ImageList::empty() const {
	return size() == 0;
}

size_t ImageList::size() const {
	return ImageList_GetImageCount(handle());
}

IconPtr ImageList::getIcon(unsigned i) const {
	return IconPtr(new Icon(ImageList_GetIcon(handle(), i, ILD_TRANSPARENT)));
}

void ImageList::setBkColor(COLORREF color) {
	ImageList_SetBkColor(handle(), color);
}

COLORREF ImageList::getBkColor() const {
	return ImageList_GetBkColor(handle());
}

void ImageList::resize( unsigned newSize )
{
	BOOL success = ::ImageList_SetImageCount( handle(), newSize );
	if ( !success ) {
		throw Win32Exception("Couldn't resize ImageList");
	}
}

}
