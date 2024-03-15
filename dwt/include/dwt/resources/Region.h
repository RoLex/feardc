/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#ifndef DWT_REGION_H
#define DWT_REGION_H

#include "../WindowsHeaders.h"
#include "../forward.h"
#include "Handle.h"
#include <dwt/Rectangle.h>

namespace dwt {

class Region : public Handle<GdiPolicy<HRGN> > {
public:
	/**
	* filling mode when multiple polygonal regions intersect.
	* more information in the ::SetPolyFillMode doc.
	*/
	enum PolyFillMode {
		Alternate = ALTERNATE,
		Winding = WINDING
	};

	Region(HRGN h, bool own = true);

	/// create a rectangular region; more information in the ::CreateRectRgn doc.
	Region(const Rectangle& rect);

	/**
	* creates a polygonal region; more information in the ::CreatePolygonRgn doc.
	* @param points coordinates of the vertices of the polygon that this region will represent.
	*/
	Region(const std::vector<Point>& points, PolyFillMode mode = Winding);

	/// @todo make an XFORM creator in dwt to simplify basic transformations
	/**
	* returns a new transformed region; more information in the ::ExtCreateRegion doc.
	* @param pxform pointer to an XFORM structure; check out its doc for more information.
	*/
	RegionPtr transform(const PXFORM pxform) const;

private:
	friend class Handle<GdiPolicy<HRGN> >;
	typedef Handle<GdiPolicy<HRGN> > ResourceType;
};

}

#endif /*DWT_REGION_H*/
