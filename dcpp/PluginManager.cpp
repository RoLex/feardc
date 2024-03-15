/*
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "PluginManager.h"

#include "Archive.h"
#include "Client.h"
#include "ClientManager.h"
#include "ConnectionManager.h"
#include "File.h"
#include "LogManager.h"
#include "QueueManager.h"
#include "ScopedFunctor.h"
#include "SimpleXML.h"
#include "UserConnection.h"
#include "version.h"

#include <utility>

#include <boost/range/adaptor/reversed.hpp>

#ifdef _WIN32
# define PLUGIN_EXT "*.dll"

# define LOAD_LIBRARY(filename) ::LoadLibrary(Text::toT(filename).c_str())
# define FREE_LIBRARY(lib) ::FreeLibrary(lib)
# define GET_ADDRESS(lib, name) ::GetProcAddress(lib, name)
# define GET_ERROR() Util::translateError(GetLastError())
#else
# include "dlfcn.h"
# define PLUGIN_EXT "*.so"

# define LOAD_LIBRARY(filename) ::dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL)
# define FREE_LIBRARY(lib) ::dlclose(lib)
# define GET_ADDRESS(lib, name) ::dlsym(lib, name)
# define GET_ERROR() ::dlerror()
#endif

namespace dcpp {

using std::make_unique;
using std::swap;

PluginManager::PluginManager() : dcCore(), shutdown(false), secNum(Util::rand()) {
}

PluginManager::~PluginManager() {
}

DcextInfo PluginManager::extract(const string& path) {
	const auto dir = Util::getTempPath() + "dcext" PATH_SEPARATOR_STR;
	const auto info_path = dir + "info.xml";
	File::deleteFile(info_path);
	Archive(path).extract(dir);

	SimpleXML xml;
	xml.fromXML(File(info_path, File::READ, File::OPEN).read());

	DcextInfo info { };

	if(xml.findChild("dcext")) {
		xml.stepIn();

		auto parse = [&xml](string tag, string& out) {
			xml.resetCurrentChild();
			if(xml.findChild(tag)) {
				out = xml.getChildData();
			}
		};

		auto checkPlatform = [&xml](bool emptyAllowed) {
			auto platform = xml.getChildAttrib("Platform");
			if(emptyAllowed && platform.empty()) {
				return true;
			}
#if defined(_WIN32) && defined(_WIN64)
			return platform == "pe-x64";
#elif defined(_WIN32)
			return platform == "pe-x86";
#elif defined(__x86_64__)
			return platform == "elf-x64";
#elif defined(__i386__)
			return platform == "elf-x86";
#else
#error Unknown platform
#endif
		};

		string version;
		parse("ApiVersion", version);
		if(Util::toInt(version) != DCAPI_CORE_VER) {
			throw Exception(str(F_("%1% is not compatible with %2%") % Util::getFileName(path) % APPNAME));
		}

		parse("UUID", info.uuid);
		parse("Name", info.name);
		version.clear(); parse("Version", version); info.version = Util::toDouble(version);
		parse("Author", info.author);
		parse("Description", info.description);
		parse("Website", info.website);

		xml.resetCurrentChild();
		while(xml.findChild("Plugin")) {
			if(checkPlatform(false)) {
				info.plugin = xml.getChildData();
			}
		}

		xml.resetCurrentChild();
		if(xml.findChild("Files")) {
			xml.stepIn();

			while(xml.findChild("File")) {
				if(checkPlatform(true)) {
					info.files.push_back(xml.getChildData());
				}
			}

			xml.stepOut();
		}

		xml.stepOut();
	}

	if(info.uuid.empty() || info.name.empty() || info.version == 0) {
		throw Exception(str(F_("%1% is not a valid plugin") % Util::getFileName(path)));
	}
	if(info.plugin.empty()) {
		throw Exception(str(F_("%1% is not compatible with %2%") % Util::getFileName(path) % APPNAME));
	}

	{
		Lock l(cs);

		auto dupe = findPlugin(info.uuid);
		if(dupe) {
			if(dupe->version < info.version) {
				info.updating = true;
			} else {
				throw Exception(str(F_("%1% is already installed") % dupe->name));
			}
		}
	}

	return info;
}

void PluginManager::install(const DcextInfo& info) {
	{
		Lock l(cs);

		auto dupe = findPlugin(info.uuid);
		if(dupe) {
			if(dupe->version < info.version) {
				// disable when updating or the file copy will fail...
				if(dupe->handle) {
					disable(*dupe, false);
				}
			} else {
				throw Exception(str(F_("%1% is already installed") % dupe->name));
			}
		}
	}

	const auto source = Util::getTempPath() + "dcext" PATH_SEPARATOR_STR;
	const auto target = getInstallPath(info.uuid);
	const auto lib = target + Util::getFileName(info.plugin);

	File::ensureDirectory(lib);
	File::renameFile(source + info.plugin, lib);

	for(auto& file: info.files) {
		File::ensureDirectory(target + file);
		File::renameFile(source + file, target + file);
	}

	addPlugin(lib);
}

void PluginManager::loadPlugins(function<void (const string&)> f) {
	TimerManager::getInstance()->addListener(this);
	ClientManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	loadSettings();

	for(auto& plugin: plugins) {
		if(!plugin.dcMain) { continue; } // a little trick to avoid an additonal "bool enabled"
		if(f) { f(plugin.name); }
		try { enable(plugin, false, false); }
		catch(const Exception& e) { LogManager::getInstance()->message(e.getError()); }
	}
}

void PluginManager::unloadPlugins() {
	TimerManager::getInstance()->removeListener(this);
	ClientManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);

	Lock l(cs);
	shutdown = true;

	saveSettings();

	// Off we go...
	for(auto& plugin: plugins | boost::adaptors::reversed) {
		if(plugin.handle) {
			disable(plugin, false);
		}
	}
	plugins.clear();

	// Really unload plugins that have been flagged inactive (ON_UNLOAD returns False)
	for(auto& i: inactive)
		FREE_LIBRARY(i);

	// Destroy hooks that may have not been correctly freed
	hooks.clear();
}

void PluginManager::addPlugin(const string& path) {
	Lock l(cs);

	Plugin plugin { };
	plugin.name = Util::getFileName(path);
	plugin.path = path;

	enable(plugin, true, true);
}

bool PluginManager::configPlugin(const string& guid, dcptr_t data) {
	Lock l(cs);
	auto p = findPlugin(guid);
	if(p && p->handle) {
		return p->dcMain(ON_CONFIGURE, &dcCore, data);
	}
	return false;
}

void PluginManager::enablePlugin(const string& guid) {
	Lock l(cs);
	auto p = findPlugin(guid);
	if(p && !p->handle) {
		enable(*p, false, true);
	}
}

void PluginManager::disablePlugin(const string& guid) {
	Lock l(cs);
	auto p = findPlugin(guid);
	if(p && p->handle) {
		disable(*p, false);
	}
}

void PluginManager::movePlugin(const string& guid, int delta) {
	Lock l(cs);
	auto i = findPluginIter(guid);
	if(i != plugins.end()) {
		swap(*i, *(i + delta));
	}
}

void PluginManager::removePlugin(const string& guid) {
	Lock l(cs);
	auto i = findPluginIter(guid);
	if(i != plugins.end()) {
		if(i->handle) {
			disable(*i, true);
		}
		plugins.erase(i);
	}
}

bool PluginManager::isLoaded(const string& guid) const {
	Lock l(cs);
	auto p = findPlugin(guid);
	return p ? bool(p->handle) : false;
}

StringList PluginManager::getPluginList() const {
	Lock l(cs);
	StringList ret(plugins.size());
	std::transform(plugins.begin(), plugins.end(), ret.begin(), [](const Plugin& p) { return p.guid; });
	return ret;
};

Plugin PluginManager::getPlugin(const string& guid) const {
	Lock l(cs);
	auto p = findPlugin(guid);
	return p ? *p : Plugin();
}

// Functions that call the plugin
bool PluginManager::onUDP(bool out, const string& ip, const string& port, const string& data) {
	UDPData udp = { ip.c_str(), static_cast<uint16_t>(Util::toUInt(port)) };
	return runHook(out ? HOOK_NETWORK_UDP_OUT : HOOK_NETWORK_UDP_IN, &udp,
		reinterpret_cast<dcptr_t>(const_cast<char*>(data.c_str())));
}

bool PluginManager::onChatTags(Tagger& tagger, OnlineUser* from) {
	TagData data = { reinterpret_cast<dcptr_t>(&tagger), True };
	return runHook(HOOK_UI_CHAT_TAGS, from, &data);
}

bool PluginManager::onChatDisplay(string& htmlMessage, OnlineUser* from) {
	StringData data = { htmlMessage.c_str() };
	bool handled = runHook(HOOK_UI_CHAT_DISPLAY, from, &data);
	if(handled && data.out) {
		htmlMessage = data.out;
		return true;
	}
	return false;
}

bool PluginManager::onChatCommand(Client* client, const string& line) {
	string cmd, param;
	auto si = line.find(' ');
	if(si != string::npos) {
		param = line.substr(si + 1);
		cmd = line.substr(1, si - 1);
	} else {
		cmd = line.substr(1);
	}

	CommandData data = { cmd.c_str(), param.c_str() };
	return runHook(HOOK_UI_CHAT_COMMAND, client, &data);
}

bool PluginManager::onChatCommandPM(const HintedUser& user, const string& line) {
	// Hopefully this is safe...
	bool res = false;
	auto lock = ClientManager::getInstance()->lock();
	OnlineUser* ou = ClientManager::getInstance()->findOnlineUser(user.user->getCID(), user.hint);

	if(ou) {
		string cmd, param;
		auto si = line.find(' ');
		if(si != string::npos) {
			param = line.substr(si + 1);
			cmd = line.substr(1, si - 1);
		} else {
			cmd = line.substr(1);
		}

		CommandData data = { cmd.c_str(), param.c_str() };
		res = runHook(HOOK_UI_CHAT_COMMAND_PM, ou, &data);
	}

	return res;
}

// Plugin interface registry
intfHandle PluginManager::registerInterface(const string& guid, dcptr_t funcs) {
	if(interfaces.find(guid) == interfaces.end())
		interfaces.insert(make_pair(guid, funcs));
	// Following ensures that only the original provider may remove this
	return reinterpret_cast<intfHandle>((uintptr_t)funcs ^ secNum);
}

dcptr_t PluginManager::queryInterface(const string& guid) {
	auto i = interfaces.find(guid);
	if(i != interfaces.end()) {
		return i->second;
	} else return NULL;
}

bool PluginManager::releaseInterface(intfHandle hInterface) {
	// Following ensures that only the original provider may remove this
	dcptr_t funcs = reinterpret_cast<dcptr_t>((uintptr_t)hInterface ^ secNum);
	for(auto i = interfaces.begin(); i != interfaces.end(); ++i) {
		if(i->second == funcs) {
			interfaces.erase(i);
			return true;
		}
	}
	return false;
}

// Plugin Hook system
PluginHook* PluginManager::createHook(const string& guid, DCHOOK defProc) {
	Lock l(csHook);

	auto i = hooks.find(guid);
	if(i != hooks.end()) return NULL;

	auto hook = make_unique<PluginHook>();
	auto pHook = hook.get();
	hook->guid = guid;
	hook->defProc = defProc;
	hooks[guid] = move(hook);
	return pHook;
}

bool PluginManager::destroyHook(PluginHook* hook) {
	Lock l(csHook);

	auto i = hooks.find(hook->guid);
	if(i != hooks.end()) {
		hooks.erase(i);
		return true;
	}
	return false;
}

HookSubscriber* PluginManager::bindHook(const string& guid, DCHOOK hookProc, void* pCommon) {
	Lock l(csHook);

	auto i = hooks.find(guid);
	if(i == hooks.end()) {
		return NULL;
	}
	auto& hook = i->second;
	{
		Lock l(hook->cs);
		auto subscription = make_unique<HookSubscriber>();
		auto pSub = subscription.get();
		subscription->hookProc = hookProc;
		subscription->common = pCommon;
		subscription->owner = hook->guid;
		hook->subscribers.push_back(move(subscription));

		return pSub;
	}
}

bool PluginManager::runHook(PluginHook* hook, dcptr_t pObject, dcptr_t pData) {
	dcassert(hook);

	// Using shared lock (csHook) might be a tad safer, but it is also slower and I prefer not to teach plugins to rely on the effect it creates
	Lock l(hook->cs);

	Bool bBreak = False;
	Bool bRes = False;
	for(auto& sub: hook->subscribers) {
		if(sub->hookProc(pObject, pData, sub->common, &bBreak))
			bRes = True;
		if(bBreak) return (bRes != False);
	}

	// Call default hook procedure for all unused hooks
	if(hook->defProc && hook->subscribers.empty()) {
		if(hook->defProc(pObject, pData, NULL, &bBreak))
			bRes = True;
	}

	return (bRes != False);
}

size_t PluginManager::releaseHook(HookSubscriber* subscription) {
	Lock l(csHook);

	if(subscription == NULL)
		return 0;

	auto i = hooks.find(subscription->owner);
	if(i == hooks.end()) {
		return 0;
	}
	auto& hook = i->second;
	{
		Lock l(hook->cs);
		hook->subscribers.erase(std::remove_if(hook->subscribers.begin(), hook->subscribers.end(),
			[subscription](const unique_ptr<HookSubscriber>& sub) { return sub.get() == subscription; }), hook->subscribers.end());

		return hook->subscribers.size();
	}
}

// Plugin configuration
void PluginManager::setPluginSetting(const string& guid, const string& setting, const string& value) {
	Lock l(cs);
	auto p = findPlugin(guid);
	if(p) {
		p->settings[setting] = value;
	}
}

const string& PluginManager::getPluginSetting(const string& guid, const string& setting) {
	Lock l(cs);

	auto p = findPlugin(guid);
	if(p) {
		auto i = p->settings.find(setting);
		if(i != p->settings.end()) {
			return i->second;
		}
	}
	return Util::emptyString;
}

void PluginManager::removePluginSetting(const string& guid, const string& setting) {
	Lock l(cs);

	auto p = findPlugin(guid);
	if(p) {
		auto i = p->settings.find(setting);
		if(i != p->settings.end()) {
			p->settings.erase(i);
		}
	}
}

string PluginManager::getInstallPath(const string& uuid) {
	return Util::getPath(Util::PATH_USER_LOCAL) + "Plugins" PATH_SEPARATOR_STR + uuid + PATH_SEPARATOR_STR;
}

void PluginManager::enable(Plugin& plugin, bool install, bool runtime) {
	Lock l(cs);

	dcassert(dcCore.apiVersion != 0);

	plugin.handle = LOAD_LIBRARY(plugin.path);
	if(!plugin.handle) {
		throw Exception(str(F_("Error loading %1%: %2%") % plugin.name % GET_ERROR()));
	}
	bool unload = true;
	ScopedFunctor(([&plugin, &unload] { if(unload) { FREE_LIBRARY(plugin.handle); plugin.handle = nullptr; plugin.dcMain = nullptr; } }));

	// todo: -Wcast-function-type
	auto pluginInfo = reinterpret_cast<DCMAIN (DCAPI *)(MetaDataPtr)>(GET_ADDRESS(plugin.handle, "pluginInit"));
	MetaData info { };
	if(!pluginInfo || !(plugin.dcMain = pluginInfo(&info))) {
		throw Exception(str(F_("%1% is not a valid plugin") % plugin.name));
	}

	// Check API compatibility (this should only block on absolutely wrecking api changes, which generally should not happen)
	if(info.apiVersion != DCAPI_CORE_VER) {
		throw Exception(str(F_("%1% is not compatible with %2%") % info.name % APPNAME));
	}

	// Check that all dependencies are loaded
	for(decltype(info.numDependencies) i = 0; i < info.numDependencies; ++i) {
		if(!isLoaded(info.dependencies[i])) {
			throw Exception(str(F_("Missing dependencies for %1%") % info.name));
		}
	}

	auto pluginPtr = &plugin;

	// When installing, check for duplicates and possible updates
	if(install) {
		auto dupe = findPlugin(info.guid);
		if(dupe) {

			if(dupe->version < info.version) {
				// in-place updating to keep settings etc.

				LogManager::getInstance()->message(str(F_("Updating the %1% plugin from version %2% to version %3%") %
					string(dupe->name) % dupe->version % info.version));

				if(dupe->handle) {
					disable(*dupe, false);
				}
				dupe->handle = plugin.handle;
				dupe->dcMain = plugin.dcMain;
				pluginPtr = dupe;

				install = false;

			} else {
				throw Exception(str(F_("%1% is already installed") % info.name));
			}

		} else {
			// add to the list before executing dcMain, to guarantee a correct state (eg for settings).
			plugins.push_back(move(plugin));
			pluginPtr = &plugins.back();
		}
	}

	{ // scope to change the plugin ref
		auto& plugin = *pluginPtr;

		if(plugin.dcMain(install ? ON_INSTALL : runtime ? ON_LOAD_RUNTIME : ON_LOAD, &dcCore, nullptr) == False) {

			if(install) {
				// remove from the list.
				FREE_LIBRARY(plugin.handle);
				plugins.pop_back();
				unload = false;
			}

			throw Exception(str(F_("Error loading %1%") % plugin.name));
		}

		unload = false;

		// refresh the info we have on the plugin.
		plugin.guid = info.guid;
		plugin.name = info.name;
		plugin.version = info.version;
		plugin.author = info.author;
		plugin.description = info.description;
		plugin.website = info.web;
	}
}

void PluginManager::disable(Plugin& plugin, bool uninstall) {
	bool isSafe = true;
	if(plugin.dcMain(uninstall ? ON_UNINSTALL : ON_UNLOAD, nullptr, nullptr) == False) {
		// Plugin performs operation critical tasks (runtime unload not possible)
		isSafe = std::find(inactive.begin(), inactive.end(), plugin.handle) != inactive.end();
		if(!isSafe) {
			inactive.push_back(plugin.handle);
		}
	}
	if(isSafe) {
		FREE_LIBRARY(plugin.handle);
		plugin.handle = nullptr;
		plugin.dcMain = nullptr;
	}
}

void PluginManager::loadSettings() noexcept {
	Lock l(cs);

	plugins.clear();

	try {
		Util::migrate(Util::getPath(Util::PATH_USER_CONFIG) + "Plugins.xml");

		SimpleXML xml;
		xml.fromXML(File(Util::getPath(Util::PATH_USER_CONFIG) + "Plugins.xml", File::READ, File::OPEN).read());
		if(xml.findChild("Plugins")) {
			xml.stepIn();

			while(xml.findChild("Plugin")) {
				Plugin plugin { xml.getChildAttrib("Guid"), xml.getChildAttrib("Name"),
					Util::toDouble(xml.getChildAttrib("Version")), xml.getChildAttrib("Author"),
					xml.getChildAttrib("Description"), xml.getChildAttrib("Website"),
					xml.getChildAttrib("Path"), nullptr,
					reinterpret_cast<DCMAIN>(xml.getBoolChildAttrib("Enabled")) };

				if(plugin.guid.empty() || plugin.path.empty()) { continue; }
				if(plugin.name.empty()) { plugin.name = Util::getFileName(plugin.path); }

				xml.stepIn();
				for(auto& i: xml.getCurrentChildren()) {
					plugin.settings[i.first] = i.second;
				}
				xml.stepOut();

				plugins.push_back(move(plugin));
			}

			xml.stepOut();
		}
	} catch(const Exception& e) {
		dcdebug("PluginManager::loadSettings: %s\n", e.getError().c_str());
	}
}

void PluginManager::saveSettings() noexcept {
	Lock l(cs);

	try {
		SimpleXML xml;
		xml.addTag("Plugins");
		xml.stepIn();

		for(auto& plugin: plugins) {
			xml.addTag("Plugin");

			xml.addChildAttrib("Guid", plugin.guid);
			xml.addChildAttrib("Name", plugin.name);
			xml.addChildAttrib("Version", plugin.version);
			xml.addChildAttrib("Author", plugin.author);
			xml.addChildAttrib("Description", plugin.description);
			xml.addChildAttrib("Website", plugin.website);
			xml.addChildAttrib("Path", plugin.path);
			xml.addChildAttrib("Enabled", static_cast<bool>(plugin.handle));

			xml.stepIn();
			for(auto& i: plugin.settings) {
				xml.addTag(i.first, i.second);
			}
			xml.stepOut();
		}

		xml.stepOut();

		string fname = Util::getPath(Util::PATH_USER_CONFIG) + "Plugins.xml";
		File f(fname + ".tmp", File::WRITE, File::CREATE | File::TRUNCATE);
		f.write(SimpleXML::utf8Header);
		f.write(xml.toXML());
		f.close();
		File::deleteFile(fname);
		File::renameFile(fname + ".tmp", fname);
	} catch(const Exception& e) {
		dcdebug("PluginManager::saveSettings: %s\n", e.getError().c_str());
	}
}

const Plugin* PluginManager::findPlugin(const string& guid) const {
	auto it = findPluginIter(guid);
	return it != plugins.end() ? &*it : nullptr;
}

Plugin* PluginManager::findPlugin(const string& guid) {
	auto it = findPluginIter(guid);
	return it != plugins.end() ? &*it : nullptr;
}

vector<Plugin>::const_iterator PluginManager::findPluginIter(const string& guid) const {
	return std::find_if(plugins.begin(), plugins.end(), [&guid](const Plugin& p) { return p.guid == guid; });
}

vector<Plugin>::iterator PluginManager::findPluginIter(const string& guid) {
	return std::find_if(plugins.begin(), plugins.end(), [&guid](const Plugin& p) { return p.guid == guid; });
}

// Listeners
void PluginManager::on(ClientManagerListener::ClientConnected, Client* aClient) noexcept {
	runHook(HOOK_HUB_ONLINE, aClient);
}

void PluginManager::on(ClientManagerListener::ClientDisconnected, Client* aClient) noexcept {
	runHook(HOOK_HUB_OFFLINE, aClient);
}

void PluginManager::on(QueueManagerListener::Added, QueueItem* qi) noexcept {
	runHook(HOOK_QUEUE_ADDED, qi);
}

void PluginManager::on(QueueManagerListener::Moved, QueueItem* qi, const string& /*aSource*/) noexcept {
	runHook(HOOK_QUEUE_MOVED, qi);
}

void PluginManager::on(QueueManagerListener::Removed, QueueItem* qi) noexcept {
	runHook(HOOK_QUEUE_REMOVED, qi);
}

void PluginManager::on(QueueManagerListener::Finished, QueueItem* qi, const string& /*dir*/, int64_t /*speed*/) noexcept {
	runHook(HOOK_QUEUE_FINISHED, qi);
}

void PluginManager::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) noexcept {
	saveSettings();
}

} // namespace dcpp
