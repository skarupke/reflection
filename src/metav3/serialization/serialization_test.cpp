#include "metav3/serialization/serialization_test.hpp"

#ifndef DISABLE_GTEST
namespace metav3
{

static MetaType::StructInfo::MemberCollection get_base_struct_a_members(int16_t)
{
    return { { MetaMember("a", &srlztest::base_struct_a::a) }, { } };
}
static MetaType::StructInfo::MemberCollection get_base_struct_b_members(int16_t)
{
    return { { MetaMember("b", &srlztest::base_struct_b::b) }, { } };
}
static MetaType::StructInfo::MemberCollection get_derived_struct_members(int16_t)
{
    return { { MetaMember("c", &srlztest::derived_struct::c) }, { } };
}
static MetaType::StructInfo::BaseClassCollection get_derived_struct_bases(int16_t)
{
    return
    {
        {
            BaseClass::Create<srlztest::base_struct_a, srlztest::derived_struct>(),
            BaseClass::Create<srlztest::base_struct_b, srlztest::derived_struct>()
        }
    };
}
static MetaType::StructInfo::MemberCollection get_base_struct_d_members(int16_t)
{
    return { { MetaMember("d", &srlztest::base_struct_d::d) }, { } };
}
static MetaType::StructInfo::MemberCollection get_derived_derived_members(int16_t)
{
    return { { MetaMember("e", &srlztest::derived_derived::e) }, { } };
}
static MetaType::StructInfo::BaseClassCollection get_derived_derived_bases(int16_t)
{
    return
    {
        {
            BaseClass::Create<srlztest::base_struct_d, srlztest::derived_derived>(),
            BaseClass::Create<srlztest::derived_struct, srlztest::derived_derived>()
        }
    };
}
static MetaType::StructInfo::MemberCollection get_pointer_to_struct_base_members(int16_t)
{
    return { { MetaMember("a", &srlztest::pointer_to_struct_base::a) }, { } };
}
static MetaType::StructInfo::MemberCollection get_pointer_to_struct_offsetting_base_members(int16_t)
{
    return { { MetaMember("c", &srlztest::pointer_to_struct_offsetting_base::c) }, { } };
}
static MetaType::StructInfo::MemberCollection get_pointer_to_struct_derived_members(int16_t)
{
    return { { MetaMember("b", &srlztest::pointer_to_struct_derived::b) }, { } };
}
static MetaType::StructInfo::BaseClassCollection get_pointer_to_struct_derived_bases(int16_t)
{
    return
    {
        {
            BaseClass::Create<srlztest::pointer_to_struct_offsetting_base, srlztest::pointer_to_struct_derived>(),
            BaseClass::Create<srlztest::pointer_to_struct_base, srlztest::pointer_to_struct_derived>()
        }
    };
}
static MetaType::StructInfo::MemberCollection get_pointer_member_members(int16_t)
{
    return { { MetaMember("ptr", &srlztest::pointer_member::ptr) }, { } };
}
static bool conditional_member_b_condition(const srlztest::conditional_member & object)
{
    return !object.which;
}
static MetaType::StructInfo::MemberCollection get_conditional_member_members(int16_t)
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
static MetaType::StructInfo::MemberCollection get_type_erasure_member_members(int16_t)
{
    return
    {
        {
            MetaMember("a", &srlztest::type_erasure_member::a)
        },
        { }
    };
}

template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::an_enum>::type = MetaType::RegisterEnum<srlztest::an_enum>({ { srlztest::EnumValueOne, "EnumValueOne" }, { srlztest::EnumValueTwo, "EnumValueTwo" } });
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::base_struct_a>::type = MetaType::RegisterStruct<srlztest::base_struct_a>("srlz_base_struct_a", 0, &get_base_struct_a_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::base_struct_b>::type = MetaType::RegisterStruct<srlztest::base_struct_b>("srlz_base_struct_b", 0, &get_base_struct_b_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::derived_struct>::type = MetaType::RegisterStruct<srlztest::derived_struct>("srlz_derived_struct", 0, &get_derived_struct_members, &get_derived_struct_bases);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::base_struct_d>::type = MetaType::RegisterStruct<srlztest::base_struct_d>("srlz_base_struct_d", 0, &get_base_struct_d_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::derived_derived>::type = MetaType::RegisterStruct<srlztest::derived_derived>("srlz_derived_derived", 0, &get_derived_derived_members, &get_derived_derived_bases);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::pointer_to_struct_base>::type = MetaType::RegisterStruct<srlztest::pointer_to_struct_base>("srlz_pointer_to_struct_base", 0, &get_pointer_to_struct_base_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::pointer_to_struct_offsetting_base>::type = MetaType::RegisterStruct<srlztest::pointer_to_struct_offsetting_base>("srlz_pointer_to_struct_offsetting_base", 0, &get_pointer_to_struct_offsetting_base_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::pointer_to_struct_derived>::type = MetaType::RegisterStruct<srlztest::pointer_to_struct_derived>("srlz_pointer_to_struct_derived", 0, &get_pointer_to_struct_derived_members, &get_pointer_to_struct_derived_bases);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::pointer_member>::type = MetaType::RegisterStruct<srlztest::pointer_member>("srlz_pointer_member", 0, &get_pointer_member_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::conditional_member>::type = MetaType::RegisterStruct<srlztest::conditional_member>("srlz_conditional_member", 0, &get_conditional_member_members);
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::type_erasure>::type = MetaType::RegisterTypeErasure<srlztest::type_erasure>();
template<>
const MetaType MetaType::MetaTypeConstructor<srlztest::type_erasure_member>::type = MetaType::RegisterStruct<srlztest::type_erasure_member>("srlz_type_erasure", 0, &get_type_erasure_member_members);
static MetaType::TypeErasureInfo::SupportedType<srlztest::type_erasure, srlztest::type_erasure_member> support_type_erasure;
}
#endif
