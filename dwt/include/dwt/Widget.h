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

#ifndef DWT_Widget_h
#define DWT_Widget_h

#include "Application.h"
#include "Atom.h"
#include "forward.h"
#include "Rectangle.h"
#include "Point.h"
#include "Message.h"
#include "Dispatcher.h"
#include <unordered_map>

namespace dwt {

using namespace std::placeholders;

template<typename T>
T hwnd_cast(HWND hwnd);

/// Base class for all Widgets
/** Basically (almost) all Widgets derive from this class, this is the root class for
  * (almost) every single Widget type. <br>
  * This class contains the functionality that all Widgets must or should have
  * support for. <br>
  * E.g. the handle to the specific Widgets are contained here, plus all the
  * commonalities between Widgets <br>
  * The Widget class inherits from boost::noncopyable to indicate it's not to be
  * copied
  */
class Widget
	: public boost::noncopyable
{
public:
	/// Returns the HWND to the Widget
	/** Returns the HWND to the inner window of the Widget. <br>
	  * If you need to do directly manipulation of the window use this function to
	  * retrieve the HWND of the Widget.
	  */
	HWND handle() const;

	Dispatcher& getDispatcher();

	/// Send a message to the Widget
	/** If you need to be able to send a message to a Widget then use this function
	  * as it will unroll into <br>
	  * a ::SendMessage from the Windows API
	  */
	LRESULT sendMessage( UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const;

	bool postMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0) const;

	/// Returns the parent Widget of the Widget
	/** Most Widgets have got a parent, this function will retrieve a pointer to the
	  * Widgets parent, if the Widget doesn't have a parent it will return a null
	  * pointer.
	  */
	Widget* getParent() const;

	/// Use this function to add or remove windows styles.
	/** The first parameter is the type of style you wish to add/remove. <br>
	  * The second argument is a boolean indicating if you wish to add or remove the
	  * style (if true add style, else remove)
	  */
	void addRemoveStyle(DWORD addStyle, bool add);

	bool hasStyle(DWORD style) const;
	bool hasExStyle(DWORD style) const;

	/// Use this function to add or remove windows exStyles.
	/** The first parameter is the type of style you wish to add/remove. <br>
	  * The second argument is a boolean indicating if you wish to add or remove the
	  * style (if true add style, else remove)
	  */
	void addRemoveExStyle(DWORD addStyle, bool add);

	typedef std::function<bool(const MSG& msg, LRESULT& ret)> CallbackType;
	typedef std::list<CallbackType> CallbackList;
	typedef CallbackList::iterator CallbackIter;

	/// Adds a new callback - multiple callbacks for the same message will be called in the order they were added
	CallbackIter addCallback(const Message& msg, const CallbackType& callback);

	/// Sets the callback for msg - clears any other callbacks registered for the same message
	CallbackIter setCallback(const Message& msg, const CallbackType& callback);

	/// Clear a callback registered to msg
	void clearCallback(const Message& msg, const CallbackIter& i);

	/// Clear all callbacks registered to msg
	void clearCallbacks(const Message& msg);

	/** Run a function bound to this widget asynchronously */
	void callAsync(const Application::Callback& f);

	/// Returns true if handled, else false
	virtual bool handleMessage(const MSG &msg, LRESULT &retVal);

	/** This will be called when it's time to delete the widget */
	virtual void kill();

	virtual Point getPreferredSize();

	/** Layout any child widgets in the widgets client area */
	virtual void layout();

	Rectangle getWindowRect() const;
	Point getWindowSize() const;
	Point getClientSize() const;

	/** Return the desktop size of the primary monitor (at coords 0, 0). */
	static Point getPrimaryDesktopSize();
	/** Return the desktop size of the monitor closest to this widget. */
	Rectangle getDesktopSize() const;

	/** Change the Z order of this widget. See the SetWindowPos doc for special values the
	"insertAfter" parameter can take. */
	void setZOrder(HWND insertAfter);

	/// Sets the enabled property of the Widget
	/** Changes the enabled property of the Widget. Use this function to change the
	  * enabled property of the Widget
	  */
	bool setEnabled(bool enabled);

	/// Retrieves the enabled property of the Widget
	/** Use this function to check if the Widget is Enabled or not. If the Widget is
	  * enabled this function will return true.
	  */
	bool getEnabled() const;

	/**
	 * Attaches the instance to an existing window.
	 */
	void setHandle(HWND hwnd);

	/// get the top-most parent window of this widget (either a main window or a modal dialog).
	Widget* getRoot() const;

	/** Callback to execute when creating the widget. */
	void onCreate(std::function<void (const CREATESTRUCT&)> f) {
		addCallback(Message(WM_CREATE), [f](const MSG& msg, LRESULT&) -> bool {
			auto cs = reinterpret_cast<const CREATESTRUCT*>(msg.lParam);
			f(*cs);
			return false;
		});
	}

	/** Callback to execute right before destroying the widget. */
	void onDestroy(std::function<void ()> f) {
		addCallback(Message(WM_DESTROY), [f](const MSG&, LRESULT&) -> bool {
			f();
			return false;
		});
	}

protected:
	/** Most Widgets can override the creational parameters which sets the style and the
	  * initial position of the Widget, those Widgets will take an object of this type to
	  * their creational function(s).
	  */
	struct Seed {
		/// Initial caption
		/** Windows with a title bar will use this string in the title bar. Controls with
		  * caption (e.g. static control, edit control) will use it in the control. <br>
		  * It is feed directly to CreateWindowEx, this means that it follows its
		  * conventions. In particular, the string "#num" has a special meaning.
		  */
		tstring caption;

		/// The style of the object (starts with WS_ or BS_ etc...)
		/** WARNING: The creation of most of the controls require WS_CHILD to be set.
		  * This is done, by default, in the appropriate controls. If you override the
		  * default style, then be sure that WS_CHILD is set (if needed).
		  */
		DWORD style;

		/// The Extended Style of the object (starts often with WS_EX_ etc)
		DWORD exStyle;

		/// The initial position / size of the Widget
		Rectangle location;

		/// Menu handle or control ID, depending on context - see CreateWindowEx doc for more info.
		HMENU menuHandle;

		/// Constructor initializing all member variables to default values
		Seed(DWORD style_ = WS_VISIBLE, DWORD exStyle_ = 0,
			const tstring& caption_ = tstring(),
			const Rectangle& location_ = letTheSystemDecide, HMENU menuHandle_ = NULL)
			: caption(caption_), style( style_ ), exStyle( exStyle_ ), location( location_ ), menuHandle( menuHandle_ )
		{}
	};

	Widget(Widget* parent, Dispatcher& dispatcher);

	virtual ~Widget();

	/**
	 * Creates the Widget, should not be called directly but overridden in the
	 * derived class - otherwise the wrong seed will be used.
	 */
	HWND create(const Seed & cs);

	HWND getParentHandle();

	void setParent(Widget* parent);

	/// Convert "this" to an LPARAM, suitable to be converted back into a Widget*.
	/// Note; it's better to use this function than casting to make sure that the correct this pointer is used
	/// for multiply inherited classes...
	LPARAM toLParam();

	/// Convert back from LPARAM to a Widget*
	static Widget* fromLParam(LPARAM lParam);

private:
	friend class Application;
	template<typename T> friend T hwnd_cast(HWND hwnd);

	static Rectangle getDesktopSize(HMONITOR mon);

	/// The atom with which the pointer to the Widget is registered on the HWND
	static GlobalAtom propAtom;

	// Contains the list of signals we're (this window) processing
	std::unordered_map<Message, CallbackList> handlers;

	HWND hwnd;

	Widget *parent;

	Dispatcher& dispatcher;
};

inline LRESULT Widget::sendMessage( UINT msg, WPARAM wParam, LPARAM lParam) const {
	return ::SendMessage(handle(), msg, wParam, lParam);
}

inline bool Widget::postMessage(UINT msg, WPARAM wParam, LPARAM lParam) const {
	return ::PostMessage(handle(), msg, wParam, lParam);
}

inline HWND Widget::handle() const {
	return hwnd;
}

inline Dispatcher& Widget::getDispatcher() {
	return dispatcher;
}

inline Widget* Widget::getParent() const {
	return parent;
}

inline bool Widget::hasStyle(DWORD style) const {
	return (::GetWindowLongPtr(handle(), GWL_STYLE) & style) == style;
}

inline bool Widget::hasExStyle(DWORD style) const {
	return (::GetWindowLongPtr(handle(), GWL_EXSTYLE) & style) == style;
}

inline HWND Widget::getParentHandle() {
	return getParent() ? getParent()->handle() : NULL;
}

inline LPARAM Widget::toLParam() {
	return reinterpret_cast<LPARAM>(this);
}

inline Widget* Widget::fromLParam(LPARAM lParam) {
	return reinterpret_cast<Widget*>(lParam);
}

inline Rectangle Widget::getWindowRect() const {
	RECT rc;
	::GetWindowRect(handle(), &rc);
	return Rectangle(rc);
}

inline Point Widget::getWindowSize() const {
	return getWindowRect().size;
}

inline Point Widget::getClientSize() const {
	RECT rc;
	::GetClientRect(handle(), &rc);
	// Left, top are always 0
	return Point(rc.right, rc.bottom);
}

inline bool Widget::setEnabled(bool enabled) {
	return ::EnableWindow(handle(), enabled ? TRUE : FALSE);
}

inline bool Widget::getEnabled() const {
	return ::IsWindowEnabled(handle()) != 0;
}

template<typename T>
T hwnd_cast(HWND hwnd) {
	Widget* w = reinterpret_cast<Widget*>(::GetProp(hwnd, Widget::propAtom));
	return dynamic_cast<T>(w);
}

}

#endif
