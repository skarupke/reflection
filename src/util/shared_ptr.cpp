#include "util/shared_ptr.hpp"

static_assert(sizeof(ptr::small_shared_ptr<int>) == sizeof(void *), "expecting the small_shared_ptr to be the size of just one pointer");

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>

TEST(small_shared_ptr, simple)
{
	struct DestructorCounter
	{
		DestructorCounter(int & count)
			: count(count)
		{
		}
		~DestructorCounter()
		{
			++count;
		}
		int & count;
	};

	int count = 0;
	{
		ptr::small_shared_ptr<DestructorCounter> a = ptr::make_shared<DestructorCounter>(count);
		ptr::small_shared_ptr<DestructorCounter> b = std::move(a);
		ptr::small_shared_ptr<DestructorCounter> c;
		{
			ptr::small_shared_ptr<DestructorCounter> d = b;
			c = d;
		}
        ASSERT_EQ(0, count);
	}
    ASSERT_EQ(1, count);
}

TEST(small_shared_ptr, get)
{
	ptr::small_shared_ptr<int> a;
    ASSERT_FALSE(a.get());
    ASSERT_FALSE(a);
	a = ptr::make_shared<int>(1);
    ASSERT_TRUE(a.get());
    ASSERT_TRUE(bool(a));
}

#endif
