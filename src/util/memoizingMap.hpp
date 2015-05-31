#pragma once

#include <map>
#include "util/copyableMutex.hpp"


template<typename T>
struct lazy_initialize
{
    lazy_initialize()
    {
    }
    lazy_initialize(const lazy_initialize & other)
    {
        if (other.initialized)
        {
            std::call_once(once, [&]
            {
                new (std::addressof(value)) T(other.value);
                initialized = true;
            });
        }
    }
    lazy_initialize & operator=(const lazy_initialize & other)
    {
        if (initialized)
        {
            if (other.initialized)
            {
                value = other.value;
            }
            else
            {
                once.~once_flag();
                new (&once) std::once_flag();
                value.~T();
                initialized = false;
            }
        }
        else if (other.initialized)
        {
            std::call_once(once, [&]
            {
                new (std::addressof(value)) T(other.value);
                initialized = true;
            });
        }
    }
    lazy_initialize(lazy_initialize && other)
    {
        if (other.initialized)
        {
            std::call_once(once, [&]
            {
                new (std::addressof(value)) T(std::move(other.value));
                initialized = true;
            });
        }
    }
    lazy_initialize & operator=(lazy_initialize && other)
    {
        if (initialized)
        {
            if (other.initialized)
            {
                value = std::move(other.value);
            }
            else
            {
                once.~once_flag();
                new (&once) std::once_flag();
                value.~T();
                initialized = false;
            }
        }
        else if (other.initialized)
        {
            std::call_once(once, [&]
            {
                new (std::addressof(value)) T(std::move(other.value));
                initialized = true;
            });
        }
        return *this;
    }
    ~lazy_initialize()
    {
        if (initialized)
            value.~T();
    }

    template<typename Func>
    const T & get(Func && initializer) const
    {
        return const_cast<lazy_initialize &>(*this).get(std::forward<Func>(initializer));
    }
    template<typename Func>
    T & get(Func && initializer)
    {
        std::call_once(once, [&]
        {
            new (std::addressof(value)) T(initializer());
            initialized = true;
        });
        return value;
    }

private:
    mutable std::once_flag once;
    mutable bool initialized = false;
    union { T value; };
};

template<typename K, typename V, typename Comp = std::less<K>, typename Allocator = std::allocator<std::pair<const K, V> > >
struct memoizing_map
{
	template<typename DefaultFunc>
	V & get(const K & key, const DefaultFunc & func)
	{
		return const_cast<V &>(const_cast<const memoizing_map &>(*this).get(key, func));
	}
	template<typename DefaultFunc>
	const V & get(const K & key, const DefaultFunc & func) const
	{
        return [&]() -> const lazy_initialize<V> &
		{
			std::lock_guard<copyable_mutex> lock(map_mutex);
			return map[key];
        }().get([&]() -> decltype(auto)
		{
			return func(key);
		});
	}

private:
	mutable copyable_mutex map_mutex;
	mutable std::map<K, lazy_initialize<V>, Comp, Allocator> map;
};
