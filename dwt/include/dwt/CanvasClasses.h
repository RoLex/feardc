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

#ifndef DWT_CanvasClasses_h
#define DWT_CanvasClasses_h

#include "Widget.h"
#include "resources/Bitmap.h"
#include "resources/Font.h"
#include "resources/Icon.h"
#include "resources/Region.h"

namespace dwt {

/// Helper class for manipulating colors
/** Helper class for manipulating COLORREF values, contains static functions for
  * darken and lightening colors etc. COLORREF values (color variables) etc.
  */
struct Color {
	/// Darkens given color by specified factor
	/** Factor is in (0..1] range, the higher the factor the more dark the result
	  * will be. <br>
	  * Returns the manipulated value
	  */
	static COLORREF darken(COLORREF color, double factor);

	/// Lightens given color by specified factor
	/** Factor is in (0..1] range, the higher the factor the more light the result
	  * will be. <br>
	  * Returns the manipulated value
	  */
	static COLORREF lighten(COLORREF color, double factor);

	/// Alpha blends color
	/** Factor is in R/255, G/255 and B/255 meaning a value of 255, 255, 255 or
	  * 0xFFFFFF will not change change source color at all while a value of 0xFF00FF
	  * will keep all read and blue and discard all green. <br>
	  * Returns the manipulated value.
	  */
	static COLORREF alphaBlend(COLORREF color, COLORREF factor);

	/// wrapper around GetSysColor, see its doc for possible params.
	static COLORREF predefined(int index);
};

// Forward declarations
class Brush;
class Region;

/// Buffered Canvas, useful for e.g. double buffering updates to a Widget
/** A BufferedCanvas is a Canvas which you can draw upon but only resides in memory
  * and will not update the actual visible Canvas before you call blast, only then
  * the results of the drawing operations will be visible, useful to avoid "tearing"
  * which is an effect which might occur if your drawing operations are lengthy. <br>
  * Class does provide RAII semantics!
  */
template<typename CanvasType>
class BufferedCanvas : public CanvasType
{
public:
	template<typename InitT> // InitT can be a widget pointer or an HDC
	BufferedCanvas(InitT initT, long width = 0, long height = 0) :
	CanvasType(initT)
	{
		init(this->CanvasType::itsHdc, width, height);
	}

	/// Destructor will free up the contained HDC
	/** Note!<br>
	  * Destructor will not flush the contained operations to the contained Canvas
	  */
	virtual ~BufferedCanvas() {
		// delete buffer bitmap
		::DeleteObject(::SelectObject(this->CanvasType::itsHdc, itsOldBitmap));

		// delete buffer
		::DeleteDC(this->CanvasType::itsHdc);

		// set back source
		this->CanvasType::itsHdc = itsSource;
	}

	/// BitBlasts buffer into specified rectangle of source
	void blast(const Rectangle& rectangle) {
		// note; ::BitBlt might fail with ERROR_INVALID_HANDLE when the desktop isn't visible
		::BitBlt(itsSource, rectangle.x(), rectangle.y(), rectangle.width(), rectangle.height(), this->CanvasType::itsHdc,
			rectangle.x(), rectangle.y(), SRCCOPY);
	}

private:
	/// Creates and inits back-buffer for the given source
	void init(HDC source, long width, long height) {
		// the buffer might have to be larger than the screen size
		width += this->getDeviceCaps(HORZRES);
		height += this->getDeviceCaps(VERTRES);

		// create memory buffer for the source and reset itsHDC
		itsSource = source;
		this->CanvasType::itsHdc = ::CreateCompatibleDC(source);

		// create and select bitmap for buffer
		itsOldBitmap = (HBITMAP)::SelectObject(this->CanvasType::itsHdc, ::CreateCompatibleBitmap(source, width, height));
	}

	HDC itsSource; /// Buffer source
	HBITMAP itsOldBitmap; /// Buffer old bitmap
};

/// Class for painting objects and drawing in a windows
/** Helper class containing functions for drawing lines and objects inside a window.
  * <br>
  * Not meant to be directly instantiated, but rather instantiated through e.g. the
  * PaintCanvas or UpdateCanvas classes. <br>
  * Related classes <br>
  * <ul>
  * <li>UpdateCanvas</li>
  * <li>PaintCanvas</li>
  * <li>FreeCanvas</li>
  * <li>Pen</li>
  * </ul>
  */

class Canvas : private boost::noncopyable
{
	class Selector : boost::noncopyable {
	public:
		template<typename T>
		Selector(Canvas& canvas_, T& t) : canvas(&canvas_), h(::SelectObject(canvas->handle(), t.handle())) { }

		Selector(Selector &&rhs) : canvas(rhs.canvas), h(rhs.h) { rhs.canvas = nullptr; }
		Selector& operator=(Selector &&rhs) { if(&rhs != this) { canvas = rhs.canvas; h = rhs.h; rhs.canvas = nullptr; } return *this; }

		~Selector() { if(canvas) ::SelectObject(canvas->handle(), h); }

	private:
		Canvas* canvas;
		HGDIOBJ h;
	};

	class BkMode : boost::noncopyable {
	public:
		BkMode(Canvas& canvas_, int mode);

		BkMode(BkMode &&rhs) : canvas(rhs.canvas), prevMode(rhs.prevMode) { rhs.canvas = nullptr; }
		BkMode& operator=(BkMode &&rhs) { if(&rhs != this) { canvas = rhs.canvas; prevMode = rhs.prevMode; rhs.canvas = nullptr; } return *this; }

		~BkMode();

	private:
		Canvas* canvas;
		int prevMode;
	};

public:
	/** select a new resource (brush / font / pen / etc).
	* @return object that restores the previous resource when destroyed.
	*/
	template<typename T> Selector select(T&& t) {
		return Selector(*this, std::forward<T>(t));
	}

	/// Returns the Device Context for the Canvas
	/** Can be used to construct e.g. a Pen object or a HdcModeSetter object
	  */
	HDC handle() const;

	/// Gets the device capabilities.
	/** HORZRES, VERTRES give pixels
	  */
	int getDeviceCaps( int nIndex );

	/// Moves to a X,Y point.  (But does not draw).
	/** Moves to x,y in the Device Context of the object.
	  */
	void moveTo( int x, int y );

	/// Moves to a specific Point.  (But does not draw).
	/** Moves to Point in the Device Context of the object.
	  */
	void moveTo( const Point & coord );

	/// Draws a line from the current position to a X,Y point.
	/** Draws to x,y in the Device Context of the object.
	  * Use line (below) if you know two coordinates.
	  */
	void lineTo( int x, int y );

	/// Draws a line from the current position to a specific Point.
	/** Draws to Point in the Device Context of the object.
	  * Use line (below) if you know two coordinates.
	  */
	void lineTo( const Point & coord );

	/// Draws a line in the Device Context.
	/** Draws a line from (xStart, yStart) to (xEnd, yEnd).
	  */
	void line( int xStart, int yStart, int xEnd, int yEnd );

	/// Draws a line from Start to End in the Device Context.
	/** Draws a line from Start to End. <br>
	  * An alternate for line( int xStart, int yStart, int xEnd, int yEnd )
	  */
	void line( const Point & start, const Point & end );

	/// Draws a line around a Rectangle without filling it.
	/** Draws a line from rect.pos to rect.pos + rect.size <br>
	  * (Use Rectangle if you want to fill it.)
	  */
	void line( const dwt::Rectangle & rect );

	/// Fills a polygon defined by vertices.
	/** Fills a polygon defined by vertices.
	  */
	void polygon( const Point points[], unsigned count );

	/// Fills a polygon defined by vertices.
	/** Fills a polygon defined by vertices.
	  */
	void polygon( POINT points[], unsigned count );

	/// Draws an ellipse in the Device Context.
	/** Draws an ellipse from (left, top) to (right, bottom).
	  */
	void ellipse( int left, int top, int right, int bottom );

	/// Draws an ellipse in the Device Context.
	/** Draws an ellipse within the given rectangle.
	  */
	void ellipse( const dwt::Rectangle & rect );

	/// Draws a Rectangle in the Device Context.
	/** Draws a Rectangle from (left, top) to (right, bottom).
	  * Uses the current Pen to outline, and the current Brush to fill it.
	  */
	void rectangle( int left, int top, int right, int bottom );

	/// Draws a Rectangle in the Device Context.
	/** Draws a Rectangle from (pos) to ( pos + size ).
	  * Uses the current Pen to outline, and the current Brush to fill it.
	  */
	void rectangle( const dwt::Rectangle & rect );

	/// Fills a Rectangle in the Device Context with the given brush.
	/** Fills a Rectangle from (left, top) to (right, bottom).
	  */
	void fill(int left, int top, int right, int bottom, const Brush& brush);

	/// Fills a Rectangle in the Device Context with the given brush.
	/** Fills a Rectangle within the given Rectangle.
	  */
	void fill(const Rectangle& rect, const Brush& brush);

	/** Fills "region" with "brush". */
	void fill(const Region& region, const Brush& brush);

	/// Sets the pixel at (x,y) to be pixcolor. Returns the old pixel color.
	/** Sets the pixel at (x,y) to be pixcol
	  */
	COLORREF setPixel( int x, int y, COLORREF pixcolor );

	/// Returns the pixel's color at (x,y).
	/** Returns the pixel's color at (x,y) in the Device Context of the object.
	  */
	COLORREF getPixel( int x, int y );

	/// Returns the pixel's color at given point.
	/** Returns the pixel's color at coord in the Device Context of the object.
	  */
	COLORREF getPixel( const Point & coord );

#ifndef WINCE
	/// Fills an area starting at (x,y) with the current brush.
	/// crColor specifies when to stop or what to fill depending on the filltype
	/// parameter.
	/** If fillTilColorFound is true filling continues outwards from (x,y) until we
	  * find the given color and stops there. <br>
	  * If it is false it will fill AS LONG as it finds the given color and stop when
	  * it finds another color. <br>
	  * Function returns true if any filling was done, if no filling was done at all
	  * it'll return false.
	  */
	bool extFloodFill( int x, int y, COLORREF color, bool fillTilColorFound );
#endif //!WINCE

	/// invert the colors in the specified region; see the InvertRgn doc for more information.
	void invert(const Region& region);

	void drawIcon(const IconPtr& icon, const Rectangle& rectangle);

	/// Draws given string within given Rectangle.
	/** Draw text within a rectangle according to given format.<br>
	  * The format can be any combination of:
	  * <ul>
	  * <li>DT_BOTTOM</li>
	  * <li>DT_CALCRECT</li>
	  * <li>DT_CENTER</li>
	  * <li>DT_EDITCONTROL</li>
	  * <li>DT_END_ELLIPSIS</li>
	  * <li>DT_EXPANDTABS</li>
	  * <li>DT_EXTERNALLEADING</li>
	  * <li>DT_HIDEPREFIX</li>
	  * <li>DT_INTERNAL</li>
	  * <li>DT_LEFT</li>
	  * <li>DT_MODIFYSTRING</li>
	  * <li>DT_NOCLIP</li>
	  * <li>DT_NOFULLWIDTHCHARBREAK</li>
	  * <li>DT_NOPREFIX</li>
	  * <li>DT_PATH_ELLIPSIS</li>
	  * <li>DT_PREFIXONLY</li>
	  * <li>DT_RIGHT</li>
	  * <li>DT_RTLREADING</li>
	  * <li>DT_SINGLELINE</li>
	  * <li>DT_TABSTOP</li>
	  * <li>DT_TOP</li>
	  * <li>DT_VCENTER</li>
	  * <li>DT_WORDBREAK</li>
	  * <li>DT_WORD_ELLIPSIS</li>
	  * </ul>
	  * Google for or look at MSDN what their different meaning are.
	  */
	int drawText(const tstring& text, Rectangle& rect, unsigned format);

	/// Draws given text inside given Rectangle
	/** Draw text within coordinates of given Rectangle according to <br>
	  * setTextColor, setTextAlign, SetTextJustification
	  */
	void extTextOut( const tstring & text, unsigned x, unsigned y );

	/// Sets the TextColor of the this Canvas.
	/** Sets the TextColor for future TextOut() calls. Returns the previous color.
	  */
	COLORREF setTextColor( COLORREF crColor );

	/// Sets the background color for the this Canvas
	/** Sets the background color for extTextOut() calls.
	  * Returns the previous background color
	  */
	COLORREF setBkColor( COLORREF crColor );

	/** Sets the background mode, see the ::SetBkMode doc for more information.
	* @return object that restores the previous bkmode value when destroyed.
	*/
	BkMode setBkMode(bool transparent);

	/// Gets the background color for the this Canvas
	/** Gets the background color for extTextOut() calls.
	  * Returns the current background color.
	  */
	COLORREF getBkColor();

	/// Sets the alignment mode for text operations
	/** Returns the previous alignement mode and changes the current mode of text
	  * operations. Possible values can be any combination of these:
	  * <ul>
	  * <li>TA_CENTER</li>
	  * <li>TA_LEFT</li>
	  * <li>TA_RIGHT</li>
	  * <li>TA_BASELINE</li>
	  * <li>TA_BOTTOM</li>
	  * <li>TA_TOP</li>
	  * <li>TA_NOUPDATECP</li>
	  * <li>TA_UPDATECP</li>
	  * </ul>
	  */
	unsigned setTextAlign( unsigned fMode );

	void getTextMetrics(TEXTMETRIC& tm);

	Point getTextExtent(const tstring& str);

protected:
	Canvas() : itsHdc(0) { }

	/// Protected Constructor to prevent deletion of class directly
	/** Derived class should delete, basically a hack to prevent deletion of a base
	  * class pointer
	  */
	virtual ~Canvas() { }

	/// Handle to the Device Context of the object.
	/** Derived classes needs access to this to e.g. call BeginPaint and EndPaint
	  */
	HDC itsHdc;
};

/// base class for a canvas bound to a widget
class BoundCanvas : public Canvas
{
protected:
	BoundCanvas(HWND hWnd);
	BoundCanvas(Widget* widget);

	HWND itsHandle;
};

/// Class for painting within a WM_PAINT message
/** Helper class for securing that EndPaint is called for every BeginPaint we call.
  * <br>
  * BeginPaint is called in Constructor and EndPaint in DTOR. <br>
  * Inside a beenPainting event handler you have access to an object of this type.
  * <br>
  * Related classes<br>
  * <ul>
  * <li>UpdateCanvas</li>
  * <li>Canvas</li>
  * <li>Pen</li>
  * </ul>
  */
class PaintCanvas : public BoundCanvas
{
public:
	/// Constructor, automatically calls BeginPaint
	template<typename W>
	PaintCanvas(W widget) :
	BoundCanvas(widget)
	{
		initialize();
	}

	/// DTOR, automatically calls EndPaint
	/** Automatically calls end paint when object goes out of scope.
	  */
	virtual ~PaintCanvas();

	Rectangle getPaintRect();

private:
	PAINTSTRUCT itsPaint;

	// Initializes the object
	void initialize();
};

/// Class for painting outside a WM_PAINT message
/** Helper class for securing that GetDC and ReleaseDC is called in Constructor and
  * DTOR. <br>
  * Instantiate an object if this type where ever you want except within a WM_PAINT
  * event handler (where you <br>
  * should rather use PaintCanvas) to get access to draw objects within a window.
  * <br>
  * Then when Widget is invalidated or updated your "painting" will show. <br>
  * Related classes<br>
  * <ul>
  * <li> PaintCanvas</li>
  * <li> Canvas</li>
  * <li> Pen</li>
  * </ul>
  */
template<bool windowDC>
class UpdateCanvas_ : public BoundCanvas
{
public:
	/// Constructor, automatically calls GetDC or GetWindowDC
	template<typename W>
	UpdateCanvas_(W widget) :
	BoundCanvas(widget)
	{
		itsHdc = windowDC ? ::GetWindowDC(itsHandle) : ::GetDC(itsHandle);
	}

	/// DTOR, automatically calls ReleaseDC
	/** Automtaically releases the Device Context
	  */
	virtual ~UpdateCanvas_() {
		::ReleaseDC(itsHandle, itsHdc);
	}
};
typedef UpdateCanvas_<false> UpdateCanvas;
typedef UpdateCanvas_<true> WindowUpdateCanvas;

/// Class for painting on an already created canvas which we don't own ourself
/** Note this class does NOT create or instantiate a HDC like the PaintCanvas and the
  * UpdateCanvas. It assumes that the given HDC is already valid and instantiated. If
  * you need to paint or update a Widget and you don't have a valid HDC use the
  * UpdateCanvas or within a onPainting event handler use the PaintCanvas. Related
  * classes<br>
  * <ul>
  * <li>PaintCanvas</li>
  * <li>UpdateCanvas</li>
  * <li>Canvas</li>
  * <li>Pen</li>
  * </ul>
  */
class FreeCanvas : public Canvas
{
public:
	/// Constructor, assigns the given HDC to the object
	FreeCanvas(HDC hdc);

	virtual ~FreeCanvas() { }
};

/// Calls CreateCompatibleDC and returns a managed canvas.
class CompatibleCanvas : public FreeCanvas
{
public:
	CompatibleCanvas(HDC hdc);

	virtual ~CompatibleCanvas();
};

inline HDC Canvas::handle() const { return itsHdc; }

}

#endif
