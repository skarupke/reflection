#pragma once

#include "metav3/metav3.hpp"

namespace metav3
{

template<typename T, size_t Size>
struct MetaType::MetaTypeConstructor<T[Size]>
{
    static const MetaType type;
};
template<typename T, size_t Size>
const MetaType MetaType::MetaTypeConstructor<T[Size]>::type = MetaType::RegisterArray<T[Size]>(GetMetaType<T>(), Size);
template<typename T>
struct MetaType::MetaTypeConstructor<T *>
{
    static const MetaType type;
};
template<typename T>
const MetaType MetaType::MetaTypeConstructor<T *>::type = MetaType::RegisterPointerToStruct<T *>();
template<typename T, typename D>
struct MetaType::MetaTypeConstructor<std::unique_ptr<T, D>>
{
    static const MetaType type;
};
template<typename T, typename D>
const MetaType MetaType::MetaTypeConstructor<std::unique_ptr<T, D>>::type = MetaType::RegisterPointerToStruct<std::unique_ptr<T, D>>();

}
