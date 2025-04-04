/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2024, Jacek Sieka

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

#ifndef DWT_WidgetCreator_h
#define DWT_WidgetCreator_h

#include "Widget.h"

namespace dwt {

/// Helper creational class
/** Since all Widgets in the SmartWin core have protected constructors this is the
  * only way to actually create widgets apart from the other Factory class unless you
  * derive from Widgets. Use this class and its functions to explicitly create
  * Widgets. By using this class you're guaranteed that all widgets are fully created
  * and not "semi half" created since it calls "create" on the objects after calling
  * the constructor
  */
template< class WidgetType >
class WidgetCreator
{
public:
	static typename WidgetType::ObjectType create( const typename WidgetType::Seed & cs )
	{
		typename WidgetType::ObjectType retVal(new WidgetType);
		retVal->create( cs );
		return retVal;
	}

	static typename WidgetType::ObjectType create( Widget * parent, const typename WidgetType::Seed & cs )
	{
		typename WidgetType::ObjectType retVal(new WidgetType( parent ));
		retVal->create( cs );
		return retVal;
	}

	template< class ContainerType >
	static typename WidgetType::ObjectType create( Widget * parent, ContainerType * container, const typename WidgetType::Seed & cs )
	{
		typename WidgetType::ObjectType retVal(new WidgetType( parent ));
		retVal->create( container, cs );
		return retVal;
	}

	static typename WidgetType::ObjectType create( Widget * parent )
	{
		typename WidgetType::ObjectType retVal( new WidgetType( parent ) );
		retVal->create();
		return retVal;
	}

	/** Attach to an existing window. */
	static typename WidgetType::ObjectType attach(Widget* parent, HWND hwnd)
	{
		typename WidgetType::ObjectType w(new WidgetType(parent));
		w->setHandle(hwnd);
		return w;
	}
};

}

#endif
