/* Tests around the wrappers the dcpp lib has around zlib. These tests are as close as possible to
 * the way the dcpp lib uses these wrappers to compress / decompress files when uploading /
 * downloading them.
 *
 * We also run tests from the zlib project itself (they are in the "zlib/test" directory). */

#include "testbase.h"

#include <dcpp/File.h>
#include <dcpp/FilteredFile.h>
#include <dcpp/HashValue.h>
#include <dcpp/TigerHash.h>
#include <dcpp/ZUtils.h>

using namespace dcpp;

auto tiger(auto path) {
	File f(path, File::READ, File::OPEN);
	TigerHash h;
	auto buf = f.read();
	h.update(buf.c_str(), buf.size());
	return TTHValue(h.finalize());
}

auto runTransfer(auto& inStream, auto& outStream, const size_t bufSize = 64 * 1024) {
	try {
		while(true) {
			auto innerBufSize = bufSize; // may be modified...
			ByteVector buf(innerBufSize);
			auto newBufSize = inStream.read(&buf[0], innerBufSize);
			if(newBufSize == 0) {
				break;
			}
			outStream.write(&buf[0], newBufSize);
		}
	} catch(const Exception& e) {
		FAIL() << e.getError();
	}
}

TEST(testzutils, test_compression)
{
	// Test a compression using dcpp::ZFilter.

	{
		File f_in("test/gtest.h", File::READ, File::OPEN);
		File f_out("test/data/out/gtest_h_compressed", File::WRITE, File::CREATE | File::TRUNCATE);
		FilteredInputStream<ZFilter, false> stream_in(&f_in);

		runTransfer(stream_in, f_out);
	}

	ASSERT_EQ(tiger("test/data/gtest_h_compressed"), tiger("test/data/out/gtest_h_compressed"));
}

TEST(testzutils, test_decompression)
{
	// Test a decompression using dcpp::UnZFilter.

	{
		File f_in("test/data/gtest_h_compressed", File::READ, File::OPEN);
		File f_out("test/data/out/gtest_h_decompressed", File::WRITE, File::CREATE | File::TRUNCATE);
		FilteredOutputStream<UnZFilter, false> stream_out(&f_out);

		runTransfer(f_in, stream_out);
	}

	ASSERT_EQ(tiger("test/gtest.h"), tiger("test/data/out/gtest_h_decompressed"));
}

TEST(testzutils, test_compression_dynamically_disabled)
{
	// Same as test_compression, but we simulate compression being dynamically disabled during a
	// transfer:
	// - By using a well-compressed file.
	// - By using a ridiculously low buffer size.
	// Would be easier to also be able to tweak ZFilter::MIN_COMPRESSION_LEVEL, but that is
	// undefined behavior which happens to always fail in our case (optimized out by the compiler).
	//
	// This test has been added to resolve <https://bugs.launchpad.net/dcplusplus/+bug/1656050>.

	// Compress (with the buffer size tweak).
	{
		File f_in("test/data/test.dcext", File::READ, File::OPEN);
		File f_out("test/data/out/compression_dynamically_disabled_compressed", File::WRITE, File::CREATE | File::TRUNCATE);
		FilteredInputStream<ZFilter, false> stream_in(&f_in);

		runTransfer(stream_in, f_out, 1024);
	}

	// Decompress.
	{
		File f_in("test/data/out/compression_dynamically_disabled_compressed", File::READ, File::OPEN);
		File f_out("test/data/out/compression_dynamically_disabled_decompressed", File::WRITE, File::CREATE | File::TRUNCATE);
		FilteredOutputStream<UnZFilter, false> stream_out(&f_out);

		runTransfer(f_in, stream_out);
	}

	ASSERT_EQ(tiger("test/data/test.dcext"), tiger("test/data/out/compression_dynamically_disabled_decompressed"));
}
