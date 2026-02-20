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

#include <dwt/widgets/DateTime.h>
#include <dwt/util/TimeSpan.h>

namespace dwt {

const TCHAR DateTime::windowClass[] = DATETIMEPICK_CLASS;

DateTime::Seed::Seed() :
	BaseType::Seed(WS_CHILD | DTS_SHORTDATEFORMAT),
	font(0),
	format(_T( "yyyy.MM.dd" )),
	backgroundColor(0x000080),
	monthBackgroundColor(0x808080),
	monthTextColor(0xFFFFFF),
	titleBackgroundColor(0x202020),
	titleTextColor(0x008080),
	trailingTextColor(0x000000)
{
	::GetSystemTime( &initialDateTime );
}

void DateTime::create( const Seed & cs )
{
	ControlType::create(cs);
	setFont(cs.font);
	setFormat( cs.format );
	setDateTime( cs.initialDateTime );
	setBackgroundColor( cs.backgroundColor );
	setMonthBackgroundColor( cs.monthBackgroundColor );
	setMonthTextColor( cs.monthTextColor );
	setTitleBackgroundColor( cs.titleBackgroundColor );
	setTrailingTextColor( cs.trailingTextColor );
	setTitleTextColor( cs.titleTextColor );
}

}
