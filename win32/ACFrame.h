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

#ifndef DCPLUSPLUS_WIN32_AC_FRAME_H
#define DCPLUSPLUS_WIN32_AC_FRAME_H

#include <dcpp/forward.h>
#include <dcpp/SettingsManager.h>

#include "ListFilter.h"
#include "StaticFrame.h"

class ACFrame :
	public StaticFrame<ACFrame>
{
	typedef StaticFrame<ACFrame> BaseType;

public:
	enum Status {
		STATUS_STATUS,
		STATUS_SETTINGS,
		STATUS_LAST
	};

	static const string id;
	const string& getId() const;

protected:
	friend class StaticFrame<ACFrame>;
	friend class MDIChildFrame<ACFrame>;

	ACFrame(TabViewPtr parent);
	virtual ~ACFrame();

	void layout();

	bool preClosing();
	void postClosing();

private:
	enum {
		COLUMN_FIRST,
		COLUMN_NAME = COLUMN_FIRST,
		COLUMN_STATUS,
		COLUMN_TYPE,
		COLUMN_VALUE,
		COLUMN_LAST
	};

	class SettingInfo {
	public:
		SettingInfo(const int index_);

		const tstring& getText(int column) const { return columns[column]; }

		int getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const;

		const tstring& getName() const {
			return columns[COLUMN_NAME];
		}

		const tstring& getValue() const {
			return columns[COLUMN_VALUE];
		}

		void update(const tstring& value) {
			auto sm = SettingsManager::getInstance();
			switch (type) {
			case SettingsManager::TYPE_STRING:
				sm->set(static_cast<SettingsManager::StrSetting>(index), Text::fromT(value));
				break;

			case SettingsManager::TYPE_INT:
				sm->set(static_cast<SettingsManager::IntSetting>(index), Util::toInt(Text::fromT(value)));
				break;

			case SettingsManager::TYPE_BOOL:
				sm->set(static_cast<SettingsManager::BoolSetting>(index), Util::toInt(Text::fromT(value)));
				break;

			case SettingsManager::TYPE_INT64:
				sm->set(static_cast<SettingsManager::Int64Setting>(index), Util::toInt64(Text::fromT(value)));
				break;

			case SettingsManager::TYPE_FLOAT:
				sm->set(static_cast<SettingsManager::FloatSetting>(index), Util::toFloat(Text::fromT(value)));
				break;

			default:
				dcassert(0);
			}

			read();
		}

		void read() {
			auto sm = SettingsManager::getInstance();
			auto getData = [&]() -> string {
				switch (type) {
				case SettingsManager::TYPE_STRING:
					isDefault = sm->isDefault(static_cast<SettingsManager::StrSetting>(index));
					return sm->get(static_cast<SettingsManager::StrSetting>(index));

				case SettingsManager::TYPE_INT:
					isDefault = sm->isDefault(static_cast<SettingsManager::IntSetting>(index));
					return std::to_string(sm->get(static_cast<SettingsManager::IntSetting>(index)));

				case SettingsManager::TYPE_BOOL:
					isDefault = sm->isDefault(static_cast<SettingsManager::BoolSetting>(index));
					return std::to_string(sm->get(static_cast<SettingsManager::BoolSetting>(index)));

				case SettingsManager::TYPE_INT64:
					isDefault = sm->isDefault(static_cast<SettingsManager::Int64Setting>(index));
					return std::to_string(sm->get(static_cast<SettingsManager::Int64Setting>(index)));

				case SettingsManager::TYPE_FLOAT:
					isDefault = sm->isDefault(static_cast<SettingsManager::FloatSetting>(index));
					return Util::toString(sm->get(static_cast<SettingsManager::FloatSetting>(index)));

				default:
					return Util::emptyString;
				}
			};

			columns[COLUMN_VALUE] = Text::toT(getData());
			columns[COLUMN_STATUS] = isDefault ? T_("Default") : T_("User set");
		}

		static int compareItems(const SettingInfo* a, const SettingInfo* b, int col) {
			return compare(a->columns[col], b->columns[col]);
		}

		const int index;
		bool isDefault;
		SettingsManager::Types type;

	private:
		static const string typeStrings[];
		tstring columns[COLUMN_LAST];
	};

	GridPtr grid;

	GridPtr upper;
	LabelPtr disclaimer;

	typedef TypedTable<SettingInfo> WidgetSettings;
	typedef WidgetSettings* WidgetSettingsPtr;
	WidgetSettingsPtr settings;

	GroupBoxPtr filterGroup;
	ListFilter filter;
	GroupBoxPtr optionsGroup;
	GridPtr options;

	unsigned int orgSize;
	unsigned int visibleSettings;

	bool handleSettingsContextMenu(dwt::ScreenCoordinate pt);

	void updateStatus(tstring text = Util::emptyStringT);

	void modify();

	void fillList();

	void setDefaultValue();
	void exportSettings();

	void dismissDisclaim();
};

#endif // !defined(DCPLUSPLUS_WIN32_AC_FRAME_H)
