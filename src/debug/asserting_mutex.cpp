#include "asserting_mutex.hpp"


#if !defined(DISABLE_TESTS) && !defined(VALGRIND_BUILD) && !defined(FINAL)
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include "assert_settings.hpp"
#include <future>

template<typename T>
bool DoesAssert(T && functor)
{
	bool did_assert = false;
	assert::ScopedSetAssertCallback change_callback([&](const ::assert::AssertContext &)
	{
		did_assert = true;
		return ::assert::ShouldNotBreak;
	});
	functor();
	return did_assert;
}

TEST(asserting_mutex, single_user)
{
	AssertingMutex mutex;
	ASSERT_FALSE(DoesAssert([&]
	{
		std::lock_guard<AssertingMutex> lock(mutex);
	}));
}
TEST(asserting_mutex, two_users)
{
	AssertingMutex mutex;
	ASSERT_TRUE(DoesAssert([&]
	{
		std::lock_guard<AssertingMutex> lock(mutex);
		std::lock_guard<AssertingMutex> second_lock(mutex);
	}));
}

TEST(asserting_mutex, threaded)
{
	ASSERT_TRUE(DoesAssert([&]
	{
		AssertingMutex asserting_mutex;
		std::promise<void> first_thread_promise;
		std::promise<void> second_thread_promise;
		std::thread first([&]
		{
			std::lock_guard<AssertingMutex> lock(asserting_mutex);
			first_thread_promise.set_value();
			second_thread_promise.get_future().get();
		});
		std::thread second([&]
		{
			std::lock_guard<AssertingMutex> lock(asserting_mutex);
			second_thread_promise.set_value();
			first_thread_promise.get_future().get();
		});
		first.join();
		second.join();
	}));
}

/*TEST(asserting_mutex, DISABLED_test_many_times)
{
	int num_tests = 1000;
	static constexpr int num_threads = 6;
	std::atomic<int> count[num_threads];
	for (int i = 0; i < num_threads; ++i) count[i] = 0;
	std::vector<std::thread> threads;
	int num_asserts = 0;
	for (int i = 0; i < num_tests; ++i)
	{
		if (DoesAssert([&]
		{
			threads.clear();
			AssertingMutex mutex;
			for (int i = 0; i < num_threads; ++i)
			{
				threads.emplace_back([i, &mutex, &count]
				{
					std::lock_guard<AssertingMutex> lock(mutex);
					++count[i];
				});
			}
			for (std::thread & thread : threads)
			{
				thread.join();
			}
		}))
		{
			++num_asserts;
		}
	}
    //std::cout << num_asserts << std::endl;
	for (std::atomic<int> & c : count)
	{
		ASSERT_EQ(num_tests, c);
	}
}*/

TEST(asserting_reader_writer_mutex, single_writer)
{
	AssertingReaderWriterMutex mutex;
	ASSERT_FALSE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Writer> lock(mutex);
	}));
}
TEST(asserting_reader_writer_mutex, only_readers)
{
	AssertingReaderWriterMutex mutex;
	ASSERT_FALSE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Reader> lock(mutex);
	}));
	ASSERT_FALSE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Reader> lock(mutex);
		std::lock_guard<AssertingReaderWriterMutex::Reader> second_lock(mutex);
	}));
	ASSERT_FALSE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Reader> lock(mutex);
		std::lock_guard<AssertingReaderWriterMutex::Reader> second_lock(mutex);
		std::lock_guard<AssertingReaderWriterMutex::Reader> third_lock(mutex);
	}));
}
TEST(asserting_reader_writer_mutex, multiple_writers)
{
	AssertingReaderWriterMutex mutex;
	ASSERT_TRUE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Writer> lock(mutex);
		std::lock_guard<AssertingReaderWriterMutex::Writer> second_lock(mutex);
	}));
}
TEST(asserting_reader_writer_mutex, writer_first)
{
	AssertingReaderWriterMutex mutex;
	ASSERT_TRUE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Writer> lock(mutex);
		std::lock_guard<AssertingReaderWriterMutex::Reader> second_lock(mutex);
	}));
}
TEST(asserting_reader_writer_mutex, reader_first)
{
	AssertingReaderWriterMutex mutex;
	ASSERT_TRUE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Reader> lock(mutex);
		std::lock_guard<AssertingReaderWriterMutex::Writer> second_lock(mutex);
	}));
}
TEST(asserting_reader_writer_mutex, readers_dont_unlock_too_early)
{
	AssertingReaderWriterMutex mutex;
	ASSERT_TRUE(DoesAssert([&]
	{
		std::lock_guard<AssertingReaderWriterMutex::Reader> lock(mutex);
		{
			std::lock_guard<AssertingReaderWriterMutex::Reader> second_lock(mutex);
		}
		std::lock_guard<AssertingReaderWriterMutex::Writer> second_lock(mutex);
	}));
}

#ifdef RUN_SLOW_TESTS

TEST(asserting_reader_writer_mutex, readers_from_different_threads)
{
	AssertingReaderWriterMutex mutex;
	int num_checks = 100000;
	int num_threads = 2;
	std::vector<std::thread> threads;
	threads.reserve(num_threads);
	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([&]
		{
			for (int i = 0; i < num_checks; ++i)
			{
				std::lock_guard<AssertingReaderWriterMutex::Reader> lock(mutex);
			}
		});
	}
	for (std::thread & thread : threads)
	{
		thread.join();
	}
}

#endif

#endif
