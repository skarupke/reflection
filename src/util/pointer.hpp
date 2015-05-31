#pragma once

#include <memory>
#include "forwarding_constructor.hpp"

// sometimes you want value semantics but you want to be able to
// transfer ownership of an object. the copying_pointer allows
// you to do that
template<typename T, typename Deleter = std::default_delete<T> >
struct copying_ptr
{
	copying_ptr()
		: ptr(new T())
	{
	}
	template<typename... Args>
	explicit copying_ptr(forwarding_constructor, Args &&... args)
		: ptr(new T(std::forward<Args>(args)...))
	{
	}
	copying_ptr(const copying_ptr & other)
		: ptr(new T(*other.ptr))
	{
	}
	copying_ptr(copying_ptr && other)
		: ptr(new T(std::move(*other.ptr)))
	{
	}
	copying_ptr & operator=(const copying_ptr & rhs)
	{
		*ptr = *rhs.ptr;
		return *this;
	}
	copying_ptr & operator=(copying_ptr && rhs)
	{
		*ptr = std::move(*rhs.ptr);
		return *this;
	}

	T * get() const noexcept
	{
		return ptr.get();
	}
	T & operator*() const noexcept
	{
		return *ptr;
	}
	T * operator->() const noexcept
	{
		return ptr.operator->();
	}

	// the copying_pointer will be in an invalid state after
	// you call this function. you can not even assign to it.
	// the only valid remaining operation is calling the destructor
	std::unique_ptr<T, Deleter> release_and_invalidate() &&
	{
		return std::move(ptr);
	}
	bool operator==(const copying_ptr & other) const
	{
		return *ptr == *other.ptr;
	}
	bool operator!=(const copying_ptr & other) const
	{
		return !(*this == other);
	}

private:
	std::unique_ptr<T, Deleter> ptr;
};

// the cloning_ptr has value semantics and it does that by calling
// the "clone" method on the object it holds whenever you copy the
// cloning_ptr. the "clone" method is expected to be of type
// std::unique_ptr<T> clone() const;
template<typename T, typename Deleter = std::default_delete<T> >
struct cloning_ptr
{
	cloning_ptr() = default;
	explicit cloning_ptr(T * ptr)
		: ptr(ptr)
	{
	}
	explicit cloning_ptr(std::unique_ptr<T> ptr)
		: ptr(std::move(ptr))
	{
	}
	template<typename... Args>
	explicit cloning_ptr(forwarding_constructor, Args &&... args)
		: cloning_ptr(new T(std::forward<Args>(args)...))
	{
	}
	cloning_ptr(const cloning_ptr & other)
		: ptr(other.ptr ? other.ptr->clone() : std::unique_ptr<T>())
	{
	}
	cloning_ptr(cloning_ptr && other) = default;
	cloning_ptr & operator=(const cloning_ptr & rhs)
	{
		if (rhs.ptr) ptr = rhs.ptr->clone();
		else ptr.reset();
		return *this;
	}
	cloning_ptr & operator=(cloning_ptr && rhs) = default;
	cloning_ptr & operator=(std::unique_ptr<T> rhs)
	{
		ptr = std::move(rhs);
		return *this;
	}

	T * get() const noexcept
	{
		return ptr.get();
	}
	const std::unique_ptr<T> & get_unique() const noexcept
	{
		return ptr;
	}
	T & operator*() const noexcept
	{
		return *ptr;
	}
	T * operator->() const noexcept
	{
		return ptr.operator->();
	}

	std::unique_ptr<T, Deleter> release()
	{
		return std::move(ptr);
	}
	bool operator==(const cloning_ptr & other) const
	{
		if (ptr) return other.ptr && *ptr == *other.ptr;
		else return !other.ptr;
	}
	bool operator!=(const cloning_ptr & other) const
	{
		return !(*this == other);
	}

private:
	std::unique_ptr<T, Deleter> ptr;
};

// behaves pretty much like std::unique_ptr but compiles
// faster than the libstdc++ version. this doesn't
// handle all features of std::unique_ptr though.
// for example if you have a zero sized deleter other
// than std::default_delete, this will waste eight bytes
template<typename T, typename Delete = std::default_delete<T> >
struct dunique_ptr
{
	dunique_ptr()
		: ptr(nullptr)
	{
	}
	explicit dunique_ptr(T * ptr, Delete deleter = Delete())
		: ptr(ptr), deleter(std::move(deleter))
	{
	}
	dunique_ptr(const dunique_ptr &) = delete;
	dunique_ptr & operator=(const dunique_ptr &) = delete;
	dunique_ptr(dunique_ptr && other)
		: ptr(other.ptr), deleter(std::move(other.deleter))
	{
		other.ptr = nullptr;
	}
	dunique_ptr & operator=(dunique_ptr && other)
	{
		swap(other);
		return *this;
	}
	~dunique_ptr()
	{
		reset();
	}
	void reset(T * value = nullptr)
	{
		T * to_delete = ptr;
		ptr = value;
		deleter(to_delete);
	}
	T * release()
	{
		T * result = ptr;
		ptr = nullptr;
		return result;
	}
	explicit operator bool() const
	{
		return bool(ptr);
	}
	T * get() const
	{
		return ptr;
	}
	T & operator*() const
	{
		return *ptr;
	}
	T * operator->() const
	{
		return ptr;
	}
	void swap(dunique_ptr & other)
	{
		using std::swap;
		swap(ptr, other.ptr);
		swap(deleter, other.deleter);
	}

	bool operator==(const dunique_ptr & other) const
	{
		return ptr == other.ptr;
	}
	bool operator!=(const dunique_ptr & other) const
	{
		return !(*this == other);
	}
	bool operator<(const dunique_ptr & other) const
	{
		return ptr < other.ptr;
	}
	bool operator<=(const dunique_ptr & other) const
	{
		return !(other < *this);
	}
	bool operator>(const dunique_ptr & other) const
	{
		return other < *this;
	}
	bool operator>=(const dunique_ptr & other) const
	{
		return !(*this < other);
	}

private:
	T * ptr;
	Delete deleter;
};

template<typename T>
struct dunique_ptr<T, std::default_delete<T> >
{
	dunique_ptr()
		: ptr(nullptr)
	{
	}
	explicit dunique_ptr(T * ptr)
		: ptr(ptr)
	{
	}
	dunique_ptr(const dunique_ptr &) = delete;
	dunique_ptr & operator=(const dunique_ptr &) = delete;
	dunique_ptr(dunique_ptr && other)
		: ptr(other.ptr)
	{
		other.ptr = nullptr;
	}
	dunique_ptr & operator=(dunique_ptr && other)
	{
		swap(other);
		return *this;
	}
	~dunique_ptr()
	{
		reset();
	}
	void reset(T * value = nullptr)
	{
		T * to_delete = ptr;
		ptr = value;
		delete to_delete;
	}
	T * release()
	{
		T * result = ptr;
		ptr = nullptr;
		return result;
	}
	explicit operator bool() const
	{
		return bool(ptr);
	}
	T * get() const
	{
		return ptr;
	}
	T & operator*() const
	{
		return *ptr;
	}
	T * operator->() const
	{
		return ptr;
	}
	void swap(dunique_ptr & other)
	{
		std::swap(ptr, other.ptr);
	}

	bool operator==(const dunique_ptr & other) const
	{
		return ptr == other.ptr;
	}
	bool operator!=(const dunique_ptr & other) const
	{
		return !(*this == other);
	}
	bool operator<(const dunique_ptr & other) const
	{
		return ptr < other.ptr;
	}
	bool operator<=(const dunique_ptr & other) const
	{
		return !(other < *this);
	}
	bool operator>(const dunique_ptr & other) const
	{
		return other < *this;
	}
	bool operator>=(const dunique_ptr & other) const
	{
		return !(*this < other);
	}

private:
	T * ptr;
};

template<typename T, typename D>
void swap(dunique_ptr<T, D> & lhs, dunique_ptr<T, D> & rhs)
{
	lhs.swap(rhs);
}

