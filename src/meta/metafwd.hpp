#pragma once

namespace meta
{

struct MetaReference;
struct ConstMetaReference;

enum AccessType
{
	PASS_BY_REFERENCE,
    PASS_BY_VALUE,
    GET_BY_REF_SET_BY_VALUE,
};

template<typename, typename = void>
struct MetaTraits;

struct MetaType;
template<typename T>
const MetaType & GetMetaType();

}
