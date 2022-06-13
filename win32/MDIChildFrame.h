/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DCPLUSPLUS_WIN32_MDI_CHILD_FRAME_H_
#define DCPLUSPLUS_WIN32_MDI_CHILD_FRAME_H_

#include <functional>
#include <string>

#include <dcpp/SettingsManager.h>
#include <dcpp/WindowInfo.h>

#include <dwt/widgets/Container.h>
#include <dwt/util/StringUtils.h>

#include "AspectStatus.h"
#include "WinUtil.h"
#include "resource.h"

using std::string;

using dwt::util::escapeMenu;

template<typename T>
class MDIChildFrame :
	public dwt::Container,
	public AspectStatus<T>
{
	typedef dwt::Container BaseType;
	typedef MDIChildFrame<T> ThisType;

	const T& t() const { return *static_cast<const T*>(this); }
	T& t() { return *static_cast<T*>(this); }

protected:
	MDIChildFrame(TabViewPtr tabView, const tstring& title, unsigned helpId = 0, unsigned iconId = 0, bool manageAccels = true) :
		BaseType(tabView),
		lastFocus(NULL),
		alwaysSameFocus(false),
		closing(false)
	{
		typename ThisType::Seed cs;
		cs.style &= ~WS_VISIBLE;
		cs.caption = title;
		cs.location = tabView->getClientSize();
		this->create(cs);

		if(helpId)
			setHelpId(helpId);

		tabView->add(this, iconId ? WinUtil::tabIcon(iconId) : dwt::IconPtr());

		this->onTabContextMenu([this](const dwt::ScreenCoordinate &sc) { return this->handleContextMenu(sc); });

		onClosing([this] { return this->handleClosing(); });
		onFocus([this] { this->handleFocus(); });
		onVisibilityChanged([this](bool b) { this->handleVisibilityChanged(b); });
		onWindowPosChanged([this](const dwt::Rectangle &r) { this->handleSized(r); });
		addDlgCodeMessage(this);

		addAccel(FCONTROL, 'W', [this] { this->close(true); });
		addAccel(FCONTROL, VK_F4, [this] { this->close(true); });
		if(manageAccels)
			initAccels();
	}

	virtual ~MDIChildFrame() {
		getParent()->remove(this);
	}

	/**
	 * The first of two close phases, used to disconnect from other threads that might affect this window.
	 * This is where all stuff that might be affected by other threads goes - it should make sure
	 * that no more messages can arrive from other threads
	 * @return True if close should be allowed, false otherwise
	 */
	bool preClosing() { return true; }
	/** Second close phase, perform any cleanup that depends only on the UI thread */
	void postClosing() { }

	enum FocusMethod { AUTO_FOCUS, ALWAYS_FOCUS, NO_FOCUS };

	template<typename W>
	void addWidget(W* widget, FocusMethod focus = AUTO_FOCUS, bool autoTab = true, bool customColor = true) {
		addDlgCodeMessage(widget, autoTab);

		if(customColor)
			addColor(widget);

		if(focus == ALWAYS_FOCUS || (focus == AUTO_FOCUS && !lastFocus)) {
			lastFocus = widget->handle();
			if(this->getVisible())
				::SetFocus(lastFocus);
		}
		if(focus == ALWAYS_FOCUS)
			alwaysSameFocus = true;
	}

	void setDirty(SettingsManager::BoolSetting setting) {
		if(SettingsManager::getInstance()->get(setting)) {
			getParent()->mark(this);
		}
	}

	void onTabContextMenu(const std::function<bool (const dwt::ScreenCoordinate&)>& f) {
		getParent()->onTabContextMenu(this, f);
	}

	void activate() {
		getParent()->setActive(this);
	}

	bool isActive() const {
		return getParent()->getActive() == this;
	}

	TabViewPtr getParent() const {
		return static_cast<TabViewPtr>(BaseType::getParent());
	}

	void setIcon(dwt::IconPtr icon) {
		getParent()->setIcon(this, icon);
	}
	void setIcon(unsigned iconId) {
		setIcon(WinUtil::tabIcon(iconId));
	}

	static bool parseActivateParam(const WindowParams& params) {
		auto i = params.find("Active");
		return i != params.end() && i->second == "1";
	}

public:
	virtual const string& getId() const {
		return Util::emptyString;
	}

	virtual WindowParams getWindowParams() const {
		return WindowParams();
	}

private:
	HWND lastFocus; // last focused widget
	bool alwaysSameFocus; // always focus the same widget

	bool closing;

	void addDlgCodeMessage(ComboBox* widget, bool autoTab = true) {
		widget->onRaw([=, this](WPARAM w, LPARAM) { return this->handleGetDlgCode(w, autoTab); }, dwt::Message(WM_GETDLGCODE));
		auto text = widget->getTextBox();
		if(text)
			text->onRaw([=, this](WPARAM w, LPARAM) { return this->handleGetDlgCode(w, autoTab); }, dwt::Message(WM_GETDLGCODE));
	}

	template<typename W>
	void addDlgCodeMessage(W* widget, bool autoTab = true) {
		widget->onRaw([=, this](WPARAM w, LPARAM) { return this->handleGetDlgCode(w, autoTab); }, dwt::Message(WM_GETDLGCODE));
	}

	void addColor(ComboBox* widget) {
		// do not apply our custom colors to the combo itself, but apply it to its drop-down and edit controls

		auto listBox = widget->getListBox();
		if(listBox)
			addColor(listBox);

		auto text = widget->getTextBox();
		if(text)
			addColor(text);
	}

	// do not apply our custom colors to Buttons and Button-derived controls
	void addColor(dwt::Button* widget) {
		// empty on purpose
	}

	template<typename A>
	void addColor(dwt::aspects::Colorable<A>* widget) {
		WinUtil::setColor(static_cast<dwt::Control*>(widget));
	}

	// Catch-rest for the above
	void addColor(void* w) {

	}

	void handleSized(const dwt::Rectangle&) {
		t().layout();
	}

	void handleFocus() {
		if(lastFocus) {
			::SetFocus(lastFocus);
		}
	}

	void handleVisibilityChanged(bool b) {
		if(!b && !alwaysSameFocus) {
			// remember the previously focused window.
			HWND focus = ::GetFocus();
			if(focus && ::IsChild(t().handle(), focus))
				lastFocus = focus;
		}
	}

	LRESULT handleGetDlgCode(WPARAM wParam, bool autoTab) {
		if(autoTab && wParam == VK_TAB)
			return 0;
		return DLGC_WANTMESSAGE;
	}

	bool handleContextMenu(const dwt::ScreenCoordinate& pt) {
		auto menu = addChild(WinUtil::Seeds::menu);

		menu->setTitle(escapeMenu(getText()), getParent()->getIcon(this));

		tabMenuImpl(menu.get());
		menu->appendItem(T_("&Close"), [this] { close(true); }, WinUtil::menuIcon(IDI_EXIT));

		menu->open(pt);
		return true;
	}

	bool handleClosing() {
		if(!closing && t().preClosing()) {
			closing = true;
			// async to make sure all other async calls have been consumed
			this->callAsync([this] {
				this->t().postClosing();
				if(this->getParent()->getActive() == this) {
					// Prevent flicker by selecting the next tab - WM_DESTROY would already be too late
					this->getParent()->next();
				}
				::DestroyWindow(handle());
			});
			return false;
		}
		return false;
	}

	virtual void tabMenuImpl(dwt::Menu* menu) {
		// empty on purpose; implement this in the derived class to modify the tab menu.
	}
};

#endif
