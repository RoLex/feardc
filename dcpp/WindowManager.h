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

#ifndef DCPLUSPLUS_DCPP_WINDOW_MANAGER_H
#define DCPLUSPLUS_DCPP_WINDOW_MANAGER_H

#include "forward.h"
#include "SettingsManager.h"
#include "WindowInfo.h"

namespace dcpp {

class WindowManager :
	public Singleton<WindowManager>,
	private SettingsManagerListener
{
	typedef std::unordered_map<string, unsigned> MaxRecentItems;
public:
	typedef std::vector<WindowInfo> WindowInfoList;
	typedef std::unordered_map<string, WindowInfoList> RecentList;

	void add(const string& id, const WindowParams& params);
	void clear();
	const WindowInfoList& getList();

	/// adds the referenced window if it doesn't exist, and moves it to the top of the stack.
	void addRecent(const string& id, const WindowParams& params);
	/// updates the title of the referenced window without changing its position in the stack.
	void updateRecent(const string& id, const WindowParams& params);
	const RecentList& getRecent() const { return recent; }
	void setMaxRecentItems(const string& id, unsigned max);
	unsigned getMaxRecentItems(const string& id) const;

	/// inform other classes about users and file lists that are of interest to us.
	void prepareSave() const;

	/// special id that designates a hub window.
	static const string& hub();

private:
	friend class Singleton<WindowManager>;

	WindowInfoList list;
	RecentList recent;
	MaxRecentItems maxRecentItems;

	WindowManager();
	virtual ~WindowManager();

	inline void addRecent_(const string& id, const WindowParams& params) { addRecent_(id, params, false); }
	void addRecent_(const string& id, const WindowParams& params, bool top);

	void prepareSave(const WindowInfoList& infoList) const;

	typedef void (WindowManager::*handler_type)(const std::string&, const WindowParams&);
	void parseTags(SimpleXML& xml, handler_type handler);
	void addTag(SimpleXML& xml, const WindowInfo& info) const;

	void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept;
	void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept;
};

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_WINDOW_MANAGER_H)
