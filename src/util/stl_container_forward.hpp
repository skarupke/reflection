#pragma once

namespace std
{
template<typename T>
class allocator;

template<typename T, size_t Size>
struct array;
template<typename T, typename A>
class deque;
template<typename T, typename A>
class list;
template<typename T, typename A>
class forward_list;
template<typename T, typename A>
class vector;

template<typename K, typename V, typename C, typename A>
class map;
template<typename K, typename V, typename C, typename A>
class multimap;
template<typename K, typename C, typename A>
class multiset;
template<typename K, typename C, typename A>
class set;

template<typename T>
struct hash;

template<typename K, typename V, typename H, typename E, typename A>
class unordered_map;
template<typename K, typename V, typename H, typename E, typename A>
class unordered_multimap;
template<typename K, typename H, typename E, typename A>
class unordered_multiset;
template<typename K, typename H, typename E, typename A>
class unordered_set;

template<typename T, typename C>
class stack;
template<typename T, typename C>
class queue;
template<typename T, typename Container, typename Compare>
class priority_queue;

template<typename C, typename T, typename A>
class basic_string;
template<typename T>
struct char_traits;
typedef basic_string<char, char_traits<char>, allocator<char>> string;
typedef basic_string<wchar_t, char_traits<wchar_t>, allocator<wchar_t>> wstring;
typedef basic_string<char16_t, char_traits<char16_t>, allocator<char16_t>> u16string;
typedef basic_string<char32_t, char_traits<char32_t>, allocator<char32_t>> u32string;

}

