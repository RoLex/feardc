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

#include "stdafx.h"
#include "PrivateFrame.h"

#include "HoldRedraw.h"
#include "MainWindow.h"
#include "resource.h"

#include <dcpp/AdcHub.h>
#include <dcpp/ChatMessage.h>
#include <dcpp/ClientManager.h>
#include <dcpp/Client.h>
#include <dcpp/ConnectionManager.h>
#include <dcpp/LogManager.h>
#include <dcpp/PluginManager.h>
#include <dcpp/User.h>
#include <dcpp/UserConnection.h>
#include <dcpp/WindowInfo.h>

#include <dwt/util/StringUtils.h>

const string PrivateFrame::id = "PM";
const string& PrivateFrame::getId() const { return id; }

PrivateFrame::FrameMap PrivateFrame::frames;

void PrivateFrame::openWindow(TabViewPtr parent, const HintedUser& replyTo_, const tstring& msg,
	const string& logPath, bool activate)
{
	auto i = frames.find(replyTo_);
	auto frame = (i == frames.end()) ? new PrivateFrame(parent, replyTo_, logPath) : i->second;
	if(activate)
		frame->activate();
	if(!msg.empty())
		frame->sendMessage(msg);
}

bool PrivateFrame::gotMessage(TabViewPtr parent, const ChatMessage& message, const string& hubHint, bool fromBot) {
	auto& user = (message.replyTo == ClientManager::getInstance()->getMe()) ? message.to : message.replyTo;

	auto i = frames.find(user);
	if(i == frames.end()) {
		// creating a new window

		if(static_cast<int>(frames.size()) >= SETTING(MAX_PM_WINDOWS)) {
			return false;
		}

		auto p = new PrivateFrame(parent, HintedUser(user, hubHint));
		if(!SETTING(POPUNDER_PM))
			p->activate();

		p->addChat(message);
		p->lastMessageTime = message.timestamp;

		if(Util::getAway() && !(SETTING(NO_AWAYMSG_TO_BOTS) && fromBot)) {
			auto awayMessage = Util::getAwayMessage();
			if(!awayMessage.empty()) {
				p->sendMessage(Text::toT(awayMessage));
			}
		}

		WinUtil::notify(WinUtil::NOTIFICATION_PM_WINDOW, Text::toT(message.message), [user] { activateWindow(user); });

	} else {
		// send the message to the existing window
		i->second->addChat(message);
		i->second->lastMessageTime = message.timestamp;
	}

	WinUtil::notify(WinUtil::NOTIFICATION_PM, Text::toT(message.message), [user] { activateWindow(user); });

	return true;
}

void PrivateFrame::activateWindow(const UserPtr& u) {
	auto i = frames.find(u);
	if(i != frames.end())
		i->second->activate();
}

void PrivateFrame::closeAll(bool offline) {
	for(auto& i: frames) {
		if(!offline || !i.second->online) {
			i.second->close(true);
		}
	}
}

WindowParams PrivateFrame::getWindowParams() const {
	WindowParams ret;
	addRecentParams(ret);
	ret["CID"] = WindowParam(replyTo.getUser().user->getCID().toBase32(), WindowParam::FLAG_IDENTIFIES | WindowParam::FLAG_CID);
	ret["Hub"] = WindowParam(replyTo.getUser().hint);
	ret["LogPath"] = WindowParam(getLogPath());
	return ret;
}

void PrivateFrame::parseWindowParams(TabViewPtr parent, const WindowParams& params) {
	auto cid = params.find("CID");
	auto hub = params.find("Hub");
	if(cid != params.end() && hub != params.end()) {
		auto logPath = params.find("LogPath");
		openWindow(parent, HintedUser(ClientManager::getInstance()->getUser(CID(cid->second)), hub->second), Util::emptyStringT,
			logPath != params.end() ? logPath->second.content : Util::emptyString, parseActivateParam(params));
	}
}

bool PrivateFrame::isFavorite(const WindowParams& params) {
	auto cid = params.find("CID");
	if(cid != params.end()) {
		UserPtr u = ClientManager::getInstance()->getUser(CID(cid->second));
		if(u)
			return FavoriteManager::getInstance()->isFavoriteUser(u);
	}
	return false;
}

PrivateFrame::PrivateFrame(TabViewPtr parent, const HintedUser& replyTo_, const string& logPath) :
BaseType(parent, _T(""), IDH_PM, IDI_PRIVATE_OFF, false),
replyTo(replyTo_),
online(false),
conn(nullptr),
lastMessageTime(time(NULL))
{
	createChat(this);
	chat->setHelpId(IDH_PM_CHAT);
	addWidget(chat);
	chat->onContextMenu([this](const dwt::ScreenCoordinate &sc) { return handleChatContextMenu(sc); });

	message->setHelpId(IDH_PM_MESSAGE);
	addWidget(message, ALWAYS_FOCUS);
	message->onKeyDown([this](int c) { return handleMessageKeyDown(c); });
	message->onSysKeyDown([this](int c) { return handleMessageKeyDown(c); });
	message->onChar([this](int c) { return handleMessageChar(c); });

	initStatus();

	status->onDblClicked(STATUS_STATUS, [this] { openLog(); });

	{
		auto f = [this] { handleChannelMenu(); };
		status->onClicked(STATUS_CHANNEL, f);
		status->onRightClicked(STATUS_CHANNEL, f);
	}

	status->setToolTip(STATUS_CHANNEL, T_("Current communication channel - click to change"));

	status->setHelpId(STATUS_STATUS, IDH_PM_STATUS);
	status->setHelpId(STATUS_CHANNEL, IDH_PM_CHANNEL);

	initAccels();

	layout();

	readLog(logPath, SETTING(PM_LAST_LOG_LINES));

	ConnectionManager::getInstance()->addListener(this);
	{
		Lock l(mutex);
		conn = WinUtil::mainWindow->getPMConn(replyTo.getUser(), this);
	}

	callAsync([this] {
		ClientManager::getInstance()->addListener(this);
		updateOnlineStatus(true);
	});

	frames.emplace(replyTo.getUser(), this);

	addRecent();
}

PrivateFrame::~PrivateFrame() {
}

void PrivateFrame::addedChat(const tstring& message) {
	setDirty(SettingsManager::BOLD_PM);

	if (ccReady() && SETTING(DONT_LOG_CCPM)) return;

	if(SETTING(LOG_PRIVATE_CHAT)) {
		ParamMap params;
		params["message"] = [&message] { return Text::toDOS(Text::fromT(message)); };
		fillLogParams(params);
		LOG(LogManager::PM, params);
	}
}

void PrivateFrame::addStatus(const tstring& text) {
	status->setText(STATUS_STATUS, Text::toT("[" + Util::getShortTimeString() + "] ") + text);

	if(SETTING(STATUS_IN_CHAT)) {
		addChat(_T("*** ") + text);
	} else {
		setDirty(SettingsManager::BOLD_PM);
	}
}

bool PrivateFrame::preClosing() {
	{
		Lock l(mutex);
		if(conn) {
			conn->removeListener(this);
			conn->disconnect(true);
		}
	}

	ClientManager::getInstance()->removeListener(this);
	ConnectionManager::getInstance()->removeListener(this);

	frames.erase(replyTo.getUser());
	return true;
}

string PrivateFrame::getLogPath() const {
	ParamMap params;
	fillLogParams(params);
	return Util::validateFileName(LogManager::getInstance()->getPath(LogManager::PM, params));
}

void PrivateFrame::openLog() {
	WinUtil::openFile(Text::toT(getLogPath()));
}

void PrivateFrame::fillLogParams(ParamMap& params) const {
	params["hubNI"] = [this] { return ClientManager::getInstance()->getHubName(replyTo.getUser()); };
	params["hubURL"] = [this] { return replyTo.getUser().hint; };
	params["userCID"] = [this] { return replyTo.getUser().user->getCID().toBase32(); };
	params["userNI"] = [this] { return ClientManager::getInstance()->getNick(replyTo.getUser()); };
	params["myCID"] = [] { return ClientManager::getInstance()->getMe()->getCID().toBase32(); };
}

void PrivateFrame::layout() {
	const int border = 2;

	dwt::Rectangle r { getClientSize() };

	r.size.y -= status->refresh();

	int ymessage = message->getTextSize(_T("A")).y * messageLines + 10;
	dwt::Rectangle rm(0, r.size.y - ymessage, r.width(), ymessage);
	message->resize(rm);

	r.size.y -= rm.size.y + border;
	chat->resize(r);
}

void PrivateFrame::updateOnlineStatus(bool newChannel) {
	auto hubs = ClientManager::getInstance()->getHubUrls(replyTo.getUser());

	if(newChannel || online != !hubs.empty()) {
		online = !hubs.empty();

		if(!newChannel) {
			addStatus(online ? T_("User went online") : T_("User went offline"));
		}
		setIcon(online ? IDI_PRIVATE : IDI_PRIVATE_OFF);
		status->setIcon(STATUS_CHANNEL, WinUtil::statusIcon(ccReady() ? IDI_SECURE : online ? IDI_HUB : IDI_HUB_OFF));
		newChannel = true;
	}

	if(online && find(hubs.begin(), hubs.end(), replyTo.getUser().hint) == hubs.end()) {
		replyTo.getUser().hint = hubs.front();
		newChannel = true;
	}

	setText(WinUtil::getNick(replyTo.getUser()) + _T(" - ") + WinUtil::getHubName(replyTo.getUser()));

	if(newChannel) {
		updateChannel();

		if(online && SETTING(ALWAYS_CCPM) && !ccReady()) {
			startCC(true);
		}
	}
}

void PrivateFrame::updateChannel() {
	auto channel = ccReady() ? T_("Direct encrypted channel") : WinUtil::getHubName(replyTo.getUser());
	dwt::util::cutStr(channel, 26);
	status->setText(STATUS_CHANNEL, channel, true);
}

void PrivateFrame::startCC(bool silent) {
	if(ccReady()) {
		if(!silent) { addStatus(T_("A direct encrypted channel is already available")); }
		return;
	}

	{
		auto lock = ClientManager::getInstance()->lock();
		auto ou = ClientManager::getInstance()->findOnlineUser(replyTo.getUser());
		if(!ou) {
			if(!silent) { addStatus(T_("User offline")); }
			return;
		}

		tstring err = ou->getUser()->isNMDC() ? T_("A secure ADC hub is required; this feature is not supported on NMDC hubs") :
			!ou->getIdentity().supports(AdcHub::CCPM_FEATURE) ? T_("The user does not support the CCPM ADC extension") : _T("");
		if(!err.empty()) {
			if(!silent) { addStatus(str(TF_("Cannot start the direct encrypted channel: %1%") % err)); }
			return;
		}
	}

	if(!silent) { addStatus(T_("Establishing a direct encrypted channel...")); }
	ClientManager::getInstance()->connect(replyTo.getUser(), ConnectionManager::getInstance()->makeToken(), CONNECTION_TYPE_PM);
}

void PrivateFrame::closeCC(bool silent) {
	if(ccReady()) {
		if(!silent) { addStatus(T_("Disconnecting the direct encrypted channel...")); }
		ConnectionManager::getInstance()->disconnect(replyTo.getUser(), CONNECTION_TYPE_PM);
	} else {
		if(!silent) { addStatus(T_("No direct encrypted channel available")); }
	}
}

bool PrivateFrame::ccReady() const {
	Lock l(mutex);
	return conn;
}

void PrivateFrame::enterImpl(const tstring& s) {
	bool resetText = true;
	bool send = false;

	// Process special commands
	if(s[0] == '/') {
		tstring cmd = s;
		tstring param;
		tstring message;
		tstring status;
		bool thirdPerson = false;

		if(PluginManager::getInstance()->onChatCommandPM(replyTo.getUser(), Text::fromT(s))) {
			// Plugins, chat commands

		} else if(WinUtil::checkCommand(cmd, param, message, status, thirdPerson)) {
			if(!message.empty()) {
				sendMessage(message, thirdPerson);
			}
			if(!status.empty()) {
				addStatus(status);
			}
		} else if(ChatType::checkCommand(cmd, param, status)) {
			if(!status.empty()) {
				addStatus(status);
			}
		} else if(Util::stricmp(cmd.c_str(), _T("grant")) == 0) {
			handleGrantSlot();
			addStatus(T_("Slot granted"));
		} else if(Util::stricmp(cmd.c_str(), _T("close")) == 0) {
			postMessage(WM_CLOSE);
		} else if(Util::stricmp(cmd.c_str(), _T("direct")) == 0 || Util::stricmp(cmd.c_str(), _T("encrypted")) == 0) {
			startCC();
		} else if((Util::stricmp(cmd.c_str(), _T("favorite")) == 0) || (Util::stricmp(cmd.c_str(), _T("fav")) == 0)) {
			handleAddFavorite();
			addStatus(T_("Favorite user added"));
		} else if(Util::stricmp(cmd.c_str(), _T("getlist")) == 0) {
			handleGetList(getParent());
		} else if(Util::stricmp(cmd.c_str(), _T("ignore")) == 0) {
			handleIgnoreChat(true);
		} else if(Util::stricmp(cmd.c_str(), _T("unignore")) == 0) {
			handleIgnoreChat(false);
		} else if(Util::stricmp(cmd.c_str(), _T("log")) == 0) {
			openLog();
		} else if(Util::stricmp(cmd.c_str(), _T("lastmessage")) == 0) {
			addStatus(Text::toT(str(F_("Last message occured %1%") % Util::getTimeString(lastMessageTime, "%c"))));
		} else if(Util::stricmp(cmd.c_str(), _T("help")) == 0) {
			bool bShowBriefCommands = !param.empty() && (Util::stricmp(param.c_str(), _T("brief")) == 0);

			if(bShowBriefCommands)
			{
				addChat(T_("*** Keyboard commands:") + _T("\r\n") + 
						WinUtil::commands + 
						_T(", /direct, /encrypted, /getlist, /grant, /close, /favorite, /ignore, /unignore, /log <system, downloads, uploads>, /lastmessage")
						);
			}
			else
			{
				addChat(T_("*** Keyboard commands:") + _T("\r\n") +
						WinUtil::getDescriptiveCommands() +
						+ _T("\r\n") _T("/direct")
						+ _T("\r\n") _T("/encrypted")
						+ _T("\r\n\t") + T_("Starts a direct encrypted communication channel to avoid spying on your private messages.")
						+ _T("\r\n") _T("/getlist")
						+ _T("\r\n\t") + T_("Adds the current user's list to the Download Queue.")
						+ _T("\r\n") _T("/grant")
						+ _T("\r\n\t") + T_("Grants the remote user a slot. Once they connect, or if they don't connect in 10 minutes, the granted slot is removed.")
						+ _T("\r\n") _T("/favorite")
						+ _T("\r\n") _T("/fav")
						+ _T("\r\n\t") + T_("Adds the current user to the list of Favorite Users.")
						+ _T("\r\n") _T("/ignore")
						+ _T("\r\n\t") + T_("Adds a user matching definition (or modifies an existing one, if possible) to ignore chat messages from the current user.")
						+ _T("\r\n") _T("/unignore")
						+ _T("\r\n\t") + T_("Adds a user matching definition (or modifies an existing one, if possible) to stop ignoring chat messages from the current user.")
						+ _T("\r\n") _T("/lastmessage")
						+ _T("\r\n\t") + T_("Lists the date and time when the last message was sent or received.")
						);
			}

		} else if(SETTING(SEND_UNKNOWN_COMMANDS)) {
			send = true;
		} else {
			addStatus(str(TF_("Unknown command: %1%") % cmd));
		}

	} else {
		send = true;
	}

	if(send) {
		if(online || ccReady()) {
			sendMessage(s);
		} else {
			message->showPopup(T_("User offline"), T_("The message cannot be delivered because the user is offline."), TTI_ERROR);
			resetText = false;
		}
	}
	if(resetText) {
		message->setText(Util::emptyStringT);
	}
}

void PrivateFrame::sendMessage(const tstring& msg, bool thirdPerson) {
	auto msg8 = Text::fromT(msg);

	{
		Lock l(mutex);
		if(conn) {
			conn->pm(msg8, thirdPerson);
			return;
		}
	}

	ClientManager::getInstance()->privateMessage(replyTo.getUser(), msg8, thirdPerson);
}

PrivateFrame::UserInfoList PrivateFrame::selectedUsersImpl() {
	return UserInfoList(1, &replyTo);
}

void PrivateFrame::tabMenuImpl(dwt::Menu* menu) {
	appendUserItems(getParent(), menu, false, false);
	prepareMenu(menu, UserCommand::CONTEXT_USER, replyTo.getUser().hint);
	menu->appendSeparator();
}

bool PrivateFrame::handleChatContextMenu(dwt::ScreenCoordinate pt) {
	if(pt.x() == -1 || pt.y() == -1) {
		pt = chat->getContextMenuPos();
	}

	auto menu = chat->getMenu();

	menu->setTitle(escapeMenu(getText()), getParent()->getIcon(this));

	prepareMenu(menu.get(), UserCommand::CONTEXT_USER, replyTo.getUser().hint);

	menu->open(pt);
	return true;
}

void PrivateFrame::handleChannelMenu() {
	auto menu = addChild(WinUtil::Seeds::menu);

	menu->setTitle(T_("Communication channel"));

	auto hubs = ClientManager::getInstance()->getHubs(replyTo.getUser());

	if(hubs.empty()) {
		menu->appendItem(T_("(User offline)"), nullptr, nullptr, false);

	} else {
		auto cc = ccReady();

		for(auto& hub: hubs) {
			auto url = hub.first;
			auto current = !cc && url == replyTo.getUser().hint;
			auto pos = menu->appendItem(dwt::util::escapeMenu(Text::toT(hub.second)),
				[this, url] { closeCC(true); replyTo.getUser().hint = url; updateChannel(); }, nullptr, !current);
			if(current) {
				menu->checkItem(pos);
			}
		}

		if(SETTING(ENABLE_CCPM)) {
			menu->appendSeparator();

			if(cc) {
				menu->appendItem(T_("Disconnect the direct encrypted channel"), [this] { closeCC(); });
			} else {
				menu->appendItem(T_("Start a direct encrypted channel"), [this] { startCC(); }, WinUtil::menuIcon(IDI_SECURE));
			}
		}
	}

	menu->open();
}

void PrivateFrame::runUserCommand(const UserCommand& uc) {
	if(!WinUtil::getUCParams(this, uc, ucLineParams))
		return;

	auto ucParams = ucLineParams;
	ClientManager::getInstance()->userCommand(replyTo.getUser(), uc, ucParams, true);
}

void PrivateFrame::on(ClientManagerListener::UserConnected, const UserPtr& aUser) noexcept {
	if(replyTo.getUser() == aUser)
		callAsync([this] { updateOnlineStatus(); });
}

void PrivateFrame::on(ClientManagerListener::UserUpdated, const OnlineUser& aUser) noexcept {
	if(replyTo.getUser() == aUser.getUser())
		callAsync([this] { updateOnlineStatus(); });
}

void PrivateFrame::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) noexcept {
	if(replyTo.getUser() == aUser)
		callAsync([this] { updateOnlineStatus(); });
}

void PrivateFrame::on(ConnectionManagerListener::Connected, ConnectionQueueItem* cqi, UserConnection* uc) noexcept {
	if(cqi->getType() == CONNECTION_TYPE_PM && cqi->getUser() == replyTo.getUser()) {
		{
			Lock l(mutex);
			if(conn) {
				conn->removeListener(this);
			}
			conn = uc;
			conn->addListener(this);
		}
		callAsync([this] {
			updateOnlineStatus(true);
			addStatus(T_("A direct encrypted channel has been established"));
		});
	}
}

void PrivateFrame::on(ConnectionManagerListener::Removed, ConnectionQueueItem* cqi) noexcept {
	if(cqi->getType() == CONNECTION_TYPE_PM && cqi->getUser() == replyTo.getUser()) {
		{
			Lock l(mutex);
			conn = nullptr;
		}
		callAsync([this] {
			updateOnlineStatus(true);
			addStatus(T_("The direct encrypted channel has been disconnected"));
		});
	}
}

void PrivateFrame::on(UserConnectionListener::PrivateMessage, UserConnection* uc, const ChatMessage& message) noexcept {
	auto user = uc->getHintedUser();
	callAsync([this, message, user] {
		addChat(message);
		lastMessageTime = message.timestamp;
		WinUtil::notify(WinUtil::NOTIFICATION_PM, Text::toT(message.message), [user] { activateWindow(user); });
		WinUtil::mainWindow->TrayPM();
	});
}
