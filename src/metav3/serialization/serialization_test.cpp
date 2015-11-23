#include "metav3/serialization/serialization_test.hpp"

#ifndef DISABLE_GTEST
#include "metafast/metafast.hpp"

REFLECT_CLASS_START(srlztest::base_struct_a, 0)
    REFLECT_MEMBER(a);
REFLECT_CLASS_END()

REFLECT_CLASS_START(srlztest::base_struct_b, 0)
    REFLECT_MEMBER(b);
REFLECT_CLASS_END()

REFLECT_CLASS_START(srlztest::derived_struct, 0)
    REFLECT_BASE(srlztest::base_struct_a);
    REFLECT_BASE(srlztest::base_struct_b);
    REFLECT_MEMBER(c);
REFLECT_CLASS_END()

REFLECT_ABSTRACT_CLASS_START(srlztest::base_struct_d, 0)
    REFLECT_MEMBER(d);
REFLECT_CLASS_END()

REFLECT_CLASS_START(srlztest::derived_derived, 0)
    REFLECT_BASE(srlztest::base_struct_d);
    REFLECT_BASE(srlztest::derived_struct);
    REFLECT_MEMBER(e);
REFLECT_CLASS_END()

REFLECT_ABSTRACT_CLASS_START(srlztest::pointer_to_struct_base, 0)
    REFLECT_MEMBER(a);
REFLECT_CLASS_END()

REFLECT_ABSTRACT_CLASS_START(srlztest::pointer_to_struct_offsetting_base, 0)
    REFLECT_MEMBER(c);
REFLECT_CLASS_END()

REFLECT_CLASS_START(srlztest::pointer_to_struct_derived, 0)
    REFLECT_BASE(srlztest::pointer_to_struct_offsetting_base);
    REFLECT_BASE(srlztest::pointer_to_struct_base);
    REFLECT_MEMBER(b);
REFLECT_CLASS_END()

REFLECT_CLASS_START(srlztest::pointer_member, 0)
    REFLECT_MEMBER(ptr);
REFLECT_CLASS_END()

namespace metav3
{
#ifdef META_SUPPORTS_CONDITIONAL_MEMBER
static bool conditional_member_b_condition(const srlztest::conditional_member & object)
{
    return !object.which;
}
static MetaType::StructInfo::MemberCollection get_conditional_member_members(int8_t)
{
    return
    {
        {
            MetaMember("which", &srlztest::conditional_member::which)
        },
        {
            MetaConditionalMember::CreateFromMember<srlztest::conditional_member, &srlztest::conditional_member::which>(MetaMember("a", &srlztest::conditional_member::a)),
            MetaConditionalMember::CreateFromFunction<srlztest::conditional_member, &conditional_member_b_condition>(MetaMember("b", &srlztest::conditional_member::b))
        }
    };
}
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::conditional_member>::type = MetaType::RegisterStruct<srlztest::conditional_member>("srlz_conditional_member", 0, &get_conditional_member_members);
#endif
#ifdef META_SUPPORTS_TYPE_ERASURE
static MetaType::StructInfo::MemberCollection get_type_erasure_member_members(int8_t)
{
    return
    {
        {
            MetaMember("a", &srlztest::type_erasure_member::a)
        },
        {}
    };
}
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::type_erasure>::type = MetaType::RegisterTypeErasure<srlztest::type_erasure>();
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::type_erasure_member>::type = MetaType::RegisterStruct<srlztest::type_erasure_member>("srlz_type_erasure", 0, &get_type_erasure_member_members);
static MetaType::TypeErasureInfo::SupportedType<srlztest::type_erasure, srlztest::type_erasure_member> support_type_erasure;
#endif

template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::an_enum>::type = MetaType::RegisterEnum<srlztest::an_enum>({ { srlztest::EnumValueOne, "EnumValueOne" }, { srlztest::EnumValueTwo, "EnumValueTwo" } });
}
#endif
