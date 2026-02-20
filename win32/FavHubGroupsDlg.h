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

#ifndef DCPLUSPLUS_WIN32_FAVHUBGROUPSDLG_H
#define DCPLUSPLUS_WIN32_FAVHUBGROUPSDLG_H

#include <dcpp/typedefs.h>
#include <dcpp/FavHubGroup.h>
#include <dcpp/FastAlloc.h>
#include <dcpp/HubEntry.h>
#include <dcpp/HubSettings.h>

#include <dwt/widgets/ModalDialog.h>

#include "forward.h"

class FavHubGroupsDlg : public dwt::ModalDialog
{
public:
	/// @param parentEntry_ entry currently being edited, to make sure we don't delete it
	FavHubGroupsDlg(dwt::Widget* parent, FavoriteHubEntry* parentEntry_ = 0);
	virtual ~FavHubGroupsDlg();

	int run();

private:
	enum {
		COLUMN_NAME,
		COLUMN_LAST
	};

	class GroupInfo {
	public:
		GroupInfo(const FavHubGroup& group_);

		const tstring& getText(int col) const;

		static int compareItems(const GroupInfo* a, const GroupInfo* b, int col);

		const FavHubGroup group;

	private:
		tstring columns[COLUMN_LAST];
	};

	struct GroupCollector {
		void operator()(const GroupInfo* group) {
			groups.insert(group->group);
		}
		FavHubGroups groups;
	};

	GridPtr grid;

	typedef TypedTable<const GroupInfo> Groups;
	Groups* groups;

	ButtonPtr update;
	ButtonPtr remove;
	GroupBoxPtr properties;
	TextBoxPtr edit;
	TextBoxPtr nick;
	TextBoxPtr description;
	TextBoxPtr email;
	TextBoxPtr userIp;
	TextBoxPtr userIp6;
	ComboBoxPtr showJoins;
	ComboBoxPtr favShowJoins;
	ComboBoxPtr logMainChat;

	FavoriteHubEntry* parentEntry;

	bool handleInitDialog();
	bool handleKeyDown(int c);
	void handleSelectionChanged();
	void handleAdd();
	void handleUpdate();
	void handleRemove();
	void handleClose();

	void add(const FavHubGroup& group, bool ensureVisible = true);
	void add(const tstring& name, HubSettings&& settings);
	HubSettings getSettings() const;
	bool addable(const tstring& name, int ignore = -1);
	void removeGroup(const GroupInfo* group);

	void layout();
};

#endif
