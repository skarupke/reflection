#pragma once
#include <algorithm>

// like std::binary_search, but returns the iterator to the element
// if it was found, and returns end otherwise
template<typename It, typename T>
It binary_find(It begin, It end, const T & value)
{
	auto lower_bound = std::lower_bound(begin, end, value);
	if (lower_bound == end || value < *lower_bound) return end;
	else return lower_bound;
}
// like std::binary_search, but returns the iterator to the element
// if it was found, and returns end otherwise
template<typename It, typename T, typename Compare>
It binary_find(It begin, It end, const T & value, const Compare & cmp)
{
	auto lower_bound = std::lower_bound(begin, end, value, cmp);
	if (lower_bound == end || cmp(value, *lower_bound)) return end;
	else return lower_bound;
}

// like std::accumulate but only one temporary gets created and the functor
// is expected to update that temporary.
template<typename It, typename T, typename Func>
T foldl(It begin, It end, T object, const Func & func)
{
	for (; begin != end; ++begin)
	{
		func(object, *begin);
	}
	return object;
}
// adaptor of the above function which takes range
template<typename Range, typename T, typename Func>
T foldl(const Range & range, T object, const Func & func)
{
	using std::begin; using std::end;
	return foldl(begin(range), end(range), std::move(object), func);
}

// will copy from the input iterator as long as the condition holds true
template<typename It, typename Out, typename Func>
Out copy_while(It from, Out to, const Func & condition)
{
	while (condition(*from))
	{
		*to++ = *from++;
	}
	return to;
}

namespace detail
{
template<typename T, typename Func>
T power_accumulate_positive(T r, T a, size_t exponent, const Func & func)
{
	while (true)
	{
		if (exponent % 2)
		{
			r = func(r, a);
			if (exponent == 1) return r;
		}
		a = func(a, a);
		exponent /= 2;
	}
}
}
template<typename T, typename Func>
T power_positive(T base, size_t exponent, const Func & func)
{
	while (!(exponent % 2))
	{
		base = func(base, base);
		exponent /= 2;
	}
	exponent /= 2;
	if (exponent == 0) return base;
	else return detail::power_accumulate_positive(base, func(base, base), exponent, func);
}
template<typename T, typename Func>
inline T power(T base, size_t exponent, const Func & func, T id)
{
	if (exponent == 0) return id;
	else return power_positive(base, exponent, func);
}
