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

#ifndef DCPLUSPLUS_DCPP_CLIENT_H
#define DCPLUSPLUS_DCPP_CLIENT_H

#include "compiler.h"

#include "BufferedSocketListener.h"
#include "ClientListener.h"
#include "ConnectionType.h"
#include "forward.h"
#include "HubSettings.h"
#include "OnlineUser.h"
#include "PluginEntity.h"
#include "Speaker.h"
#include "TimerManager.h"

#include <boost/core/noncopyable.hpp>
#include <atomic>

namespace dcpp {

/** Yes, this should probably be called a Hub */
class Client :
	public PluginEntity<HubData>,
	public Speaker<ClientListener>,
	public BufferedSocketListener,
	protected TimerManagerListener,
	public HubSettings,
	private boost::noncopyable
{
public:
	virtual void connect();
	virtual void disconnect(bool graceless);

	virtual void connect(const OnlineUser& user, const string& token, ConnectionType type) = 0;
	virtual void hubMessage(const string& aMessage, bool thirdPerson = false) = 0;
	virtual void privateMessage(const OnlineUser& user, const string& aMessage, bool thirdPerson = false) = 0;
	virtual void sendUserCmd(const UserCommand& command, const ParamMap& params) = 0;
	virtual void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken, const StringList& aExtList, const string& aKey) = 0;
	virtual void password(const string& pwd) = 0;
	/** Send new information about oneself. Thread-safe. */
	void info();

	virtual size_t getUserCount() const = 0;
	virtual int64_t getAvailable() const = 0;

	virtual void emulateCommand(const string& cmd) = 0;
	virtual void send(const AdcCommand& command) = 0;

	bool isConnected() const;
	bool isSecure() const;
	bool isTrusted() const;
	string getCipherName() const;
	ByteVector getKeyprint() const;

	bool isOp() const { return getMyIdentity().isOp(); }

	const string& getPort() const { return port; }
	const string& getAddress() const { return address; }

	const string& getIp() const { return ip; }
	string getIpPort() const { return getIp() + ':' + port; }

	/** Send a ClientListener::Updated signal for every connected user. */
	void updateUsers();

	static string getCounts();

	void setActive();
	void reconnect();
	void shutdown();
	bool isActive() const;
	bool isActiveV4() const;
	bool isActiveV6() const;

	void send(const string& aMessage) { send(aMessage.c_str(), aMessage.length()); }
	void send(const char* aMessage, size_t aLen);

	HubData* getPluginObject() noexcept;

	string getMyNick() const { return getMyIdentity().getNick(); }
	string getHubName() const { return getHubIdentity().getNick().empty() ? getHubUrl() : getHubIdentity().getNick(); }
	string getHubDescription() const { return getHubIdentity().getDescription(); }

	const string& getHubUrl() const { return hubUrl; }

	GETSET(Identity, hubIdentity, HubIdentity);
	Identity& getHubIdentity() { return hubIdentity; }

	// break this GETSET up to add a setSelf call.
private:
	Identity myIdentity;
public:
	const Identity& getMyIdentity() const { return myIdentity; }
	template<typename GetSetT> void setMyIdentity(GetSetT&& myIdentity) {
		myIdentity.setSelf();
		this->myIdentity = std::forward<GetSetT>(myIdentity);
	}

	GETSET(uint32_t, uniqueId, UniqueId);
	GETSET(string, defpassword, Password);
	GETSET(uint32_t, reconnDelay, ReconnDelay);
	GETSET(uint64_t, lastActivity, LastActivity);
	GETSET(bool, registered, Registered);
	GETSET(bool, autoReconnect, AutoReconnect);
	GETSET(string, encoding, Encoding);

protected:
	friend class ClientManager;
	Client(const string& hubURL, char separator, bool secure_);
	virtual ~Client();

	enum CountType {
		COUNT_NORMAL,
		COUNT_REGISTERED,
		COUNT_OP,
		COUNT_UNCOUNTED,
	};

	static std::atomic_long counts[COUNT_UNCOUNTED];

	enum State {
		STATE_CONNECTING,	///< Waiting for socket to connect
		STATE_PROTOCOL,		///< Protocol setup
		STATE_IDENTIFY,		///< Nick setup
		STATE_VERIFY,		///< Checking password
		STATE_NORMAL,		///< Running
		STATE_DISCONNECTED,	///< Nothing in particular
	};
	std::atomic<State> state;

	BufferedSocket *sock;

	/** Update hub counts. Thread-safe. */
	void updateCounts(bool aRemove);
	void updateActivity() { lastActivity = GET_TICK(); }

	void updated(OnlineUser& user);
	void updated(OnlineUserList& users);

	/** Reload details from favmanager or settings */
	void reloadSettings(bool updateNick);
	/// Get the external IP the user has defined for this hub, if any.
	const string& getUserIp4() const;
	const string& getUserIp6() const;

	virtual void checkNick(string& nick) = 0;

	// TimerManagerListener
	virtual void on(Second, uint64_t aTick) noexcept;

	// BufferedSocketListener
	virtual void on(Connecting) noexcept { fire(ClientListener::Connecting(), this); }
	virtual void on(Connected) noexcept;
	virtual void on(Line, const string& aLine) noexcept;
	virtual void on(Failed, const string&) noexcept;

	virtual bool v4only() const = 0;

private:
	virtual OnlineUserList getUsers() const = 0;
	virtual void infoImpl() = 0;

	string hubUrl;
	string address;
	string ip;
	string keyprint;
	string port;
	char separator;
	bool secure;
	CountType countType;
};

} // namespace dcpp

#endif // !defined(CLIENT_H)
