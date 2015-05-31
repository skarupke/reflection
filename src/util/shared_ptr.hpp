#pragma once

#include <utility>
#include <atomic>

namespace ptr
{
// a shared_ptr that's only 8 bytes in size
template<typename T>
struct small_shared_ptr
{
	small_shared_ptr() noexcept
		: storage(nullptr)
	{
	}
	template<typename... Arguments>
	static small_shared_ptr make(Arguments &&... arguments)
	{
		small_shared_ptr to_return;
		to_return.storage = new Storage(std::forward<Arguments>(arguments)...);
		return to_return;
	}
	small_shared_ptr(const small_shared_ptr & other) noexcept
		: storage(other.storage)
	{
		if (storage) storage->ref_count.increment();
	}
	small_shared_ptr(small_shared_ptr && other) noexcept
		: storage(other.storage)
	{
		other.storage = nullptr;
	}
	small_shared_ptr & operator=(small_shared_ptr other) noexcept
	{
		swap(other);
		return *this;
	}
	~small_shared_ptr() noexcept
	{
		if (storage && storage->ref_count.decrement()) delete storage;
	}
	explicit operator bool() const noexcept
	{
		return static_cast<bool>(storage);
	}
	T & operator*() const noexcept
	{
		return storage->object;
	}
	T * operator->() const noexcept
	{
		return get();
	}
	T * get() const noexcept
	{
		return std::addressof(storage->object);
	}
	void reset() noexcept
	{
		*this = small_shared_ptr();
	}
	void reset(std::nullptr_t) noexcept
	{
		*this = small_shared_ptr();
	}
    bool unique() const noexcept
    {
        return storage->ref_count.unique();
    }
	bool operator==(const small_shared_ptr & other) const noexcept
	{
		return storage == other.storage;
	}
	bool operator==(std::nullptr_t) const noexcept
	{
		return storage == nullptr;
	}
	bool operator!=(const small_shared_ptr & other) const noexcept
	{
		return !(*this == other);
	}
	bool operator!=(std::nullptr_t) const noexcept
	{
		return !(*this == nullptr);
	}
	bool operator<(const small_shared_ptr & other) const noexcept
	{
		return storage < other.storage;
	}
	bool operator<=(const small_shared_ptr & other) const noexcept
	{
		return !(other < *this);
	}
	bool operator>(const small_shared_ptr & other) const noexcept
	{
		return other < *this;
	}
	bool operator>=(const small_shared_ptr & other) const noexcept
	{
		return !(*this < other);
	}
	void swap(small_shared_ptr & other) noexcept
	{
		std::swap(storage, other.storage);
	}

private:
	struct ReferenceCount
	{
		ReferenceCount()
			: ref_count(1)
		{
		}

		void increment()
		{
			ref_count.fetch_add(1, std::memory_order_relaxed);
		}
		bool decrement()
		{
			if (ref_count.fetch_sub(1, std::memory_order_release) == 1)
			{
				std::atomic_thread_fence(std::memory_order_acquire);
				return true;
			}
            else
                return false;
        }
        bool unique() const
        {
            return ref_count == 1;
        }
	private:
		std::atomic<int> ref_count;
	};

	struct Storage
	{
		template<typename... Arguments>
		Storage(Arguments &&... arguments)
			: object(std::forward<Arguments>(arguments)...)
		{
		}
		T object;
		ReferenceCount ref_count;
	};
	Storage * storage;
};
template<typename T>
inline bool operator==(std::nullptr_t, const small_shared_ptr<T> & rhs)
{
	return rhs == nullptr;
}
template<typename T>
inline bool operator!=(std::nullptr_t, const small_shared_ptr<T> & rhs)
{
	return rhs != nullptr;
}

template<typename T>
inline void swap(small_shared_ptr<T> & lhs, small_shared_ptr<T> & rhs)
{
	lhs.swap(rhs);
}

template<typename T, typename... Arguments>
inline small_shared_ptr<T> make_shared(Arguments &&... arguments)
{
	return small_shared_ptr<T>::make(std::forward<Arguments>(arguments)...);
}

}
