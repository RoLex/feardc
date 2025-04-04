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
#include "ZUtils.h"

#include "Exception.h"
#include "File.h"
#include "format.h"
#include "ScopedFunctor.h"

namespace dcpp {

using std::max;

const double ZFilter::MIN_COMPRESSION_LEVEL = 0.95;

ZFilter::ZFilter() : totalIn(0), totalOut(0), compressing(true) {
	memset(&zs, 0, sizeof(zs));

	if(deflateInit(&zs, 3) != Z_OK) {
		throw Exception(_("Error during compression"));
	}
}

ZFilter::~ZFilter() {
#ifdef ZLIB_DEBUG
	dcdebug("ZFilter end, %ld/%ld = %.04f\n", zs.total_out, zs.total_in, (float)zs.total_out / max((float)zs.total_in, (float)1));
#endif
	deflateEnd(&zs);
}

bool ZFilter::operator()(const void* in, size_t& insize, void* out, size_t& outsize) {
	if(outsize == 0)
		return false;

	zs.next_in = (Bytef*)in;
	zs.next_out = (Bytef*)out;

#ifdef ZLIB_DEBUG
	dcdebug("ZFilter: totalOut = %lld, totalIn = %lld, outsize = %d\n", totalOut, totalIn, outsize);
#endif

	// Check if there's any use compressing; if not, save some cpu...
	if(compressing && insize > 0 && outsize > 16 && (totalIn > (64 * 1024)) &&
		(static_cast<double>(totalOut) / totalIn) > MIN_COMPRESSION_LEVEL)
	{
		zs.avail_in = 0;
		zs.avail_out = outsize;

		// Starting with zlib 1.2.9, the deflateParams API has changed.
		auto err = ::deflateParams(&zs, 0, Z_DEFAULT_STRATEGY);

		if(err == Z_STREAM_ERROR) {
			throw Exception(_("Error during compression"));
		}

		zs.avail_in = insize;
		compressing = false;
		dcdebug("ZFilter: Dynamically disabled compression\n");

		// Check if we ate all space already...
		// Starting with zlib 1.2.12, generation of Z_BUF_ERROR in deflateParams has changed.
		if(zs.avail_out == 0) {
			outsize = outsize - zs.avail_out;
			insize = insize - zs.avail_in;
			totalOut += outsize;
			totalIn += insize;
			return true;
		}
	} else {
		zs.avail_in = insize;
		zs.avail_out = outsize;
	}

	if(insize == 0) {
		int err = ::deflate(&zs, Z_FINISH);
		if(err != Z_OK && err != Z_STREAM_END)
			throw Exception(_("Error during compression"));

		outsize = outsize - zs.avail_out;
		insize = insize - zs.avail_in;
		totalOut += outsize;
		totalIn += insize;
		return err == Z_OK;
	} else {
		int err = ::deflate(&zs, Z_NO_FLUSH);
		if(err != Z_OK)
			throw Exception(_("Error during compression"));

		outsize = outsize - zs.avail_out;
		insize = insize - zs.avail_in;
		totalOut += outsize;
		totalIn += insize;
		return true;
	}
}

UnZFilter::UnZFilter() {
	memset(&zs, 0, sizeof(zs));

	if(inflateInit(&zs) != Z_OK)
		throw Exception(_("Error during decompression"));
}

UnZFilter::~UnZFilter() {
#ifdef ZLIB_DEBUG
	dcdebug("UnZFilter end, %ld/%ld = %.04f\n", zs.total_out, zs.total_in, (float)zs.total_out / max((float)zs.total_in, (float)1));
#endif
	inflateEnd(&zs);
}

bool UnZFilter::operator()(const void* in, size_t& insize, void* out, size_t& outsize) {
	if(outsize == 0)
		return 0;

	zs.avail_in = insize;
	zs.next_in = (Bytef*)in;
	zs.avail_out = outsize;
	zs.next_out = (Bytef*)out;

	int err = ::inflate(&zs, Z_NO_FLUSH);

	// see zlib/contrib/minizip/unzip.c, Z_BUF_ERROR means we should have padded
	// with a dummy byte if at end of stream - since we don't do this it's not a real
	// error
	if(!(err == Z_OK || err == Z_STREAM_END || (err == Z_BUF_ERROR && in == NULL)))
		throw Exception(_("Error during decompression"));

	outsize = outsize - zs.avail_out;
	insize = insize - zs.avail_in;
	return err == Z_OK;
}

void GZ::decompress(const string& source, const string& target) {
#ifdef _WIN32
	auto gz = gzopen_w(Text::toT(source).c_str(), "rb");
#else
	auto gz = gzopen(source.c_str(), "rb");
#endif
	if(!gz) {
		throw Exception(_("Error during decompression"));
	}
	ScopedFunctor([&gz] { gzclose(gz); });

	File f(target, File::WRITE, File::CREATE | File::TRUNCATE);

	const size_t BUF_SIZE = 64 * 1024;
	const int BUF_SIZE_INT = static_cast<int>(BUF_SIZE);
	ByteVector buf(BUF_SIZE);

	while(true) {
		auto read = gzread(gz, &buf[0], BUF_SIZE);
		if(read > 0) {
			f.write(&buf[0], read);
		}
		if(read < BUF_SIZE_INT) {
			break;
		}
	}
}

} // namespace dcpp
