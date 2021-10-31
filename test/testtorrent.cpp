#include "testbase.h"

#include <dcpp/Torrent.h>
#include <unordered_map>

using namespace dcpp;

const char* ubuntu = "d8:announce39:http://torrent.ubuntu.com:6969/announce13:announce-listll39:http://torrent.ubuntu.com:6969/announceel44:http://ipv6.torrent.ubuntu.com:6969/announceee7:comment29:Ubuntu CD releases.ubuntu.com13:creation datei1286702721e4:infod6:lengthi728754176e4:name30:ubuntu-10.10-desktop-amd64.iso12:piece lengthi524288eee";

TEST(testtorrent, test_single )
{
	Torrent u(ubuntu);

	ASSERT_EQ(1, u.getFiles().size());
	ASSERT_EQ(728754176, u.getFiles()[0].length);
	ASSERT_EQ(1, u.getFiles()[0].path.size());
	ASSERT_EQ("ubuntu-10.10-desktop-amd64.iso", u.getFiles()[0].path[0]);
}
