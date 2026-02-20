/*
 * Copyright (C) 2001-2025 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_TRANSFER_VIEW_H
#define DCPLUSPLUS_WIN32_TRANSFER_VIEW_H

#include <functional>
#include <list>

#include <dcpp/DownloadManagerListener.h>
#include <dcpp/UploadManagerListener.h>
#include <dcpp/ConnectionManagerListener.h>
#include <dcpp/QueueManagerListener.h>
#include <dcpp/HttpManagerListener.h>
#include <dcpp/forward.h>
#include <dcpp/MerkleTree.h>
#include <dcpp/Util.h>
#include <dcpp/QueueItem.h>

#include <dwt/widgets/Container.h>

#include "AspectUserCommand.h"

#include "UserInfoBase.h"

using std::function;
using std::list;

class TransferView :
	public dwt::Container,
	private DownloadManagerListener,
	private UploadManagerListener,
	private ConnectionManagerListener,
	private QueueManagerListener,
	private HttpManagerListener,
	public AspectUserInfo<TransferView>,
	public AspectUserCommand<TransferView>
{
	friend class AspectUserInfo<TransferView>;
	friend class AspectUserCommand<TransferView>;

public:
	TransferView(dwt::Widget* parent, TabViewPtr mdi_);
	virtual ~TransferView();

	void prepareClose();

private:
	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_PATH,
		COLUMN_STATUS,
		COLUMN_USER,
		COLUMN_HUB,
		COLUMN_TIMELEFT,
		COLUMN_SPEED,
		COLUMN_TRANSFERRED,
		COLUMN_SIZE,
		COLUMN_CIPHER,
		COLUMN_IP,
		COLUMN_COUNTRY,
		COLUMN_LAST
	};

	enum Status {
		STATUS_RUNNING,		///< Transferring
		STATUS_WAITING		///< Idle
	};

	struct TransferInfo;
	struct UpdateInfo;

	struct ItemInfo {
		ItemInfo();
		virtual ~ItemInfo() { }

		const tstring& getText(int col) const;
		virtual int getImage(int col) const;
		static int compareItems(const ItemInfo* a, const ItemInfo* b, int col);

		virtual TransferInfo& transfer() = 0;

		virtual double barPos() const = 0;

		virtual void force() = 0;
		virtual void disconnect() = 0;
		virtual void removeFileFromQueue() = 0;
		virtual void setPriority(QueueItem::Priority p) = 0;

		int64_t timeleft;
		int64_t speed;
		int64_t actual;
		int64_t transferred;
		int64_t size;

		tstring columns[COLUMN_LAST];
	};

	struct ConnectionInfo : public ItemInfo, public UserInfoBase {
		ConnectionInfo(const HintedUser& u, TransferInfo& parent);

		bool operator==(const ConnectionInfo& other) const;

		int getImage(int col) const;

		void update(const UpdateInfo& ui);

		TransferInfo& transfer();

		double barPos() const;

		virtual void force();
		virtual void disconnect();
		virtual void removeFileFromQueue();
		virtual void setPriority(QueueItem::Priority p);

		bool transferFailed;
		Status status;
		TransferInfo& parent;
	};

	struct TransferInfo : public ItemInfo {
		TransferInfo(const TTHValue& tth, ConnectionType type, const string& path, const string& tempPath);

		bool operator==(const TransferInfo& other) const;

		int getImage(int col) const;

		void update();
		void updatePath();

		TransferInfo& transfer();

		double barPos() const;

		virtual void force();
		virtual void disconnect();
		virtual void removeFileFromQueue();
		virtual void setPriority(QueueItem::Priority p);

		TTHValue tth;
		ConnectionType type;
		string path;
		string tempPath;

		list<ConnectionInfo> conns;
	};

	struct HttpInfo : public TransferInfo {
		HttpInfo(const string& url);

		void update(const UpdateInfo& ui);

		virtual void disconnect();

		Status status;
	};

	struct UpdateInfo {
		enum {
			MASK_STATUS = 1 << 0,
			MASK_STATUS_STRING = 1 << 1,
			MASK_SPEED = 1 << 2,
			MASK_TRANSFERRED = 1 << 3,
			MASK_CIPHER = 1 << 4,
			MASK_IP = 1 << 5,
			MASK_COUNTRY = 1 << 6,
			MASK_PATH = 1 << 7,
			MASK_HTTP = 1 << 8
		};

		UpdateInfo(const HintedUser& user, ConnectionType type, bool transferFailed = false) :
			updateMask(0), user(user), type(type), transferFailed(transferFailed) { }
		UpdateInfo(ConnectionType type) : updateMask(0), type(type), transferFailed(false) { }

		uint32_t updateMask;

		HintedUser user;
		ConnectionType type;
		bool transferFailed;

		TTHValue tth;
		void setTTH(const TTHValue& tth) { this->tth = tth; }
		string path;
		void setFile(const string& path) { this->path = path; updateMask |= MASK_PATH; }

		string tempPath;
		void setTempPath(const string& path) { tempPath = path; }

		void setStatus(Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		Status status;
		void setTransferred(int64_t aTransferred, int64_t aActual, int64_t aSize) {
			transferred = aTransferred; actual = aActual; size = aSize; updateMask |= MASK_TRANSFERRED;
		}
		int64_t actual;
		int64_t transferred;
		int64_t size;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed;
		void setStatusString(tstring&& aStatusString) { statusString = move(aStatusString); updateMask |= MASK_STATUS_STRING; }
		tstring statusString;

		void setCipher(const tstring& aCipher) { cipher = aCipher; updateMask |= MASK_CIPHER; }
		tstring cipher;
		void setIP(const tstring& aIp) { ip = aIp; updateMask |= MASK_IP; }
		tstring ip;
		void setCountry(const tstring& aCountry) { country = aCountry; updateMask |= MASK_COUNTRY; }
		tstring country;

		void setHttp() { updateMask |= MASK_HTTP; }
		bool isHttp() const { return updateMask & MASK_HTTP; }
	};

	typedef TypedTable<ItemInfo, false, dwt::TableTree> WidgetTransfers;
	typedef WidgetTransfers* WidgetTransfersPtr;
	WidgetTransfersPtr transfers;

	list<TransferInfo> transferItems; /* the LPARAM data of table entries are direct pointers to
									  objects stored by this container, hence the std::list. */
	list<HttpInfo> httpItems;

	TabViewPtr mdi;

	const dwt::IconPtr downloadIcon;
	const dwt::IconPtr uploadIcon;

	vector<pair<function<void (const UpdateInfo&)>, unique_ptr<UpdateInfo>>> tasks;
	bool updateList;

	ParamMap ucLineParams;

	void handleDestroy();
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	void handleForce();
	void handleDisconnect();
	void handleRemoveFileFromQueue();
	void handlePriority(QueueItem::Priority p);
	bool handleKeyDown(int c);
	void handleDblClicked();
	LRESULT handleCustomDraw(NMLVCUSTOMDRAW& data);
	bool handleTimer();

	void runUserCommand(const UserCommand& uc);

	void layout();

	void addConn(const UpdateInfo& ui);
	void updateConn(const UpdateInfo& ui);
	void removeConn(const UpdateInfo& ui);

	ConnectionInfo* findConn(const HintedUser& user, ConnectionType type);
	TransferInfo* findTransfer(const string& path, ConnectionType type);
	void removeConn(ConnectionInfo& conn);
	void removeTransfer(TransferInfo& transfer);

	void addHttpConn(const UpdateInfo& ui);
	void updateHttpConn(const UpdateInfo& ui);
	void removeHttpConn(const UpdateInfo& ui);

	HttpInfo* findHttpItem(const string& url);
	void removeHttpItem(HttpInfo& item);

	// AspectUserInfo
	UserInfoList selectedUsersImpl() const;

	void execTasks();

	virtual void on(ConnectionManagerListener::Added, ConnectionQueueItem* aCqi) noexcept;
	virtual void on(ConnectionManagerListener::Removed, ConnectionQueueItem* aCqi) noexcept;
	virtual void on(ConnectionManagerListener::Failed, ConnectionQueueItem* aCqi, const string& aReason) noexcept;
	virtual void on(ConnectionManagerListener::StatusChanged, ConnectionQueueItem* aCqi) noexcept;

	virtual void on(DownloadManagerListener::Complete, Download* d) noexcept;
	virtual void on(DownloadManagerListener::Failed, Download* d, const string& aReason) noexcept;
	virtual void on(DownloadManagerListener::Starting, Download* d) noexcept;
	virtual void on(DownloadManagerListener::Tick, const DownloadList& dl) noexcept;
	virtual void on(DownloadManagerListener::Requesting, Download* d) noexcept;

	virtual void on(UploadManagerListener::Complete, Upload* u) noexcept;
	virtual void on(UploadManagerListener::Starting, Upload* u) noexcept;
	virtual void on(UploadManagerListener::Tick, const UploadList& ul) noexcept;

	virtual void on(QueueManagerListener::CRCFailed, Download* d, const string& aReason) noexcept;

	virtual void on(HttpManagerListener::Added, HttpConnection*) noexcept;
	virtual void on(HttpManagerListener::Updated, HttpConnection*, const uint8_t* data, size_t len) noexcept;
	virtual void on(HttpManagerListener::Failed, HttpConnection*, const string&) noexcept;
	virtual void on(HttpManagerListener::Complete, HttpConnection*, OutputStream*) noexcept;
	virtual void on(HttpManagerListener::Removed, HttpConnection*) noexcept;

	void addedConn(UpdateInfo* ui);
	void updatedConn(UpdateInfo* ui);
	void removedConn(UpdateInfo* ui);

	void starting(UpdateInfo* ui, Transfer* t);
	void onTransferTick(Transfer* t, bool download);
	void onTransferComplete(Transfer* t, bool download);
	void onFailed(Download* aDownload, const string& aReason);
	UpdateInfo* makeHttpUI(HttpConnection* c);
};

#endif // !defined(TRANSFER_VIEW_H)
