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

#include "stdafx.h"

#include "AboutDlg.h"

#include <dcpp/format.h>
#include <dcpp/HttpManager.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/Streams.h>
#include <dcpp/version.h>
#include <dcpp/GeoManager.h>
#include <openssl/opensslv.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/Link.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;
using dwt::Link;

static const char thanks[] =
"Big thanks to all donators and people who have contributed with ideas and code!\r\n\r\n"

"This product uses following components:\r\n\r\n"

"* Boost <https://www.boost.org/>\r\n"
"* BZip2 <https://sourceware.org/bzip2/>\r\n"
"* DWARF <https://www.prevanders.net/dwarf.html>\r\n"
"* LibIntl <https://gnuwin32.sourceforge.net/packages/libintl.htm>\r\n"
"* MaxMindDB <https://github.com/maxmind/libmaxminddb/>\r\n"
"* GeoLite2 <https://www.maxmind.com/en/geoip2-databases/>\r\n"
"* MiniUPnPc <https://miniupnp.tuxfamily.org/>\r\n"
"* NAT-PMP <https://miniupnp.tuxfamily.org/libnatpmp.html>\r\n"
"* OpenSSL <https://openssl-library.org/>\r\n"
"* PO4A <https://www.po4a.org/>\r\n"
"* ZLib <https://www.zlib.net/>\r\n\r\n"

"The following people have contributed code to DC++:\r\n\r\n"

"geoff, carxor, luca rota, dan kline, mike, anton, zc, sarf, farcry, kyrre aalerud, opera, "
"patbateman, xeroc, fusbar, vladimir marko, kenneth skovhede, ondrea, who, "
"sedulus, sandos, henrik engstr\303\266m, dwomac, robert777, saurod, atomicjo, bzbetty, orkblutt, "
"distiller, citruz, dan fulger, cologic, christer palm, twink, ilkka sepp\303\244l\303\244, johnny, ciber, "
"theparanoidone, gadget, naga, tremor, joakim tosteberg, pofis, psf8500, lauris ievins, "
"defr, ullner, fleetcommand, liny, xan, olle svensson, mark gillespie, jeremy huddleston, "
"bsod, sulan, jonathan stone, tim burton, izzzo, guitarm, paka, nils maier, jens oknelid, yoji, "
"krzysztof tyszecki, poison, mikejj, pur, bigmuscle, martin, jove, bart vullings, "
"steven sheehy, tobias nygren, dorian, stephan hohe, mafa_45, mikael eman, james ross, "
"stanislav maslovski, david grundberg, pavel andreev, yakov suraev, kulmegil, smir, emtee, individ, "
"pseudonym, crise, ben, ximin luo, razzloss, andrew browne, darkklor, vasily.n, netcelli, "
"gennady proskurin, iceman50, flow84, alexander sashnov, yorhel, irainman, maksis, "
"pavel pimenov, konstantin, night, klondike, rolex\r\n\r\n"

"Keep it coming! I hope I haven't missed someone, they're roughly in chronological order... =)";

AboutDlg::AboutDlg(dwt::Widget* parent) :
dwt::ModalDialog(parent),
grid(0),
version(0),
c(nullptr)
{
	onInitDialog([this] { return handleInitDialog(); });
}

AboutDlg::~AboutDlg() {
}

int AboutDlg::run() {
	create(dwt::Point(500, 750));
	return show();
}

bool AboutDlg::handleInitDialog() {
	grid = addChild(Grid::Seed(5, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	// horizontally centered seeds
	GroupBox::Seed gs;
	gs.style |= BS_CENTER;
	gs.padding.y = 10;
	gs.caption = Text::toT(dcpp::fullVersionString);

	Label::Seed ls;
	ls.style |= SS_CENTER;

	{ // 1 - texts
		auto cur = grid->addChild(gs)->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::CENTER;

		cur->addChild(Label::Seed(WinUtil::createIcon(IDI_DCPP, 128))); // 1 - icon

		ls.caption = _T("\nBased on " DCAPPNAME " " DCVERSIONSTRING);
		ls.caption += T_("\nCopyright © 2001-2025 Jacek Sieka");
		ls.caption += T_("\nEx-main project contributors: Todd Pederzani, poy");
		ls.caption += T_("\nEx-codeveloper: Per Lind\303\251n");
		ls.caption += T_("\nOriginal DC++ logo design: Martin Skogevall");
		ls.caption += T_("\nGraphics: Radox and various GPL and CC authors");
		ls.caption += _T("\n\nTTH: " + WinUtil::tth);
		ls.caption += T_("\nCompiler version: ");

// see also CrashLogger.cpp for similar tests
#ifdef __MINGW64_VERSION_MAJOR
		ls.caption += Text::toT("MinGW GCC " __VERSION__);
#elif defined(_MSC_VER)
		ls.caption += Text::toT("MSVS " + Util::toString(_MSC_VER));
#else
		ls.caption += T_("Unknown");
#endif

#ifdef _DEBUG
		ls.caption += T_(" Debug");
#endif

#ifdef _WIN64
		ls.caption += T_(" x64");
#else
		ls.caption += T_(" x86");
#endif

		ls.caption += _T("\nOpenSSL version: " OPENSSL_VERSION_TEXT);
		ls.caption += Text::toT("\nMMDB version: " + GeoManager::getInstance()->getVersion() + ", " + GeoManager::getInstance()->getEpoch() + "\n");

		cur->addChild(ls); // 2 - info

		cur->addChild(Link::Seed(_T("https://client.feardc.net/"), true)); // 3 - website

		ls.caption = T_("DC++ and FearDC are licenced under GNU General Public License");
		cur->addChild(ls); // 4 - license
	}

	{ // 2 - greets
		gs.caption = T_("Greetz and Contributors");

		auto seed = WinUtil::Seeds::Dialog::textBox;
		seed.style &= ~ES_AUTOHSCROLL;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_READONLY;
		seed.caption = Text::toT(thanks);

		grid->addChild(gs)->addChild(seed);
	}

	{ // 3 - totals
		gs.caption = T_("Totals");

		auto cur = grid->addChild(gs)->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		ls.caption = str(TF_("Upload: %1%, Download: %2%") % Text::toT(Util::formatBytes(SETTING(TOTAL_UPLOAD))) % Text::toT(Util::formatBytes(SETTING(TOTAL_DOWNLOAD))));
		cur->addChild(ls); // 1 - bytes

		ls.caption = (SETTING(TOTAL_DOWNLOAD) > 0)
			? str(TF_("Ratio (up/down): %1$0.2f") % (((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD))))
			: T_("No transfers yet");

		cur->addChild(ls); // 2 - ratio
	}

	{ // 4 - version
		gs.caption = T_("Latest stable version");
		ls.caption = T_("Downloading...");
		version = grid->addChild(gs)->addChild(ls);
	}

	{ // 5 - buttons
		auto buttons = WinUtil::addDlgButtons(grid,
			[this] { endDialog(IDOK); },
			[this] { endDialog(IDCANCEL); });

		buttons.first->setFocus();
		buttons.second->setVisible(false);
	}

	setText(T_("About FearDC"));
	setSmallIcon(WinUtil::createIcon(IDI_DCPP, 16));
	setLargeIcon(WinUtil::createIcon(IDI_DCPP, 32));

	layout();
	centerWindow();

	HttpManager::getInstance()->addListener(this);
	onDestroy([this] { HttpManager::getInstance()->removeListener(this); });
	c = HttpManager::getInstance()->download("https://client.feardc.net/version.xml");

	return false;
}

void AboutDlg::layout() {
	dwt::Point sz = getClientSize();
	grid->resize(dwt::Rectangle(3, 3, sz.x - 6, sz.y - 6));
}

void AboutDlg::completeDownload(bool success, const string& result) {
	tstring str;

	if (success && result.size()) {
		try {
			SimpleXML xml;
			xml.fromXML(result);

			if (xml.findChild("DCUpdate")) {
				xml.stepIn();

				if (xml.findChild("VersionString")) {
					const auto& ver = xml.getChildData();

					if (ver.size())
						str = Text::toT(ver);
				}
			}

		} catch (const SimpleXMLException&) {
			str = T_("Error processing version information");
		}
	}

	version->setText(str.empty() ? Text::toT(result) : str);
}

void AboutDlg::on(HttpManagerListener::Failed, HttpConnection* c, const string& str) noexcept {
	if(c != this->c) { return; }
	c = nullptr;

	callAsync([str, this] { completeDownload(false, str); });
}

void AboutDlg::on(HttpManagerListener::Complete, HttpConnection* c, OutputStream* stream) noexcept {
	if(c != this->c) { return; }
	c = nullptr;

	auto str = static_cast<StringOutputStream*>(stream)->getString();
	callAsync([str, this] { completeDownload(true, str); });
}
