#pragma once

#include <type_traits>
#include <string>
#include <memory>
#include <map>
#include <vector>

namespace meta
{
template<typename T>
struct is_pointer : std::is_pointer<T>
{
};
template<typename T>
struct is_pointer<std::unique_ptr<T>> : std::true_type
{
};
template<typename T>
struct remove_pointer : std::remove_pointer<T>
{
};
template<typename T>
struct remove_pointer<std::unique_ptr<T>>
{
	typedef T type;
};

template<typename T>
struct has_reserve
{
	template<typename U>
	static int check(decltype(std::declval<U>().reserve(std::declval<typename U::size_type>())) *);
	template<typename I>
	static float check(...);

	static constexpr bool value = std::is_same<int, decltype(check<T>(nullptr))>::value;
};

template<typename T>
struct is_simple_type
	: std::integral_constant<bool, std::is_same<T, std::string>::value || std::is_fundamental<T>::value>
{
};
template<typename T>
struct is_map_type
{
	template<typename U>
	static int check(typename U::key_type *, typename U::mapped_type *);
	template<typename U>
	static float check(...);

	static constexpr bool value = std::is_same<int, decltype(check<T>(nullptr, nullptr))>::value;
};
template<typename T>
struct is_list_type
{
	template<typename U>
	static int check(typename U::value_type *);
	template<typename U>
	static float check(...);

	static constexpr bool value = std::is_same<int, decltype(check<T>(nullptr))>::value;
};
template<typename T>
struct is_class_type
	: std::is_class<T>
{
};
template<typename T>
struct is_pointer_to_class
	: std::integral_constant<bool, is_pointer<T>::value && is_class_type<typename remove_pointer<T>::type>::value>
{
};

namespace detail
{
template<typename T, typename = void>
struct less_than_comparable_helper
	: std::false_type
{
};
template<typename T>
struct less_than_comparable_helper<T, typename std::enable_if<true, decltype(static_cast<void>(std::declval<T&>() < std::declval<T&>()))>::type>
	: std::true_type
{
};
}

template<typename T>
struct is_less_than_comparable
	: detail::less_than_comparable_helper<T>
{
};

template<typename T, typename A>
struct is_less_than_comparable<std::vector<T, A>>
	: meta::is_less_than_comparable<T>
{
};
template<typename K, typename V, typename C, typename A>
struct is_less_than_comparable<std::multimap<K, V, C, A> >
	: std::integral_constant<bool, meta::is_less_than_comparable<K>::value && meta::is_less_than_comparable<V>::value>
{
};

namespace detail
{
template<typename T, typename = void>
struct equality_comparable_helper
	: std::false_type
{
};

template<typename T>
struct equality_comparable_helper<T, typename std::enable_if<true, decltype(static_cast<void>(std::declval<T&>() == std::declval<T&>()))>::type>
	: std::true_type
{
};
}

template<typename T>
struct is_equality_comparable
	: detail::equality_comparable_helper<T>
{
};
template<typename T, typename A>
struct is_equality_comparable<std::vector<T, A>>
	: meta::is_equality_comparable<T>
{
};
template<typename K, typename V, typename C, typename A>
struct is_equality_comparable<std::multimap<K, V, C, A> >
	: std::integral_constant<bool, meta::is_equality_comparable<K>::value && meta::is_equality_comparable<V>::value>
{
};

namespace detail
{
template<typename T, typename Arg>
struct is_constructible_helper
{
	template<typename U, typename V, typename = decltype(::new U(std::declval<V>()))>
	static std::true_type test(int);

	template<typename, typename>
	static std::false_type test(...);

	typedef decltype(test<T, Arg>(0)) type;
};
}

template<typename T>
struct is_copy_constructible
	: detail::is_constructible_helper<T, const T &>::type
{
};
template<typename K, typename V, typename C, typename A>
struct is_copy_constructible<std::map<K, V, C, A> >
	: std::integral_constant<bool, meta::is_copy_constructible<K>::value && meta::is_copy_constructible<V>::value>
{
};
template<typename K, typename V, typename C, typename A>
struct is_copy_constructible<std::multimap<K, V, C, A> >
	: std::integral_constant<bool, meta::is_copy_constructible<K>::value && meta::is_copy_constructible<V>::value>
{
};
template<typename T, typename A>
struct is_copy_constructible<std::vector<T, A> >
	: meta::is_copy_constructible<T>
{
};

template<typename T>
struct is_move_constructible
	: detail::is_constructible_helper<T, T &&>::type
{
};

template<typename T>
struct is_copy_assignable : std::is_copy_assignable<T>
{
};

template<typename K, typename V, typename C, typename A>
struct is_copy_assignable<std::map<K, V, C, A> >
	: std::integral_constant<bool, meta::is_copy_assignable<K>::value && meta::is_copy_assignable<V>::value>
{
};
template<typename K, typename V, typename C, typename A>
struct is_copy_assignable<std::multimap<K, V, C, A> >
	: std::integral_constant<bool, meta::is_copy_assignable<K>::value && meta::is_copy_assignable<V>::value>
{
};
template<typename T, typename A>
struct is_copy_assignable<std::vector<T, A> >
	: meta::is_copy_assignable<T>
{
};

}
