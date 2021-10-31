/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#include <boost/range/algorithm/transform.hpp>
#include <dwt/resources/Region.h>

#include <dwt/DWTException.h>
#include <dwt/Point.h>

namespace dwt {

Region::Region(HRGN h, bool own) : ResourceType(h, own) { }

Region::Region(const Rectangle& rect) : Region(::CreateRectRgn(rect.left(), rect.top(), rect.right(), rect.bottom()), true) { }

Region::Region(const std::vector<Point>& points, PolyFillMode mode)
{
	std::vector<POINT> tmp(points.size());
	boost::transform(points, tmp.begin(), [](const Point& pt) { return pt.toPOINT(); });
	init(::CreatePolygonRgn(&tmp[0], tmp.size(), mode), true);
}

RegionPtr Region::transform(const PXFORM pxform) const {
	DWORD bytes = ::GetRegionData(handle(), 0, NULL);
	if(!bytes)
		throw Win32Exception("1st GetRegionData in Region::transform failed");

	std::vector<char> data(bytes);
	if(!::GetRegionData(handle(), bytes, reinterpret_cast<PRGNDATA>(&data[0])))
		throw Win32Exception("2nd GetRegionData in Region::transform failed");

	HRGN transformed = ::ExtCreateRegion(pxform, bytes, reinterpret_cast<PRGNDATA>(&data[0]));
	if(!transformed)
		throw Win32Exception("ExtCreateRegion in Region::transform failed");

	return RegionPtr(new Region(transformed));
}

}
