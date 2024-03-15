#include "testbase.h"

#include <dcpp/Archive.h>
#include <dcpp/File.h>
#include <dcpp/HashValue.h>
#include <dcpp/TigerHash.h>

using namespace dcpp;

TEST(testarchive, test_archive)
{
	File::deleteFile("test/data/out/gtest.h");

	try {
		Archive("test/data/gtest_h.zip").extract("test/data/out/");

	} catch(const Exception& e) {
		FAIL() << e.getError();
	}

	auto tiger = [](string path) {
		File f(path, File::READ, File::OPEN);
		TigerHash h;
		auto buf = f.read();
		h.update(buf.c_str(), buf.size());
		return TTHValue(h.finalize());
	};

	ASSERT_EQ(tiger("test/gtest.h"), tiger("test/data/out/gtest.h"));
}
