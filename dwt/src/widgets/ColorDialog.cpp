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

#include <dwt/widgets/ColorDialog.h>

namespace dwt {

bool ColorDialog::open( ColorParams & colorParams, bool basic, bool allowFullOpen ) {
	CHOOSECOLOR cc = { sizeof( CHOOSECOLOR ) };
	cc.hwndOwner = getParentHandle();
	cc.rgbResult = colorParams.itsColor;
	cc.lpCustColors = itsColorParams.itsCustomColors;
	cc.Flags = CC_ANYCOLOR | CC_RGBINIT;
	if ( !basic )
		cc.Flags |= CC_FULLOPEN;
	if ( !allowFullOpen )
		cc.Flags |= CC_PREVENTFULLOPEN;

	if(::ChooseColor( & cc )) {
		colorParams.itsColor = cc.rgbResult;
		return true;
	}
	return false;
}

COLORREF defaultColors[16] = {
	0x00000000, 0x33333333, 0x66666666, 0x99999999,
	0xCCCCCCCC, 0xFFFFFFFF,	0x0000FFFF, 0x000000FF,
	0x0000FF00, 0x00FF0000, 0x00FFFF00,	0x00FF00FF,
	0x002244BB,	0x0000CC99,	0x00AA7700,	0x00997755
};

ColorDialog::ColorParams::ColorParams()
	: itsColor( 0x0000FFFF )
{
	memcpy(itsCustomColors, defaultColors, sizeof(itsCustomColors));
}

ColorDialog::ColorParams::ColorParams( COLORREF defaultColor )
	: itsColor( defaultColor )
{
	memcpy(itsCustomColors, defaultColors, sizeof(itsCustomColors));
}

}
