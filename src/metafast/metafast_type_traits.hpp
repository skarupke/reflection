#pragma once

#include "util/stl_container_forward.hpp"

namespace metaf
{

namespace detail
{
template<typename T, typename = void>
struct equality_comparable_helper
{
    static constexpr bool value = false;
};

template<typename T>
struct equality_comparable_helper<T, typename std::enable_if<true, decltype(static_cast<void>(std::declval<T&>() == std::declval<T&>()))>::type>
{
    static constexpr bool value = true;
};
}

template<typename T>
struct is_equality_comparable
    : detail::equality_comparable_helper<T>
{
};
template<typename T, size_t Size>
struct is_equality_comparable<T[Size]>
    : metaf::is_equality_comparable<T>
{
};
template<typename T, size_t Size>
struct is_equality_comparable<std::array<T,Size>>
    : metaf::is_equality_comparable<T>
{
};
template<typename T, typename A>
struct is_equality_comparable<std::deque<T, A>>
    : metaf::is_equality_comparable<T>
{
};
template<typename T, typename A>
struct is_equality_comparable<std::list<T, A>>
    : metaf::is_equality_comparable<T>
{
};
template<typename T, typename A>
struct is_equality_comparable<std::forward_list<T, A>>
    : metaf::is_equality_comparable<T>
{
};
template<typename T, typename A>
struct is_equality_comparable<std::vector<T, A>>
    : metaf::is_equality_comparable<T>
{
};

template<typename K, typename V, typename C, typename A>
struct is_equality_comparable<std::map<K, V, C, A>>
{
    static constexpr bool value = metaf::is_equality_comparable<K>::value
                                && metaf::is_equality_comparable<V>::value;
};
template<typename K, typename V, typename C, typename A>
struct is_equality_comparable<std::multimap<K, V, C, A>>
{
    static constexpr bool value = metaf::is_equality_comparable<K>::value
                                && metaf::is_equality_comparable<V>::value;
};
template<typename K, typename C, typename A>
struct is_equality_comparable<std::multiset<K, C, A>>
    : metaf::is_equality_comparable<K>
{
};
template<typename K, typename C, typename A>
struct is_equality_comparable<std::set<K, C, A>>
    : metaf::is_equality_comparable<K>
{
};

template<typename K, typename V, typename H, typename E, typename A>
struct is_equality_comparable<std::unordered_map<K, V, H, E, A>>
{
    static constexpr bool value = metaf::is_equality_comparable<K>::value
                                && metaf::is_equality_comparable<V>::value;
};
template<typename K, typename V, typename H, typename E, typename A>
struct is_equality_comparable<std::unordered_multimap<K, V, H, E, A>>
{
    static constexpr bool value = metaf::is_equality_comparable<K>::value
                                && metaf::is_equality_comparable<V>::value;
};
template<typename K, typename H, typename E, typename A>
struct is_equality_comparable<std::unordered_multiset<K, H, E, A>>
    : metaf::is_equality_comparable<K>
{
};
template<typename K, typename H, typename E, typename A>
struct is_equality_comparable<std::unordered_set<K, H, E, A>>
    : metaf::is_equality_comparable<K>
{
};

}
