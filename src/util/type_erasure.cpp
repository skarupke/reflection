#include "type_erasure.hpp"

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>
#include <memory>
namespace
{
TEST(type_erasure, moveable)
{
    BaseTypeErasure<8, MoveVTable> movable(std::unique_ptr<int>(new int(5)));
    ASSERT_EQ(typeid(std::unique_ptr<int>), movable.target_type());
    ASSERT_EQ(5, **movable.target<std::unique_ptr<int>>());
}

TEST(type_erasure, copyable)
{
    BaseTypeErasure<8, CopyVTable> a(5);
    ASSERT_EQ(typeid(int), a.target_type());
    ASSERT_EQ(5, *a.target<int>());
    a = 3.3f;
    ASSERT_EQ(typeid(float), a.target_type());
    ASSERT_EQ(3.3f, *a.target<float>());
    a = a;
    ASSERT_EQ(typeid(float), a.target_type());
    ASSERT_EQ(3.3f, *a.target<float>());
    const BaseTypeErasure<8, CopyVTable> b = a;
    ASSERT_EQ(typeid(float), b.target_type());
    ASSERT_EQ(3.3f, *b.target<float>());
}

struct CtorDtorCounter
{
    CtorDtorCounter(int & ctor_count, int & dtor_count)
        : ctor_count(ctor_count), dtor_count(dtor_count)
    {
        ++ctor_count;
    }
    CtorDtorCounter(const CtorDtorCounter & other)
        : ctor_count(other.ctor_count), dtor_count(other.dtor_count)
    {
        ++ctor_count;
    }
    ~CtorDtorCounter()
    {
        ++dtor_count;
    }

    int & ctor_count;
    int & dtor_count;
};

TEST(type_erasure, copy_assign)
{
    int ctor_count = 0;
    int dtor_count = 0;
    {
        BaseTypeErasure<8, CopyVTable> a(CtorDtorCounter(ctor_count, dtor_count));
        BaseTypeErasure<8, CopyVTable> b(CtorDtorCounter(ctor_count, dtor_count));
        a = b;
    }
    ASSERT_EQ(ctor_count, dtor_count);
}

struct LargerEnoughToStompTheStack
{
    LargerEnoughToStompTheStack(int value)
    {
        for (auto it = std::begin(a); it != std::end(a); ++it)
        {
            *it = value + int(it - std::begin(a));
        }
    }

    int a[8192];

    bool operator==(const LargerEnoughToStompTheStack & other) const
    {
        return std::equal(std::begin(a), std::end(a), std::begin(other.a));
    }
};

TEST(type_erasure, heap_allocated)
{
    static_assert(sizeof(LargerEnoughToStompTheStack) > sizeof(BaseTypeErasure<8, CopyVTable>), "has to be heap allocated for this test");
    BaseTypeErasure<8, CopyVTable> a = LargerEnoughToStompTheStack(5);
    ASSERT_EQ(typeid(LargerEnoughToStompTheStack), a.target_type());
    ASSERT_EQ(LargerEnoughToStompTheStack(5), *a.target<LargerEnoughToStompTheStack>());
    a = LargerEnoughToStompTheStack(6);
    ASSERT_EQ(typeid(LargerEnoughToStompTheStack), a.target_type());
    ASSERT_EQ(LargerEnoughToStompTheStack(6), *a.target<LargerEnoughToStompTheStack>());
    a = 5;
    ASSERT_EQ(typeid(int), a.target_type());
    ASSERT_EQ(5, *a.target<int>());
}

template<size_t Size, size_t Alignment, typename Allocator>
struct RegularCopyVtable : RegularVTable<Size, Alignment, Allocator>, CopyVTable<Size, Alignment, Allocator>
{
    template<typename T>
    RegularCopyVtable(T *)
        : RegularVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , CopyVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
    {
    }
};

TEST(type_erasure, comparisons)
{
    typedef RegularTypeErasure<sizeof(void *), RegularCopyVtable> Regular;
    ASSERT_LT(Regular(5), Regular(6));
    ASSERT_GT(Regular(5), Regular(4));
    ASSERT_EQ(Regular(5), Regular(5));
    ASSERT_NE(Regular(6), Regular(5));
}

template<typename Func>
using MovableFunction = CallableTypeErasure<Func, sizeof(void *) * 2, BaseTypeErasure, MoveVTable>;
static_assert(sizeof(MovableFunction<void ()>) == sizeof(void *) * 4, "this should only have a vtable pointer, a call pointer and my 16 bytes of storage");

TEST(type_erasure, callable_no_argument)
{
    int a = 1;
    MovableFunction<void ()> no_arg = [&a]{ a = 5; };
    no_arg();
    ASSERT_EQ(5, a);
}
TEST(type_erasure, callable_with_argument)
{
    int a = 1;

    MovableFunction<bool (int)> with_arg = [&a](int value)
    {
        return a++ == value;
    };
    ASSERT_TRUE(with_arg(1));
    ASSERT_EQ(2, a);
}
TEST(type_erasure, callable_mutable)
{
    MovableFunction<int ()> with_state = [a = 5]() mutable
    {
        return a++;
    };
    ASSERT_EQ(5, with_state());
    ASSERT_EQ(6, with_state());
}
}
#endif

