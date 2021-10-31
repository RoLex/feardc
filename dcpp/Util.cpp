/*
 * Copyright (C) 2001-2021 Jacek Sieka, arnetheduck on gmail point com
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

#include "stdinc.h"
#include "Util.h"

#ifdef _WIN32

#include "w.h"
#include <iphlpapi.h>
#include <shlobj.h>

#endif

#include <cmath>

#include <boost/algorithm/string/trim.hpp>

#include "CID.h"
#include "ClientManager.h"
#include "ConnectivityManager.h"
#include "FastAlloc.h"
#include "File.h"
#include "LogManager.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "version.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <cctype>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#endif
#include <clocale>

namespace dcpp {

using std::abs;
using std::make_pair;

#ifndef _DEBUG
FastCriticalSection FastAllocBase::cs;
#endif

string Util::emptyString;
wstring Util::emptyStringW;
tstring Util::emptyStringT;

uint8_t Util::away = 0;
string Util::awayMsg;
time_t Util::awayTime;

string Util::paths[Util::PATH_LAST];

bool Util::localMode = true;

static void sgenrand(unsigned long seed);

extern "C" void bz_internal_error(int errcode) {
	dcdebug("bzip2 internal error: %d\n", errcode);
}

// def taken from <gettextP.h>
extern "C" const char *_nl_locale_name_default(void);

#ifdef _WIN32

static string getDownloadsPath(const string& def) {
	// Try Vista downloads path
	PWSTR path = NULL;

	// Defined in KnownFolders.h.
	static GUID downloads = {0x374de290, 0x123f, 0x4565, {0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b}};

	if(::SHGetKnownFolderPath(downloads, 0, NULL, &path) == S_OK) {
		string ret = Text::fromT(path) + "\\";
		::CoTaskMemFree(path);
		return ret;
	}

	return def + "Downloads\\";
}

#endif

void Util::initialize(PathsMap pathOverrides) {
	Text::initialize();

	sgenrand((unsigned long)time(NULL));

#ifdef _WIN32
	TCHAR buf[MAX_PATH+1] = { 0 };
	::GetModuleFileName(NULL, buf, MAX_PATH);

	string exePath = Util::getFilePath(Text::fromT(buf));

	// Global config path is DC++ executable path...
	paths[PATH_GLOBAL_CONFIG] = exePath;

	paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG];

	loadBootConfig();

	if(!File::isAbsolute(paths[PATH_USER_CONFIG])) {
		paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + paths[PATH_USER_CONFIG];
	}

	paths[PATH_USER_CONFIG] = validateFileName(paths[PATH_USER_CONFIG]);

	if(localMode) {
		paths[PATH_USER_LOCAL] = paths[PATH_USER_CONFIG];

		paths[PATH_DOWNLOADS] = paths[PATH_USER_CONFIG] + "Downloads\\";

	} else {
		if(::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK) {
			paths[PATH_USER_CONFIG] = Text::fromT(buf) + "\\DC++\\";
		}

		paths[PATH_DOWNLOADS] = getDownloadsPath(paths[PATH_USER_CONFIG]);

		paths[PATH_USER_LOCAL] = ::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf) == S_OK ? Text::fromT(buf) + "\\DC++\\" : paths[PATH_USER_CONFIG];
	}

	paths[PATH_RESOURCES] = exePath;

	// libintl doesn't support wide path names so we use the short (8.3) format.
	// https://sourceforge.net/p/gnuwin32/discussion/74807/thread/724990a4/
	tstring localePath_ = Text::toT(exePath) + _T("locale\\");
	memset(buf, 0, sizeof(buf));
	::GetShortPathName(localePath_.c_str(), buf, sizeof(buf)/sizeof(TCHAR));

	paths[PATH_LOCALE] = Text::fromT(buf);

#else
	paths[PATH_GLOBAL_CONFIG] = "/etc/";
	const char* home_ = getenv("HOME");
	string home = home_ ? Text::toUtf8(home_) : "/tmp/";

	paths[PATH_USER_CONFIG] = home + "/.dc++/";

	loadBootConfig();

	if(!File::isAbsolute(paths[PATH_USER_CONFIG])) {
		paths[PATH_USER_CONFIG] = paths[PATH_GLOBAL_CONFIG] + paths[PATH_USER_CONFIG];
	}

	paths[PATH_USER_CONFIG] = validateFileName(paths[PATH_USER_CONFIG]);

	if(localMode) {
		// @todo implement...
	}

	paths[PATH_USER_LOCAL] = paths[PATH_USER_CONFIG];
	paths[PATH_RESOURCES] = "/usr/share/";
	paths[PATH_LOCALE] = paths[PATH_RESOURCES] + "locale/";
	paths[PATH_DOWNLOADS] = home + "/Downloads/";
#endif

	paths[PATH_FILE_LISTS] = paths[PATH_USER_LOCAL] + "FileLists" PATH_SEPARATOR_STR;
	paths[PATH_HUB_LISTS] = paths[PATH_USER_LOCAL] + "HubLists" PATH_SEPARATOR_STR;
	paths[PATH_NOTEPAD] = paths[PATH_USER_CONFIG] + "Notepad.txt";

	// Override core generated paths
	for(auto& it: pathOverrides) {
		if(!it.second.empty())
			paths[it.first] = it.second;
	}

	File::ensureDirectory(paths[PATH_USER_CONFIG]);
	File::ensureDirectory(paths[PATH_USER_LOCAL]);

}

void Util::migrate(const string& file) {
	if(localMode) {
		return;
	}

	if(File::getSize(file) != -1) {
		return;
	}

	string fname = getFileName(file);
	string old = paths[PATH_GLOBAL_CONFIG] + fname;
	if(File::getSize(old) == -1) {
		return;
	}

	File::renameFile(old, file);
}

void Util::loadBootConfig() {
	// Load boot settings
	try {
		SimpleXML boot;
		boot.fromXML(File(getPath(PATH_GLOBAL_CONFIG) + "dcppboot.xml", File::READ, File::OPEN).read());
		boot.stepIn();

		if(boot.findChild("LocalMode")) {
			localMode = boot.getChildData() != "0";
		}

		if(boot.findChild("ConfigPath")) {
			ParamMap params;
#ifdef _WIN32
			/// @todo load environment variables instead? would make it more useful on *nix
			params["APPDATA"] = []() -> string {
				TCHAR path[MAX_PATH];
				return Text::fromT((::SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path), path));
			};
			params["PERSONAL"] = []() -> string {
				TCHAR path[MAX_PATH];
				return Text::fromT((::SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path), path));
			};
#endif
			paths[PATH_USER_CONFIG] = Util::formatParams(boot.getChildData(), params);
		}
	} catch(const Exception& ) {
		// Unable to load boot settings...
	}
}

string Util::getIETFLang() {
	auto lang = SETTING(LANGUAGE);
	if(lang.empty()) {
		lang = _nl_locale_name_default();
	}
	if(lang.empty() || lang == "C") {
		lang = "en-US";
	}

	// replace separation signs by hyphens.
	size_t i = 0;
	while((i = lang.find_first_of("_@.", i)) != string::npos) {
		lang[i] = '-';
		++i;
	}

	return lang;
}

#ifdef _WIN32
static const char badChars[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '/', '"', '|', '?', '*', 0
};
#else

static const char badChars[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
	17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, '<', '>', '\\', '"', '|', '?', '*', 0
};
#endif

/**
 * Replaces all strange characters in a file with '_'
 * @todo Check for invalid names such as nul and aux...
 */
string Util::validateFileName(string tmp) {
	string::size_type i = 0;

	// First, eliminate forbidden chars
	while( (i = tmp.find_first_of(badChars, i)) != string::npos) {
		tmp[i] = '_';
		i++;
	}

	// Then, eliminate all ':' that are not the second letter ("c:\...")
	i = 0;
	while( (i = tmp.find(':', i)) != string::npos) {
		if(i == 1) {
			i++;
			continue;
		}
		tmp[i] = '_';
		i++;
	}

	// Remove the .\ that doesn't serve any purpose
	i = 0;
	while( (i = tmp.find("\\.\\", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}
	i = 0;
	while( (i = tmp.find("/./", i)) != string::npos) {
		tmp.erase(i+1, 2);
	}

	// Remove any double \\ that are not at the beginning of the path...
	i = 1;
	while( (i = tmp.find("\\\\", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}
	i = 1;
	while( (i = tmp.find("//", i)) != string::npos) {
		tmp.erase(i+1, 1);
	}

	// And last, but not least, the infamous ..\! ...
	i = 0;
	while( ((i = tmp.find("\\..\\", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}
	i = 0;
	while( ((i = tmp.find("/../", i)) != string::npos) ) {
		tmp[i + 1] = '_';
		tmp[i + 2] = '_';
		tmp[i + 3] = '_';
		i += 2;
	}

	// Dots at the end of path names aren't popular
	i = 0;
	while( ((i = tmp.find(".\\", i)) != string::npos) ) {
		tmp[i] = '_';
		i += 1;
	}
	i = 0;
	while( ((i = tmp.find("./", i)) != string::npos) ) {
		tmp[i] = '_';
		i += 1;
	}


	return tmp;
}

bool Util::checkExtension(const string& tmp) {
	for(size_t i = 0, n = tmp.size(); i < n; ++i) {
		if (tmp[i] < 0 || tmp[i] == 32 || tmp[i] == ':') {
			return false;
		}
	}
	if(tmp.find_first_of(badChars, 0) != string::npos) {
		return false;
	}
	return true;
}

string Util::cleanPathChars(const string& str) {
	string ret(str);
	string::size_type i = 0;
	while((i = ret.find_first_of("/.\\", i)) != string::npos) {
		ret[i] = '_';
	}
	return ret;
}

string Util::addBrackets(const string& s) {
	return '<' + s + '>';
}

void Util::sanitizeUrl(string& url) {
	boost::algorithm::trim_if(url, boost::is_space() || boost::is_any_of("<>\""));
}

/**
 * Decodes a URL the best it can...
 * Default ports:
 * http:// -> port 80
 * dchub:// -> port 411
 */
void Util::decodeUrl(const string& url, string& protocol, string& host, string& port, string& path, string& query, string& fragment) {
	auto fragmentEnd = url.size();
	auto fragmentStart = url.rfind('#');

	size_t queryEnd;
	if(fragmentStart == string::npos) {
		queryEnd = fragmentStart = fragmentEnd;
	} else {
		dcdebug("f");
		queryEnd = fragmentStart;
		fragmentStart++;
	}

	auto queryStart = url.rfind('?', queryEnd);
	size_t fileEnd;

	if(queryStart == string::npos) {
		fileEnd = queryStart = queryEnd;
	} else {
		dcdebug("q");
		fileEnd = queryStart;
		queryStart++;
	}

	auto protoStart = 0;
	auto protoEnd = url.find("://", protoStart);

	auto authorityStart = protoEnd == string::npos ? protoStart : protoEnd + 3;
	auto authorityEnd = url.find_first_of("/#?", authorityStart);

	size_t fileStart;
	if(authorityEnd == string::npos) {
		authorityEnd = fileStart = fileEnd;
	} else {
		dcdebug("a");
		fileStart = authorityEnd;
	}

	protocol = (protoEnd == string::npos ? Util::emptyString : url.substr(protoStart, protoEnd - protoStart));

	if(authorityEnd > authorityStart) {
		dcdebug("x");
		size_t portStart = string::npos;
		if(url[authorityStart] == '[') {
			// IPv6?
			auto hostEnd = url.find(']');
			if(hostEnd == string::npos) {
				return;
			}

			host = url.substr(authorityStart + 1, hostEnd - authorityStart - 1);
			if(hostEnd + 1 < url.size() && url[hostEnd + 1] == ':') {
				portStart = hostEnd + 2;
			}
		} else {
			size_t hostEnd;
			portStart = url.find(':', authorityStart);
			if(portStart != string::npos && portStart > authorityEnd) {
				portStart = string::npos;
			}

			if(portStart == string::npos) {
				hostEnd = authorityEnd;
			} else {
				hostEnd = portStart;
				portStart++;
			}

			dcdebug("h");
			host = url.substr(authorityStart, hostEnd - authorityStart);
		}

		if (portStart == string::npos) {
			if (protocol == "http") {
				port = "80";
			} else if (protocol == "https") {
				port = "443";
			} else if (protocol == "dchub" || protocol == "nmdcs" || protocol.empty()) {
				port = "411";
			}
		} else {
			dcdebug("p");
			port = url.substr(portStart, authorityEnd - portStart);
		}
	}

	dcdebug("\n");
	path = url.substr(fileStart, fileEnd - fileStart);
	query = url.substr(queryStart, queryEnd - queryStart);
	fragment = url.substr(fragmentStart, fragmentEnd - fragmentStart);
}

map<string, string> Util::decodeQuery(const string& query) {
	map<string, string> ret;
	size_t start = 0;
	while(start < query.size()) {
		auto eq = query.find('=', start);
		if(eq == string::npos) {
			break;
		}

		auto param = eq + 1;
		auto end = query.find('&', param);

		if(end == string::npos) {
			end = query.size();
		}

		if(eq > start && end > param) {
			ret[query.substr(start, eq-start)] = query.substr(param, end - param);
		}

		start = end + 1;
	}

	return ret;
}

string Util::getAwayMessage() {
	const auto& msg = awayMsg.empty() ? SETTING(DEFAULT_AWAY_MESSAGE) : awayMsg;
	if (SETTING(AWAY_TIMESTAMP)) {
		return msg.empty() ? msg : formatTime(msg + "[%x %X]", awayTime) + " <DC++ v" VERSIONSTRING ">";
	} else {
		return msg.empty() ? msg : formatTime(msg, awayTime) + " <DC++ v" VERSIONSTRING ">";
	}
}

string Util::formatBytes(int64_t aBytes) {
	char buf[128] = { 0 };
	if(aBytes < 1024) {
		snprintf(buf, sizeof(buf), _("%d B"), (int)(aBytes&0xffffffff));
	} else if(aBytes < 1024*1024) {
		snprintf(buf, sizeof(buf), _("%.02f KiB"), (double)aBytes/(1024.0));
	} else if(aBytes < 1024*1024*1024) {
		snprintf(buf, sizeof(buf), _("%.02f MiB"), (double)aBytes/(1024.0*1024.0));
	} else if(aBytes < (int64_t)1024*1024*1024*1024) {
		snprintf(buf, sizeof(buf), _("%.02f GiB"), (double)aBytes/(1024.0*1024.0*1024.0));
	} else if(aBytes < (int64_t)1024*1024*1024*1024*1024) {
		snprintf(buf, sizeof(buf), _("%.02f TiB"), (double)aBytes/(1024.0*1024.0*1024.0*1024.0));
	} else {
		snprintf(buf, sizeof(buf), _("%.02f PiB"), (double)aBytes/(1024.0*1024.0*1024.0*1024.0*1024.0));
	}

	return buf;
}

string Util::formatExactSize(int64_t aBytes) {
#ifdef _WIN32
		TCHAR tbuf[128];
		TCHAR number[64];
		NUMBERFMT nf;
		_sntprintf(number, 64, _T("%I64d"), aBytes);
		TCHAR Dummy[16];
		TCHAR sep[2] = _T(",");

		/*No need to read these values from the system because they are not
		used to format the exact size*/
		nf.NumDigits = 0;
		nf.LeadingZero = 0;
		nf.NegativeOrder = 0;
		nf.lpDecimalSep = sep;

		GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_SGROUPING, Dummy, 16 );
		nf.Grouping = Util::toInt(Text::fromT(Dummy));
		GetLocaleInfo( LOCALE_SYSTEM_DEFAULT, LOCALE_STHOUSAND, Dummy, 16 );
		nf.lpThousandSep = Dummy;

		GetNumberFormat(LOCALE_USER_DEFAULT, 0, number, &nf, tbuf, sizeof(tbuf)/sizeof(tbuf[0]));

		char buf[128];
		_snprintf(buf, sizeof(buf), _("%s B"), Text::fromT(tbuf).c_str());
		return buf;
#else
		char buf[128];
		snprintf(buf, sizeof(buf), _("%'lld B"), (long long int)aBytes);
		return string(buf);
#endif
}

string Util::getLocalIp(bool v6) {
	// First off, find out whether a specific IP is defined in settings.
	const auto& bindAddr = v6 ? CONNSETTING(BIND_ADDRESS6) : CONNSETTING(BIND_ADDRESS);
	if(!bindAddr.empty() && bindAddr != SettingsManager::getInstance()->getDefault(v6 ? SettingsManager::BIND_ADDRESS6 : SettingsManager::BIND_ADDRESS)) {
		return bindAddr;
	}

	// No bind address configured; get the adapter list to see what we can find.
	auto adapterDataList = getIpAddresses(v6);
	if(adapterDataList.empty()) {
		return string();
	}

	// Prefer public IP addresses.
	for(const auto& adapterData: adapterDataList)
		if(isPublicIp(adapterData.ip, v6))
			return adapterData.ip;

	// No public IP; prefer private (non-local) ones.
	for(const auto& adapterData: adapterDataList)
		if(isPrivateIp(adapterData.ip, v6) && !isLocalIp(adapterData.ip, v6))
			return adapterData.ip;

	// Dang - Return the first local IP of the list.
	return adapterDataList.front().ip;
}

bool Util::isLocalIp(const string& ip, bool v6) noexcept {
	if(v6) {
		return (ip.length() > 4 && ip.substr(0, 4) == "fe80") || ip == "::1";
	}

	return (ip.length() > 3 && strncmp(ip.c_str(), "169", 3) == 0) || ip == "127.0.0.1";
}

bool Util::isPrivateIp(const string& ip, bool v6) noexcept {
	if(v6) {
		// https://en.wikipedia.org/wiki/Unique_local_address
		return ip.length() > 2 && ip.substr(0, 2) == "fd";

	} else {
		in_addr addr;

		addr.s_addr = inet_addr(ip.c_str());

		if(addr.s_addr != INADDR_NONE) {
			unsigned long haddr = ntohl(addr.s_addr);
			return ((haddr & 0xff000000) == 0x0a000000 || // 10.0.0.0/8
					(haddr & 0xfff00000) == 0xac100000 || // 172.16.0.0/12
					(haddr & 0xffff0000) == 0xc0a80000);  // 192.168.0.0/16
		}
	}

	return false;
}

bool Util::isPublicIp(const string& ip, bool v6) noexcept {
	return !isLocalIp(ip, v6) && !isPrivateIp(ip, v6);
}

bool Util::isIpV4(const string& ip) noexcept {
	return inet_addr(ip.c_str()) != INADDR_NONE;
}

vector<Util::AddressInfo> Util::getIpAddresses(bool v6) {
	vector<Util::AddressInfo> adapterData;

#ifdef _WIN32
	ULONG len = 15000; // MSDN states the recommended size should be 15 KB to prevent buffer overflows
	// Prepare 3 runs in case that buffer size is not enough.
	for (int i = 0; i < 3; ++i) {
		auto adapterInfo = (PIP_ADAPTER_ADDRESSES) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, len);

		ULONG ret = GetAdaptersAddresses(v6 ? AF_INET6 : AF_INET, GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, NULL, adapterInfo, &len);
		bool freeObject = true;

		if (ret == ERROR_SUCCESS) {
			for (PIP_ADAPTER_ADDRESSES pAdapterInfo = adapterInfo; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next) {
				// we want only enabled Ethernet interfaces
				if (pAdapterInfo->OperStatus == IfOperStatusUp && (pAdapterInfo->IfType == IF_TYPE_ETHERNET_CSMACD || pAdapterInfo->IfType == IF_TYPE_IEEE80211)) {
					PIP_ADAPTER_UNICAST_ADDRESS ua;
					for (ua = pAdapterInfo->FirstUnicastAddress; ua; ua = ua->Next) {
						// get the name of the adapter
						char buf[BUFSIZ] = { 0 };
						getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
						adapterData.emplace_back(Text::fromT(tstring(pAdapterInfo->FriendlyName)), buf, ua->OnLinkPrefixLength);
					}
					freeObject = false;
				}
			}
		}

		if (freeObject) {
			HeapFree(GetProcessHeap(), 0, adapterInfo);
		}

		if (ret != ERROR_BUFFER_OVERFLOW) {
			break;
		}
	}


#else

#ifdef HAVE_IFADDRS_H
	struct ifaddrs *ifap = nullptr;

	if (getifaddrs(&ifap) == 0) {
		for (struct ifaddrs *i = ifap; i != NULL; i = i->ifa_next) {
			struct sockaddr *sa = i->ifa_addr;

			// If the interface is up, is not a loopback and it has an address
			if ((i->ifa_flags & IFF_UP) && !(i->ifa_flags & IFF_LOOPBACK) && sa != NULL) {
				void* src = nullptr;
				socklen_t len;
				uint32_t scope = 0;

				if (!v6 && sa->sa_family == AF_INET) {
					// IPv4 address
					struct sockaddr_in* sai = (struct sockaddr_in*)sa;
					src = (void*) &(sai->sin_addr);
					len = INET_ADDRSTRLEN;
					scope = 4;
				} else if (v6 && sa->sa_family == AF_INET6) {
					// IPv6 address
					struct sockaddr_in6* sai6 = (struct sockaddr_in6*)sa;
					src = (void*) &(sai6->sin6_addr);
					len = INET6_ADDRSTRLEN;
					scope = sai6->sin6_scope_id;
				}
				char *name = i->ifa_name;
				// Convert the binary address to a string and add it to the output list
				if (src) {
					char address[len];
					inet_ntop(sa->sa_family, src, address, len);
					adapterData.emplace_back(string(name), string(address), scope);
				}
			}
		}
		freeifaddrs(ifap);
	}
#endif

#endif
	return adapterData;
}

typedef const uint8_t* ccp;
static wchar_t utf8ToLC(ccp& str) {
	wchar_t c = 0;
	if(str[0] & 0x80) {
		if(str[0] & 0x40) {
			if(str[0] & 0x20) {
				if(str[1] == 0 || str[2] == 0 ||
					!((((unsigned char)str[1]) & ~0x3f) == 0x80) ||
					!((((unsigned char)str[2]) & ~0x3f) == 0x80))
				{
					str++;
					return 0;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0xf) << 12 |
					((wchar_t)(unsigned char)str[1] & 0x3f) << 6 |
					((wchar_t)(unsigned char)str[2] & 0x3f);
				str += 3;
			} else {
				if(str[1] == 0 ||
					!((((unsigned char)str[1]) & ~0x3f) == 0x80))
				{
					str++;
					return 0;
				}
				c = ((wchar_t)(unsigned char)str[0] & 0x1f) << 6 |
					((wchar_t)(unsigned char)str[1] & 0x3f);
				str += 2;
			}
		} else {
			str++;
			return 0;
		}
	} else {
		wchar_t c = Text::asciiToLower((char)str[0]);
		str++;
		return c;
	}

	return Text::toLower(c);
}

string Util::toString(const StringList& lst) {
	if(lst.empty())
		return emptyString;
	if(lst.size() == 1)
		return lst[0];
	return '[' + toString(",", lst) + ']';
}

string::size_type Util::findSubString(const string& aString, const string& aSubString, string::size_type start) noexcept {
	if(aString.length() < start)
		return (string::size_type)string::npos;

	if(aString.length() - start < aSubString.length())
		return (string::size_type)string::npos;

	if(aSubString.empty())
		return 0;

	// Hm, should start measure in characters or in bytes? bytes for now...
	const uint8_t* tx = (const uint8_t*)aString.c_str() + start;
	const uint8_t* px = (const uint8_t*)aSubString.c_str();

	const uint8_t* end = tx + aString.length() - start - aSubString.length() + 1;

	wchar_t wp = utf8ToLC(px);

	while(tx < end) {
		const uint8_t* otx = tx;
		if(wp == utf8ToLC(tx)) {
			const uint8_t* px2 = px;
			const uint8_t* tx2 = tx;

			for(;;) {
				if(*px2 == 0)
					return otx - (uint8_t*)aString.c_str();

				if(utf8ToLC(px2) != utf8ToLC(tx2))
					break;
			}
		}
	}
	return (string::size_type)string::npos;
}

wstring::size_type Util::findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type pos) noexcept {
	if(aString.length() < pos)
		return static_cast<wstring::size_type>(wstring::npos);

	if(aString.length() - pos < aSubString.length())
		return static_cast<wstring::size_type>(wstring::npos);

	if(aSubString.empty())
		return 0;

	wstring::size_type j = 0;
	wstring::size_type end = aString.length() - aSubString.length() + 1;

	for(; pos < end; ++pos) {
		if(Text::toLower(aString[pos]) == Text::toLower(aSubString[j])) {
			wstring::size_type tmp = pos+1;
			bool found = true;
			for(++j; j < aSubString.length(); ++j, ++tmp) {
				if(Text::toLower(aString[tmp]) != Text::toLower(aSubString[j])) {
					j = 0;
					found = false;
					break;
				}
			}

			if(found)
				return pos;
		}
	}
	return static_cast<wstring::size_type>(wstring::npos);
}

int Util::stricmp(const char* a, const char* b) {
	wchar_t ca = 0, cb = 0;
	while(*a) {
		ca = cb = 0;
		int na = Text::utf8ToWc(a, ca);
		int nb = Text::utf8ToWc(b, cb);
		ca = Text::toLower(ca);
		cb = Text::toLower(cb);
		if(ca != cb) {
			return (int)ca - (int)cb;
		}
		a += abs(na);
		b += abs(nb);
	}
	ca = cb = 0;
	Text::utf8ToWc(a, ca);
	Text::utf8ToWc(b, cb);
	return (int)Text::toLower(ca) - (int)Text::toLower(cb);
}

int Util::strnicmp(const char* a, const char* b, size_t n) {
	const char* end = a + n;
	wchar_t ca = 0, cb = 0;
	while(*a && a < end) {
		ca = cb = 0;
		int na = Text::utf8ToWc(a, ca);
		int nb = Text::utf8ToWc(b, cb);
		ca = Text::toLower(ca);
		cb = Text::toLower(cb);
		if(ca != cb) {
			return (int)ca - (int)cb;
		}
		a += abs(na);
		b += abs(nb);
	}
	ca = cb = 0;
	Text::utf8ToWc(a, ca);
	Text::utf8ToWc(b, cb);
	return (a >= end) ? 0 : ((int)Text::toLower(ca) - (int)Text::toLower(cb));
}

int compare(const std::string& a, const std::string& b) {
	return compare(a.c_str(), b.c_str());
}
int compare(const std::wstring& a, const std::wstring& b) {
	return compare(a.c_str(), b.c_str());
}
int compare(const char* a, const char* b) {
	// compare wide chars because the locale is usually not *.utf8 (never on Win)
	wchar_t ca[2] = { 0 }, cb[2] = { 0 };
	while(*a) {
		ca[0] = cb[0] = 0;
		int na = Text::utf8ToWc(a, ca[0]);
		int nb = Text::utf8ToWc(b, cb[0]);
		auto comp = compare(const_cast<const wchar_t*>(ca), const_cast<const wchar_t*>(cb));
		if(comp) {
			return comp;
		}
		a += abs(na);
		b += abs(nb);
	}
	ca[0] = cb[0] = 0;
	Text::utf8ToWc(a, ca[0]);
	Text::utf8ToWc(b, cb[0]);
	return compare(const_cast<const wchar_t*>(ca), const_cast<const wchar_t*>(cb));
}
int compare(const wchar_t* a, const wchar_t* b) {
	return wcscoll(a, b);
}

string Util::cssColor(int color) {
#ifdef _WIN32
	// assume it's a COLORREF.
	char buf[8];
	snprintf(buf, sizeof(buf), "%.2X%.2X%.2X", GetRValue(color), GetGValue(color), GetBValue(color));
	return buf;
#else
	///@todo
	return string();
#endif
}

string Util::cssFont(const string& font) {
#ifdef _WIN32
	StringTokenizer<string> st(font, ',');
	auto& l = st.getTokens();
	if(l.size() >= 4) {
		string ret = Util::toInt(l[3]) ? "italic" : "normal";
		ret += ' ';
		ret += l[2];
		ret += ' ';
		ret += Util::toString(abs(Util::toFloat(l[1])));
		ret += "px '";
		ret += l[0];
		ret += "'";
		return ret;
	}
	return string();
#else
	///@todo
	return string();
#endif
}

string Util::encodeURI(const string& aString, bool reverse) {
	// reference: rfc2396
	string tmp = aString;
	if(reverse) {
		string::size_type idx;
		for(idx = 0; idx < tmp.length(); ++idx) {
			if(tmp.length() > idx + 2 && tmp[idx] == '%' && isxdigit(tmp[idx+1]) && isxdigit(tmp[idx+2])) {
				tmp[idx] = fromHexEscape(tmp.substr(idx+1,2));
				tmp.erase(idx+1, 2);
			} else { // reference: rfc1630, magnet-uri draft
				if(tmp[idx] == '+')
					tmp[idx] = ' ';
			}
		}
	} else {
		const string disallowed = ";/?:@&=+$," // reserved
								  "<>#%\" "    // delimiters
								  "{}|\\^[]`"; // unwise
		string::size_type idx, loc;
		for(idx = 0; idx < tmp.length(); ++idx) {
			if(tmp[idx] == ' ') {
				tmp[idx] = '+';
			} else {
				if(tmp[idx] <= 0x1F || tmp[idx] >= 0x7f || (loc = disallowed.find_first_of(tmp[idx])) != string::npos) {
					tmp.replace(idx, 1, toHexEscape(tmp[idx]));
					idx+=2;
				}
			}
		}
	}
	return tmp;
}

// used to parse the boost::variant params of the formatParams function.
struct GetString : boost::static_visitor<string> {
	string operator()(const string& s) const { return s; }
	string operator()(const std::function<string ()>& f) const { return f(); }
};

/**
 * This function takes a string and a set of parameters and transforms them according to
 * a simple formatting rule, similar to strftime. In the message, every parameter should be
 * represented by %[name]. It will then be replaced by the corresponding item in
 * the params stringmap. After that, the string is passed through strftime with the current
 * date/time and then finally written to the log file. If the parameter is not present at all,
 * it is removed from the string completely...
 */
string Util::formatParams(const string& msg, const ParamMap& params, FilterF filter) {
	string result = msg;

	string::size_type i, j, k;
	i = 0;
	while (( j = result.find("%[", i)) != string::npos) {
		if( (result.size() < j + 2) || ((k = result.find(']', j + 2)) == string::npos) ) {
			break;
		}

		auto param = params.find(result.substr(j + 2, k - j - 2));

		if(param == params.end()) {
			result.erase(j, k - j + 1);
			i = j;

		} else {
			auto replacement = boost::apply_visitor(GetString(), param->second);

			// replace all % in params with %% for strftime
			replace("%", "%%", replacement);

			if(filter) {
				replacement = filter(replacement);
			}

			result.replace(j, k - j + 1, replacement);
			i = j + replacement.size();
		}
	}

	result = formatTime(result, time(NULL));

	return result;
}

string Util::formatTime(const string &msg, const time_t t) {
	if(!msg.empty()) {
		tm* loc = localtime(&t);
		if(!loc) {
			return Util::emptyString;
		}

		size_t bufsize = msg.size() + 256;
		string buf(bufsize, 0);

		errno = 0;

		buf.resize(strftime(&buf[0], bufsize-1, msg.c_str(), loc));

		while(buf.empty()) {
			if(errno == EINVAL)
				return Util::emptyString;

			bufsize+=64;
			buf.resize(bufsize);
			buf.resize(strftime(&buf[0], bufsize-1, msg.c_str(), loc));
		}

#ifdef _WIN32
		if(!Text::validateUtf8(buf))
#endif
		{
			buf = Text::toUtf8(buf);
		}
		return buf;
	}
	return Util::emptyString;
}

/* Below is a high-speed random number generator with much
   better granularity than the CRT one in msvc...(no, I didn't
   write it...see copyright) */
/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.
   Any feedback is very welcome. For any question, comments,
   see http://www.math.keio.ac.jp/matumoto/emt.html or email
   matumoto@math.keio.ac.jp */
/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y) (y >> 11)
#define TEMPERING_SHIFT_S(y) (y << 7)
#define TEMPERING_SHIFT_T(y) (y << 15)
#define TEMPERING_SHIFT_L(y) (y >> 18)

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializing the array with a NONZERO seed */
static void sgenrand(unsigned long seed) {
	/* setting initial seeds to mt[N] using         */
	/* the generator Line 25 of Table 1 in          */
	/* [KNUTH 1981, The Art of Computer Programming */
	/*    Vol. 2 (2nd Ed.), pp102]                  */
	mt[0]= seed & 0xffffffff;
	for (mti=1; mti<N; mti++)
		mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

uint32_t Util::rand() {
	unsigned long y;
	static unsigned long mag01[2]={0x0, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */

	if (mti >= N) { /* generate N words at one time */
		int kk;

		if (mti == N+1)   /* if sgenrand() has not been called, */
			sgenrand(4357); /* a default initial seed is used   */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

		mti = 0;
	}

	y = mt[mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return y;
}

string Util::getShortTimeString(time_t t) {
	char buf[255];
	tm* _tm = localtime(&t);
	if(_tm == NULL) {
		strcpy(buf, "xx:xx");
	} else {
		strftime(buf, 254, SETTING(TIME_STAMPS_FORMAT).c_str(), _tm);
	}
	return Text::toUtf8(buf);
}

string Util::getTimeString() {
	time_t _tt;
	time(&_tt);

	return getTimeString(_tt);
}

string Util::getTimeString(time_t _tt) {
	return getTimeString(_tt, "%X");
}

string Util::getTimeString(time_t _tt, const string& formatting) {
	char buf[254];
	tm* _tm = localtime(&_tt);
	if(_tm == NULL) {
		strcpy(buf, "xx:xx:xx");
	} else {
		strftime(buf, 254, formatting.c_str(), _tm);
	}
	return buf;
}

string Util::toAdcFile(const string& file) {
	if(file == "files.xml.bz2" || file == "files.xml")
		return file;

	string ret;
	ret.reserve(file.length() + 1);
	ret += '/';
	ret += file;
	for(string::size_type i = 0; i < ret.length(); ++i) {
		if(ret[i] == '\\') {
			ret[i] = '/';
		}
	}
	return ret;
}
string Util::toNmdcFile(const string& file) {
	if(file.empty())
		return Util::emptyString;

	string ret(file.substr(1));
	for(string::size_type i = 0; i < ret.length(); ++i) {
		if(ret[i] == '/') {
			ret[i] = '\\';
		}
	}
	return ret;
}

string Util::translateError(int aError) {
#ifdef _WIN32
	LPTSTR lpMsgBuf;
	DWORD chars = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		aError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
		);
	if(chars == 0) {
		return string();
	}
	string tmp = Text::fromT(lpMsgBuf);
	// Free the buffer.
	LocalFree( lpMsgBuf );
	string::size_type i = 0;

	while( (i = tmp.find_first_of("\r\n", i)) != string::npos) {
		tmp.erase(i, 1);
	}
	return tmp;
#else // _WIN32
	return Text::toUtf8(strerror(aError));
#endif // _WIN32
}

bool Util::getAway() {
	return away != 0;
}

void Util::incAway() {
	setAwayCounter(away + 1);
}

void Util::decAway() {
	setAwayCounter((away > 0) ? away - 1 : 0);
}

void Util::setAway(bool b) {
	setAwayCounter(b ? (away + 1) : 0);
}

void Util::switchAway() {
	setAway(!getAway());
}

void Util::setAwayCounter(uint8_t i) {
	auto prev = getAway();

	away = i;

	if(getAway() != prev) {
		if(getAway()) {
			awayTime = time(0);
		}
		ClientManager::getInstance()->infoUpdated();
	}
}

string Util::getTempPath() {
#ifdef _WIN32
	TCHAR buf[MAX_PATH + 1];
	DWORD x = GetTempPath(MAX_PATH, buf);
	return Text::fromT(tstring(buf, x));
#else
	return "/tmp/";
#endif
}

bool Util::isAdcUrl(const string& aHubURL) {
	return Util::strnicmp("adc://", aHubURL.c_str(), 6) == 0;
}

bool Util::isAdcsUrl(const string& aHubURL) {
	return Util::strnicmp("adcs://", aHubURL.c_str(), 7) == 0;
}

bool Util::isNmdcUrl(const string& aHubURL) { // note: above functions are used only for FavHubProperties::handleInitDialog() so these are useless
	return Util::strnicmp("dchub://", aHubURL.c_str(), 8) == 0;
}

bool Util::isNmdcsUrl(const string& aHubURL) {
	return Util::strnicmp("nmdcs://", aHubURL.c_str(), 8) == 0;
}

size_t noCaseStringHash::operator()(const string& s) const {
	size_t x = 0;
	auto end = s.data() + s.size();
	for(auto str = s.data(); str < end; ) {
		wchar_t c = 0;
		int n = Text::utf8ToWc(str, c);
		if(n < 0) {
			x = x * 32 - x + '_';
			str += abs(n);
		} else {
			x = x * 32 - x + static_cast<size_t>(Text::toLower(c));
			str += n;
		}
	}
	return x;
}

size_t noCaseStringHash::operator()(const wstring& s) const {
	size_t x = 0;
	auto y = s.data();
	for(decltype(s.size()) i = 0, j = s.size(); i < j; ++i) {
		x = x * 31 + static_cast<size_t>(Text::toLower(y[i]));
	}
	return x;
}

} // namespace dcpp
