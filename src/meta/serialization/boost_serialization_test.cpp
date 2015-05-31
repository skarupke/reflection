#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include <sstream>
//#define TEST_BOOST_COMPILE_TIME
#ifdef TEST_BOOST_COMPILE_TIME
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

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

	template<typename Archive>
	void serialize(Archive & ar, unsigned)
	{
		ar & static_cast<TestStruct<i - 1> &>(*this);
		ar & int_member;
		ar & struct_member;
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

	template<typename Archive>
	void serialize(Archive & ar, unsigned)
	{
		ar & last_member;
	}

	float last_member;
};

TEST(boost, compile_time)
{
	std::stringstream str;
	boost::archive::binary_oarchive oa(str);
	typedef TestStruct<96> ChosenStruct;
	std::unique_ptr<ChosenStruct> original(new ChosenStruct(5));
	oa << *original;
	std::unique_ptr<ChosenStruct> copy(new ChosenStruct());
	std::cout << str.str().size() << std::endl;
	boost::archive::binary_iarchive ia(str);
	ia >> *copy;
	ASSERT_EQ(*original, *copy);
}

#endif
#endif
