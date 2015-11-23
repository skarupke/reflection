#pragma once

#include <string>
#include <memory>
#include <functional>

template<typename HashType = size_t, typename HashFunction = std::hash<std::string>>
struct HashedString
{
    HashedString(const char * str)
        : str(ptr::make_shared<const std::string>(str)), hash(HashFunction()(*this->str))
    {
    }

    HashedString(std::string str)
        : str(ptr::make_shared<const std::string>(std::move(str))), hash(HashFunction()(*this->str))
    {
    }

    bool operator==(const HashedString & other) const noexcept
    {
        return hash == other.hash && (str == other.str || *str == *other.str);
    }
    bool operator==(const std::string & other) const noexcept
    {
        return *str == other;
    }
    bool operator==(const char * other) const noexcept
    {
        return *str == other;
    }
    bool operator!=(const HashedString & other) const noexcept
    {
        return !(*this == other);
    }
    bool operator!=(const std::string & other) const noexcept
    {
        return !(*this == other);
    }
    bool operator!=(const char * other) const noexcept
    {
        return !(*this == other);
    }
    bool operator<(const HashedString & other) const noexcept
    {
        return *str < *other.str;
    }
    bool operator<(const std::string & other) const noexcept
    {
        return *str < other;
    }
    bool operator<(const char * other) const noexcept
    {
        return *str < other;
    }
    bool operator>(const HashedString & other) const noexcept
    {
        return other < *this;
    }
    bool operator>(const std::string & other) const noexcept
    {
        return *str > other;
    }
    bool operator>(const char * other) const noexcept
    {
        return *str > other;
    }
    bool operator<=(const HashedString & other) const noexcept
    {
        return !(*this > other);
    }
    bool operator<=(const std::string & other) const noexcept
    {
        return *str <= other;
    }
    bool operator<=(const char * other) const noexcept
    {
        return *str <= other;
    }
    bool operator>=(const HashedString & other) const noexcept
    {
        return !(*this < other);
    }
    bool operator>=(const std::string & other) const noexcept
    {
        return *str >= other;
    }
    bool operator>=(const char * other) const noexcept
    {
        return *str >= other;
    }

    const std::string & get() const noexcept
    {
        return *str;
    }
    const char * c_str() const noexcept
    {
        return str->c_str();
    }
    HashType get_hash() const noexcept
    {
        return hash;
    }

private:
    ptr::small_shared_ptr<const std::string> str;
    HashType hash;
};

namespace std
{
template<typename H, typename F>
struct hash<HashedString<H, F>>
{
    size_t operator()(const HashedString<H, F> & object) const noexcept
    {
        return object.get_hash();
    }
};
}

template<typename H, typename F>
inline bool operator==(const std::string & lhs, const HashedString<H, F> & rhs) noexcept
{
    return rhs == lhs;
}
template<typename H, typename F>
inline bool operator==(const char * lhs, const HashedString<H, F> & rhs) noexcept
{
    return rhs == lhs;
}
template<typename H, typename F>
inline bool operator!=(const std::string & lhs, const HashedString<H, F> & rhs) noexcept
{
    return !(lhs == rhs);
}
template<typename H, typename F>
inline bool operator!=(const char * lhs, const HashedString<H, F> & rhs) noexcept
{
    return !(lhs == rhs);
}
template<typename H, typename F>
inline bool operator<(const std::string & lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs < rhs.get();
}
template<typename H, typename F>
inline bool operator<(const char * lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs < rhs.get();
}
template<typename H, typename F>
inline bool operator>(const std::string & lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs > rhs.get();
}
template<typename H, typename F>
inline bool operator>(const char * lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs > rhs.get();
}
template<typename H, typename F>
inline bool operator<=(const std::string & lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs <= rhs.get();
}
template<typename H, typename F>
inline bool operator<=(const char * lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs <= rhs.get();
}
template<typename H, typename F>
inline bool operator>=(const std::string & lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs >= rhs.get();
}
template<typename H, typename F>
inline bool operator>=(const char * lhs, const HashedString<H, F> & rhs) noexcept
{
    return lhs >= rhs.get();
}
