#include "metafast/metafast.hpp"
#include <unordered_map>

namespace metaf
{
namespace detail
{
static std::unordered_map<const metav3::MetaType *, std::pair<void (*)(BinaryOutput &, metav3::ConstMetaReference), void (*)(BinaryInput &, metav3::MetaReference)> > & registered_functions_by_type()
{
    static std::unordered_map<const metav3::MetaType *, std::pair<void (*)(BinaryOutput &, metav3::ConstMetaReference), void (*)(BinaryInput &, metav3::MetaReference)> > result;
    return result;
}

void register_type_erased_functions(const metav3::MetaType & type, void (*serialize)(BinaryOutput &, metav3::ConstMetaReference), void (*deserialize)(BinaryInput &, metav3::MetaReference))
{
    registered_functions_by_type()[&type] = { serialize, deserialize };
}
std::pair<void (*)(BinaryOutput &, metav3::ConstMetaReference), void (*)(BinaryInput &, metav3::MetaReference)> get_type_erased_functions(const metav3::MetaType & type)
{
    return registered_functions_by_type()[&type];
}
void serialize_struct(BinaryOutput & output, metav3::ConstMetaReference ref)
{
    get_type_erased_functions(ref.GetType()).first(output, ref);
}
void deserialize_struct(BinaryInput & input, metav3::MetaReference ref)
{
    get_type_erased_functions(ref.GetType()).second(input, ref);
}
}
}

#ifndef DISABLE_TESTS
#include <gtest/gtest.h>
#include <vector>
#include "metav3/metav3_stl.hpp"

struct HasMembers
{
    int i = 0;
    float f = 0.0f;
    bool b = false;
};

REFLECT_CLASS_START(HasMembers, 0)
    REFLECT_MEMBER(i);
    REFLECT_MEMBER(f);
    REFLECT_MEMBER(b);
REFLECT_CLASS_END()

template<typename T>
ArrayView<const unsigned char> as_bytes(const T & object)
{
    auto begin = reinterpret_cast<const unsigned char *>(std::addressof(object));
    return { begin, begin + sizeof(T) };
}

metaf::BinaryInput StringToBinaryInput(const std::string & input)
{
    auto begin = reinterpret_cast<const unsigned char *>(input.data());
    return { { begin, begin + input.size() } };
}

struct InMemoryBinaryInput
{
    InMemoryBinaryInput(std::string data)
        : data(std::move(data))
        , input(StringToBinaryInput(this->data))
    {
    }

    operator metaf::BinaryInput &()
    {
        return input;
    }

    std::string data;
    metaf::BinaryInput input;
};

template<typename T>
InMemoryBinaryInput serialize_to_buffer(const T & value)
{
    std::stringstream buffer;
    metaf::BinaryOutput output(buffer);
    metaf::write_binary(output, value);
    return { buffer.str() };
}

template<typename T>
T roundtrip(const T & value)
{
    auto as_input = serialize_to_buffer(value);
    T result;
    metaf::read_binary(as_input, result);
    return result;
}

template<typename T>
::testing::AssertionResult TestRoundtrip(const T & value)
{
    T second = roundtrip(value);
    if (value == second)
        return ::testing::AssertionSuccess();
    else
        return ::testing::AssertionFailure();
}
template<typename T>
::testing::AssertionResult TestRoundtrip(std::initializer_list<T> values)
{
    for (const T & value : values)
    {
        ::testing::AssertionResult result = TestRoundtrip(value);
        if (!result)
            return result;
    }
    return ::testing::AssertionSuccess();
}

#define ASSERT_ROUNDTRIP(...) ASSERT_TRUE(TestRoundtrip(__VA_ARGS__))

#ifdef STORE_AS_FLOAT8_IF_POSSIBLE
metaf::detail::FloatComponents compression_roundtrip(metaf::detail::FloatComponents input)
{
    return metaf::detail::FloatComponents(metaf::detail::FloatComponentsCompressed(input));
}

::testing::AssertionResult TestCompressedFloat(float f)
{
    metaf::detail::FloatComponents a(f);
    if (!a.can_be_compressed())
        return ::testing::AssertionFailure();
    if (a.f != compression_roundtrip(a).f)
        return ::testing::AssertionFailure();
    return testing::AssertionSuccess();
}

TEST(metafast, float_compressed)
{
    ASSERT_TRUE(TestCompressedFloat(0.0f));
    ASSERT_TRUE(TestCompressedFloat(1.0f));
    ASSERT_TRUE(TestCompressedFloat(2.0f));
    ASSERT_TRUE(TestCompressedFloat(3.0f));
    ASSERT_TRUE(TestCompressedFloat(4.0f));
    ASSERT_TRUE(TestCompressedFloat(0.125f));
    ASSERT_TRUE(TestCompressedFloat(0.25f));
    ASSERT_TRUE(TestCompressedFloat(0.5f));
    ASSERT_TRUE(TestCompressedFloat(0.75f));
    ASSERT_TRUE(TestCompressedFloat(-0.0f));
    ASSERT_TRUE(TestCompressedFloat(-1.0f));
    ASSERT_TRUE(TestCompressedFloat(-2.0f));
    ASSERT_TRUE(TestCompressedFloat(-3.0f));
    ASSERT_TRUE(TestCompressedFloat(-4.0f));
    ASSERT_TRUE(TestCompressedFloat(-0.125f));
    ASSERT_TRUE(TestCompressedFloat(-0.25f));
    ASSERT_TRUE(TestCompressedFloat(-0.5f));
    ASSERT_TRUE(TestCompressedFloat(-0.75f));
}
TEST(metafast, float_compressed_denormalized)
{
    metaf::detail::FloatComponents denormalized(0.0f);
    ++denormalized.mantissa;
    ASSERT_FALSE(denormalized.can_be_compressed());
    ASSERT_TRUE(denormalized.is_denormal());
    ASSERT_ROUNDTRIP(denormalized.f);
}

#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
TEST(metafast, float_compressed_special_values)
{
    metaf::detail::FloatComponents qnan(std::numeric_limits<float>::quiet_NaN());
    ASSERT_TRUE(qnan.is_nan_or_infinity());
    ASSERT_FALSE(qnan.is_infinity());
    ASSERT_TRUE(qnan.quiet_nan_bit);
    metaf::detail::FloatComponents qnan_roundtrip = compression_roundtrip(qnan);
    ASSERT_TRUE(qnan_roundtrip.is_nan_or_infinity());
    ASSERT_FALSE(qnan_roundtrip.is_infinity());
    ASSERT_TRUE(qnan_roundtrip.quiet_nan_bit);
    metaf::detail::FloatComponents snan(std::numeric_limits<float>::signaling_NaN());
    ASSERT_TRUE(snan.is_nan_or_infinity());
    ASSERT_FALSE(snan.is_infinity());
    ASSERT_FALSE(snan.quiet_nan_bit);
    metaf::detail::FloatComponents snan_roundtrip = compression_roundtrip(snan);
    ASSERT_TRUE(snan_roundtrip.is_nan_or_infinity());
    ASSERT_FALSE(snan_roundtrip.is_infinity());
    ASSERT_FALSE(snan_roundtrip.quiet_nan_bit);
    metaf::detail::FloatComponents infinity(std::numeric_limits<float>::infinity());
    ASSERT_TRUE(infinity.is_nan_or_infinity());
    ASSERT_TRUE(infinity.is_infinity());
    ASSERT_TRUE(TestCompressedFloat(std::numeric_limits<float>::infinity()));
    metaf::detail::FloatComponents negative_infinity(-std::numeric_limits<float>::infinity());
    ASSERT_TRUE(negative_infinity.is_nan_or_infinity());
    ASSERT_TRUE(negative_infinity.is_infinity());
    ASSERT_TRUE(TestCompressedFloat(-std::numeric_limits<float>::infinity()));
}
#endif
TEST(metafast, DISABLED_all_compressed_floats)
{
    metaf::detail::FloatComponentsCompressed compressed(metaf::detail::FloatComponents(0.0f));
    std::cout << metaf::detail::FloatComponents(compressed).f << '\n';
    for (++compressed.exponent; compressed.exponent; ++compressed.exponent)
    {
        do
        {
            std::cout << metaf::detail::FloatComponents(compressed).f << '\n';
            ++compressed.mantissa;
        }
        while(compressed.mantissa);
    }
}

#endif

TEST(metafast, simple)
{
    HasMembers foo;
    foo.i = 5;
    foo.f = 10.0f;
    foo.b = true;
    HasMembers copy = roundtrip(foo);
    ASSERT_EQ(foo.i, copy.i);
    ASSERT_EQ(foo.f, copy.f);
    ASSERT_EQ(foo.b, copy.b);
}

#ifdef COMPRESS_INT
template<typename T>
::testing::AssertionResult SerializesToSize(const T & value, size_t size)
{
    auto serialized = serialize_to_buffer(value);
    if (serialized.data.size() == size)
        return TestRoundtrip(value);
    else
        return ::testing::AssertionFailure();
}

TEST(metafast, compress_int)
{
    ASSERT_TRUE(SerializesToSize(-1000, 2));
    ASSERT_TRUE(SerializesToSize(-10, 1));
    ASSERT_TRUE(SerializesToSize(-1, 1));
    ASSERT_TRUE(SerializesToSize(0, 1));
    ASSERT_TRUE(SerializesToSize(1, 1));
    ASSERT_TRUE(SerializesToSize(10, 1));
    ASSERT_TRUE(SerializesToSize(1000, 2));
    ASSERT_TRUE(SerializesToSize(-55555, 3));
    ASSERT_TRUE(SerializesToSize(55555, 3));
    ASSERT_TRUE(SerializesToSize(0u, 1));
    ASSERT_TRUE(SerializesToSize(1u, 1));
    ASSERT_TRUE(SerializesToSize(10u, 1));
    ASSERT_TRUE(SerializesToSize(1000u, 2));
    ASSERT_TRUE(SerializesToSize(55555u, 3));
    ASSERT_TRUE(SerializesToSize(-1000ll, 2));
    ASSERT_TRUE(SerializesToSize(-10ll, 1));
    ASSERT_TRUE(SerializesToSize(-1ll, 1));
    ASSERT_TRUE(SerializesToSize(0ll, 1));
    ASSERT_TRUE(SerializesToSize(1ll, 1));
    ASSERT_TRUE(SerializesToSize(10ll, 1));
    ASSERT_TRUE(SerializesToSize(1000ll, 2));
    ASSERT_TRUE(SerializesToSize(-55555ll, 3));
    ASSERT_TRUE(SerializesToSize(55555ll, 3));
    ASSERT_TRUE(SerializesToSize(0ull, 1));
    ASSERT_TRUE(SerializesToSize(1ull, 1));
    ASSERT_TRUE(SerializesToSize(10ull, 1));
    ASSERT_TRUE(SerializesToSize(1000ull, 2));
    ASSERT_TRUE(SerializesToSize(55555ull, 3));
}

#endif

struct StructWithVector
{
    std::vector<float> vec;
};
REFLECT_CLASS_START(StructWithVector, 0)
    REFLECT_MEMBER(vec);
REFLECT_CLASS_END()

TEST(metafast, vector)
{
    StructWithVector to_deserialize = { { 1.0f, 2.0f, 3.0f } };
    StructWithVector copy = roundtrip(to_deserialize);
    ASSERT_EQ(to_deserialize.vec, copy.vec);
}

struct Base
{
    Base()
    {
    }

    Base(int foo)
        : foo(foo)
    {
    }

    FRIEND_REFLECT_CLASSES(Base);

    int GetFoo() const
    {
        return foo;
    }
private:
    int foo = 0;
};
struct Derived : Base
{
    Derived()
    {
    }

    Derived(int foo, int bar)
        : Base(foo), bar(bar)
    {
    }
    FRIEND_REFLECT_CLASSES(Derived);

    int GetBar() const
    {
        return bar;
    }
private:
    int bar = 0;
};

REFLECT_CLASS_START(Base, 0)
    REFLECT_MEMBER(foo);
REFLECT_CLASS_END()
REFLECT_CLASS_START(Derived, 0)
    REFLECT_BASE(Base);
    REFLECT_MEMBER(bar);
REFLECT_CLASS_END()

TEST(metafast, base)
{
    Derived a = { 5, 6 };
    Derived b = roundtrip(a);
    ASSERT_EQ(5, b.GetFoo());
    ASSERT_EQ(6, b.GetBar());
}
TEST(metafast, roundtrip_float)
{
    float a = 65791;
    ASSERT_ROUNDTRIP(a);

    a = 508;
    ASSERT_ROUNDTRIP(a);
}

struct StructWithDefaults
{
    int a = 5;
    std::vector<float> b = { 1.0f, 2.0f, 3.0f };
    int c[3] = { 4, 5, 6 };

    bool operator==(const StructWithDefaults & other) const
    {
        return a == other.a && b == other.b && std::equal(c, c + 3, other.c);
    }
    bool operator!=(const StructWithDefaults & other) const
    {
        return !(*this == other);
    }
};

REFLECT_CLASS_START(StructWithDefaults, 0)
    REFLECT_MEMBER(a);
    REFLECT_MEMBER(b);
    REFLECT_MEMBER(c);
REFLECT_CLASS_END()

TEST(metafast, defaults)
{
    StructWithDefaults a;
    ASSERT_ROUNDTRIP(a);

    ++a.a;
    ASSERT_ROUNDTRIP(a);
    --a.a;

    a.b.push_back(5.0f);
    ASSERT_ROUNDTRIP(a);
    a.b.pop_back();

    --a.c[2];
    ASSERT_ROUNDTRIP(a);

    a.b.push_back(6.0f);
    ASSERT_ROUNDTRIP(a);
}

struct DerivedWithDefaults : StructWithDefaults
{
    bool d = true;

    bool operator==(const DerivedWithDefaults & other) const
    {
        return d == other.d && StructWithDefaults::operator==(other);
    }
    bool operator!=(const DerivedWithDefaults & other) const
    {
        return !(*this == other);
    }
};

REFLECT_CLASS_START(DerivedWithDefaults, 0)
    REFLECT_BASE(StructWithDefaults);
    REFLECT_MEMBER(d);
REFLECT_CLASS_END()

TEST(metafast, default_base)
{
    DerivedWithDefaults a;
    ASSERT_ROUNDTRIP(a);

    a.d = false;
    ASSERT_ROUNDTRIP(a);

    a.d = true;
    a.b.push_back(5.0f);
    ASSERT_ROUNDTRIP(a);

    a.d = false;
    ASSERT_ROUNDTRIP(a);
}

TEST(metafast, two_defaults)
{
    std::vector<DerivedWithDefaults> a(2);
    a[1].b.push_back(6.0f);

    ASSERT_ROUNDTRIP(a);
}

struct SixtyFourMembers
{
    SixtyFourMembers()
    {
        std::fill(all64, all64 + 64, 0);
    }

    union
    {
        struct
        {
            int i0;
            int i1;
            int i2;
            int i3;
            int i4;
            int i5;
            int i6;
            int i7;
            int i8;
            int i9;
            int i10;
            int i11;
            int i12;
            int i13;
            int i14;
            int i15;
            int i16;
            int i17;
            int i18;
            int i19;
            int i20;
            int i21;
            int i22;
            int i23;
            int i24;
            int i25;
            int i26;
            int i27;
            int i28;
            int i29;
            int i30;
            int i31;
            int i32;
            int i33;
            int i34;
            int i35;
            int i36;
            int i37;
            int i38;
            int i39;
            int i40;
            int i41;
            int i42;
            int i43;
            int i44;
            int i45;
            int i46;
            int i47;
            int i48;
            int i49;
            int i50;
            int i51;
            int i52;
            int i53;
            int i54;
            int i55;
            int i56;
            int i57;
            int i58;
            int i59;
            int i60;
            int i61;
            int i62;
            int i63;
        };
        int all64[64];
    };

    bool operator==(const SixtyFourMembers & other) const
    {
        return std::equal(all64, all64 + 64, other.all64);
    }
    bool operator!=(const SixtyFourMembers & other) const
    {
        return !(*this == other);
    }
};

REFLECT_CLASS_START(SixtyFourMembers, 0)
    REFLECT_MEMBER(i0);
    REFLECT_MEMBER(i1);
    REFLECT_MEMBER(i2);
    REFLECT_MEMBER(i3);
    REFLECT_MEMBER(i4);
    REFLECT_MEMBER(i5);
    REFLECT_MEMBER(i6);
    REFLECT_MEMBER(i7);
    REFLECT_MEMBER(i8);
    REFLECT_MEMBER(i9);
    REFLECT_MEMBER(i10);
    REFLECT_MEMBER(i11);
    REFLECT_MEMBER(i12);
    REFLECT_MEMBER(i13);
    REFLECT_MEMBER(i14);
    REFLECT_MEMBER(i15);
    REFLECT_MEMBER(i16);
    REFLECT_MEMBER(i17);
    REFLECT_MEMBER(i18);
    REFLECT_MEMBER(i19);
    REFLECT_MEMBER(i20);
    REFLECT_MEMBER(i21);
    REFLECT_MEMBER(i22);
    REFLECT_MEMBER(i23);
    REFLECT_MEMBER(i24);
    REFLECT_MEMBER(i25);
    REFLECT_MEMBER(i26);
    REFLECT_MEMBER(i27);
    REFLECT_MEMBER(i28);
    REFLECT_MEMBER(i29);
    REFLECT_MEMBER(i30);
    REFLECT_MEMBER(i31);
    REFLECT_MEMBER(i32);
    REFLECT_MEMBER(i33);
    REFLECT_MEMBER(i34);
    REFLECT_MEMBER(i35);
    REFLECT_MEMBER(i36);
    REFLECT_MEMBER(i37);
    REFLECT_MEMBER(i38);
    REFLECT_MEMBER(i39);
    REFLECT_MEMBER(i40);
    REFLECT_MEMBER(i41);
    REFLECT_MEMBER(i42);
    REFLECT_MEMBER(i43);
    REFLECT_MEMBER(i44);
    REFLECT_MEMBER(i45);
    REFLECT_MEMBER(i46);
    REFLECT_MEMBER(i47);
    REFLECT_MEMBER(i48);
    REFLECT_MEMBER(i49);
    REFLECT_MEMBER(i50);
    REFLECT_MEMBER(i51);
    REFLECT_MEMBER(i52);
    REFLECT_MEMBER(i53);
    REFLECT_MEMBER(i54);
    REFLECT_MEMBER(i55);
    REFLECT_MEMBER(i56);
    REFLECT_MEMBER(i57);
    REFLECT_MEMBER(i58);
    REFLECT_MEMBER(i59);
    REFLECT_MEMBER(i60);
    REFLECT_MEMBER(i61);
    REFLECT_MEMBER(i62);
    REFLECT_MEMBER(i63);
REFLECT_CLASS_END()

TEST(metafast, sixty_four_members)
{
    SixtyFourMembers a;
    a.i31 = 2;
    a.i63 = 1;
    ASSERT_ROUNDTRIP(a);
}

TEST(metafast, all_the_int_types)
{
    ASSERT_ROUNDTRIP(static_cast<signed char>(1));
    ASSERT_ROUNDTRIP(static_cast<unsigned char>(2));

    ASSERT_ROUNDTRIP(static_cast<signed short>(3));
    ASSERT_ROUNDTRIP(static_cast<short>(4));
    ASSERT_ROUNDTRIP(static_cast<unsigned short>(5));

    ASSERT_ROUNDTRIP(static_cast<signed>(6));
    ASSERT_ROUNDTRIP(static_cast<int>(7));
    ASSERT_ROUNDTRIP(static_cast<unsigned>(8));

    ASSERT_ROUNDTRIP(static_cast<signed long>(9));
    ASSERT_ROUNDTRIP(static_cast<long>(10));
    ASSERT_ROUNDTRIP(static_cast<unsigned long>(11));

    ASSERT_ROUNDTRIP(static_cast<signed long long>(12));
    ASSERT_ROUNDTRIP(static_cast<long long>(13));
    ASSERT_ROUNDTRIP(static_cast<unsigned long long>(14));
}

TEST(metafast, all_the_char_types)
{
    ASSERT_ROUNDTRIP('a');
    ASSERT_ROUNDTRIP(L'b');
    ASSERT_ROUNDTRIP(u'c');
    ASSERT_ROUNDTRIP(U'd');
}

TEST(metafast, all_the_string_types)
{
    ASSERT_ROUNDTRIP(std::string("hi there"));
    ASSERT_ROUNDTRIP(std::wstring(L"hullo"));
    ASSERT_ROUNDTRIP(std::u16string(u"ahoy"));
    ASSERT_ROUNDTRIP(std::u32string(U"and hi"));
}

TEST(metafast, uint32)
{
    ASSERT_ROUNDTRIP({ std::numeric_limits<uint32_t>::max(),
                       std::numeric_limits<uint32_t>::max() - 1,
                       std::numeric_limits<uint32_t>::max() - 128,
                       std::numeric_limits<uint32_t>::max() - (1u << 31) });
}
TEST(metafast, int32)
{
    ASSERT_ROUNDTRIP({ 13435,
                       -13435,
                       55555,
                       -55555,
                       std::numeric_limits<int32_t>::max(),
                       std::numeric_limits<int32_t>::max() - 1,
                       std::numeric_limits<int32_t>::max() - 128,
                       std::numeric_limits<int32_t>::max() - (1 << 30),
                       std::numeric_limits<int32_t>::lowest(),
                       std::numeric_limits<int32_t>::lowest() + 1,
                       std::numeric_limits<int32_t>::lowest() + 128,
                       std::numeric_limits<int32_t>::lowest() + (1 << 30),
                       -1 });
}

TEST(metafast, uint64)
{
    ASSERT_ROUNDTRIP({ std::numeric_limits<uint64_t>::max(),
                       std::numeric_limits<uint64_t>::max() - 1,
                       std::numeric_limits<uint64_t>::max() - 128,
                       std::numeric_limits<uint64_t>::max() - (uint64_t(1) << 63) });
}

TEST(metafast, int64)
{
    ASSERT_ROUNDTRIP({ int64_t(3412360507562),
                       int64_t(-3412360507562),
                       int64_t(94),
                       int64_t(-94),
                       int64_t(1469481),
                       int64_t(-1469481),
                       int64_t(55555),
                       int64_t(-55555),
                       std::numeric_limits<int64_t>::max(),
                       std::numeric_limits<int64_t>::max() - 1,
                       std::numeric_limits<int64_t>::max() - 128,
                       std::numeric_limits<int64_t>::max() - (1 << 30),
                       std::numeric_limits<int64_t>::lowest(),
                       std::numeric_limits<int64_t>::lowest() + 1,
                       std::numeric_limits<int64_t>::lowest() + 128,
                       std::numeric_limits<int64_t>::lowest() + (1 << 30),
                       int64_t(-1) });
}

#include <thread>
#include <random>

TEST(metafast, DISABLED_many_ints)
{
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 8; ++i)
    {
        threads.emplace_back([i]
        {
            std::mt19937_64 generator(i + time(nullptr));
            std::uniform_int_distribution<uint64_t> distribution;
            for (int i = 0; i < 100000; ++i)
            {
                uint64_t to_check = distribution(generator);
                uint64_t b = roundtrip(to_check);
                if (to_check != b)
                {
                    std::cout << to_check << '\n';
                    ASSERT_TRUE(false);
                }
                int64_t as_int(to_check);
                int64_t c = roundtrip(as_int);
                if (as_int != c)
                {
                    std::cout << to_check << '\n';
                    ASSERT_TRUE(false);
                }
                as_int = -as_int;
                c = roundtrip(as_int);
                if (as_int != c)
                {
                    std::cout << to_check << '\n';
                    ASSERT_TRUE(false);
                }
            }
        });
    }
    for (std::thread & thread : threads)
        thread.join();
}

#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

TEST(metafast, stl_containers)
{
    ASSERT_ROUNDTRIP(std::array<int, 4>{{ 1, 2, 3, 4 }});
    ASSERT_ROUNDTRIP(std::deque<int>{ 1, 2, 3 });
    ASSERT_ROUNDTRIP(std::list<int>{ 4, 5, 6 });
    ASSERT_ROUNDTRIP(std::forward_list<int>{ 7, 8, 9 });
    ASSERT_ROUNDTRIP(std::set<int>{ 7, 8, 14 });
    ASSERT_ROUNDTRIP(std::multiset<int>{ 7, 8, 9, 7, 10 });
    ASSERT_ROUNDTRIP(std::unordered_set<int>{ 7, 8, 13 });
    ASSERT_ROUNDTRIP(std::unordered_multiset<int>{ 7, 8, 9, 11, 8 });
    ASSERT_ROUNDTRIP(std::map<int, int>{ { 7, 5 }, { 8, 0 }, { 14, 4 } });
    ASSERT_ROUNDTRIP(std::multimap<int, int>{ { 7, 4 }, { 8, 2 }, { 9, 5 }, { 7, 1243 }, { 10, 453434 } });
    ASSERT_ROUNDTRIP(std::unordered_map<int, int>{ { 7, 13435 }, { 8, 0 }, { 13, -134245 } });
    ASSERT_ROUNDTRIP(std::unordered_multimap<int, int>{ { 7, 145 }, { 8, 245 }, { 9, 3 }, { 11, 2 }, { 8, 1 } });
}

enum MetaFastTest
{
    EnumA,
    EnumB,
    EnumC
};

TEST(metafast, enum)
{
    ASSERT_ROUNDTRIP(EnumC);
}

struct VirtualBase
{
    VirtualBase() = default;
    VirtualBase(int a)
        : a(a)
    {
    }

    virtual ~VirtualBase()
    {
    }
    int a = 5;
};
REFLECT_CLASS_START(VirtualBase, 0)
    REFLECT_MEMBER(a);
REFLECT_CLASS_END()
struct VirtualDerived : VirtualBase
{
    VirtualDerived() = default;
    VirtualDerived(int a, int b)
        : VirtualBase(a), b(b)
    {
    }

    int b = 6;

    bool operator==(const VirtualDerived & other) const
    {
        return a == other.a && b == other.b;
    }
    bool operator!=(const VirtualDerived & other) const
    {
        return !(*this == other);
    }
};
REFLECT_CLASS_START(VirtualDerived, 0)
    REFLECT_BASE(VirtualBase);
    REFLECT_MEMBER(b);
REFLECT_CLASS_END()

TEST(metafast, pointer_to_base)
{
    std::unique_ptr<VirtualBase> empty;
    std::unique_ptr<VirtualBase> b = roundtrip(empty);
    ASSERT_FALSE(bool(b));
    VirtualDerived * a_derived = new VirtualDerived(10, 20);
    std::unique_ptr<VirtualBase> a(a_derived);
    b = roundtrip(a);
    ASSERT_TRUE(bool(b));
    VirtualDerived * b_derived = dynamic_cast<VirtualDerived *>(b.get());
    ASSERT_TRUE(b_derived);
    ASSERT_EQ(*a_derived, *b_derived);
}



struct TestBinarySerializer
{
    template<typename T>
    std::string serialize(const T & object) const
    {
        std::stringstream result;
        metaf::BinaryOutput output(result);
        metaf::write_binary(output, object);
        return result.str();
    }
    template<typename T>
    bool deserialize(T & object, const std::string & str) const
    {
        metaf::BinaryInput input({ reinterpret_cast<const metaf::byte *>(str.data()), reinterpret_cast<const metaf::byte *>(str.data() + str.size()) });
        metaf::read_binary(input, object);
        return true;
    }
};

#include "metav3/serialization/serialization_test.hpp"
INSTANTIATE_TYPED_TEST_CASE_P(fast_optimistic_binary, SerializationTest, TestBinarySerializer);

#endif
