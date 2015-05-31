#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include "debug/profile.hpp"
//#define TEST_COMPILE_TIME_NEW_META
#ifdef TEST_COMPILE_TIME_NEW_META
#include "Meta/newMeta.hpp"
#include "Meta/Serialization/json.hpp"
#include "Meta/Serialization/simpleBinary.hpp"

using namespace meta;

template<int i>
struct TestStruct : TestStruct<i - 1>
{
	TestStruct(int j = 0)
		: TestStruct<i - 1>(j), int_member(i + j), struct_member(j)
	{
	}

	int int_member;

	TestStruct<i / 2> & GetStructMember()
	{
		return struct_member;
	}

	bool operator==(const TestStruct & other) const
	{
		return int_member == other.int_member && struct_member == other.struct_member && TestStruct<i - 1>::operator==(other);
	}

private:
	TestStruct<i / 2> struct_member;
};

template<>
struct TestStruct<0>
{
	TestStruct(int = 0)
		: last_member(0.5f)
	{
	}

	bool operator==(const TestStruct & other) const
	{
		return last_member == other.last_member;
	}

	float last_member;
};

template<int i>
static MetaType::StructInfo::MemberCollection get_test_struct_members(int16_t)
{
	return
	{
		{
			MetaMember::CreateFromMemPtr<int, TestStruct<i>, &TestStruct<i>::int_member>("int_member_" + std::to_string(i)),
			MetaMember::CreateFromGetRef<TestStruct<i / 2>, TestStruct<i>, &TestStruct<i>::GetStructMember>("struct_member_" + std::to_string(i))
		},
		{
		}
	};
}
template<>
MetaType::StructInfo::MemberCollection get_test_struct_members<0>(int16_t)
{
	return
	{
		{
			MetaMember::CreateFromMemPtr<float, TestStruct<0>, &TestStruct<0>::last_member>("last_member")
		},
		{
		}
	};
}

template<int i>
static MetaType::StructInfo::BaseClassCollection get_test_struct_bases(int16_t)
{
	return
	{
		{
			MetaBaseClass::Create<TestStruct<i - 1>, TestStruct<i> >()
		}
	};
}
template<>
MetaType::StructInfo::BaseClassCollection get_test_struct_bases<0>(int16_t)
{
	return { { } };
}

namespace meta
{
template<int i>
struct MetaType::MetaTypeConstructor<TestStruct<i> >
{
	static const MetaType type;
};
template<int i>
const MetaType MetaType::MetaTypeConstructor<TestStruct<i> >::type = MetaType::RegisterStruct<TestStruct<i> >("TestStruct_" + std::to_string(i), 0, &get_test_struct_members<i>, &get_test_struct_bases<i>);
}

#include <sstream>

TEST(new_meta, TestCompileTime)
{
	typedef TestStruct<128> ChosenStruct;
	std::unique_ptr<ChosenStruct> original(new ChosenStruct(5));
	std::unique_ptr<ChosenStruct> copy;
#define USE_BINARY
#ifdef USE_BINARY
	std::stringstream out;
	{
		ScopedMeasurer write("write");
		write_binary(ConstMetaReference(original), out);
	}
	std::cout << out.str().size() << std::endl;
	MetaReference as_reference(copy);
	{
		ScopedMeasurer read("read");
		read_binary(as_reference, out);
	}
#else
	std::string as_json = to_json(ConstMetaReference(original));
	MetaReference as_reference(copy);
	from_json(as_reference, as_json);
#endif
	ASSERT_TRUE(bool(copy));
	ASSERT_EQ(*original, *copy);
}

#endif
#endif
