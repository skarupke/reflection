#pragma once

#include <cstddef>
#include <string>
#include <iosfwd>
#include <algorithm>
#include <functional>
#include <numeric>
#include <iterator>

template<typename T>
struct Range;

namespace detail
{
template<typename T>
struct BaseRange
{
    BaseRange() noexcept
        : begin_(nullptr), end_(nullptr)
    {
    }
    BaseRange(T * begin, T * end) noexcept
        : begin_(begin), end_(end)
    {
    }

    operator Range<const T>() const noexcept
    {
        return Range<const T>(begin_, end_);
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

    bool operator<(const BaseRange & other) const
    {
        return std::lexicographical_compare(begin_, end_, other.begin_, other.end_);
    }
    bool operator>(const BaseRange & other) const
    {
        return other < *this;
    }
    bool operator<=(const BaseRange & other) const
    {
        return !(other < *this);
    }
    bool operator>=(const BaseRange & other) const
    {
        return !(*this < other);
    }
    bool operator==(const BaseRange & other) const
    {
        if (begin_ == other.begin_) return end_ == other.end_;
        else return size() == other.size() && std::equal(begin_, end_, other.begin_);
    }
    bool operator!=(const BaseRange & other) const
    {
        return !(*this == other);
    }
    Range<T> subrange(size_t index, size_t length = std::numeric_limits<size_t>::max()) const
    {
        size_t my_length = size();
        if (my_length <= index)
        {
            return Range<T>(end_, end_);
        }
        T * new_begin = begin_ + index;
        length = std::min(length, my_length - index);
        return Range<T>(new_begin, new_begin + length);
    }

    bool startswith(Range<T> other) const
    {
        return size() >= other.size() && std::equal(other.begin_, other.end_, begin_);
    }
    bool endswith(Range<T> other) const
    {
        return size() >= other.size() && std::equal(other.rbegin(), other.rend(), rbegin());
    }

private:
    T * begin_;
    T * end_;
};
}

template<typename T>
struct Range : detail::BaseRange<T>
{
    using detail::BaseRange<T>::BaseRange;
};

template<typename C>
struct ConstCharacterRange : detail::BaseRange<const C>
{
    using detail::BaseRange<const C>::BaseRange;
    ConstCharacterRange() noexcept
        : detail::BaseRange<const C>()
    {
    }
    template<size_t Size>
    ConstCharacterRange(const C (&static_string)[Size]) noexcept
        : detail::BaseRange<const C>(static_string, static_string + Size - 1)
    {
    }
    ConstCharacterRange(const std::basic_string<C> & str)
        : detail::BaseRange<const C>(str.data(), str.data() + str.size())
    {
    }

    std::basic_string<C> str() const
    {
        return std::basic_string<C>(this->begin(), this->end());
    }

    using detail::BaseRange<const C>::operator<;
    using detail::BaseRange<const C>::operator<=;
    using detail::BaseRange<const C>::operator>;
    using detail::BaseRange<const C>::operator>=;
    using detail::BaseRange<const C>::operator==;
    using detail::BaseRange<const C>::operator!=;
    bool operator<(const std::basic_string<C> & rhs) const
    {
        return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(), rhs.end());
    }
    inline bool operator<=(const std::basic_string<C> & rhs) const;
    inline bool operator>(const std::basic_string<C> & rhs) const;
    bool operator>=(const std::basic_string<C> & rhs) const
    {
        return !(*this < rhs);
    }
    bool operator==(const std::basic_string<C> & rhs) const
    {
        return this->size() == rhs.size() && std::equal(this->begin(), this->end(), rhs.begin());
    }
    bool operator!=(const std::basic_string<C> & rhs) const
    {
        return !(*this == rhs);
    }
    std::basic_string<C> operator+(std::basic_string<C> rhs) const
    {
        rhs.insert(rhs.begin(), this->begin(), this->end());
        return rhs;
    }

    Range<const C> substr(size_t index, size_t length = std::numeric_limits<size_t>::max()) const
    {
        return this->subrange(index, length);
    }
};
template<>
struct Range<const char> : ConstCharacterRange<char>
{
    using ConstCharacterRange<char>::ConstCharacterRange;
};
template<>
struct Range<const char32_t> : ConstCharacterRange<char32_t>
{
    using ConstCharacterRange<char32_t>::ConstCharacterRange;
};

inline bool operator<(const std::string & lhs, const Range<const char> & rhs)
{
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}
inline bool operator<=(const std::string & lhs, const Range<const char> & rhs)
{
    return !(rhs < lhs);
}
inline bool operator>(const std::string & lhs, const Range<const char> & rhs)
{
    return rhs < lhs;
}
inline bool operator>=(const std::string & lhs, const Range<const char> & rhs)
{
    return !(lhs < rhs);
}
inline bool operator==(const std::string & lhs, const Range<const char> & rhs)
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}
inline bool operator!=(const std::string & lhs, const Range<const char> & rhs)
{
    return !(lhs == rhs);
}
template<typename C>
inline bool operator==(const C * lhs, ConstCharacterRange<C> rhs)
{
    for (auto it = rhs.begin(), end = rhs.end(); it != end;)
    {
        if (*it++ != *lhs++)
            return false;
    }
    return *lhs == 0;
}
template<typename C>
inline bool operator!=(const C * lhs, ConstCharacterRange<C> rhs)
{
    return !(lhs == rhs);
}
template<typename C, size_t Size>
inline bool operator==(const C (&lhs)[Size], Range<const C> rhs)
{
    return rhs == lhs;
}
template<typename C, size_t Size>
inline bool operator!=(const C (&lhs)[Size], Range<const C> rhs)
{
    return !(lhs == rhs);
}
template<typename C>
inline bool operator==(ConstCharacterRange<C> lhs, const C * rhs)
{
    for (auto it = lhs.begin(), end = lhs.end(); it != end;)
    {
        if (*it++ != *rhs++)
            return false;
    }
    return *rhs == 0;
}
template<typename C>
inline bool operator!=(ConstCharacterRange<C> lhs, const C * rhs)
{
    return !(lhs == rhs);
}

template<typename C>
inline bool ConstCharacterRange<C>::operator<=(const std::basic_string<C> & rhs) const
{
    return !(rhs < *this);
}
template<typename C>
inline bool ConstCharacterRange<C>::operator>(const std::basic_string<C> & rhs) const
{
    return rhs < *this;
}
template<typename C>
inline std::basic_string<C> operator+(std::basic_string<C> lhs, ConstCharacterRange<C> rhs)
{
    lhs.insert(lhs.end(), rhs.begin(), rhs.end());
    return lhs;
}

namespace std
{
template<typename T>
struct hash<detail::BaseRange<T>>
{
    size_t operator()(detail::BaseRange<T> range) const
    {
        return std::accumulate(range.begin(), range.end(), size_t(), [](size_t lhs, const T & rhs)
        {
            return lhs ^ (std::hash<T>()(rhs) + 0x9e3779b9 + (lhs << 6) + (lhs >> 2));
        });
    }
};
template<typename T>
struct hash<detail::BaseRange<const T>>
{
    size_t operator()(detail::BaseRange<const T> range) const
    {
        return std::accumulate(range.begin(), range.end(), size_t(), [](size_t lhs, const T & rhs)
        {
            return lhs ^ (std::hash<T>()(rhs) + 0x9e3779b9 + (lhs << 6) + (lhs >> 2));
        });
    }
};
template<typename T>
struct hash<Range<T>> : hash<detail::BaseRange<T>>
{
};
template<typename T>
struct hash<ConstCharacterRange<T>> : hash<detail::BaseRange<const T>>
{
};
}

std::ostream & operator<<(std::ostream & lhs, Range<const char> rhs);
