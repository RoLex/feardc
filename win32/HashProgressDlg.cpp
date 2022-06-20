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

#include "stdafx.h"

#include "resource.h"

#include "HashProgressDlg.h"

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/ProgressBar.h>

#include <dcpp/format.h>
#include <dcpp/HashManager.h>
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::ProgressBar;

namespace { const int progressBarMax = 10000; }

HashProgressDlg::HashProgressDlg(dwt::Widget* parent, bool aAutoClose) :
GridDialog(parent, 657, DS_CONTEXTHELP),
file(0),
stat(0),
speed(0),
left(0),
progress(0),
pauseResume(0),
autoClose(aAutoClose)
{
	onInitDialog([this] { return handleInitDialog(); });
	onHelp(&WinUtil::help);
}

HashProgressDlg::~HashProgressDlg() {
	HashManager::getInstance()->setPriority(Thread::IDLE);
}

bool HashProgressDlg::handleInitDialog() {
	setHelpId(IDH_HASH_PROGRESS);

	grid = addChild(Grid::Seed(5, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(3).size = 20;
	grid->row(3).mode = GridInfo::STATIC;
	grid->row(3).align = GridInfo::STRETCH;

	grid->addChild(Label::Seed(T_("Please wait while FearDC indexes your files (they won't be shared until they've been indexed)...")));

	file = grid->addChild(Label::Seed());

	{
		GridPtr cur = grid->addChild(GroupBox::Seed(T_("Statistics")))->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;

		Label::Seed seed;
		stat = cur->addChild(seed);
		speed = cur->addChild(seed);
		left = cur->addChild(seed);
	}

	progress = grid->addChild(ProgressBar::Seed());
	progress->setRange(0, progressBarMax);

	{
		GridPtr cur = grid->addChild(Grid::Seed(1, 2));
		cur->column(1).mode = GridInfo::FILL;
		cur->column(1).align = GridInfo::BOTTOM_RIGHT;

		pair<ButtonPtr, ButtonPtr> buttons = WinUtil::addDlgButtons(cur,
			[this] { endDialog(IDOK); },
			[this] { endDialog(IDCANCEL); });
		buttons.first->setText(T_("Run in background"));
		buttons.second->setVisible(false);

		pauseResume = cur->addChild(Button::Seed());
		cur->setWidget(pauseResume, 0, 1);
		pauseResume->onClicked([this, cur] { handlePauseResume(); cur->layout(); pauseResume->redraw(); });
		setButtonState();
	}

	string tmp;
	startTime = GET_TICK();
	HashManager::getInstance()->getStats(tmp, startBytes, startFiles);
	updateStats();

	HashManager::getInstance()->setPriority(Thread::NORMAL);
	setTimer([this] { return updateStats(); }, 1000);

	setText(T_("Creating file index..."));
	setSmallIcon(WinUtil::createIcon(IDI_INDEXING, 16));
	setLargeIcon(WinUtil::createIcon(IDI_INDEXING, 32));

	layout();
	centerWindow();

	return false;
}

bool HashProgressDlg::updateStats() {
	string path;
	uint64_t bytes = 0;
	size_t files = 0;
	uint64_t tick = GET_TICK();

	HashManager::getInstance()->getStats(path, bytes, files);

	if(bytes > startBytes)
		startBytes = bytes;

	if(files > startFiles)
		startFiles = files;

	if(autoClose && files == 0) {
		close(true);
		return false;
	}

	file->setText(files ? Text::toT(path) : T_("Done"));

	double timeDiff = tick - startTime;

	bool paused = HashManager::getInstance()->isHashingPaused();
	if(timeDiff < 1000 || files == 0 || bytes == 0 || paused) {
		stat->setText(str(TF_("-.-- files/h, %1% files left") % (uint32_t)files));
		speed->setText(str(TF_("-.-- B/s, %1% left") % Text::toT(Util::formatBytes(bytes))));
		if(paused) {
			left->setText(T_("Paused"));
			progress->sendMessage(PBM_SETSTATE, PBST_PAUSED, 0);
		} else {
			left->setText(str(TF_("%1% left") % T_("-:--:--")));
			progress->setPosition(0);
		}

	} else {
		double fileDiff = startFiles - files;
		double byteDiff = startBytes - bytes;

		double fileStat = fileDiff * 60. * 60. * 1000. / timeDiff;
		double speedStat = byteDiff * 1000. / timeDiff;

		stat->setText(str(TF_("%1% files/h, %2% files left") % fileStat % static_cast<uint32_t>(files)));
		speed->setText(str(TF_("%1%/s, %2% left") % Text::toT(Util::formatBytes(speedStat)) % Text::toT(Util::formatBytes(bytes))));

		double timeLeft = speedStat ? static_cast<double>(bytes) / speedStat : fileStat ? static_cast<double>(files) * 60. * 60. / fileStat : 0;
		left->setText(str(TF_("%1% left") % (timeLeft ? Text::toT(Util::formatSeconds(timeLeft)) : T_("-:--:--"))));

		progress->setPosition(progressBarMax * (startBytes ? byteDiff / static_cast<double>(startBytes) :
			startFiles ? fileDiff / static_cast<double>(startFiles) : 0));
	}

	return true;
}

void HashProgressDlg::handlePauseResume() {
	if(HashManager::getInstance()->isHashingPaused()) {
		HashManager::getInstance()->resumeHashing();
		progress->sendMessage(PBM_SETSTATE, PBST_NORMAL, 0);
	} else {
		HashManager::getInstance()->pauseHashing();
	}

	setButtonState();
}

void HashProgressDlg::setButtonState() {
	pauseResume->setText(HashManager::getInstance()->isHashingPaused() ? T_("Resume hashing") : T_("Pause hashing"));
}
