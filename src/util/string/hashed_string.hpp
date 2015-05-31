#pragma once

#include <string>
#include <memory>
#include <functional>

struct HashedString
{
    HashedString(const char * str)
        : str(ptr::make_shared<const std::string>(str)), hash(std::hash<std::string>()(*this->str))
    {
    }

    HashedString(std::string str)
        : str(ptr::make_shared<const std::string>(std::move(str))), hash(std::hash<std::string>()(*this->str))
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
    uint32_t get_hash() const noexcept
    {
        return hash;
    }

private:
    ptr::small_shared_ptr<const std::string> str;
    uint32_t hash;
};

namespace std
{
template<>
struct hash<HashedString>
{
    size_t operator()(const HashedString & object) const noexcept
    {
        return object.get_hash();
    }
};
}

inline bool operator==(const std::string & lhs, const HashedString & rhs) noexcept
{
    return rhs == lhs;
}
inline bool operator==(const char * lhs, const HashedString & rhs) noexcept
{
    return rhs == lhs;
}
inline bool operator!=(const std::string & lhs, const HashedString & rhs) noexcept
{
    return !(lhs == rhs);
}
inline bool operator!=(const char * lhs, const HashedString & rhs) noexcept
{
    return !(lhs == rhs);
}
inline bool operator<(const std::string & lhs, const HashedString & rhs) noexcept
{
    return lhs < rhs.get();
}
inline bool operator<(const char * lhs, const HashedString & rhs) noexcept
{
    return lhs < rhs.get();
}
inline bool operator>(const std::string & lhs, const HashedString & rhs) noexcept
{
    return lhs > rhs.get();
}
inline bool operator>(const char * lhs, const HashedString & rhs) noexcept
{
    return lhs > rhs.get();
}
inline bool operator<=(const std::string & lhs, const HashedString & rhs) noexcept
{
    return lhs <= rhs.get();
}
inline bool operator<=(const char * lhs, const HashedString & rhs) noexcept
{
    return lhs <= rhs.get();
}
inline bool operator>=(const std::string & lhs, const HashedString & rhs) noexcept
{
    return lhs >= rhs.get();
}
inline bool operator>=(const char * lhs, const HashedString & rhs) noexcept
{
    return lhs >= rhs.get();
}
