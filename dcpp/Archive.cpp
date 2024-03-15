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

#include "stdinc.h"
#include "Archive.h"

#include "debug.h"
#include "File.h"
#include "Util.h"

#include <unzip.h>
#ifdef _WIN32
#include <iowin32.h>
#endif

namespace dcpp {

Archive::Archive(const string& path) {
#ifdef _WIN32
	zlib_filefunc64_def funcs;
	fill_win32_filefunc64(&funcs);
	file = unzOpen2_64(Text::toT(path).c_str(), &funcs);
#else
	file = unzOpen64(path.c_str());
#endif
	if(!file) {
		throw Exception(_("Invalid archive"));
	}
}

Archive::~Archive() {
	if(file) {
		unzClose(file);
	}
}

void Archive::extract(const string& path) {
	auto isDir = [](const string& path) { return *(path.end() - 1) == '/' || *(path.end() - 1) == '\\'; };

	dcassert(!path.empty() && isDir(path));

	if(check(unzGoToFirstFile(file)) != UNZ_OK) { return; }

	do {
		char pathBuf[PATH_MAX];
		if(check(unzGetCurrentFileInfo(file, nullptr, pathBuf, PATH_MAX, nullptr, 0, nullptr, 0)) != UNZ_OK) { continue; }
		if(check(unzOpenCurrentFile(file)) != UNZ_OK) { continue; }

		string path_out(pathBuf);
		if(path_out.empty() || isDir(path_out)) {
			continue;
		}
		path_out = path + path_out;

		File::ensureDirectory(path_out);
		File f_out(path_out, File::WRITE, File::CREATE | File::TRUNCATE);

		const size_t BUF_SIZE = 64 * 1024;
		ByteVector buf(BUF_SIZE);
		while(true) {
			auto read = unzReadCurrentFile(file, &buf[0], BUF_SIZE);
			if(read > 0) {
				f_out.write(&buf[0], read);
			} else if(read == UNZ_EOF) {
				break;
			} else {
				check(read);
			}
		}

		// this does the CRC check.
		check(unzCloseCurrentFile(file));

	} while(check(unzGoToNextFile(file)) == UNZ_OK);
}

int Archive::check(int ret) {
	if(ret != UNZ_OK && ret != UNZ_END_OF_LIST_OF_FILE && ret != UNZ_EOF) {
		throw Exception(_("Invalid archive"));
	}
	return ret;
}

} // namespace dcpp
