#include "memoizingMap.hpp"


#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <exception>
#include <future>

namespace
{
struct test_exception
{
};

struct throw_exception_once
{
	throw_exception_once()
	{
        if (!threw.exchange(true))
		{
			throw test_exception();
		}
	}

private:
	static std::atomic<bool> threw;
};
std::atomic<bool> throw_exception_once::threw(false);
}

// call_once doesn't work...
TEST(call_once, DISABLED_exception)
{
    std::once_flag once;
    try
    {
        std::call_once(once, []{ throw 5; });
    }
    catch(int)
    {
    }
    int b = 0;
    std::call_once(once, [&b]{ b = 5; });
    ASSERT_EQ(5, b);
}

// re-enable when the test above this works again
TEST(memoizing_map, DISABLED_exception)
{
	struct compare_ok
	{
        compare_ok(std::atomic<bool> & signal_ok, std::atomic<bool> & to_wait_for)
            : signal_ok(signal_ok), to_wait_for(to_wait_for)
		{
		}
		bool operator<(const compare_ok &) const
        {
            signal_ok = true;
            while (!to_wait_for) {}
			return false;
		}

	private:
        std::atomic<bool> & signal_ok;
        std::atomic<bool> & to_wait_for;
	};

	memoizing_map<compare_ok, throw_exception_once> throwing_map;
    std::promise<void> signal_second;
    std::atomic<bool> signal_first(false);
    std::atomic<bool> did_throw(false);
	std::exception_ptr first_exception;
	std::thread a([&]
	{
		try
		{
			throwing_map.get(compare_ok(signal_first, did_throw), [&](const compare_ok &)
			{
                signal_second.set_value();
                while (!signal_first) {};
				return throw_exception_once();
			});
		}
		catch(...)
		{
			first_exception = std::current_exception();
            did_throw = true;
		}
    });
    signal_second.get_future().get();
	std::thread b([&]
	{
		throwing_map.get(compare_ok(signal_first, did_throw), [&](const compare_ok &)
		{
			return throw_exception_once();
		});
	});
	a.join();
	b.join();
	ASSERT_THROW(std::rethrow_exception(std::move(first_exception)), test_exception);
}

#endif
