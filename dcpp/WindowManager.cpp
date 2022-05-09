/*
 * Copyright (C) 2001-2022 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "WindowManager.h"

#include "SimpleXML.h"
#include "ClientManager.h"
#include "QueueManager.h"

namespace dcpp {

static const unsigned MAX_RECENTS_DEFAULT = 10;

const string& WindowManager::hub() {
	static const string hub("Hub");
	return hub;
}

WindowManager::WindowManager() {
	SettingsManager::getInstance()->addListener(this);
}

WindowManager::~WindowManager() {
	SettingsManager::getInstance()->removeListener(this);
}

void WindowManager::add(const string& id, const WindowParams& params) {
	list.emplace_back(id, params);
}

void WindowManager::clear() {
	list.clear();
}

const WindowManager::WindowInfoList& WindowManager::getList() {
	return list;
}

void WindowManager::addRecent(const string& id, const WindowParams& params) {
	addRecent_(id, params, true);
}

void WindowManager::addRecent_(const string& id, const WindowParams& params, bool top) {
	unsigned max;
	{
		auto i = maxRecentItems.find(id);
		if(i == maxRecentItems.end()) {
			maxRecentItems[id] = max = MAX_RECENTS_DEFAULT;
		} else {
			max = i->second;
		}
	}
	if(max == 0)
		return;

	WindowInfo info(id, params);

	if(recent.find(id) == recent.end()) {
		recent[id] = WindowInfoList(1, info);
		return;
	}

	WindowInfoList& infoList = recent[id];
	if(top) {
		auto i = std::find(infoList.begin(), infoList.end(), info);
		if(i == infoList.end()) {
			infoList.insert(infoList.begin(), info);
			if(infoList.size() > max)
				infoList.erase(infoList.end() - 1);
		} else {
			infoList.erase(i);
			infoList.insert(infoList.begin(), info);
		}
	} else if(infoList.size() < max)
		infoList.push_back(info);
}

void WindowManager::updateRecent(const string& id, const WindowParams& params) {
	auto ri = recent.find(id);
	if(ri != recent.end()) {
		WindowInfo info(id, params);
		auto i = std::find(ri->second.begin(), ri->second.end(), info);
		if(i != ri->second.end())
			i->setParams(params);
	}
}

void WindowManager::setMaxRecentItems(const string& id, unsigned max) {
	maxRecentItems[id] = max;

	auto i = recent.find(id);
	if(i != recent.end()) {
		if(max == 0) {
			recent.erase(i);
		} else {
			while(i->second.size() > max)
				i->second.erase(i->second.end() - 1);
		}
	}
}

unsigned WindowManager::getMaxRecentItems(const string& id) const {
	auto i = maxRecentItems.find(id);
	if(i == maxRecentItems.end())
		return MAX_RECENTS_DEFAULT;
	return i->second;
}

void WindowManager::prepareSave() const {
	prepareSave(list);
	for(auto& i: recent)
		prepareSave(i.second);
}

void WindowManager::prepareSave(const WindowInfoList& infoList) const {
	for(auto& wi: infoList) {
		for(auto& i: wi.getParams()) {
			auto& param = i.second;
			if(param.empty())
				continue;

			if(param.isSet(WindowParam::FLAG_CID))
				ClientManager::getInstance()->saveUser(CID(param));

			if(param.isSet(WindowParam::FLAG_FILELIST))
				QueueManager::getInstance()->noDeleteFileList(param);
		}
	}
}

void WindowManager::parseTags(SimpleXML& xml, handler_type handler) {
	xml.stepIn();

	while(xml.findChild("Window")) {
		const string& id = xml.getChildAttrib("Id");
		if(id.empty())
			continue;

		WindowParams params;
		xml.stepIn();

		while(xml.findChild("Param")) {
			const string& id_ = xml.getChildAttrib("Id");
			if(id_.empty())
				continue;

			WindowParam param(xml.getChildData());

			if(!xml.getBoolChildAttrib("Opt") && id_ != "Title") /// @todo "Title" check for back compat - remove later
				param.setFlag(WindowParam::FLAG_IDENTIFIES);
			if(xml.getBoolChildAttrib("CID"))
				param.setFlag(WindowParam::FLAG_CID);
			if(xml.getBoolChildAttrib("FileList"))
				param.setFlag(WindowParam::FLAG_FILELIST);

			params[id_] = param;
		}

		xml.stepOut();

		(this->*handler)(id, params);
	}

	xml.stepOut();
}

void WindowManager::addTag(SimpleXML& xml, const WindowInfo& info) const {
	xml.addTag("Window");
	xml.addChildAttrib("Id", info.getId());

	if(!info.getParams().empty()) {
		xml.stepIn();

		for(auto& i: info.getParams()) {
			xml.addTag("Param", i.second);
			xml.addChildAttrib("Id", i.first);

			if(!i.second.isSet(WindowParam::FLAG_IDENTIFIES))
				xml.addChildAttrib("Opt", true);
			if(i.second.isSet(WindowParam::FLAG_CID))
				xml.addChildAttrib("CID", true);
			if(i.second.isSet(WindowParam::FLAG_FILELIST))
				xml.addChildAttrib("FileList", true);
		}

		xml.stepOut();
	}
}

void WindowManager::on(SettingsManagerListener::Load, SimpleXML& xml) noexcept {
	clear();

	xml.resetCurrentChild();
	if(xml.findChild("Windows"))
		parseTags(xml, &WindowManager::add);

	if(xml.findChild("Recent")) {
		xml.stepIn();
		while(xml.findChild("Configuration")) {
			const string& id = xml.getChildAttrib("Id");
			if(id.empty())
				continue;
			setMaxRecentItems(id, xml.getIntChildAttrib("MaxItems"));
		}
		xml.stepOut();
		parseTags(xml, &WindowManager::addRecent_);
	}
}

void WindowManager::on(SettingsManagerListener::Save, SimpleXML& xml) noexcept {
	xml.addTag("Windows");
	xml.stepIn();
	for(auto& i: list)
		addTag(xml, i);
	xml.stepOut();

	xml.addTag("Recent");
	xml.stepIn();
	for(auto& i: maxRecentItems) {
		xml.addTag("Configuration");
		xml.addChildAttrib("Id", i.first);
		xml.addChildAttrib("MaxItems", i.second);
	}
	for(auto& ri: recent) {
		for(auto& i: ri.second)
			addTag(xml, i);
	}
	xml.stepOut();
}

} // namespace dcpp
