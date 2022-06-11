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

#ifndef DCPLUSPLUS_WIN32_SEARCH_FRAME_H
#define DCPLUSPLUS_WIN32_SEARCH_FRAME_H

#include <list>
#include <set>

#include <dcpp/Client.h>
#include <dcpp/ClientManagerListener.h>
#include <dcpp/SearchManager.h>
#include <dcpp/SettingsManager.h>

#include "AspectUserCommand.h"
#include "ListFilter.h"
#include "MDIChildFrame.h"

using std::list;
using std::set;

class SearchFrame :
	public MDIChildFrame<SearchFrame>,
	private SearchManagerListener,
	private SettingsManagerListener,
	private ClientManagerListener,
	public AspectUserCommand<SearchFrame>
{
	typedef MDIChildFrame<SearchFrame> BaseType;
public:
	enum Status {
		STATUS_SHOW_UI,
		STATUS_STATUS,
		STATUS_COUNT,
		STATUS_DROPPED,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

	static void openWindow(TabViewPtr parent, const tstring& str = Util::emptyStringT,
		SearchManager::TypeModes type = SearchManager::TYPE_ANY, const tstring& url = Util::emptyStringT );
	static void closeAll();

private:
	friend class MDIChildFrame<SearchFrame>;
	friend class AspectUserCommand<SearchFrame>;

	enum {
		COLUMN_FIRST,
		COLUMN_FILENAME = COLUMN_FIRST,
		COLUMN_HITS,
		COLUMN_NICK,
		COLUMN_TYPE,
		COLUMN_SIZE,
		COLUMN_PATH,
		COLUMN_SLOTS,
		COLUMN_CONNECTION,
		COLUMN_HUB,
		COLUMN_EXACT_SIZE,
		COLUMN_IP,
		COLUMN_TTH,
		COLUMN_CID,
		COLUMN_LAST
	};

	enum {
		OPTION_ONLY_AS_OP = 0,
		OPTION_ONLY_SECURE = 1
	};

	struct SearchInfo {
		SearchInfo(const SearchResultPtr& aSR);
		~SearchInfo();

		void view();

		struct Download {
			Download(const tstring& aTarget) : total(0), ignored(0), tgt(aTarget) { }
			void operator()(SearchInfo* si);

			unsigned total;

			unsigned ignored;
			string error;

		protected:
			const tstring& tgt;

			void addFile(SearchInfo* si, const string& target);
			void addDir(SearchInfo* si, const string& target = Util::emptyString);
		};

		struct DownloadWhole : Download {
			DownloadWhole(const tstring& aTarget) : Download(aTarget) { }
			void operator()(SearchInfo* si);
		};

		struct DownloadTarget : Download {
			DownloadTarget(const tstring& aTarget) : Download(aTarget) { }
			void operator()(SearchInfo* si);
		};

		struct CheckTTH {
			CheckTTH() : firstHubs(true), op(true), hasTTH(false), firstTTH(true) { }
			void operator()(SearchInfo* si);
			bool firstHubs;
			StringList hubs;
			bool op;
			bool hasTTH;
			bool firstTTH;
			TTHValue tth;
			tstring name;
			int64_t size;
		};

		const tstring& getText(int col) const { return columns[col]; }
		int getImage(int col) const;
		int getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int col) const;

		static int compareItems(const SearchInfo* a, const SearchInfo* b, int col);

		void update();

		SearchResultList srs;
		SearchInfo* parent;

		tstring columns[COLUMN_LAST];
	};

	struct HubInfo : public FastAlloc<HubInfo> {
		HubInfo(const tstring& aUrl, const tstring& aName, bool aOp, bool aSecure) : url(aUrl),
			name(aName), op(aOp), secure(aSecure) { }
		HubInfo(Client* client) : url(Text::toT(client->getHubUrl())),
			name(Text::toT(client->getHubName())), op(client->getMyIdentity().isOp()), secure(client->isSecure()) { }

		const tstring& getText(int col) const {
			return (col == 0) ? name : Util::emptyStringT;
		}
		int getImage(int) const {
			return 0;
		}
		static int compareItems(const HubInfo* a, const HubInfo* b, int col) {
			return (col == 0) ? compare(a->name, b->name) : 0;
		}
		tstring url;
		tstring name;
		bool op;
		bool secure;
	};

	typedef set<SearchFrame*> FrameSet;
	static FrameSet frames;

	SplitterContainerPtr paned;

	GridPtr options;

	ComboBoxPtr searchBox;
	bool isHash;

	ComboBoxPtr mode;

	TextBoxPtr size;
	ComboBoxPtr sizeMode;

	ComboBoxPtr fileType;

	bool onlyFree;
	bool hideShared;

	typedef TypedTable<HubInfo> WidgetHubs;
	typedef WidgetHubs* WidgetHubsPtr;
	WidgetHubsPtr hubs;

	typedef TypedTable<SearchInfo, false, dwt::TableTree> WidgetResults;
	typedef WidgetResults* WidgetResultsPtr;
	WidgetResultsPtr results;

	list<SearchInfo> searchResults; /* the LPARAM data of table entries are direct pointers to
									objects stored by this container, hence the std::list. */

	ListFilter filter;

	SearchManager::TypeModes initialType;

	static TStringList lastSearches;

	size_t droppedResults;

	TStringList currentSearch;
	StringList targets;

	ParamMap ucLineParams;

	std::string token;

	SearchFrame(TabViewPtr parent, const tstring& initialString, SearchManager::TypeModes initialType_, const tstring& url = Util::emptyStringT);
	virtual ~SearchFrame();

	void handlePurgeClicked();
	void handleSlotsClicked();
	void handleHideSharedClicked();
	LRESULT handleHubItemChanged(WPARAM wParam, LPARAM lParam);
	bool handleKeyDown(int c);
	bool handleContextMenu(dwt::ScreenCoordinate pt);
	void handleDownload();
	void handleDownloadFavoriteDirs(unsigned index);
	void handleDownloadTo();
	void handleDownloadTarget(unsigned index);
	void handleDownloadDir();
	void handleDownloadWholeFavoriteDirs(unsigned index);
	void handleDownloadWholeTarget(unsigned index);
	void handleDownloadDirTo();
	void handleViewAsText();
	void handleRemove();
	bool handleSearchKeyDown(int c);
	bool handleSearchChar(int c);

	void layout();
	bool preClosing();
	void postClosing();
	void initSecond();
	bool eachSecond();

	void fillFileType(const tstring& toSelect);
	void searchTypesChanged();

	void treat(const SearchInfo::Download& dl);

	void runUserCommand(const UserCommand& uc);
	void runSearch();
	void updateStatusCount();
	void addDropped();
	void addResult(SearchResultPtr psr);
	void addToList(SearchInfo* si);
	void updateList();

	MenuPtr makeMenu();
	void addTargetMenu(Menu* menu, const StringPairList& favoriteDirs, const SearchInfo::CheckTTH& checkTTH);
	void addTargetDirMenu(Menu* menu, const StringPairList& favoriteDirs);

	// SearchManagerListener
	virtual void on(SearchManagerListener::SR, const SearchResultPtr& sr) noexcept;

	// SettingsManagerListener
	virtual void on(SettingsManagerListener::SearchTypesChanged) noexcept;

	// ClientManagerListener
	virtual void on(ClientConnected, Client* c) noexcept;
	virtual void on(ClientUpdated, Client* c) noexcept;
	virtual void on(ClientDisconnected, Client* c) noexcept;

	void onHubAdded(HubInfo* info, bool defaultHubState = true);
	void onHubChanged(HubInfo* info);
	void onHubRemoved(HubInfo* info);
};

#endif // !defined(DCPLUSPLUS_WIN32_SEARCH_FRAME_H)
