#pragma once

#include <type_traits>

namespace metav3
{
template<typename T>
struct remove_pointer : std::remove_pointer<T>
{
};
template<typename T>
struct remove_pointer<std::unique_ptr<T>>
{
    typedef T type;
};
}
