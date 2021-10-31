/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2021, Jacek Sieka

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

#ifndef DWT_RichTextBox_h
#define DWT_RichTextBox_h

#include <boost/algorithm/string.hpp>

#include "TextBox.h"
#include "../tstring.h"
#include <richedit.h>

namespace dwt {

/// RichEdit Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html richedit.PNG
  * Class for creating a Rich Edit control. <br>
  * A Rich Edit Control is derived from TextBox and inherits ( mostly ) all
  * properties and member functions of the TextBox Widget. <br>
  * In addition to the TextBox RichTextBox can display colored text,
  * hyperlinks, OLE objects etc. <br>
  * A good example of the difference between those Widgets is the difference between
  * notepad ( TextBox ) and wordpad ( RichTextBox )
  */
class RichTextBox :
	public TextBoxBase
{
	friend class WidgetCreator<RichTextBox>;
	typedef TextBoxBase BaseType;

public:
	/// Class type
	typedef RichTextBox ThisType;

	/// Object type
	typedef ThisType * ObjectType;

	/// Seed class
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed
	{
		typedef RichTextBox::ThisType WidgetType;

		FontPtr font;
		bool scrollBarHorizontallyFlag;
		bool scrollBarVerticallyFlag;
		int events; /// additional events caught by this control; see the EM_SETEVENTMASK doc.

		/// Fills with default parameters
		Seed();
	};

	/// Actually creates the Rich Edit Control
	/** You should call WidgetFactory::createRichTextBox if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create( const Seed & cs = Seed() );

	int charFromPos(const ScreenCoordinate& pt);

	int lineFromPos(const ScreenCoordinate& pt);

	Point posFromChar(int charOffset);

	/** Declared in TextBoxBase. Override to use EM_EXLINEFROMCHAR instead of EM_LINEFROMCHAR. */
	int lineFromChar(int c = -1);

	tstring getSelection() const;

	Point getScrollPos() const;

	void setScrollPos(Point& scrollPos);

	tstring textUnderCursor(const ScreenCoordinate& p, bool includeSpaces = false);

	void setText(const tstring& txt);

	/// Appends text to the richtext box
	/** The txt parameter is the new text to append to the text box.
	  */
	void addText( const std::string & txt );

	/** Append text into the box, keeping the scroll position steady. */
	void addTextSteady(const tstring& txtRaw);

	void findText(tstring const& needle);

	void clearCurrentNeedle();

	static std::string escapeUnicode(const tstring& str);
	/// escape Rich Edit control chars: {, }, and \, as well as \n which becomes \line.
	static tstring rtfEscape(const tstring& str);

	COLORREF getTextColor() const { return textColor; }
	COLORREF getBgColor() const { return bgColor; }

	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

	/** Utility structure to surround "scroll-steady" rich text-box operations; when an instance of
	 * this structure goes out of scope, the scroll position is guaranteed to be kept. */
	struct HoldScroll  {
		HoldScroll(RichTextBox* box);
		~HoldScroll();
		RichTextBox* box;
		Point scrollPos;
		bool scroll;
	};

protected:
	tstring currentNeedle;		// search in chat window
	int currentNeedlePos;		// search in chat window

	// Constructor Taking pointer to parent
	explicit RichTextBox( dwt::Widget * parent );

	// Protected to avoid direct instantiation, you can inherit and use
	// WidgetFactory class which is friend
	virtual ~RichTextBox() { }

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	static Dispatcher& makeDispatcher();

	int fixupLineEndings(tstring::const_iterator begin, tstring::const_iterator end,
		tstring::difference_type ibo) const;

	typedef boost::iterator_range<boost::range_const_iterator<tstring>::type> tstring_range;
	static std::string unicodeEscapeFormatter(const tstring_range& match);
	static tstring rtfEscapeFormatter(const tstring_range& match);

	void setTextA(const std::string& txt);
	void setTextEx(const std::string& txt, DWORD format);

	void updateColors(COLORREF text, COLORREF background, bool updateFont = false);

	// aspects::Colorable
	void setColorImpl(COLORREF text, COLORREF background);

	// aspects::Font
	void setFontImpl();

	COLORREF textColor;
	COLORREF bgColor;
};

// end namespace dwt
}

#endif
