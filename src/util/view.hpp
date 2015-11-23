#pragma once

#include <cstddef>
#include <iosfwd>
#include <algorithm>
#include <numeric>
#include <iterator>
#include "util/stl_container_forward.hpp"

template<typename T>
struct ArrayView;
template<typename T>
struct StringView;

namespace detail
{
template<typename T>
struct template_add_const
{
    typedef T type;
};
template<template<typename> class T, typename N>
struct template_add_const<T<N>>
{
    typedef T<const N> type;
};
template<typename T, typename Derived>
struct BaseArrayView
{
    BaseArrayView() noexcept
        : begin_(nullptr), end_(nullptr)
    {
    }
    BaseArrayView(T * begin, T * end) noexcept
        : begin_(begin), end_(end)
    {
    }

    typedef typename template_add_const<Derived>::type ConstDerived;

    operator ConstDerived() const noexcept
    {
        return { begin_, end_ };
    }

    typedef T * iterator;
    typedef T * const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    iterator begin() const noexcept
    {
        return begin_;
    }
    iterator end() const noexcept
    {
        return end_;
    }
    const_iterator cbegin() const noexcept
    {
        return begin_;
    }
    const_iterator cend() const noexcept
    {
        return end_;
    }
    reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(cend());
    }
    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(cbegin());
    }
    T & operator[](std::ptrdiff_t offset) const noexcept
    {
        return begin_[offset];
    }
    T & front() const
    {
        return begin_[0];
    }
    T & back() const
    {
        return end_[-1];
    }

    size_t size() const noexcept
    {
        return end_ - begin_;
    }
    bool empty() const noexcept
    {
        return begin_ == end_;
    }

    bool operator<(BaseArrayView other) const
    {
        return std::lexicographical_compare(begin_, end_, other.begin_, other.end_);
    }
    bool operator>(BaseArrayView other) const
    {
        return other < *this;
    }
    bool operator<=(BaseArrayView other) const
    {
        return !(other < *this);
    }
    bool operator>=(BaseArrayView other) const
    {
        return !(*this < other);
    }
    bool operator==(BaseArrayView other) const
    {
        if (begin_ == other.begin_) return end_ == other.end_;
        else return size() == other.size() && std::equal(begin_, end_, other.begin_);
    }
    bool operator!=(BaseArrayView other) const
    {
        return !(*this == other);
    }
    Derived subview(size_t index, size_t length = std::numeric_limits<size_t>::max()) const
    {
        size_t my_length = size();
        if (my_length <= index)
        {
            return Derived(end_, end_);
        }
        T * new_begin = begin_ + index;
        length = std::min(length, my_length - index);
        return Derived(new_begin, new_begin + length);
    }

    bool startswith(Derived other) const
    {
        return size() >= other.size() && std::equal(other.begin_, other.end_, begin_);
    }
    bool endswith(Derived other) const
    {
        return size() >= other.size() && std::equal(other.rbegin(), other.rend(), rbegin());
    }

private:
    T * begin_;
    T * end_;
};
template<typename T>
struct remove_const
{
    typedef T type;
};
template<typename T>
struct remove_const<const T>
{
    typedef T type;
};
}

template<typename T>
struct ArrayView : detail::BaseArrayView<T, ArrayView<T>>
{
    typedef detail::BaseArrayView<T, ArrayView> Base;
    using Base::Base;
};

template<typename C>
struct StringView : detail::BaseArrayView<C, StringView<C>>
{
    typedef detail::BaseArrayView<C, StringView> Base;
    typedef std::basic_string<typename detail::remove_const<C>::type> string_type;
    using Base::Base;
    StringView() noexcept = default;
    template<size_t Size>
    StringView(C (&static_string)[Size]) noexcept
        : Base(static_string, static_string + Size - 1)
    {
    }
    StringView(string_type & str)
        : StringView(const_cast<C *>(str.data()), const_cast<C *>(str.data() + str.size()))
    {
    }
    StringView(const string_type & str)
        : StringView(str.data(), str.data() + str.size())
    {
    }
    StringView(string_type && str) = delete;
    StringView(const string_type && str) = delete;

    string_type str() const
    {
        return string_type(this->begin(), this->end());
    }

    using Base::operator<;
    using Base::operator<=;
    using Base::operator>;
    using Base::operator>=;
    using Base::operator==;
    using Base::operator!=;
    bool operator<(const string_type & rhs) const
    {
        return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(), rhs.end());
    }
    inline bool operator<=(const string_type & rhs) const;
    inline bool operator>(const string_type & rhs) const;
    bool operator>=(const string_type & rhs) const
    {
        return !(*this < rhs);
    }
    bool operator==(const string_type & rhs) const
    {
        return this->size() == rhs.size() && std::equal(this->begin(), this->end(), rhs.begin());
    }
    bool operator!=(const string_type & rhs) const
    {
        return !(*this == rhs);
    }
    string_type operator+(string_type rhs) const
    {
        rhs.insert(rhs.begin(), this->begin(), this->end());
        return rhs;
    }

    StringView<C> substr(size_t index, size_t length = std::numeric_limits<size_t>::max()) const
    {
        return this->subview(index, length);
    }
};

template<typename C>
inline bool operator<(const typename StringView<C>::string_type & lhs, const StringView<C> & rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
template<typename C>
inline bool operator<=(const typename StringView<C>::string_type & lhs, const StringView<C> & rhs)
{
    return !(rhs < lhs);
}
template<typename C>
inline bool operator>(const typename StringView<C>::string_type & lhs, const StringView<C> & rhs)
{
    return rhs < lhs;
}
template<typename C>
inline bool operator>=(const typename StringView<C>::string_type & lhs, const StringView<C> & rhs)
{
    return !(lhs < rhs);
}
template<typename C>
inline bool operator==(const typename StringView<C>::string_type & lhs, const StringView<C> & rhs)
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
template<typename C>
inline bool operator!=(const typename StringView<C>::string_type & lhs, const StringView<C> & rhs)
{
    return !(lhs == rhs);
}
template<typename C>
inline bool operator==(const C * lhs, StringView<const C> rhs)
{
    return rhs == lhs;
}
template<typename C>
inline bool operator!=(const C * lhs, StringView<const C> rhs)
{
    return !(lhs == rhs);
}
template<typename C>
inline bool operator==(StringView<const C> lhs, const C * rhs)
{
    for (auto it = lhs.begin(), end = lhs.end(); it != end;)
    {
        if (*it++ != *rhs++)
            return false;
    }
    return *rhs == 0;
}
template<typename C>
inline bool operator!=(StringView<const C> lhs, const C * rhs)
{
    return !(lhs == rhs);
}

template<typename C>
inline bool StringView<C>::operator<=(const typename StringView<C>::string_type & rhs) const
{
    return !(rhs < *this);
}
template<typename C>
inline bool StringView<C>::operator>(const typename StringView<C>::string_type & rhs) const
{
    return rhs < *this;
}
template<typename C>
inline typename StringView<C>::string_type operator+(typename StringView<C>::string_type lhs, StringView<C> rhs)
{
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

namespace std
{
template<typename T, typename D>
struct hash<detail::BaseArrayView<T, D>>
{
    size_t operator()(detail::BaseArrayView<T, D> range) const
    {
        return hash<typename detail::BaseArrayView<T, D>::ConstDerived>()(range);
    }
};
template<typename T, typename D>
struct hash<detail::BaseArrayView<const T, D>>
{
    size_t operator()(detail::BaseArrayView<const T, D> range) const
    {
        return std::accumulate(range.begin(), range.end(), size_t(), [](size_t lhs, const T & rhs)
        {
            return lhs ^ (std::hash<T>()(rhs) + 0x9e3779b9 + (lhs << 6) + (lhs >> 2));
        });
    }
};
template<typename T>
struct hash<ArrayView<T>> : hash<typename ArrayView<T>::Base>
{
};
template<typename T>
struct hash<StringView<T>> : hash<typename StringView<T>::Base>
{
};
}

std::ostream & operator<<(std::ostream & lhs, StringView<const char> rhs);
