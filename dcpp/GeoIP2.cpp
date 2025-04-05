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

#include "stdinc.h"
#include "GeoIP2.h"

#include "File.h"
#include "format.h"
#include "SettingsManager.h"
#include "Util.h"
#include "ZUtils.h"

#include <maxminddb.h>

namespace dcpp {

// our codes mapped to supported geoip codes
// https://www.maxmind.com/en/geoip2-databases
// includes localized names for select locations in:
// simplified chinese, french, german, japanese, spanish, brazilian portuguese, russian
// todo: generate a lang_map as in win32\AppearancePage.cpp

map<string, string> localeGeoMappings = {
	// "en_*" dont require mapping
	{ "de", "de" },
	{ "es", "es" },
	{ "fr", "fr" },
	{ "ja", "ja" },
	{ "ru", "ru" },
	{ "pt_BR", "pt-BR" },
	{ "zh_CN", "zh-CN" }
};

static string parseLanguage() noexcept {
	const string lang = SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(SettingsManager::LANGUAGE));
	auto i = localeGeoMappings.find(lang);
	return (i != localeGeoMappings.end() ? i->second : "en");
}

GeoIP::GeoIP(string&& aPath):
geo(0),
path(std::move(aPath)),
language(parseLanguage())
{
	if (File::getSize(path) > 0 || decompress())
		open();
}

GeoIP::~GeoIP()
{
	Lock l(cs);
	close();
}

namespace {

static string parseData(MMDB_lookup_result_s res, ...) {
	MMDB_entry_data_s entry_data;
	va_list keys;
	va_start(keys, res);
	auto status = MMDB_vget_value(&res.entry, &entry_data, keys);
	va_end(keys);

	if (status != MMDB_SUCCESS)
		return Util::emptyString;

	if (!entry_data.has_data)
		return Util::emptyString;

	if (entry_data.type != MMDB_DATA_TYPE_UTF8_STRING && entry_data.type != MMDB_DATA_TYPE_DOUBLE && entry_data.type != MMDB_DATA_TYPE_UINT16) {
		dcdebug("Invalid GeoIP2 data type %d:\n", entry_data.type);
		return Util::emptyString;
	}

	if (entry_data.type == MMDB_DATA_TYPE_UTF8_STRING)
		return ((entry_data.data_size > 0) ? string(entry_data.utf8_string, entry_data.data_size) : Util::emptyString);
	else if (entry_data.type == MMDB_DATA_TYPE_DOUBLE)
		return std::to_string(entry_data.double_value);
	else if (entry_data.type == MMDB_DATA_TYPE_UINT16)
		return std::to_string(entry_data.uint16);
	else
		return Util::emptyString;
}

} // unnamed namespace

string GeoIP::getCountry(const string& ip) const {
	Lock l(cs);

	if (!geo)
		return Util::emptyString;

	int gai_error, mmdb_error;
	MMDB_lookup_result_s res = MMDB_lookup_string(const_cast<MMDB_s*>(geo), ip.c_str(), &gai_error, &mmdb_error);

	if (res.found_entry && mmdb_error == MMDB_SUCCESS && gai_error == 0) {
		const string& setting = SETTING(COUNTRY_FORMAT);

		ParamMap params;
		params["2code"] = [&] { return parseData(res, "country", "iso_code", NULL); };
		params["continent"] = [&] { return parseData(res, "continent", "code", NULL); };
		params["engname"] = [&] { return parseData(res, "country", "names", "en", NULL); };
		params["name"] = [&] { return parseData(res, "country", "names", language.c_str(), NULL); };
		params["officialname"] = [&] { return parseData(res, "country", "names", language.c_str(), NULL); };
		params["areacode"] = [&] { return parseData(res, "subdivisions", "0", "iso_code", NULL); };
		params["city"] = [&] { return parseData(res, "city", "names", language.c_str(), NULL); };
		params["lat"] = [&] { return parseData(res, "location", "latitude", NULL); };
		params["long"] = [&] { return parseData(res, "location", "longitude", NULL); };
		params["metrocode"] = [&] { return parseData(res, "location", "metro_code", NULL); };
		params["postcode"] = [&] { return parseData(res, "postal", "code", NULL); };
		params["region"] = [&] { return parseData(res, "subdivisions", "0", "names", language.c_str(), NULL); };

		return Util::formatParams(setting, params);
	}

	if (gai_error != 0) {
#ifdef WIN32
		dcdebug("Error from getaddrinfo for %s - %ls\n",
#else
		dcdebug("Error from getaddrinfo for %s - %s\n",
#endif
			ip.c_str(), gai_strerror(gai_error));
	}

	if (mmdb_error != MMDB_SUCCESS)
		dcdebug("Got an error from libmaxminddb (MMDB_lookup_string): %s\n", MMDB_strerror(mmdb_error));

	return Util::emptyString;
}

string GeoIP::getEpoch() const {
	Lock l(cs);

	if (!geo)
		return Util::emptyString;

	time_t epoch = geo->metadata.build_epoch;
	return Util::formatTime("%d %b %Y", time(&epoch));
}

string GeoIP::getVersion() const {
	return MMDB_lib_version();
}

void GeoIP::update() {
	Lock l(cs);
	close();

	if (decompress())
		open();
}

bool GeoIP::decompress() const {
	if (File::getSize(path + ".gz") <= 0)
		return false;

	try {
		GZ::decompress(path + ".gz", path);
	} catch(const Exception&) {
		return false;
	}

	return true;
}

void GeoIP::open() {
	geo = (MMDB_s*)malloc(sizeof(MMDB_s));
	auto res = MMDB_open(Text::fromUtf8(path).c_str(), MMDB_MODE_MMAP, geo);

	if (res != MMDB_SUCCESS) {
		dcdebug("Failed to open MMDB database %s\n", MMDB_strerror(res));

		if (geo) {
			free(geo);
			geo = 0;
		}
	}
}

void GeoIP::close() {
	MMDB_close(geo);
	free(geo);
	geo = 0;
}

} // namespace dcpp
