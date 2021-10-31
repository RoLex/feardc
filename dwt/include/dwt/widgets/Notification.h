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
#ifndef DWT_NOTIFICATION_H_
#define DWT_NOTIFICATION_H_

#include <dwt/forward.h>
#include <dwt/resources/Icon.h>

#include <deque>
#include <functional>

namespace dwt {

/** A notification object represents a tray icon and a short message notification service */
class Notification {
public:
	typedef Notification ThisType;
	typedef NotificationPtr ObjectType;

	struct Seed {
		typedef ThisType WidgetType;

		Seed(const IconPtr& icon_ = IconPtr(), const tstring& tip_ = tstring()) : icon(icon_), tip(tip_) { }

		IconPtr icon;
		tstring tip;
	};

	Notification(Widget* parent);
	~Notification();

	void create(const Seed& seed);

	void setIcon(const IconPtr& icon_);

	void setVisible(bool visible_);
	bool isVisible() const { return visible; }

	void setTooltip(const tstring& tip);

	typedef std::function<void ()> Callback;

	/** show a balloon popup.
	@param callback callback called when the balloon has been clicked.
	@param balloonIcon icon shown next to the title, only available on >= Vista. */
	void addMessage(const tstring& title, const tstring& message, const Callback& callback, const IconPtr& balloonIcon = 0);

	void onContextMenu(Callback callback) { contextMenu = callback; }

	/// The icon was left-clicked / selected
	void onIconClicked(Callback callback) { iconClicked = callback; }

	/// The icon was double-clicked - this will swallow the next left-click message
	void onIconDbClicked(Callback callback) { iconDbClicked = callback; }

	/// This is sent when the tooltip text should be updated
	void onUpdateTip(Callback callback) { updateTip = callback; }

private:
	Widget* parent;
	IconPtr icon;

	bool visible;
	bool ignoreNextClick; // true after a double-click

	tstring tip;

	Callback contextMenu;
	Callback iconClicked;
	Callback iconDbClicked;
	Callback updateTip;

	std::deque<std::pair<Callback, IconPtr>> balloons; // keep a ref of the icon until the balloon has been shown.
	bool onlyBalloons; /// the tray icon has been created solely for balloons; it will disappear afterwards.

	NOTIFYICONDATA makeNID() const;

	/// Last tick that tip was updated
	ULONGLONG lastTick;
	bool trayHandler(const MSG& msg);
	bool redisplay();

	static const UINT message;
};
}

#endif /* NOTIFICATION_H_ */
