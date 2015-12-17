#include "util/view.hpp"
#include <iterator>

std::ostream & operator<<(std::ostream & lhs, StringView<const char> rhs)
{
    std::copy(rhs.begin(), rhs.end(), std::ostreambuf_iterator<char>(lhs));
    return lhs;
}

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>

TEST(range, equal)
{
    StringView<const char> a = "hello, world";
    ASSERT_EQ(a, a);
    ASSERT_EQ("hello, world", a);
    ASSERT_NE("hello, world!", a);
    ASSERT_NE("hello world", a);
}

TEST(range, subrange)
{
    StringView<const char> a = "hello, world";
    ASSERT_EQ("hello", a.subview(0, 5));
    ASSERT_EQ("world", a.subview(7, 5));
    ASSERT_EQ("orld", a.subview(8));
    ASSERT_EQ("orl", a.subview(8, 3));
}

TEST(range, startswith)
{
    StringView<const char> a = "hello, world";
    ASSERT_TRUE(a.startswith("hello"));
    ASSERT_FALSE(a.startswith("world"));
    ASSERT_FALSE(a.endswith("hello"));
    ASSERT_TRUE(a.endswith("world"));
}

#include <unordered_map>

TEST(range, as_key)
{
    std::unordered_map<StringView<const char>, int> a;
    a["hi"] = 5;
    a["there"] = 6;
    ASSERT_EQ(5, a["hi"]);
    ASSERT_EQ(6, a["there"]);

    std::string non_const_first = "first";
    std::string non_const_second = "second";
    std::unordered_map<StringView<char>, int> b;
    b[non_const_first] = 7;
    b[non_const_second] = 8;
    ASSERT_EQ(7, b[non_const_first]);
    ASSERT_EQ(8, b[non_const_second]);
}

#endif

