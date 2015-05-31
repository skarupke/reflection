#include "pointer.hpp"


#ifndef DISABLE_GTEST
#include <gtest/gtest.h>

TEST(copying_ptr, simple)
{
	copying_ptr<int> a(forwarding_constructor{}, 5);
	copying_ptr<int> b = a;
	ASSERT_EQ(*a, *b);
	ASSERT_EQ(a, b);
}

namespace
{
struct clonable
{
	clonable(int a)
		: a(a)
	{
	}

	std::unique_ptr<clonable> clone() const
	{
		return std::unique_ptr<clonable>(new clonable(*this));
	}

	int a = 5;

	bool operator==(const clonable & other) const
	{
		return a == other.a;
	}
};
}

TEST(cloning_ptr, simple)
{
	cloning_ptr<clonable> a(forwarding_constructor{}, 10);
	cloning_ptr<clonable> b = a;
	ASSERT_EQ(*a, *b);
	ASSERT_EQ(a, b);
}

TEST(dunique_ptr, double_delete)
{
	struct self_referential
	{
		self_referential(dunique_ptr<self_referential> & self, int & destructor_count)
			: self(self), destructor_count(destructor_count)
		{
		}
		~self_referential()
		{
			++destructor_count;
			self.reset();
		}

	private:
		dunique_ptr<self_referential> & self;
		int & destructor_count;
	};
	int destructor_count = 0;
	{
		dunique_ptr<self_referential> a;
		a.reset(new self_referential(a, destructor_count));
	}
	ASSERT_EQ(1, destructor_count);
	{
		dunique_ptr<self_referential> a;
		a.reset(new self_referential(a, destructor_count));
		a.reset();
	}
	ASSERT_EQ(2, destructor_count);
}

#endif

