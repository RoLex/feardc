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

#ifndef DWT_TextBox_h
#define DWT_TextBox_h

#include "../aspects/Caption.h"
#include "../aspects/Scrollable.h"
#include "../aspects/Update.h"
#include "Control.h"
#include "Menu.h"

namespace dwt {

/// Text Box Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html textbox.PNG
  * Class for creating a Text Box Control. <br>
  * An Text Box is a window in which you can write and copy, paste into, normally
  * you're favourite text editor which you're developing C++ in would be a Text Box
  * control. ( or perhaps a Rich Edit Control which has some additional features )
  * <br>
  * You can send and retrieve the text contained in the control and do lots of other
  * types of manipulation of the control. <br>
  * Related classes <br>
  * < ul > < li >RichTextBox< /li > < /ul >
  */
class TextBoxBase :
	public CommonControl,
	public aspects::Caption<TextBoxBase>,
	public aspects::Scrollable<TextBoxBase>,
	public aspects::Update<TextBoxBase>
{
	typedef CommonControl BaseType;
	friend class WidgetCreator<TextBoxBase>;
	friend class aspects::Update<TextBoxBase>;

public:
	/// Sets the current selection of the Edit Control
	/** Start means the offset of where the current selection shall start, if it is
	  * omitted it defaults to 0. <br>
	  * end means where it shall end, if it is omitted it defaults to - 1 or "the
	  * rest from start".
	  */
	void setSelection( int start = 0, int end = - 1 );

	/// Replaces the currently selected text in the text box with the given text parameter
	/** If canUndo is true this operation is stacked into the undo que ( can be
	  * undone ), else this operation cannot be undone. <br>
	  * Note! <br>
	  * If there is not currently any selected text, the input text is inserted at
	  * the current location of the caret.
	  */
	void replaceSelection( const tstring & txt, bool canUndo = true );

	/// Finds the given text in the text field and returns true if successfully
	int findText( const tstring & txt, unsigned offset = 0 ) const;

	/// Returns the position of the caret
	int getCaretPos();

	/// Returns the currently selected text offset range
	std::pair<int, int> getCaretPosRange() const;

	/// Return first visible line of edit control
	long getFirstVisibleLine();

	/// Call this function to scroll the caret into view
	/** If the caret is not visible within the currently scrolled in area, the Text
	  * Box will scroll either down or up until the caret is visible.
	  */
	void showCaret();

	/// Adds (or removes) the control horizontal scroll bars
	/** If you pass false you REMOVE the horizontal scrollbars of the control ( if
	  * there are any ) <br>
	  * If you pass true to the function, you ADD horizontal scrollbars. <br>
	  * Value defaults to true!
	  */
	void setScrollBarHorizontally( bool value = true );

	/// Adds (or removes) the control vertical scroll bars
	/** If you pass false you REMOVE the vertical scrollbars of the control ( if
	  * there are any ) <br>
	  * If you pass true to the function, you ADD vertical scrollbars.
	  */
	void setScrollBarVertically( bool value = true );

	/// Adds (or removes) the readonly property
	/** If you pass false you remove this ability <br>
	  * If you pass true or call function without arguments you force the control to
	  * display as a readonly text field.
	  */
	void setReadOnly( bool value = true );

	bool isReadOnly();

	/// Adds (or removes) a border surrounding the control
	/** If you pass false you REMOVE the border of the control ( if there is on )
	  * <br>
	  * If you pass true to the function, you ADD a border.
	  */
	void setBorder( bool value = true );

	/// Set the maximum number of characters that can be entered.
	/** Although this prevents user from entering more maxChars, Paste can overrun the limit.
	  */
	void setTextLimit( int maxChars );

	/// Returns the maximum number of characters that can be entered.
	/** Note that the maxChars returned will vary by OS if left unset.
	  */
	int getTextLimit() const ;

	int lineIndex(int line = -1);

	/** Retrieve the index of the line that contains the character at the specified index. */
	virtual int lineFromChar(int c = -1);

	int lineLength(int c);

	unsigned getLineCount() const;

	virtual int charFromPos(const ScreenCoordinate& pt) = 0;
	virtual int lineFromPos(const ScreenCoordinate& pt) = 0;

	virtual tstring textUnderCursor(const ScreenCoordinate& p, bool includeSpaces = false) = 0;
	virtual tstring getSelection() const = 0;

	void setModify(bool modify = false);

	bool getModify();

	void scrollToBottom();

	ClientCoordinate ptFromPos(int pos);
	ScreenCoordinate getContextMenuPos();

	virtual Point getPreferredSize();

	virtual MenuPtr getMenu();

	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

protected:
	// Constructor Taking pointer to parent
	explicit TextBoxBase(Widget *parent, Dispatcher& dispatcher);

	// To assure nobody accidentally deletes any heaped object of this type, parent
	// is supposed to do so when parent is killed...
	virtual ~TextBoxBase()
	{}

	struct Seed : public BaseType::Seed {
		Seed(DWORD style, DWORD exStyle = 0, const tstring& caption = tstring());

		/// number of lines this control occupies, used to guess its vertical size
		unsigned lines;

		/// seed to use for context menus
		Menu::Seed menuSeed;
	};

	void create(const Seed& cs);

	// aspects::Font
	virtual void setFontImpl();

	// Contract needed by aspects::Update Aspect class
	static Message getUpdateMessage();

private:
	class Dropper;

	unsigned lines;
	Menu::Seed menuSeed;
};

class TextBox :
	public TextBoxBase
{
	typedef TextBoxBase BaseType;

public:
	typedef TextBox ThisType;

	typedef ThisType* ObjectType;

	/// Info for creation
	/** This class contains all of the values needed to create the widget. It also
	  * knows the type of the class whose seed values it contains. Every widget
	  * should define one of these.
	  */
	struct Seed : public BaseType::Seed {
		typedef ThisType WidgetType;

		FontPtr font;

		/// Fills with default parameters
		Seed(const tstring& caption = tstring());
	};

	/// Appends text to the text box
	/** The txt parameter is the new text to append to the text box.
	  */
	void addText( const tstring & txt );

	void setText(const tstring& txt);

	/** Set a "cue banner text", a text that will be displayed in dim color when the control
	doesn't have any text set. Only works for single-line controls. Only available on >= Vista. */
	void setCue(const tstring& text);

	/// Returns the current selected text from the text box
	/** The selected text of the text box is the return value from this.
	  */
	tstring getSelection() const;

	/// Adds (or removes) the numbers property
	/** If you pass false you remove this ability <br>
	  * If you pass true or call function without arguments you force the control to
	  * display only numbers.
	  */
	void setNumbersOnly( bool value = true );

	/// Adds (or removes) the password property
	/** If you pass false you remove this ability <br>
	  * If you pass true or call function without arguments you force the control to
	  * display all its content as "hidden" meaning it will only display e.g. "*"
	  * instead of letters, useful for Text Box Controls which contains passwords or
	  * similar "secret" information.
	  */
	void setPassword( bool value = true, TCHAR pwdChar = '*' );

	/// Adds (or removes) upper case forcing
	/** If you pass false you remove this ability <br>
	  * If you pass true or call function without arguments you force the control to
	  * display all characters in UPPER CASE.
	  */
	void setUpperCase( bool value = true );

	/// Adds (or removes) lower case forcing
	/** If you pass false you remove this ability <br>
	  * If you pass true or call function without arguments you force the control to
	  * display all characters in lower case.
	  */
	void setLowerCase( bool value = true );

	int charFromPos(const ScreenCoordinate& pt);

	int lineFromPos(const ScreenCoordinate& pt);

	tstring getLine(int line);

	tstring textUnderCursor(const ScreenCoordinate& p, bool includeSpaces = false);

	/** Show a balloon popup; see the EM_SHOWBALLOONTIP doc for more info.
	@param icon see the EDITBALLOONTIP doc for possible values. */
	void showPopup(const tstring& title, const tstring& text, int icon);

	/// Actually creates the TextBox
	/** You should call WidgetFactory::createTextBox if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create( const Seed & cs = Seed() );

	virtual bool handleMessage(const MSG& msg, LRESULT& retVal);

protected:
	friend class WidgetCreator< TextBox >;

	// Constructor Taking pointer to parent
	explicit TextBox( Widget * parent );

	// To assure nobody accidentally deletes any heaped object of this type, parent
	// is supposed to do so when parent is killed...
	virtual ~TextBox()
	{}

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	// aspects::Scrollable
	int scrollOffsetImpl() const {
		return 1;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline Message TextBoxBase::getUpdateMessage() {
	return Message( WM_COMMAND, EN_UPDATE );
}

inline void TextBoxBase::setSelection( int start, int end )
{
	this->sendMessage(EM_SETSEL, start, end );
}

inline void TextBoxBase::replaceSelection( const tstring & txt, bool canUndo )
{
	this->sendMessage(EM_REPLACESEL, static_cast< WPARAM >( canUndo ? TRUE : FALSE ), reinterpret_cast< LPARAM >( txt.c_str() ) );
}

inline void TextBox::addText( const tstring & addtxt )
{
	setSelection( length() );
	replaceSelection( addtxt );
}

inline int TextBoxBase::findText( const tstring & txt, unsigned offset ) const
{
	tstring txtOfBox = this->getText();
	size_t position = txtOfBox.find( txt, offset );
	if ( position == std::string::npos )
		return -1;
	return static_cast< int >( position );
}

inline int TextBoxBase::getCaretPos() {
	DWORD start, end;
	this->sendMessage(EM_GETSEL, reinterpret_cast< WPARAM >( & start ), reinterpret_cast< LPARAM >( & end ) );
	return static_cast< int >( end );
}

inline std::pair<int, int> TextBoxBase::getCaretPosRange() const {
	DWORD start, end;
	this->sendMessage(EM_GETSEL, reinterpret_cast< WPARAM >( & start ), reinterpret_cast< LPARAM >( & end ) );
	return std::make_pair(static_cast< int >( start ), static_cast< int >( end ));
}

inline long TextBoxBase::getFirstVisibleLine() {
	return this->sendMessage(EM_GETFIRSTVISIBLELINE, 0, 0);
}

inline void TextBoxBase::showCaret() {
	this->sendMessage(EM_SCROLLCARET);
}

inline void TextBoxBase::setScrollBarHorizontally( bool value ) {
	Widget::addRemoveStyle( WS_HSCROLL, value );
}

inline void TextBoxBase::setScrollBarVertically( bool value ) {
	Widget::addRemoveStyle( WS_VSCROLL, value );
}

inline void TextBoxBase::setReadOnly( bool value ) {
	this->sendMessage(EM_SETREADONLY, static_cast< WPARAM >( value ) );
}

inline bool TextBoxBase::isReadOnly( ) {
	return hasStyle(ES_READONLY);
}

inline void TextBoxBase::setBorder( bool value ) {
	this->Widget::addRemoveStyle( WS_BORDER, value );
}

inline void TextBoxBase::setTextLimit( int maxChars ) {
	this->sendMessage(EM_LIMITTEXT, static_cast< WPARAM >(maxChars) );
}

inline int TextBoxBase::getTextLimit() const {
	return static_cast< int >( this->sendMessage(EM_GETLIMITTEXT) );
}

inline int TextBoxBase::lineFromChar( int c ) {
	return this->sendMessage( EM_LINEFROMCHAR, c );
}

inline int TextBoxBase::lineIndex(int line) {
	return static_cast<int>(this->sendMessage(EM_LINEINDEX, static_cast<WPARAM>(line)));
}

inline void TextBoxBase::setModify( bool modify ) {
	this->sendMessage( EM_SETMODIFY, modify );
}

inline bool TextBoxBase::getModify( ) {
	return this->sendMessage( EM_GETMODIFY ) > 0;
}

inline void TextBox::setPassword( bool value, TCHAR pwdChar ) {
	this->sendMessage(EM_SETPASSWORDCHAR, static_cast< WPARAM >( value ? pwdChar : 0 ));
}

inline void TextBox::setNumbersOnly( bool value ) {
	this->Widget::addRemoveStyle( ES_NUMBER, value );
}

inline void TextBox::setLowerCase( bool value ) {
	this->Widget::addRemoveStyle( ES_LOWERCASE, value );
}

inline void TextBox::setUpperCase( bool value ) {
	this->Widget::addRemoveStyle( ES_UPPERCASE, value );
}

inline int TextBox::charFromPos(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	LPARAM lp = MAKELPARAM(cc.x(), cc.y());
	return LOWORD(sendMessage(EM_CHARFROMPOS, 0, lp));
}

inline int TextBox::lineFromPos(const ScreenCoordinate& pt) {
	ClientCoordinate cc(pt, this);
	LPARAM lp = MAKELPARAM(cc.x(), cc.y());
	return HIWORD(sendMessage(EM_CHARFROMPOS, 0, lp));
}

inline int TextBoxBase::lineLength(int c) {
	return static_cast<int>(sendMessage(EM_LINELENGTH, static_cast<WPARAM>(c)));
}

inline unsigned TextBoxBase::getLineCount() const {
	return sendMessage(EM_GETLINECOUNT);
}

}

#endif
