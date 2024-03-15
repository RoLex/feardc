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

#ifndef DWT_HEADER_H_
#define DWT_HEADER_H_

#include "Control.h"

#include "../aspects/Collection.h"
#include "../aspects/Data.h"

namespace dwt {

/** Header control like the one used for Table */
class Header :
	public CommonControl,
	public aspects::Collection<Header, int>,
	public aspects::Data<Header, int>
{
	typedef CommonControl BaseType;

	friend class WidgetCreator<Header>;
	friend class aspects::Collection<Header, int>;
	friend class aspects::Data<Header, int>;
public:
	/// Class type
	typedef Header ThisType;

	/// Object type
	typedef ThisType* ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		/// Fills with default parameters
		Seed();
	};

	/// Actually creates the Header
	void create( const Seed & cs = Seed() );

	int insert(const tstring& header, int width, LPARAM lParam = 0, int after = -1);

	int getWidth(int i) const;

	virtual Point getPreferredSize();

protected:
	/// Constructor Taking pointer to parent
	explicit Header( Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~Header()
	{}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	// aspects::Collection
	void eraseImpl(int row);
	void clearImpl();
	size_t sizeImpl() const;

	// aspects::Data
	int findDataImpl(LPARAM data, int start = -1);
	LPARAM getDataImpl(int idx);
	void setDataImpl(int i, LPARAM data);

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline Header::Header( Widget * parent )
	: BaseType(parent, ChainingDispatcher::superClass<Header>())
{
}

inline void Header::eraseImpl( int row ) {
	Header_DeleteItem(handle(), row);
}

inline size_t Header::sizeImpl() const {
	return Header_GetItemCount(handle());
}

inline void Header::clearImpl() {
	for(size_t i = 0, iend = size(); i < iend; ++i) {
		erase(0);
	}
}

}

#endif /* HEADER_H_ */
