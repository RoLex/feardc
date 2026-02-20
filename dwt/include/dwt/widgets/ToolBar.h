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

#ifndef DWT_ToolBar_h
#define DWT_ToolBar_h

#include "../Dispatchers.h"
#include "../aspects/CustomDraw.h"
#include "../resources/ImageList.h"
#include "../DWTException.h"
#include "Control.h"

namespace dwt {

// TODO: Give support for multiple bitmaps...
/// Toolbar Control class
/** \ingroup WidgetControls
  * \WidgetUsageInfo
  * \image html toolbar.PNG
  * A toolbar is a strip of buttons normally associated with menu commands, like for
  * instance Internet Explorer has ( unless you have made them invisible ) a toolbar
  * of buttons, one for going "home", one to stop rendering of the current page, one
  * to view the log of URL's you have been to etc...
  */
class ToolBar :
	public Control,
	public aspects::CustomDraw<ToolBar, NMTBCUSTOMDRAW>
{
	typedef Control BaseType;
	typedef Dispatchers::VoidVoid<> Dispatcher;
	typedef std::function<void (const ScreenCoordinate&)> DropDownFunction;
	friend class WidgetCreator<ToolBar>;
public:
	/// Class type
	typedef ToolBar ThisType;

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

	// TODO: Outfactor into Aspect, also StatusBar...
	/// Refreshes the toolbar, must be called after main window has been resized
	/** Refreshes the toolbar, call this one whenever you need to redraw the toolbar,
	  * typical example is when you have resized the main window.
	  */
	void refresh();

	/// Adds a bitmap to the toolbar that later can be referenced while adding buttons
	/** Loads a bitmap that is contained in a BitmapPtr. <br>
	  * noButtonsInBitmap is how many buttons there actually exists in the bitmap
	  */
	//void addBitmap( BitmapPtr bitmap, unsigned int noButtonsInBitmap );

	/**
	* add a button to the toolbar. this will only create the internal structures for holding the
	* button; call setLayout when all buttons have been created to actually add them to the bar.
	* @param showText show text on the button itself (otherwise, it will only be used for tooltips).
	*/
	void addButton(const std::string& id, const IconPtr& icon, const IconPtr& hotIcon, const tstring& text, bool showText = false,
		unsigned helpId = 0, const Dispatcher::F& f = nullptr, const DropDownFunction& dropDownF = nullptr);
	void addButton(const std::string& id, int image, const tstring& text, bool showText = false,
		unsigned helpId = 0, const Dispatcher::F& f = nullptr, const DropDownFunction& dropDownF = nullptr);

	/**
	* fills a vector with ids of the current buttons, to represent the current state of the
	* toolbar. an empty id represents a separator bar.
	*/
	std::vector<std::string> getLayout() const;

	/**
	* define a new layout; all previous buttons will be removed, and the buttons specified in the
	* "ids" argument will then be added (provided they have been previously created, with
	* addButton). an empty id represents a separator bar.
	*/
	void setLayout(const std::vector<std::string>& ids);

	/// open the "Customize Toolbar" dialog.
	void customize();

	/// Set the image list with the normal button images.
	/** normalImageList is the image list that contains the images
	  * for the toolbar buttons in "normal" state.
	  */
	void setNormalImageList( ImageListPtr normalImageList );

	/// Set the image list with the hot button images.
	/** hotImageList is the image list that contains the images for the toolbar
	  * buttons in "hot" state (being hovered / pressed). <br>
	  * Note, hot button images requires the TBSTYLE_FLAT, TBSTYLE_LIST or
	  * TBSTYLE_TRANSPARENT style upon Toolbar creation.
	  */
	void setHotImageList( ImageListPtr hotImageList );

	/// Set the image list with the normal button images.
	/** disabledImageList is the image list that contains the images for the
	  * toolbar buttons in "disabled" state.
	  */
	void setDisabledImageList( ImageListPtr disabledImageList );
	/// Shows (or hides) the button in the toolbar with the given id
	/** id is the identification of which button you want to show.
	  */
	void setButtonVisible( unsigned int id, bool show );

	/// Returns a boolean indicating if the button with the current id is visible or not
	/** id is the identification you supplied when you called addButton.
	  */
	bool getButtonVisible( unsigned id );

	/// Enables (or disables) the button in the toolbar with the given id
	/** id is the identification of which button you want to enable.
	  */
	void setButtonEnabled( unsigned id, bool enable );

	/// Enables (or disables) the button in the toolbar with the given id
	/** id is the identification of which button you want to enable.
	  */
	void setButtonEnabled( const std::string& id, bool enable );

	/// Returns a boolean indicating if the button with the current id is enabled or not
	/** id is the identification you supplied when you called addButton.
	  */
	bool getButtonEnabled( unsigned int id );

	/// Returns a boolean indicating if the button with the current id is enabled or not
	/** id is the identification you supplied when you called addButton.
	  */
	bool getButtonEnabled( const std::string& id );

	/// Returns a boolean indicating if the button with the current id is checked or not
	/** id is the identification you supplied when you called addButton.
	  */
	bool getButtonChecked( unsigned int id );

	void setButtonChecked(unsigned id, bool check);
	void setButtonChecked(const std::string& id, bool check);

	unsigned size() const;

	int hitTest(const ScreenCoordinate& pt);

	void removeButton(unsigned index);

	/**
	* define a function called after the toolbar has been customized:
	* - after the "Customize Toolbar" dialog has been closed.
	* - after an item has been moved with shift + click.
	*/
	void onCustomized(const Dispatcher::F& f) {
		customized = f;
	}

	/**
	* define a function called when the user presses the "Help" button of the "Customize Toolbar"
	* dialog. if no function is defined, the "Help" button will be hidden.
	*/
	void onCustomizeHelp(const Dispatcher::F& f) {
		customizeHelp = f;
	}

	/// Actually creates the Toolbar
	/** You should call WidgetFactory::createToolbar if you instantiate class
	  * directly. <br>
	  * Only if you DERIVE from class you should call this function directly.
	  */
	void create(const Seed& cs = Seed());

	virtual Point getPreferredSize();

protected:
	// Constructor Taking pointer to parent
	explicit ToolBar(Widget* parent);

	// To assure nobody accidentally deletes any heaped object of this type, parent
	// is supposed to do so when parent is killed...
	virtual ~ToolBar()
	{}

	virtual bool handleMessage( const MSG & msg, LRESULT & retVal );

private:
	friend class ChainingDispatcher;
	static const TCHAR windowClass[];

	// Keep references
	ImageListPtr itsNormalImageList;
	ImageListPtr itsHotImageList;
	ImageListPtr itsDisabledImageList;

	struct Button {
		TBBUTTON button;
		std::string id;
		tstring text;
		unsigned helpId;
		Dispatcher::F f;
		DropDownFunction dropDownF;
	};
	typedef std::vector<Button> Buttons;
	Buttons buttons;
	static const unsigned id_offset = 100; // because 0-based ids seem to confuse the toolbar...

	// customization-related members
	bool customizing; /// the "Customize Toolbar" dialog is opened
	Dispatcher::F customized;
	Dispatcher::F customizeHelp;
	/// layout of the toolbar before the "Customize Toolbar" dialog was opened, in case the user hits "Reset"
	std::vector<std::string> prevLayout;

	LRESULT handleDropDown(LPARAM lParam);
	LRESULT handleToolTip(LPARAM lParam);

	// customization-related events
	LRESULT handleBeginAdjust();
	LRESULT handleChange();
	LRESULT handleCustHelp();
	LRESULT handleEndAdjust();
	LRESULT handleGetButtonInfo(LPARAM lParam);
	LRESULT handleInitCustomize();
	LRESULT handleQuery();
	LRESULT handleReset();

	const TBBUTTON& getSeparator() const;
	const Button* getButton(unsigned position) const;
	int getIntId(const std::string& id) const;

	// aspects::Help
	void helpImpl(unsigned& id);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline void ToolBar::refresh()
{
	sendMessage(TB_AUTOSIZE);
}

/*

void ToolBar::addBitmap( HBITMAP hBit, unsigned int noButtonsInBitmap )
{
	TBADDBITMAP tb;
	tb.hInst = NULL;
	tb.nID = ( UINT_PTR )hBit;
	if(sendMessage(TB_ADDBITMAP, static_cast< WPARAM >( noButtonsInBitmap ), reinterpret_cast< LPARAM >(&tb ) ) == - 1 )
	{
		xCeption x( _T("Error while trying to add a bitmap to toolbar...") );
		throw x;
	}
}


	void ToolBar::addBitmap( BitmapPtr bitmap, unsigned int noButtonsInBitmap )
{
	itsBitmaps.push_back( bitmap );
	addBitmap( bitmap->getBitmap(), noButtonsInBitmap );
}
*/

inline void ToolBar::setNormalImageList( ImageListPtr normalImageList )
{
	itsNormalImageList = normalImageList;
	sendMessage(TB_SETIMAGELIST, 0, reinterpret_cast< LPARAM >( itsNormalImageList->getImageList() ) );
}

inline void ToolBar::setHotImageList( ImageListPtr hotImageList )
{
	itsHotImageList = hotImageList;
	sendMessage(TB_SETHOTIMAGELIST, 0, reinterpret_cast< LPARAM >( itsHotImageList->getImageList() ) );
}

inline void ToolBar::setDisabledImageList( ImageListPtr disabledImageList )
{
	itsDisabledImageList = disabledImageList;
	sendMessage(TB_SETDISABLEDIMAGELIST, 0, reinterpret_cast< LPARAM >( itsDisabledImageList->getImageList() ) );
}

inline void ToolBar::setButtonVisible( unsigned int id, bool show )
{
	sendMessage(TB_HIDEBUTTON, static_cast< LPARAM >( id ), MAKELONG( ( show ? FALSE : TRUE ), 0 ) );
}

inline bool ToolBar::getButtonVisible( unsigned int id )
{
	TBBUTTONINFO tb = { sizeof(TBBUTTONINFO), TBIF_STATE, static_cast<int>(id) };
	sendMessage(TB_GETBUTTONINFO, id, reinterpret_cast< LPARAM >( & tb ) );
	return ( tb.fsState & TBSTATE_HIDDEN ) == 0;
}

inline void ToolBar::setButtonEnabled( unsigned id, bool enable )
{
	sendMessage(TB_ENABLEBUTTON, static_cast< LPARAM >( id ), MAKELONG( ( enable ? TRUE : FALSE ), 0 ) );
}

inline void ToolBar::setButtonEnabled( const std::string& id, bool enable )
{
	int intId = getIntId(id);
	if(intId != -1)
		setButtonEnabled(intId, enable);
}

inline bool ToolBar::getButtonEnabled( unsigned int id )
{
	TBBUTTONINFO tb = { sizeof(TBBUTTONINFO), TBIF_STATE, static_cast<int>(id) };
	sendMessage(TB_GETBUTTONINFO, id, reinterpret_cast< LPARAM >( & tb ) );
	return ( tb.fsState & TBSTATE_ENABLED ) == TBSTATE_ENABLED;
}

inline bool ToolBar::getButtonEnabled( const std::string& id )
{
	int intId = getIntId(id);
	if(intId != -1)
		return getButtonEnabled(intId);

	return false;
}

inline bool ToolBar::getButtonChecked( unsigned int id )
{
	TBBUTTONINFO tb = { sizeof(TBBUTTONINFO), TBIF_STATE, static_cast<int>(id) };
	sendMessage(TB_GETBUTTONINFO, id, reinterpret_cast< LPARAM >( & tb ) );
	return ( tb.fsState & TBSTATE_CHECKED ) == TBSTATE_CHECKED;
}

// end namespace dwt
}

#endif
