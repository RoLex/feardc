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

#ifndef DCPLUSPLUS_DCPP_UTIL_H
#define DCPLUSPLUS_DCPP_UTIL_H

#include "compiler.h"

#include <cstdlib>
#include <ctime>
#include <random>

#include <map>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/remove_if.hpp>

#ifdef _WIN32

# define PATH_SEPARATOR '\\'
# define PATH_SEPARATOR_STR "\\"

#else

# define PATH_SEPARATOR '/'
# define PATH_SEPARATOR_STR "/"

#include <sys/stat.h>
#include <unistd.h>

#endif

#include "Text.h"

namespace dcpp {

using std::map;

#define LIT(x) x, (sizeof(x)-1)

/** Evaluates op(pair<T1, T2>.first, compareTo) */
template<class T1, class T2, class op = std::equal_to<T1> >
class CompareFirst {
public:
	CompareFirst(const T1& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, T2>& p) { return op()(p.first, a); }
private:
	const T1& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class op = std::equal_to<T2> >
class CompareSecond {
public:
	CompareSecond(const T2& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, T2>& p) { return op()(p.second, a); }
private:
	const T2& a;
};

/**
 * Compares two values
 * @return -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2
 */
template<typename T>
int compare(const T& v1, const T& v2) {
	return (v1 < v2) ? -1 : ((v1 == v2) ? 0 : 1);
}
/** Locale-aware string comparison, to be used when sorting. */
int compare(const std::string& a, const std::string& b);
int compare(const std::wstring& a, const std::wstring& b);
int compare(const char* a, const char* b);
int compare(const wchar_t* a, const wchar_t* b);

/** Uses SFINAE to determine whether a type provides a function; stores the result in "value".
Inspired by <https://stackoverflow.com/a/8752988>. */
#define HAS_FUNC(name, funcRet, funcTest) \
	template<typename HAS_FUNC_T> struct name { \
		typedef char yes[1]; \
		typedef char no[2]; \
		template<typename HAS_FUNC_U> static yes& check( \
			typename std::enable_if<std::is_same<funcRet, decltype(std::declval<HAS_FUNC_U>().funcTest)>::value>::type*); \
		template<typename> static no& check(...); \
		static const bool value = sizeof(check<HAS_FUNC_T>(nullptr)) == sizeof(yes); \
	}

class Util
{
public:
	static tstring emptyStringT;
	static string emptyString;
	static wstring emptyStringW;

	enum Paths {
		/** Global configuration */
		PATH_GLOBAL_CONFIG,
		/** Per-user configuration (queue, favorites, ...) */
		PATH_USER_CONFIG,
		/** Per-user local data (cache, temp files, ...) */
		PATH_USER_LOCAL,
		/** Various resources (help files etc) */
		PATH_RESOURCES,
		/** Translations */
		PATH_LOCALE,
		/** Default download location */
		PATH_DOWNLOADS,
		/** Default file list location */
		PATH_FILE_LISTS,
		/** Default hub list cache */
		PATH_HUB_LISTS,
		/** Where the notepad file is stored */
		PATH_NOTEPAD,
		PATH_LAST
	};

	typedef std::map<Util::Paths, std::string> PathsMap;
	static void initialize(PathsMap pathOverrides = PathsMap());

	/** Path of temporary storage */
	static string getTempPath();

	/** Path of configuration files */
	static const string& getPath(Paths path) { return paths[path]; }

	/** Migrate from pre-localmode config location */
	static void migrate(const string& file);

	/** Path of file lists */
	static string getListPath() { return getPath(PATH_FILE_LISTS); }
	/** Path of hub lists */
	static string getHubListsPath() { return getPath(PATH_HUB_LISTS); }
	/** Notepad filename */
	static string getNotepadFile() { return getPath(PATH_NOTEPAD); }

	static time_t getStartTime() { return startTime; }

	/** IETF language tag of the language currently in use. */
	static string getIETFLang();

	static string translateError(int aError);

	static string getFilePath(const string& path, char separator = PATH_SEPARATOR) {
		string::size_type i = path.rfind(separator);
		return (i != string::npos) ? path.substr(0, i + 1) : path;
	}
	static string getFileName(const string& path, char separator = PATH_SEPARATOR) {
		string::size_type i = path.rfind(separator);
		return (i != string::npos) ? path.substr(i + 1) : path;
	}
	static string getFileExt(const string& path) {
		string::size_type i = path.rfind('.');
		return (i != string::npos) ? path.substr(i) : Util::emptyString;
	}
	static string getLastDir(const string& path, char separator = PATH_SEPARATOR) {
		string::size_type i = path.rfind(separator);
		if(i == string::npos)
			return Util::emptyString;
		string::size_type j = path.rfind(separator, i-1);
		return (j != string::npos) ? path.substr(j+1, i-j-1) : path;
	}

	static wstring getFilePath(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(0, i + 1) : path;
	}
	static wstring getFileName(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(i + 1) : path;
	}
	static wstring getFileExt(const wstring& path) {
		wstring::size_type i = path.rfind('.');
		return (i != wstring::npos) ? path.substr(i) : Util::emptyStringW;
	}
	static wstring getLastDir(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == wstring::npos)
			return Util::emptyStringW;
		wstring::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != wstring::npos) ? path.substr(j+1, i-j-1) : path;
	}

	template<typename string_t>
	static void replace(const string_t& search, const string_t& replacement, string_t& str) {
		typename string_t::size_type i = 0;
		while((i = str.find(search, i)) != string_t::npos) {
			str.replace(i, search.size(), replacement);
			i += replacement.size();
		}
	}
	template<typename string_t>
	static inline void replace(const typename string_t::value_type* search, const typename string_t::value_type* replacement, string_t& str) {
		replace(string_t(search), string_t(replacement), str);
	}

	static void sanitizeUrl(string& url);
	static void decodeUrl(const string& aUrl, string& protocol, string& host, string& port, string& path, string& query, string& fragment);
	static map<string, string> decodeQuery(const string& query);

	static string validateFileName(string aFile);
	static bool checkExtension(const string& tmp);
	static string cleanPathChars(const string& str);
	static string addBrackets(const string& s);

	static string formatBytes(const string& aString) { return formatBytes(toInt64(aString)); }

	static string getShortTimeString(time_t t = time(NULL) );

	static string getTimeString();
	static string getTimeString(time_t t);
	static string getTimeString(time_t t, const string& formatting);
	static string toAdcFile(const string& file);
	static string toNmdcFile(const string& file);

	static string formatBytes(int64_t aBytes);

	static string formatExactSize(int64_t aBytes);

	static string formatSeconds(int64_t aSec) {
		char buf[64];
		snprintf(buf, sizeof(buf), "%01lu:%02d:%02d", (unsigned long)(aSec / (60*60)), (int)((aSec / 60) % 60), (int)(aSec % 60));
		return buf;
	}

	typedef string (*FilterF)(const string&);
	static string formatParams(const string& msg, const ParamMap& params, FilterF filter = 0);

	static string formatTime(const string &msg, const time_t t);

	static inline int64_t roundDown(int64_t size, int64_t blockSize) {
		return ((size + blockSize / 2) / blockSize) * blockSize;
	}

	static inline int64_t roundUp(int64_t size, int64_t blockSize) {
		return ((size + blockSize - 1) / blockSize) * blockSize;
	}

	static inline int roundDown(int size, int blockSize) {
		return ((size + blockSize / 2) / blockSize) * blockSize;
	}

	static inline int roundUp(int size, int blockSize) {
		return ((size + blockSize - 1) / blockSize) * blockSize;
	}


	static int64_t toInt64(const string& aString) {
#ifdef _WIN32
		return _atoi64(aString.c_str());
#else
		return strtoll(aString.c_str(), (char **)NULL, 10);
#endif
	}

	static int toInt(const string& aString) {
		return atoi(aString.c_str());
	}
	static uint32_t toUInt32(const string& str) {
		return toUInt32(str.c_str());
	}
	static uint32_t toUInt32(const char* c) {
#ifdef _MSC_VER
		/*
		* MSVC's atoi returns INT_MIN/INT_MAX if out-of-range; hence, a number
		* between INT_MAX and UINT_MAX can't be converted back to uint32_t.
		*/
		uint32_t ret = atoi(c);
		if(errno == ERANGE)
			return (uint32_t)_atoi64(c);
		return ret;
#else
		return (uint32_t)atoi(c);
#endif
	}

	static unsigned toUInt(const string& s) {
		if(s.empty())
			return 0;
		int ret = toInt(s);
		if(ret < 0)
			return 0;
		return ret;
	}

	static double toDouble(const string& aString) {
		// Work-around for atof and locales...
		lconv* lv = localeconv();
		string::size_type i = aString.find_last_of(".,");
		if(i != string::npos && aString[i] != lv->decimal_point[0]) {
			string tmp(aString);
			tmp[i] = lv->decimal_point[0];
			return atof(tmp.c_str());
		}
		return atof(aString.c_str());
	}

	static float toFloat(const string& aString) {
		return (float)toDouble(aString.c_str());
	}

	// Overloads used by SimpleXML::addChildAttrib
	// https://sourceforge.net/p/dcplusplus/code/ci/b508c2f2047949e6f815f9f9e6db08239facd7cb/tree/dcpp/SimpleXML.h#l63
	// Don't use unconstrained templates to avoid modifying overload
	// resolution
	static string toString(int val) {
		return std::to_string(val);
	}
	static string toString(unsigned int val) {
		return std::to_string(val);
	}
	static string toString(long val) {
		return std::to_string(val);
	}
	static string toString(long long val) {
		return std::to_string(val);
	}

	// Ensure these aren't used and that unexpected overload resolution
	// doesn't occur. If/when toString is removed, these can too
	static string toString(short val) = delete;
	static string toString(unsigned short val) = delete;
	static string toString(unsigned long val) = delete;
	static string toString(unsigned long long val) = delete;

	static string toString(double val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%0.2f", val);
		return buf;
	}

	template<typename string_t>
	static string_t toString(const string_t& sep, const vector<string_t>& lst) {
		string_t ret;
		for(auto i = lst.begin(), iend = lst.end(); i != iend; ++i) {
			ret += *i;
			if(i + 1 != iend)
				ret += sep;
		}
		return ret;
	}
	template<typename string_t>
	static inline string_t toString(const typename string_t::value_type* sep, const vector<string_t>& lst) {
		return toString(string_t(sep), lst);
	}
	static string toString(const StringList& lst);

	static string toHexEscape(char val) {
		char buf[sizeof(int)*2+1+1];
		snprintf(buf, sizeof(buf), "%%%X", val&0x0FF);
		return buf;
	}
	static char fromHexEscape(const string aString) {
		unsigned int res = 0;
		sscanf(aString.c_str(), "%X", &res);
		return static_cast<char>(res);
	}

	template<typename T>
	static T& intersect(T& t1, const T& t2) {
		t1.erase(boost::remove_if(t1, [&](const typename T::value_type& v1) {
			return boost::find_if(t2, [&](const typename T::value_type& v2) { return v2 == v1; }) == t2.end();
		}), t1.end());
		return t1;
	}

	template<typename T>
	static void trim(T& s) {
		auto l = std::locale{""};
		s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [&l](char c){ return std::isspace(c, l); }));
 		s.erase(std::find_if_not(s.rbegin(), s.rend(), [&l](char c){ return std::isspace(c, l); }).base(), s.end());
	}

	/// make a color suitable for a CSS declaration (implementation dependant).
	static string cssColor(int color);
	/// make a font suitable for a CSS declaration (implementation dependant).
	static string cssFont(const string& font);

	static string encodeURI(const string& /*aString*/, bool reverse = false);

	/** Get an address to bind to, in the following order of preference:
	 * - Bind address setting.
	 * - Public IP.
	 * - Private (but not local) IP.
	 * - Local IP.
	 */
	static string getLocalIp(bool v6);

	// Return whether the IP is localhost or a link-local address (169.254.0.0/16 or fe80)
	static bool isLocalIp(const string& ip, bool v6) noexcept;

	// Return whether the IP is a private one (non-local)
	//
	// Private ranges:
	// IPv4: 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16
	// IPv6: fd prefix
	static bool isPrivateIp(const string& ip, bool v6) noexcept;

	static bool isPublicIp(const string& ip, bool v6) noexcept;

	// Return whether the IP is a well formatted IPv4 address
	static bool isIpV4(const string& ip) noexcept;

	struct AddressInfo {
		AddressInfo(const string& aName, const string& aIP, uint8_t aPrefix) : adapterName(aName), ip(aIP), prefix(aPrefix) { }
		string adapterName;
		string ip;
		uint32_t prefix;
	};
	static vector<AddressInfo> getIpAddresses(bool v6);

	/**
	 * Case insensitive substring search.
	 * @return First position found or string::npos
	 */
	static string::size_type findSubString(const string& aString, const string& aSubString, string::size_type start = 0) noexcept;
	static wstring::size_type findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type start = 0) noexcept;

	/* Utf-8 versions of strnicmp and stricmp, unicode char code order (!) */
	static int stricmp(const char* a, const char* b);
	static int strnicmp(const char* a, const char* b, size_t n);

	static int stricmp(const wchar_t* a, const wchar_t* b) {
		while(*a && Text::toLower(*a) == Text::toLower(*b))
			++a, ++b;
		return ((int)Text::toLower(*a)) - ((int)Text::toLower(*b));
	}
	static int strnicmp(const wchar_t* a, const wchar_t* b, size_t n) {
		while(n && *a && Text::toLower(*a) == Text::toLower(*b))
			--n, ++a, ++b;

		return n == 0 ? 0 : ((int)Text::toLower(*a)) - ((int)Text::toLower(*b));
	}

	static int stricmp(const string& a, const string& b) { return stricmp(a.c_str(), b.c_str()); }
	static int strnicmp(const string& a, const string& b, size_t n) { return strnicmp(a.c_str(), b.c_str(), n); }
	static int stricmp(const wstring& a, const wstring& b) { return stricmp(a.c_str(), b.c_str()); }
	static int strnicmp(const wstring& a, const wstring& b, size_t n) { return strnicmp(a.c_str(), b.c_str(), n); }

	static bool getAway();
	static void incAway();
	static void decAway();
	static void setAway(bool b);
	static void switchAway();

	static string getAwayMessage();
	static void setAwayMessage(const string& aMsg) { awayMsg = aMsg; }

	static uint32_t rand(uint32_t low = 0, uint32_t high = UINT32_MAX);
	inline static std::mt19937 mt;

	static bool isAdcUrl(const string& aHubURL);
	static bool isAdcsUrl(const string& aHubURL);
	static bool isNmdcUrl(const string& aHubURL);
	static bool isNmdcsUrl(const string& aHubURL);

private:
	/** In local mode, all config and temp files are kept in the same dir as the executable */
	static bool localMode;

	static string paths[PATH_LAST];

	static uint8_t away; // in away mode when != 0.
	static string awayMsg;
	static time_t awayTime;
	static time_t startTime;

	static void loadBootConfig();
	static void setAwayCounter(uint8_t i);
};

/** Case insensitive hash function for strings */
struct noCaseStringHash {
	size_t operator()(const string* s) const {
		return operator()(*s);
	}
	size_t operator()(const string& s) const;

	size_t operator()(const wstring* s) const {
		return operator()(*s);
	}
	size_t operator()(const wstring& s) const;

	bool operator()(const string* a, const string* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) < 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) < 0;
	}
};

/** Case insensitive string comparison */
struct noCaseStringEq {
	bool operator()(const string* a, const string* b) const {
		return a == b || Util::stricmp(*a, *b) == 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) == 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return a == b || Util::stricmp(*a, *b) == 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) == 0;
	}
};

/** Case insensitive string ordering */
struct noCaseStringLess {
	bool operator()(const string* a, const string* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) < 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) < 0;
	}
};

} // namespace dcpp

#endif // !defined(UTIL_H)
