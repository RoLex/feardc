#include "testbase.h"

#include <dcpp/atomic.h>

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/stack.hpp>

using namespace dcpp;

TEST(testlockfree, test_atomic)
{
	ASSERT_TRUE(atomic<bool>().is_lock_free());
	ASSERT_TRUE(atomic<int32_t>().is_lock_free());
	ASSERT_TRUE(atomic<uint32_t>().is_lock_free());
	ASSERT_TRUE(atomic<int64_t>().is_lock_free());
	ASSERT_TRUE(atomic<uint64_t>().is_lock_free());
}

TEST(testlockfree, test_queue)
{
	boost::lockfree::queue<int> container(128);
	ASSERT_TRUE(container.is_lock_free());
}

TEST(testlockfree, test_spsc_queue)
{
	boost::lockfree::spsc_queue<int> container(128);
	ASSERT_TRUE(container.is_lock_free());
}

TEST(testlockfree, test_stack)
{
	boost::lockfree::stack<int> container(128);
	ASSERT_TRUE(container.is_lock_free());
}
