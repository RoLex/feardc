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

#include "stdafx.h"

#include "AboutDlg.h"

#include <dcpp/format.h>
#include <dcpp/HttpManager.h>
#include <dcpp/SettingsManager.h>
#include <dcpp/SimpleXML.h>
#include <dcpp/Streams.h>
#include <dcpp/version.h>
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

static const char thanks[] = "Big thanks to all donators and people who have contributed with ideas and code! "
"This product uses BZip2 <https://sourceware.org/bzip2/>, thanks to Julian Seward and team for providing it. "
"This product uses ZLib <https://www.zlib.net/>, thanks to Jean-loup Gailly and Mark Adler for providing it. "
"This product includes parts of the MaxMindDB library created by MaxMind Inc, available from <https://github.com/maxmind/libmaxminddb/>. "
"This product includes updated GeoLite2 databases provided by MaxMind Inc, available from <https://www.maxmind.com/en/geoip2-databases/>. "
"This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit <https://www.openssl.org/>. "
"This product uses the MiniUPnP client library <https://miniupnp.tuxfamily.org/> and NAT-PMP library by Thomas Bernard. "
"This product uses DWARF library <https://www.prevanders.net/dwarf.html> by SGI & David Anderson. "
"The following people have contributed code to DC++ and FearDC (I hope I haven't missed someone, they're "
"roughly in chronological order... =):\r\n"
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
"pavel pimenov, konstantin, night, klondike, rolex. Keep it coming!";

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
	create(dwt::Point(400, 600));
	return show();
}

bool AboutDlg::handleInitDialog() {
	grid = addChild(Grid::Seed(6, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(1).mode = GridInfo::FILL;
	grid->row(1).align = GridInfo::STRETCH;

	// horizontally centered seeds
	GroupBox::Seed gs;
	gs.style |= BS_CENTER;
	gs.padding.y = 2;
	Label::Seed ls;
	ls.style |= SS_CENTER;

	{
		auto cur = grid->addChild(gs)->addChild(Grid::Seed(6, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::CENTER;

		cur->addChild(Label::Seed(WinUtil::createIcon(IDI_DCPP, 48)));

		ls.caption = Text::toT(dcpp::fullVersionString) +
			_T("\nBased on: " DCAPPNAME " " DCVERSIONSTRING "\n") +
			_T("\n(c) Copyright 2001-2024 Jacek Sieka\n");

		ls.caption += T_("Ex-main project contributors: Todd Pederzani, poy\nEx-codeveloper: Per Lind\303\251n\nOriginal DC++ logo design: Martin Skogevall\nGraphics: Radox and various GPL and CC authors\n\nFearDC is licenced under GPL.");
		cur->addChild(ls);

		cur->addChild(Link::Seed(_T("https://client.feardc.net"), true));

		auto ts = WinUtil::Seeds::Dialog::textBox;
		ts.style |= ES_READONLY;
		ts.exStyle &= ~WS_EX_CLIENTEDGE;

		gs.caption = T_("TTH");
		ts.caption = WinUtil::tth;
		cur->addChild(gs)->addChild(ts);

		gs.caption = T_("Compiler");
		// see also CrashLogger.cpp for similar tests.
#ifdef __MINGW64_VERSION_MAJOR
		ts.caption = Text::toT("MinGW-w64's GCC " __VERSION__);
#elif defined(_MSC_VER)
		ts.caption = Text::toT("MS Visual Studio " + Util::toString(_MSC_VER));
#else
		ts.caption = _T("Unknown");
#endif
#ifdef _DEBUG
		ts.caption += _T(" (debug)");
#endif
#ifdef _WIN64
		ts.caption += _T(" (x64)");
#endif
		cur->addChild(gs)->addChild(ts);

		gs.caption = T_("OpenSSL version");
		ts.caption = Text::toT(OPENSSL_VERSION_TEXT);
		cur->addChild(gs)->addChild(ts);
	}

	{
		gs.caption = T_("Greetz and Contributors");
		auto seed = WinUtil::Seeds::Dialog::textBox;
		seed.style &= ~ES_AUTOHSCROLL;
		seed.style |= ES_MULTILINE | WS_VSCROLL | ES_READONLY;
		seed.caption = Text::toT(thanks);
		grid->addChild(gs)->addChild(seed);
	}

	{
		gs.caption = T_("Totals");
		auto cur = grid->addChild(gs)->addChild(Grid::Seed(2, 1));
		cur->column(0).mode = GridInfo::FILL;

		ls.caption = str(TF_("Upload: %1%, Download: %2%") % Text::toT(Util::formatBytes(SETTING(TOTAL_UPLOAD))) % Text::toT(Util::formatBytes(SETTING(TOTAL_DOWNLOAD))));
		cur->addChild(ls);

		ls.caption = (SETTING(TOTAL_DOWNLOAD) > 0)
			? str(TF_("Ratio (up/down): %1$0.2f") % (((double)SETTING(TOTAL_UPLOAD)) / ((double)SETTING(TOTAL_DOWNLOAD))))
			: T_("No transfers yet");
		cur->addChild(ls);
	}

	gs.caption = T_("Latest stable version");
	ls.caption = T_("Downloading...");
	version = grid->addChild(gs)->addChild(ls);

	auto buttons = WinUtil::addDlgButtons(grid,
		[this] { endDialog(IDOK); },
		[this] { endDialog(IDCANCEL); });
	buttons.first->setFocus();
	buttons.second->setVisible(false);

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
