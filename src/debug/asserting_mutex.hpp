#pragma once

#include "assert.hpp"
#include <atomic>
#include <mutex>

struct AssertingMutex
{
	AssertingMutex()
		: is_in_use(0)
	{
	}

	void lock()
	{
        RAW_ASSERT(++is_in_use == 1);
	}
	void unlock()
	{
        RAW_ASSERT(--is_in_use == 0);
	}

private:
	std::atomic<signed char> is_in_use;
};

struct AssertingReaderWriterMutex
{
	struct Writer
	{
		Writer()
			: counter(0)
		{
		}

		void lock()
		{
            RAW_ASSERT(++counter == 1);
		}
		void unlock()
		{
            RAW_ASSERT(--counter == 0);
		}

	protected:
		std::atomic<int> counter;
	};
	struct Reader : private Writer
	{
		friend struct AssertingReaderWriterMutex;

		void lock()
		{
            RAW_ASSERT(--counter < 0);
		}
		void unlock()
		{
            RAW_ASSERT(++counter <= 0);
		}
	};

	operator Writer &()
	{
		return GetWriter();
	}
	operator Reader &()
	{
		return GetReader();
	}
	Writer & GetWriter()
	{
		return reader;
	}
	Reader & GetReader()
	{
		return reader;
	}

private:
	Reader reader;
};
