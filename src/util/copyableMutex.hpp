#pragma once

#include <mutex>

// mutexes aren't really copyable but I just want my mutex to be default-initialized
// after a copy. so that's what this mutex class does
struct copyable_mutex
{
	copyable_mutex()
	{
	}
	copyable_mutex(const copyable_mutex &)
	{
	}
	copyable_mutex(copyable_mutex &&)
	{
	}
	copyable_mutex & operator=(const copyable_mutex &)
	{
		return *this;
	}
	copyable_mutex & operator=(copyable_mutex &&)
	{
		return *this;
	}

	void lock()
	{
		mutex.lock();
	}
	bool try_lock()
	{
		return mutex.try_lock();
	}
	void unlock()
	{
		mutex.unlock();
	}

private:
	std::mutex mutex;
};

struct copyable_recursive_mutex
{
	copyable_recursive_mutex()
	{
	}
	copyable_recursive_mutex(const copyable_recursive_mutex &)
	{
	}
	copyable_recursive_mutex(copyable_recursive_mutex &&)
	{
	}
	copyable_recursive_mutex & operator=(const copyable_recursive_mutex &)
	{
		return *this;
	}
	copyable_recursive_mutex & operator=(copyable_recursive_mutex &&)
	{
		return *this;
	}

	void lock()
	{
		recursive_mutex.lock();
	}
	bool try_lock()
	{
		return recursive_mutex.try_lock();
	}
	void unlock()
	{
		recursive_mutex.unlock();
	}

private:
	std::recursive_mutex recursive_mutex;
};
