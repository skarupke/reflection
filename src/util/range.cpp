#include "util/range.hpp"
#include <iterator>

std::ostream & operator<<(std::ostream & lhs, Range<const char> rhs)
{
    std::copy(rhs.begin(), rhs.end(), std::ostreambuf_iterator<char>(lhs));
    return lhs;
}

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>

TEST(range, subrange)
{
    Range<const char> a = "hello, world";
    ASSERT_EQ("hello", a.subrange(0, 5));
    ASSERT_EQ("world", a.subrange(7, 5));
    ASSERT_EQ("orld", a.subrange(8));
    ASSERT_EQ("orl", a.subrange(8, 3));
}

TEST(range, startswith)
{
    Range<const char> a = "hello, world";
    ASSERT_TRUE(a.startswith("hello"));
    ASSERT_FALSE(a.startswith("world"));
    ASSERT_FALSE(a.endswith("hello"));
    ASSERT_TRUE(a.endswith("world"));
}

#endif

