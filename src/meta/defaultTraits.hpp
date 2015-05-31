// this file is included from newMeta.hpp
#pragma once

#include "meta/meta.hpp"
#include <memory>

namespace meta
{

#define INSTANTIATE_CHEAP_TRAITS(type)\
template<>\
struct MetaTraits<type>\
{\
    static constexpr const AccessType access_type = PASS_BY_VALUE;\
}
INSTANTIATE_CHEAP_TRAITS(bool);
INSTANTIATE_CHEAP_TRAITS(char);
INSTANTIATE_CHEAP_TRAITS(int8_t);
INSTANTIATE_CHEAP_TRAITS(uint8_t);
INSTANTIATE_CHEAP_TRAITS(int16_t);
INSTANTIATE_CHEAP_TRAITS(uint16_t);
INSTANTIATE_CHEAP_TRAITS(int32_t);
INSTANTIATE_CHEAP_TRAITS(uint32_t);
INSTANTIATE_CHEAP_TRAITS(int64_t);
INSTANTIATE_CHEAP_TRAITS(uint64_t);
INSTANTIATE_CHEAP_TRAITS(float);
INSTANTIATE_CHEAP_TRAITS(double);

#undef INSTANTIATE_CHEAP_TRAITS

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
struct MetaTraits<std::unique_ptr<T, D>>
{
    static constexpr const AccessType access_type = GET_BY_REF_SET_BY_VALUE;
};
template<typename T, typename D>
const MetaType MetaType::MetaTypeConstructor<std::unique_ptr<T, D>>::type = MetaType::RegisterPointerToStruct<std::unique_ptr<T, D>>();

}
