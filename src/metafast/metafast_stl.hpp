#pragma once

#include "metafast/metafast.hpp"
#include "util/stl_container_forward.hpp"

namespace metaf
{
namespace detail
{
template<typename T, size_t Size>
struct reference_specialization<std::array<T, Size>>
{
    void operator()(BinaryInput & input, std::array<T, Size> & data)
    {
        array(input, data.data(), data.data() + Size);
    }
    void operator()(BinaryOutput & output, const std::array<T, Size> & data)
    {
        array(output, data.data(), data.data() + Size);
    }
};

template<typename T>
struct linear_stl_container_reference_specialization
{
    void operator()(BinaryInput & input, T & data)
    {
        size_t size = 0;
        reference(input, size);
        data.resize(size);
        for (auto & element : data)
        {
            reference(input, element);
        }
    }
    void operator()(BinaryOutput & output, const T & data)
    {
        reference(output, data.size());
        for (const auto & element : data)
        {
            reference(output, element);
        }
    }
};

template<typename S, typename A>
struct reference_specialization<std::deque<S, A>>
    : linear_stl_container_reference_specialization<std::deque<S, A>>
{
};
template<typename S, typename A>
struct reference_specialization<std::list<S, A>>
    : linear_stl_container_reference_specialization<std::list<S, A>>
{
};
template<typename T, typename C, typename A>
struct reference_specialization<std::basic_string<T, C, A>>
    : linear_stl_container_reference_specialization<std::basic_string<T, C, A>>
{
};
template<typename S, typename A>
struct reference_specialization<std::vector<S, A>>
    : linear_stl_container_reference_specialization<std::vector<S, A>>
{
};
template<typename S, typename A>
struct reference_specialization<std::forward_list<S, A>>
    : linear_stl_container_reference_specialization<std::forward_list<S, A>>
{
    using linear_stl_container_reference_specialization<std::forward_list<S, A>>::operator();
    void operator()(BinaryOutput & output, const std::forward_list<S, A> & data)
    {
        reference(output, std::distance(data.begin(), data.end()));
        for (const S & element : data)
        {
            reference(output, element);
        }
    }
};

template<typename C>
struct reference_specialization<StringView<const C>>
{
    void operator()(BinaryInput & input, StringView<const C> & data)
    {
        size_t size = 0;
        reference(input, size);
        auto end_of_string = input.input.begin() + size;
        data = { reinterpret_cast<const C *>(input.input.begin()), reinterpret_cast<const C *>(end_of_string) };
        input.input = { end_of_string, input.input.end() };
    }
    void operator()(BinaryOutput & output, const StringView<const C> & data)
    {
        reference(output, data.size());
        for (const auto & element : data)
        {
            reference(output, element);
        }
    }
};

template<typename T>
struct stl_set_reference_specialization
{
    void operator()(BinaryInput & input, T & data)
    {
        data.clear();
        size_t size = 0;
        reference(input, size);
        while(size --> 0)
        {
            typename T::value_type value;
            reference(input, value);
            data.emplace(std::move(value));
        }
    }
    void operator()(BinaryOutput & output, const T & data)
    {
        reference(output, data.size());
        for (const auto & element : data)
        {
            reference(output, element);
        }
    }
};
template<typename T, typename C, typename A>
struct reference_specialization<std::set<T, C, A>>
    : stl_set_reference_specialization<std::set<T, C, A>>
{
};
template<typename T, typename C, typename A>
struct reference_specialization<std::multiset<T, C, A>>
    : stl_set_reference_specialization<std::multiset<T, C, A>>
{
};
template<typename T, typename H, typename E, typename A>
struct reference_specialization<std::unordered_set<T, H, E, A>>
    : stl_set_reference_specialization<std::unordered_set<T, H, E, A>>
{
};
template<typename T, typename H, typename E, typename A>
struct reference_specialization<std::unordered_multiset<T, H, E, A>>
    : stl_set_reference_specialization<std::unordered_multiset<T, H, E, A>>
{
};


template<typename T>
struct stl_map_reference_specialization
{
    void operator()(BinaryInput & input, T & data)
    {
        data.clear();
        size_t size = 0;
        reference(input, size);
        while(size --> 0)
        {
            typename T::key_type key;
            reference(input, key);
            reference(input, data[std::move(key)]);
        }
    }
    void operator()(BinaryOutput & output, const T & data)
    {
        reference(output, data.size());
        for (const auto & element : data)
        {
            reference(output, element.first);
            reference(output, element.second);
        }
    }
};
template<typename K, typename V, typename C, typename A>
struct reference_specialization<std::map<K, V, C, A>>
    : stl_map_reference_specialization<std::map<K, V, C, A>>
{
};
template<typename K, typename V, typename H, typename E, typename A>
struct reference_specialization<std::unordered_map<K, V, H, E, A>>
    : stl_map_reference_specialization<std::unordered_map<K, V, H, E, A>>
{
};
template<typename T>
struct stl_multimap_reference_specialization
    : stl_map_reference_specialization<T>
{
    using stl_map_reference_specialization<T>::operator();
    void operator()(BinaryInput & input, T & data)
    {
        data.clear();
        size_t size = 0;
        reference(input, size);
        while(size --> 0)
        {
            typename T::key_type key;
            reference(input, key);
            typename T::mapped_type value;
            reference(input, value);
            data.emplace(std::move(key), std::move(value));
        }
    }
};
template<typename K, typename V, typename C, typename A>
struct reference_specialization<std::multimap<K, V, C, A>>
    : stl_multimap_reference_specialization<std::multimap<K, V, C, A>>
{
};
template<typename K, typename V, typename H, typename E, typename A>
struct reference_specialization<std::unordered_multimap<K, V, H, E, A>>
    : stl_multimap_reference_specialization<std::unordered_multimap<K, V, H, E, A>>
{
};
}
}
