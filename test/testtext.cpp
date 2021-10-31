#include "testbase.h"

#include <dcpp/Text.h>

using namespace dcpp;

TEST(testtext, test_tolower)
{
	Text::initialize();

	ASSERT_EQ("abcd1", Text::toLower("ABCd1"));

#ifdef _WIN32
	ASSERT_EQ(_T("abcd1"), Text::toLower(_T("ABCd1")));
#endif

	ASSERT_EQ('a', Text::toLower('A'));
}
