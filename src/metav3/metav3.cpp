#include "metav3/metav3.hpp"

#include "debug/assert.hpp"
#include <stdexcept>
#include <algorithm>
#include <util/algorithm.hpp>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <cstring>

namespace metav3
{

namespace detail
{
void assert_range_size_and_alignment(ArrayView<unsigned char> memory, size_t size, size_t alignment)
{
    static_cast<void>(memory); static_cast<void>(size); static_cast<void>(alignment);
    RAW_ASSERT(memory.size() == size && (reinterpret_cast<uintptr_t>(memory.begin()) % alignment) == 0);
}
}

MetaType::MetaType(GeneralInformation general, AllTypesThatExist category)
    : category(category), simple_info(), general(std::move(general))
{
}
MetaType::MetaType(StringInfo string_info, GeneralInformation general)
    : category(String), string_info(std::move(string_info)), general(std::move(general))
{
}
MetaType::MetaType(GeneralInformation general, std::map<int32_t, std::string> enum_values)
    : category(Enum), enum_info(std::move(enum_values)), general(std::move(general))
{
}
MetaType::MetaType(ListInfo list_info, GeneralInformation general)
    : category(List), list_info(std::move(list_info)), general(std::move(general))
{
}
MetaType::MetaType(ArrayInfo array_info, GeneralInformation general)
    : category(Array), array_info(std::move(array_info)), general(std::move(general))
{
    if (array_info.array_size == 0) RAW_THROW(std::runtime_error("invalid array size. you can't register a zero size array"));
}
MetaType::MetaType(SetInfo set_info, GeneralInformation general)
    : category(Set), set_info(std::move(set_info)), general(std::move(general))
{
}
MetaType::MetaType(MapInfo map_info, GeneralInformation general)
    : category(Map), map_info(std::move(map_info)), general(std::move(general))
{
}
namespace
{
struct GlobalStructStorage
{
    const MetaType & GetByName(const ReflectionHashedString & name) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = by_name.find(name);
        if (found == by_name.end()) RAW_THROW(std::runtime_error("tried to get a type that wasn't registered"));
        else return *found->second;
    }
    const MetaType & GetByType(const std::type_info & type) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = by_type.find(std::type_index(type));
        if (found == by_type.end()) RAW_THROW(std::runtime_error("tried to get a type that wasn't registered"));
        else return *found->second;
    }
    const ReflectionHashedString & GetStoredName(StringView<const char> name) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = stored_names.find(name);
        if (found == stored_names.end()) RAW_THROW(std::runtime_error("tried to get a type that wasn't registered"));
        return found->second;
    }
    const MetaType & GetByHash(uint32_t hash) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = by_hash.find(hash);
        if (found == by_hash.end()) RAW_THROW(std::runtime_error("tried to get a type that wasn't registered"));
        return *found->second;
    }
    void AddType(const MetaType & to_add)
    {
        RAW_ASSERT(to_add.category == MetaType::Struct);
        std::lock_guard<std::mutex> lock(mutex);
        const ReflectionHashedString & name = to_add.GetStructInfo()->GetName();
        RAW_VERIFY(by_name.emplace(name, &to_add).second); // this will trigger if you register two structs with the same name
        RAW_VERIFY(by_type.emplace(to_add.GetTypeInfo(), &to_add).second); // this will trigger if you register a struct under two different names
        RAW_VERIFY(stored_names.emplace(name.get(), name).second);
        RAW_VERIFY(by_hash.emplace(name.get_hash(), &to_add).second); // this will trigger if there's a hash collision in the registered names
    }

private:
    mutable std::mutex mutex;
    std::map<ReflectionHashedString, const MetaType *> by_name;
    std::map<std::type_index, const MetaType *> by_type;
    std::unordered_map<StringView<const char>, ReflectionHashedString> stored_names;
    std::unordered_map<uint32_t, const MetaType *> by_hash;
};
GlobalStructStorage & GetGlobalStructStorage()
{
    static GlobalStructStorage storage;
    return storage;
}
// this one is intentionally statically allocated so that clang's address sanitizer
// will complain if I try to use it too early. the global initialization happens in
// two stages: first every type adds itself using the function above, then they get
// information from other types using the static reference below. by organizing the
// code like this, address sanitizer will complain if I get it wrong
static GlobalStructStorage & global_struct_storage = GetGlobalStructStorage();
}
MetaType::MetaType(StructInfo struct_info, GeneralInformation general)
    : category(Struct), struct_info(std::move(struct_info)), general(std::move(general))
{
    GetGlobalStructStorage().AddType(*this);
}
MetaType::MetaType(PointerToStructInfo pointer_to_struct_info, GeneralInformation general)
    : category(PointerToStruct), pointer_to_struct_info(std::move(pointer_to_struct_info)), general(std::move(general))
{
}
MetaType::MetaType(TypeErasureInfo type_erasure_info, GeneralInformation general)
    : category(TypeErasure), type_erasure_info(std::move(type_erasure_info)), general(std::move(general))
{
}

MetaType::MetaType(MetaType && other)
    : category(std::move(other.category)), general(std::move(other.general))
{
    switch(category)
    {
    case Bool:
    case Char:
    case Int8:
    case Uint8:
    case Int16:
    case Uint16:
    case Int32:
    case Uint32:
    case Int64:
    case Uint64:
    case Float:
    case Double:
        new (&simple_info) SimpleInfo(std::move(other.simple_info));
        break;
    case String:
        new (&string_info) StringInfo(std::move(other.string_info));
        break;
    case Enum:
        new (&enum_info) EnumInfo(std::move(other.enum_info));
        break;
    case List:
        new (&list_info) ListInfo(std::move(other.list_info));
        break;
    case Array:
        new (&array_info) ArrayInfo(std::move(other.array_info));
        break;
    case Set:
        new (&set_info) SetInfo(std::move(other.set_info));
        break;
    case Map:
        new (&map_info) MapInfo(std::move(other.map_info));
        break;
    case Struct:
        new (&struct_info) StructInfo(std::move(other.struct_info));
        break;
    case PointerToStruct:
        new (&pointer_to_struct_info) PointerToStructInfo(std::move(other.pointer_to_struct_info));
        break;
    case TypeErasure:
        new (&type_erasure_info) TypeErasureInfo(std::move(other.type_erasure_info));
        break;
    }
}

MetaType::~MetaType()
{
    switch(category)
    {
    case Bool:
    case Char:
    case Int8:
    case Uint8:
    case Int16:
    case Uint16:
    case Int32:
    case Uint32:
    case Int64:
    case Uint64:
    case Float:
    case Double:
        simple_info.~SimpleInfo();
        break;
    case String:
        string_info.~StringInfo();
        break;
    case Enum:
        enum_info.~EnumInfo();
        break;
    case List:
        list_info.~ListInfo();
        break;
    case Array:
        array_info.~ArrayInfo();
        break;
    case Set:
        set_info.~SetInfo();
        break;
    case Map:
        map_info.~MapInfo();
        break;
    case Struct:
        struct_info.~StructInfo();
        break;
    case PointerToStruct:
        pointer_to_struct_info.~PointerToStructInfo();
        break;
    case TypeErasure:
        type_erasure_info.~TypeErasureInfo();
        break;
    }
}
MetaType::StringInfo::StringInfo(StringView<const char> (*get_as_range)(ConstMetaReference), void (*set_from_range)(MetaReference, StringView<const char>))
    : get_as_range(get_as_range), set_from_range(set_from_range)
{
}

MetaType::EnumInfo::EnumInfo(std::map<int32_t, std::string> values)
    : values(std::move(values))
{
    for (const auto & pair : this->values)
    {
        int_values[pair.second] = pair.first;
    }
}

MetaType::ArrayInfo::ArrayInfo(const MetaType & value_type, size_t array_size, MetaRandomAccessIterator (*begin)(MetaReference))
    : begin(begin), value_type(value_type), array_size(array_size)
{
}
MetaType::StructInfo::StructInfo(ReflectionHashedString name, int8_t current_version, GetInfoFunction get_both)
    : name(std::move(name)), current_version(current_version), get_both(get_both)
{
}

MetaType::PointerToStructInfo::PointerToStructInfo(const MetaType & target_type, pointer_function get_as_pointer, assign_function assign)
    : target_type(target_type), get_as_pointer(get_as_pointer), assign(assign)
{
}

ptrdiff_t MetaType::PointerToStructInfo::get_offset_for_struct(const MetaType & struct_type) const
{
    return cached_offsets.get(&struct_type, [&](const MetaType *)
    {
        const MetaType::StructInfo::BaseClassCollection & bases = struct_type.GetStructInfo()->GetAllBaseClasses(struct_type.GetStructInfo()->GetCurrentHeaders());
        auto found_base = std::find_if(bases.bases.begin(), bases.bases.end(), [&](const BaseClass & base)
        {
            return &base.GetBase() == &target_type;
        });
        if (found_base == bases.bases.end()) RAW_THROW(std::runtime_error("a '" + struct_type.GetStructInfo()->GetName().get() + "' was stored in a pointer to '" + target_type.GetStructInfo()->GetName().get() + "' even though it doesn't have '" + target_type.GetStructInfo()->GetName().get() + "' registered as a base class"));
        else return found_base->GetOffset();
    });
}

MetaReference MetaType::PointerToStructInfo::cast_reference(const MetaType & struct_type, MetaReference to_cast) const
{
    if (&struct_type == &to_cast.GetType()) return to_cast;
    else
    {
        unsigned char * new_begin = to_cast.GetMemory().begin() - get_offset_for_struct(struct_type);
        unsigned char * new_end = new_begin + struct_type.GetSize();
        return MetaReference(struct_type, { new_begin, new_end });
    }
}
MetaReference MetaType::PointerToStructInfo::AssignNew(MetaReference pointer, const MetaType & type) const
{
    allocate_pointer memory = type.Allocate();
    MetaReference as_reference(type, { memory.get(), memory.get() + type.GetSize() });
    type.Construct(as_reference.GetMemory());
    Assign(pointer, as_reference);
    memory.release();
    return as_reference;
}

MetaType::TypeErasureInfo::TypeErasureInfo(const MetaType & self, target_type_function target_type)
    : self(&self), target_type(std::move(target_type))
{
}
const MetaType * MetaType::TypeErasureInfo::TargetType(ConstMetaReference reference) const
{
    return target_type(reference);
}
MetaReference MetaType::TypeErasureInfo::Target(MetaReference reference) const
{
    auto found = GetSupportedTypes().find(std::make_pair(self, TargetType(reference)));
    RAW_ASSERT(found != GetSupportedTypes().end());
    return found->second.target(reference);
}
MetaReference MetaType::TypeErasureInfo::Assign(MetaReference type_erasure, MetaReference && target) const
{
    auto found = GetSupportedTypes().find(std::make_pair(self, &target.GetType()));
    RAW_ASSERT(found != GetSupportedTypes().end());
    return found->second.assign(type_erasure, std::move(target));
}
MetaReference MetaType::TypeErasureInfo::AssignNew(MetaReference pointer, const MetaType & type) const
{
    unsigned char * stack_memory = static_cast<unsigned char *>(alloca(type.GetSize()));
    MetaOwningMemory memory(type, { stack_memory, stack_memory + type.GetSize() });
    type.Construct(memory.GetMemory());
    return Assign(pointer, std::move(memory));
}
std::map<std::pair<const MetaType *, const MetaType *>, MetaType::TypeErasureInfo::RegisteredSupportedType> & MetaType::TypeErasureInfo::GetSupportedTypes()
{
    static std::map<std::pair<const MetaType *, const MetaType *>, RegisteredSupportedType> supported_types;
    return supported_types;
}

const MetaType & MetaType::GetStructType(const ReflectionHashedString & name)
{
    return global_struct_storage.GetByName(name);
}
const MetaType & MetaType::GetStructType(const std::type_info & type)
{
    return global_struct_storage.GetByType(type);
}
const ReflectionHashedString & MetaType::GetRegisteredStructName(StringView<const char> name)
{
    return global_struct_storage.GetStoredName(name);
}
const MetaType & MetaType::GetRegisteredStruct(uint32_t hash)
{
    return global_struct_storage.GetByHash(hash);
}

int32_t MetaType::EnumInfo::GetAsInt(ConstMetaReference reference) const
{
    RAW_ASSERT(reference.GetType().GetEnumInfo() == this);
    return *reinterpret_cast<const int32_t *>(reference.GetMemory().begin());
}
const std::string & MetaType::EnumInfo::GetAsString(ConstMetaReference reference) const
{
    RAW_ASSERT(reference.GetType().GetEnumInfo() == this);
    auto found = values.find(GetAsInt(reference));
    if (found != values.end()) return found->second;
    else RAW_THROW(std::runtime_error("invalid value for enum"));
}
void MetaType::EnumInfo::SetFromString(MetaReference to_fill, const std::string & value) const
{
    auto found = int_values.find(value);
    if (found == int_values.end()) RAW_THROW(std::runtime_error("invalid text value for enum"));
    else
    {
        *reinterpret_cast<int32_t *>(to_fill.GetMemory().begin()) = found->second;
    }
}

const ClassHeaderList & MetaType::StructInfo::GetCurrentHeaders() const
{
    return current_headers.get([&]
    {
        ClassHeaderList headers;
        headers.emplace_back(GetName(), GetCurrentVersion());
        auto add_base = [](ClassHeaderList & lhs, const BaseClass & rhs)
        {
            const ClassHeaderList & base = rhs.GetBase().GetStructInfo()->GetCurrentHeaders();
#			ifdef DEBUG
                for (const MetaClassHeader & header : lhs)
                {
                    RAW_ASSERT(base.find(header.GetClassName()) == base.end(), "duplicate base class. I don't support this at the moment");
                }
#			endif
            lhs.insert(lhs.end(), base.begin(), base.end());
        };
        return foldl(GetDirectBaseClasses(GetCurrentVersion()).bases, std::move(headers), add_base);
    });
}

MetaType::StructInfo::AllMemberCollection::AllMemberCollection(MemberCollection direct_members, BaseClass::BaseMemberCollection base_members)
    : direct_members(std::move(direct_members)), base_members(std::move(base_members))
{
}

const MetaType::StructInfo::BaseClassCollection & MetaType::StructInfo::GetDirectBaseClasses(int8_t version) const
{
    if (version > GetCurrentVersion()) RAW_THROW(std::runtime_error("wrong version"));
    return direct_bases.get(version, [&](int8_t version)
    {
        return get_both(version).bases;
    });
}

static ClassHeaderList GetRelevantHeaders(const ClassHeaderList & all_headers, const MetaType & struct_type)
{
    const MetaType::StructInfo * info = struct_type.GetStructInfo();
    if (!info) RAW_THROW(std::runtime_error("wrong argument for GetRelevantHeaders"));
    ClassHeaderList result;
    result.reserve(all_headers.size());
    const ClassHeaderList & current_headers = info->GetCurrentHeaders();
    std::copy_if(all_headers.begin(), all_headers.end(), std::back_inserter(result), [&](const ClassHeader & header)
    {
        return header.GetClassName() == info->GetName() || current_headers.find(header.GetClassName()) != current_headers.end();
    });
    return result;
}

const MetaType::StructInfo::BaseClassCollection & MetaType::StructInfo::GetAllBaseClasses(const ClassHeaderList & versions) const
{
    return all_bases.get(versions, [&](const ClassHeaderList & versions)
    {
        auto found = versions.find(GetName());
        if (found == versions.end()) RAW_THROW(std::runtime_error("this struct was not in the class header list"));
        const BaseClassCollection & direct_bases = GetDirectBaseClasses(found->GetVersion());

        auto add_to_bases = [&](std::vector<BaseClass> & lhs, const BaseClass & rhs)
        {
            const BaseClassCollection & base_bases = rhs.GetBase().GetStructInfo()->GetAllBaseClasses(GetRelevantHeaders(versions, rhs.GetBase()));
            for (const BaseClass & base_base : base_bases.bases)
            {
                lhs.emplace_back(BaseClass::Combine(base_base, rhs));
            }
        };
        return BaseClassCollection(foldl(direct_bases.bases, direct_bases.bases, add_to_bases));
    });
}

const MetaType::StructInfo::MemberCollection & MetaType::StructInfo::GetDirectMembers(int8_t version) const
{
    if (version > GetCurrentVersion()) RAW_THROW(std::runtime_error("wrong version"));
    return direct_members.get(version, [&](int8_t version)
    {
        return get_both(version).members;
    });
}

const MetaType::StructInfo::AllMemberCollection & MetaType::StructInfo::GetAllMembers(const ClassHeaderList & versions) const
{
    return all_members.get(versions, [&](const ClassHeaderList & versions)
    {
        auto found_version = versions.find(GetName());
        if (found_version == versions.end())
        {
            RAW_THROW(std::runtime_error("this struct was not in the class header list"));
        }
        auto add_to_member_collection = [&](BaseClass::BaseMemberCollection & lhs, const BaseClass & rhs)
        {
            const BaseClass::BaseMemberCollection & base_members = rhs.GetMembers(GetRelevantHeaders(versions, rhs.GetBase()));

            lhs.members.insert(lhs.members.end(), base_members.members.begin(), base_members.members.end());
            lhs.conditional_members.insert(lhs.conditional_members.end(), base_members.conditional_members.begin(), base_members.conditional_members.end());
        };
        return AllMemberCollection(GetDirectMembers(found_version->GetVersion()),
                                            foldl(GetDirectBaseClasses(found_version->GetVersion()).bases, BaseClass::BaseMemberCollection(), add_to_member_collection)
                                            );
    });
}

const BaseClass::BaseMemberCollection & BaseClass::GetMembers(const ClassHeaderList & versions) const
{
    return members.get(versions, [&](const ClassHeaderList & versions)
    {
        struct AddToBaseMemberCollection
        {
            AddToBaseMemberCollection(const ClassHeaderList & versions, ptrdiff_t offset)
                : versions(versions), offset(offset)
            {
            }
            void operator()(BaseMemberCollection & lhs, const MetaMember & rhs) const
            {
                lhs.members.emplace_back(rhs, offset);
            }
            void operator()(BaseMemberCollection & lhs, const MetaConditionalMember & rhs) const
            {
                lhs.conditional_members.emplace_back(rhs, offset);
            }
            void operator()(BaseMemberCollection & lhs, const BaseMemberCollection::BaseMember & rhs) const
            {
                lhs.members.emplace_back(rhs.member, rhs.offset + offset);
            }
            void operator()(BaseMemberCollection & lhs, const BaseMemberCollection::BaseConditionalMember & rhs) const
            {
                lhs.conditional_members.emplace_back(rhs.member, rhs.offset + offset);
            }
            void operator()(BaseMemberCollection & lhs, const BaseClass & rhs) const
            {
                const BaseMemberCollection & base_members = rhs.GetMembers(GetRelevantHeaders(versions, rhs.GetBase()));
                lhs = foldl(base_members.members, std::move(lhs), *this);
                lhs = foldl(base_members.conditional_members, std::move(lhs), *this);
            }

            const ClassHeaderList & versions;
            ptrdiff_t offset;
        };

        auto found_base = versions.find(base->GetStructInfo()->GetName());
        if (found_base == versions.end())
        {
            RAW_THROW(std::runtime_error("this struct was not in the class header list"));
        }
        const MetaType::StructInfo::MemberCollection & base_members = base->GetStructInfo()->GetDirectMembers(found_base->GetVersion());
        AddToBaseMemberCollection adder(versions, offset);

        return foldl(base->GetStructInfo()->GetDirectBaseClasses(found_base->GetVersion()).bases,
                    foldl(base_members.conditional_members,
                            foldl(base_members.members, BaseMemberCollection(), adder),
                                    adder),
                            adder);
    });
}


#define CreateSimpleType(cpp_type, meta_tag)\
template<>\
const MetaType MetaType::MetaTypeConstructor<cpp_type>::type = MetaType::RegisterSimple<cpp_type>(MetaType::meta_tag)
CreateSimpleType(bool, Bool);
CreateSimpleType(char, Char);
CreateSimpleType(int8_t, Int8);
CreateSimpleType(uint8_t, Uint8);
CreateSimpleType(int16_t, Int16);
CreateSimpleType(uint16_t, Uint16);
CreateSimpleType(int, Int32);
CreateSimpleType(unsigned, Uint32);
CreateSimpleType(long, Int64);
CreateSimpleType(unsigned long, Uint64);
CreateSimpleType(long long, Int64);
CreateSimpleType(unsigned long long, Uint64);
CreateSimpleType(float, Float);
CreateSimpleType(double, Double);
#undef CreateSimpleType
template<>
const MetaType MetaType::MetaTypeConstructor<StringView<const char> >::type = MetaType::RegisterString<StringView<const char> >();
template<>
const MetaType MetaType::MetaTypeConstructor<std::string>::type = MetaType::RegisterString<std::string>();
template<>
struct MetaType::StringInfo::Specialization<std::string>
{
    static StringView<const char> GetAsRange(ConstMetaReference ref)
    {
        return ref.Get<std::string>();
    }
    static void SetFromRange(MetaReference ref, StringView<const char> range)
    {
        ref.Get<std::string>().assign(range.begin(), range.end());
    }
};

const MetaType::SimpleInfo * MetaType::GetSimpleInfo() const
{
    switch(category)
    {
    case Bool:
    case Char:
    case Int8:
    case Uint8:
    case Int16:
    case Uint16:
    case Int32:
    case Uint32:
    case Int64:
    case Uint64:
    case Float:
    case Double:
        return &simple_info;
    default:
        return nullptr;
    }
}
const MetaType::StringInfo * MetaType::GetStringInfo() const
{
    if (category == String) return &string_info;
    else return nullptr;
}
const MetaType::EnumInfo * MetaType::GetEnumInfo() const
{
    if (category == Enum) return &enum_info;
    else return nullptr;
}
const MetaType::ListInfo * MetaType::GetListInfo() const
{
    if (category == List) return &list_info;
    else return nullptr;
}
const MetaType::ArrayInfo * MetaType::GetArrayInfo() const
{
    if (category == Array) return &array_info;
    else return nullptr;
}
const MetaType::SetInfo * MetaType::GetSetInfo() const
{
    if (category == Set) return &set_info;
    else return nullptr;
}
const MetaType::MapInfo * MetaType::GetMapInfo() const
{
    if (category == Map) return &map_info;
    else return nullptr;
}
const MetaType::StructInfo * MetaType::GetStructInfo() const
{
    if (category == Struct) return &struct_info;
    else return nullptr;
}
const MetaType::PointerToStructInfo * MetaType::GetPointerToStructInfo() const
{
    if (category == PointerToStruct) return &pointer_to_struct_info;
    else return nullptr;
}
const MetaType::TypeErasureInfo * MetaType::GetTypeErasureInfo() const
{
    if (category == TypeErasure) return &type_erasure_info;
    else return nullptr;
}

MetaType::StructInfo::MemberCollection::MemberCollection(std::vector<MetaMember> members, std::vector<MetaConditionalMember> conditional_members)
    : members(std::move(members)), conditional_members(std::move(conditional_members))
{
}
MetaType::StructInfo::BaseClassCollection::BaseClassCollection(std::vector<BaseClass> bases)
    : bases(std::move(bases))
{
    for (const BaseClass & base : this->bases)
    {
        if (!base.GetBase().GetStructInfo())
        {
            RAW_THROW(std::runtime_error("tried to register a type as base that is not a struct"));
        }
    }
}

MetaConditionalMember::MetaConditionalMember(MetaMember member, bool (*condition)(ConstMetaReference object))
    : member(std::move(member)), condition(condition)
{
    RAW_ASSERT(condition);
}

MetaConditionalMember::Condition::Condition(ConditionFunction condition)
    : condition(condition)
{
}


ClassHeader::ClassHeader(ReflectionHashedString class_name, int8_t version)
    : class_name(std::move(class_name)), version(version)
{
}

bool ClassHeader::operator<(const ClassHeader & other) const
{
    if (class_name.get_hash() < other.class_name.get_hash()) return true;
    else if (class_name.get_hash() > other.class_name.get_hash()) return false;
    else return version < other.version;
}
bool ClassHeader::operator<=(const ClassHeader & other) const
{
    return !(*this > other);
}
bool ClassHeader::operator>(const ClassHeader & other) const
{
    return other < *this;
}
bool ClassHeader::operator>=(const ClassHeader & other) const
{
    return !(*this < other);
}
bool ClassHeader::operator==(const ClassHeader & other) const
{
    return version == other.version && class_name == other.class_name;
}
bool ClassHeader::operator!=(const ClassHeader & other) const
{
    return !(*this == other);
}

ClassHeaderList::iterator ClassHeaderList::find(const ClassHeader & header)
{
    return std::find(begin(), end(), header);
}
ClassHeaderList::iterator ClassHeaderList::find(const ReflectionHashedString & class_name)
{
    return std::find_if(begin(), end(), [&](const ClassHeader & header)
    {
        return header.GetClassName() == class_name;
    });
}

ClassHeaderList::const_iterator ClassHeaderList::find(const ClassHeader & header) const
{
    return const_cast<ClassHeaderList &>(*this).find(header);
}
ClassHeaderList::const_iterator ClassHeaderList::find(const ReflectionHashedString & class_name) const
{
    return const_cast<ClassHeaderList &>(*this).find(class_name);
}


BaseClass::BaseClass(const MetaType & base, const MetaType & derived, ptrdiff_t offset)
    : base(&base), derived(&derived), offset(offset)
{
}
BaseClass BaseClass::Combine(const BaseClass & basebase, const BaseClass & base)
{
    if (basebase.derived != base.base)
    {
        RAW_THROW(std::runtime_error("invalid argument to MetaBaseClass::combine. basebase.derived needs to be equal to base.base"));
    }
    else
    {
        return BaseClass(*basebase.base, *base.derived, basebase.offset + base.offset);
    }
}

} // end namespace meta

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include "metav3/metav3_stl.hpp"
#include <algorithm>
#include "os/memoryManager.hpp"
#include <array>

using namespace metav3;

namespace
{
TEST(new_meta, int_list)
{
    std::vector<int> a_list = { 1, 2, 3 };
    MetaReference variant(a_list);
    ASSERT_EQ(MetaType::List, variant.GetType().category);
    const MetaType::ListInfo * info = variant.GetType().GetListInfo();
    ASSERT_EQ(3u, info->size(variant));
    {
        auto it = info->begin(variant);
        ASSERT_EQ(MetaType::Int32, it->GetType().category);
        ASSERT_EQ(1, it->Get<int>());
        ++it;
        ASSERT_EQ(MetaType::Int32, it->GetType().category);
        ASSERT_EQ(2, it->Get<int>());
        ++it;
        ASSERT_EQ(MetaType::Int32, it->GetType().category);
        ASSERT_EQ(3, it->Get<int>());
        ++it;
        ASSERT_EQ(info->end(variant), it);
    }
}
TEST(new_meta, deque)
{
    std::deque<int> a_list = { 1, 2, 3 };
    MetaReference variant(a_list);
    ASSERT_EQ(MetaType::List, variant.GetType().category);
    const MetaType::ListInfo * info = variant.GetType().GetListInfo();
    ASSERT_EQ(3llu, info->size(variant));
    {
        auto it = info->begin(variant);
        ASSERT_EQ(MetaType::Int32, it->GetType().category);
        ASSERT_EQ(1, it->Get<int>());
        ++it;
        ASSERT_EQ(MetaType::Int32, it->GetType().category);
        ASSERT_EQ(2, it->Get<int>());
        ++it;
        ASSERT_EQ(MetaType::Int32, it->GetType().category);
        ASSERT_EQ(3, it->Get<int>());
        ++it;
        ASSERT_EQ(info->end(variant), it);
    }
}

TEST(new_meta, string_list)
{
    std::vector<std::string> a_list = { "bar", "baz", "foo", "really_long_string_without_small_string_optimization" };
    MetaReference variant(a_list);
    ASSERT_EQ(MetaType::List, variant.GetType().category);
    const MetaType::ListInfo * info = variant.GetType().GetListInfo();
    ASSERT_EQ(4llu, info->size(variant));
    {
        size_t num_allocations_before = mem::MemoryManager::GetNumAllocations();
        auto it = info->begin(variant);
        ASSERT_EQ(MetaType::String, it->GetType().category);
        ASSERT_EQ("bar", it->Get<std::string>());
        ++it;
        ASSERT_EQ(MetaType::String, it->GetType().category);
        ASSERT_EQ("baz", it->Get<std::string>());
        ++it;
        ASSERT_EQ(MetaType::String, it->GetType().category);
        ASSERT_EQ("foo", it->Get<std::string>());
        ++it;
        ASSERT_EQ(MetaType::String, it->GetType().category);
        ASSERT_EQ("really_long_string_without_small_string_optimization", it->Get<std::string>());
        ++it;
        ASSERT_EQ(info->end(variant), it);
        EXPECT_EQ(num_allocations_before, mem::MemoryManager::GetNumAllocations());
    }
}

/*#include "Util/profile.hpp"
#include <random>

TEST(new_meta, profile_vector)
{
    {
        std::mt19937 engine;
        engine.seed(5);
        std::uniform_int_distribution<> distribution;
        ScopedMeasurer first("direct");
        std::vector<int> a;
        for (size_t i = 0; i < 1000000; ++i) a.push_back(distribution(engine));
        ScopedMeasurer first_sort_only("direct_sort_only");
        std::sort(a.begin(), a.end());
    }
    {
        std::mt19937 engine;
        engine.seed(5);
        std::uniform_int_distribution<> distribution;
        ScopedMeasurer second("meta");
        std::vector<int> a;
        MetaReference as_variant(a);
        const MetaType::ListInfo * info = as_variant.GetType().GetListInfo();
        ASSERT_TRUE(info);
        for (size_t i = 0; i < 1000000; ++i)
        {
            int random = distribution(engine);
            info->push_back(as_variant, MetaReference(random));
        }
        ScopedMeasurer second_sort_only("meta_sort_only");
        std::sort(info->begin(as_variant), info->end(as_variant));
    }
}*/

TEST(new_meta, set)
{
    std::set<std::string> a_set = { "foo", "bar", "baz" };
    MetaReference as_meta(a_set);
    const MetaType::SetInfo * set_info = as_meta.GetType().GetSetInfo();
    ASSERT_TRUE(set_info);
    {
        auto meta_begin = set_info->begin(as_meta);
        auto begin = a_set.begin();
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(set_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, multi_set)
{
    std::multiset<std::string> a_set = { "foo", "bar", "baz", "bar" };
    MetaReference as_meta(a_set);
    const MetaType::SetInfo * set_info = as_meta.GetType().GetSetInfo();
    ASSERT_TRUE(set_info);
    {
        auto meta_begin = set_info->begin(as_meta);
        auto begin = a_set.begin();
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(set_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, unordered_set)
{
    std::unordered_set<std::string> a_set = { "foo", "bar", "baz" };
    MetaReference as_meta(a_set);
    const MetaType::SetInfo * set_info = as_meta.GetType().GetSetInfo();
    ASSERT_TRUE(set_info);
    {
        auto meta_begin = set_info->begin(as_meta);
        auto begin = a_set.begin();
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(set_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, unordered_multiset)
{
    std::unordered_multiset<std::string> a_set = { "foo", "bar", "baz", "bar" };
    MetaReference as_meta(a_set);
    const MetaType::SetInfo * set_info = as_meta.GetType().GetSetInfo();
    ASSERT_TRUE(set_info);
    {
        auto meta_begin = set_info->begin(as_meta);
        auto begin = a_set.begin();
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(*begin, meta_begin->Get<std::string>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(set_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, map)
{
    std::map<std::string, int> a_map = { { "foo", 1 }, { "bar", 2 }, { "baz", 3 } };
    MetaReference as_meta(a_map);
    const MetaType::MapInfo * map_info = as_meta.GetType().GetMapInfo();
    ASSERT_TRUE(map_info);
    {
        auto meta_begin = map_info->begin(as_meta);
        auto begin = a_map.begin();
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(map_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, multimap)
{
    std::multimap<std::string, int> a_map = { { "foo", 1 }, { "bar", 2 }, { "baz", 3 }, { "bar", 5 } };
    MetaReference as_meta(a_map);
    const MetaType::MapInfo * map_info = as_meta.GetType().GetMapInfo();
    ASSERT_TRUE(map_info);
    {
        auto meta_begin = map_info->begin(as_meta);
        auto begin = a_map.begin();
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(map_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, unordered_map)
{
    std::unordered_map<std::string, int> a_map = { { "foo", 1 }, { "bar", 2 }, { "baz", 3 } };
    MetaReference as_meta(a_map);
    const MetaType::MapInfo * map_info = as_meta.GetType().GetMapInfo();
    ASSERT_TRUE(map_info);
    {
        auto meta_begin = map_info->begin(as_meta);
        auto begin = a_map.begin();
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(map_info->end(as_meta), meta_begin);
    }
}

TEST(new_meta, unordered_multimap)
{
    std::unordered_multimap<std::string, int> a_map = { { "foo", 1 }, { "bar", 2 }, { "baz", 3 }, { "bar", 5 } };
    MetaReference as_meta(a_map);
    const MetaType::MapInfo * map_info = as_meta.GetType().GetMapInfo();
    ASSERT_TRUE(map_info);
    {
        auto meta_begin = map_info->begin(as_meta);
        auto begin = a_map.begin();
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(begin->first, meta_begin->first.Get<std::string>());
        ASSERT_EQ(begin->second, meta_begin->second.Get<int>());
        ++meta_begin;
        ++begin;
        ASSERT_EQ(map_info->end(as_meta), meta_begin);
    }
}

struct too_large
{
    too_large()
    {
    }

    unsigned char stuff[1024];
};
struct small_struct
{
    small_struct(int i = 0)
        : i(i)
    {
    }

    int i;
};
static MetaType::StructInfo::MembersAndBases CreateEmptyMemberCollection(int8_t)
{
    return {{{}, {}}, {{}}};
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<too_large>::type = MetaType::RegisterStruct<too_large>("testing_too_large", 0, &CreateEmptyMemberCollection);
template<>
const MetaType MetaType::MetaTypeConstructor<small_struct>::type = MetaType::RegisterStruct<small_struct>("testing_small_struct", 0, &CreateEmptyMemberCollection);
namespace
{
struct struct_with_members
{
    struct_with_members(int a = 0, float b = 0.0f)
        : a(a), b(b)
    {
        c.push_back(a);
        c.push_back(static_cast<int>(b));
    }
    int a;
    float b;
    std::vector<int> c;
};
static MetaType::StructInfo::MembersAndBases get_struct_with_members_members(int8_t)
{
    return
    {
        {
            {
                MetaMember("a", &struct_with_members::a),
                MetaMember("b", &struct_with_members::b),
                MetaMember("c", &struct_with_members::c)
            },
            {
            }
        },
        {
            {
            }
        }
    };
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<struct_with_members>::type = MetaType::RegisterStruct<struct_with_members>("testing_struct_with_members", 0, &get_struct_with_members_members);
namespace
{
TEST(new_meta, members)
{
    struct_with_members object(5, 7.3f);
    MetaReference as_variant(object);
    const MetaType::StructInfo * info = as_variant.GetType().GetStructInfo();
    ASSERT_TRUE(info);
    ASSERT_EQ(3llu, info->GetDirectMembers(0).members.size());
    std::vector<int> expected = { 5, 7 };
    size_t num_allocations_before = mem::MemoryManager::GetNumAllocations();
    for (const MetaMember & member : info->GetDirectMembers(0).members)
    {
        if (member.GetName() == "a")
        {
            ASSERT_EQ(5, member.Get<int>(as_variant));
        }
        else if (member.GetName() == "b")
        {
            ASSERT_EQ(7.3f, member.Get<float>(as_variant));
        }
        else if (member.GetName() == "c")
        {
            ASSERT_EQ(expected, member.Get<std::vector<int>>(as_variant));
        }
    }
    ASSERT_EQ(num_allocations_before, mem::MemoryManager::GetNumAllocations());
}

TEST(new_meta, array)
{
    {
        int a[5] = { -10, 20, -30, 40, 0 };
        MetaReference as_variant(a);
        const MetaType::ArrayInfo * info = as_variant.GetType().GetArrayInfo();
        ASSERT_TRUE(info);
        {
            auto begin = info->begin(as_variant);
            ASSERT_EQ(-10, (*begin).Get<int>());
            ++begin;
            ASSERT_EQ(20, (*begin).Get<int>());
            ++begin;
            ASSERT_EQ(-30, (*begin).Get<int>());
            ++begin;
            ASSERT_EQ(40, (*begin).Get<int>());
            ++begin;
            ASSERT_EQ(0, (*begin).Get<int>());
            ++begin;
            ASSERT_EQ(info->end(as_variant), begin);
        }
    }
    {
        std::string a[2] = { "foo", "bar" };
        MetaReference as_variant(a);
        const MetaType::ArrayInfo * info = as_variant.GetType().GetArrayInfo();
        ASSERT_TRUE(info);
        {
            auto begin = info->begin(as_variant);
            ASSERT_EQ("foo", (*begin).Get<std::string>());
            ++begin;
            ASSERT_EQ("bar", (*begin).Get<std::string>());
            ++begin;
            ASSERT_EQ(info->end(as_variant), begin);
        }
    }
}

struct AVector3Type
{
    float x;
    float y;
    float z;
};
float * begin(AVector3Type & vector)
{
    return &vector.x;
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<AVector3Type>::type = MetaType::RegisterArray<AVector3Type>(GetMetaType<float>(), 3);
namespace
{
TEST(new_meta, custom_array)
{
    AVector3Type vec = { 3.0f, 2.0f, 1.0f };
    MetaReference as_variant(vec);
    const MetaType::ArrayInfo * info = as_variant.GetType().GetArrayInfo();
    ASSERT_TRUE(info);
    {
        auto begin = info->begin(as_variant);
        ASSERT_EQ(3.0f, (*begin).Get<float>());
        ++begin;
        ASSERT_EQ(2.0f, (*begin).Get<float>());
        ++begin;
        ASSERT_EQ(1.0f, (*begin).Get<float>());
        ++begin;
        ASSERT_EQ(info->end(as_variant), begin);
    }
}

TEST(new_meta, nested_array)
{
    std::array<std::array<std::string, 2>, 2> arr;
    arr[0][0] = "foo";
    arr[0][1] = "bar";
    arr[1][0] = "baz";
    arr[1][1] = "something a bit longer to prevent small string optimiaztion";
    MetaReference as_variant(arr);
    const MetaType::ArrayInfo * info_outer = as_variant.GetType().GetArrayInfo();
    ASSERT_TRUE(info_outer);
    auto non_variant_outer = begin(arr);
    for (auto outer = info_outer->begin(as_variant); outer != info_outer->end(as_variant); ++outer, ++non_variant_outer)
    {
        MetaReference outer_variant = *outer;
        const MetaType::ArrayInfo * info_inner = outer_variant.GetType().GetArrayInfo();
        ASSERT_TRUE(info_inner);
        auto non_variant_inner = begin(*non_variant_outer);
        for (auto inner = info_inner->begin(outer_variant); inner != info_inner->end(outer_variant); ++inner, ++non_variant_inner)
        {
            ASSERT_EQ(*non_variant_inner, (*inner).Get<std::string>());
        }
    }
}

enum ABC
{
    A,
    B,
    C
};
}
template<>
const MetaType MetaType::MetaTypeConstructor<ABC>::type = MetaType::RegisterEnum<ABC>({ { A, "A" }, { B, "B" }, { C, "C" } });
namespace
{
TEST(new_meta, enum)
{
    ABC a = A;
    MetaReference as_reference(a);
    const MetaType::EnumInfo * info = as_reference.GetType().GetEnumInfo();
    ASSERT_TRUE(info);
    ASSERT_EQ(A, info->GetAsInt(as_reference));
    ASSERT_EQ("A", info->GetAsString(as_reference));
    ABC b = B;
    as_reference.Get<ABC>() = b;
    ASSERT_EQ(B, info->GetAsInt(as_reference));
    ASSERT_EQ("B", info->GetAsString(as_reference));
}

struct base_struct_a
{
    base_struct_a(int a = 0)
        : a(a)
    {
    }
    int a;
};
struct base_struct_b
{
    base_struct_b(int b = 0)
        : b(b)
    {
    }
    int b;
};
struct derived_struct : base_struct_a, base_struct_b
{
    derived_struct(int a = 0, int b = 0, int c = 0)
        : base_struct_a(a), base_struct_b(b), c(c)
    {
    }
    int c;
};

static MetaType::StructInfo::MembersAndBases get_base_struct_a_members(int8_t)
{
    return { { { MetaMember("a", &base_struct_a::a) }, { } }, { { } } };
}
static MetaType::StructInfo::MembersAndBases get_base_struct_b_members(int8_t)
{
    return { { { MetaMember("b", &base_struct_b::b) }, { } }, { { { } } } };
}
static MetaType::StructInfo::MembersAndBases get_derived_struct_info(int8_t)
{
    return
    {
        { { MetaMember("c", &derived_struct::c) }, { } },
        {
            {
                BaseClass::Create<base_struct_a, derived_struct>(),
                BaseClass::Create<base_struct_b, derived_struct>()
            }
        }
    };
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<base_struct_a>::type = MetaType::RegisterStruct<base_struct_a>("base_struct_a", 0, &get_base_struct_a_members);
template<>
const MetaType MetaType::MetaTypeConstructor<base_struct_b>::type = MetaType::RegisterStruct<base_struct_b>("base_struct_b", 0, &get_base_struct_b_members);
template<>
const MetaType MetaType::MetaTypeConstructor<derived_struct>::type = MetaType::RegisterStruct<derived_struct>("derived_struct", 0, &get_derived_struct_info);
namespace
{
TEST(new_meta, derived)
{
    derived_struct derived(5, 6, 7);
    MetaReference as_meta(derived);
    ASSERT_TRUE(as_meta.GetType().GetStructInfo());
    const MetaType::StructInfo::AllMemberCollection & all_members = as_meta.GetType().GetStructInfo()->GetAllMembers(as_meta.GetType().GetStructInfo()->GetCurrentHeaders());
    int sum = 0;
    for (const MetaMember & meta_member : all_members.direct_members.members)
    {
        if (meta_member.GetName() == "c")
        {
            ASSERT_EQ(7, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
    }
    for (const BaseClass::BaseMemberCollection::BaseMember & meta_member : all_members.base_members.members)
    {
        if (meta_member.GetName() == "a")
        {
            ASSERT_EQ(5, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
        else if (meta_member.GetName() == "b")
        {
            ASSERT_EQ(6, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
    }
    ASSERT_EQ(derived.a + derived.b + derived.c, sum);
}

struct base_struct_d
{
    base_struct_d(int d = 0)
        : d(d)
    {
    }

    int d;
};
struct derived_derived : base_struct_d, derived_struct
{
    derived_derived(int a = 0, int b = 0, int c = 0, int d = 0, int e = 0)
        : base_struct_d(d), derived_struct(a, b, c), e(e)
    {
    }
    int e;
};

static MetaType::StructInfo::MembersAndBases get_base_struct_d_members(int8_t)
{
    return { { { MetaMember("d", &base_struct_d::d) }, { } }, { { } } };
}
static MetaType::StructInfo::MembersAndBases get_derived_derived_info(int8_t)
{
    return
    {
        { { MetaMember("e", &derived_derived::e) }, { } },
        {
            {
                BaseClass::Create<base_struct_d, derived_derived>(),
                BaseClass::Create<derived_struct, derived_derived>()
            }
        }
    };
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<base_struct_d>::type = MetaType::RegisterStruct<base_struct_d>("base_struct_d", 0, &get_base_struct_d_members);
template<>
const MetaType MetaType::MetaTypeConstructor<derived_derived>::type = MetaType::RegisterStruct<derived_derived>("derived_derived", 0, &get_derived_derived_info);
namespace
{
TEST(new_meta, derived_two_levels)
{
    derived_derived derived(5, 6, 7, 8, 9);
    MetaReference as_meta(derived);
    ASSERT_TRUE(as_meta.GetType().GetStructInfo());
    const MetaType::StructInfo::AllMemberCollection & all_members = as_meta.GetType().GetStructInfo()->GetAllMembers(as_meta.GetType().GetStructInfo()->GetCurrentHeaders());
    int sum = 0;
    for (const MetaMember & meta_member : all_members.direct_members.members)
    {
        if (meta_member.GetName() == "e")
        {
            ASSERT_EQ(9, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
    }
    for (const BaseClass::BaseMemberCollection::BaseMember & meta_member : all_members.base_members.members)
    {
        if (meta_member.GetName() == "a")
        {
            ASSERT_EQ(5, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
        else if (meta_member.GetName() == "b")
        {
            ASSERT_EQ(6, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
        else if (meta_member.GetName() == "c")
        {
            ASSERT_EQ(7, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
        else if (meta_member.GetName() == "d")
        {
            ASSERT_EQ(8, meta_member.Get<int>(as_meta));
            sum += meta_member.Get<int>(as_meta);
        }
    }
    ASSERT_EQ(derived.a + derived.b + derived.c + derived.d + derived.e, sum);
}
struct derived_derived_derived : derived_derived
{
    derived_derived_derived(int a = 0, int b = 0, int c = 0, int d = 0, int e = 0)
        : derived_derived(a, b, c, d, e)
    {
    }
};

static MetaType::StructInfo::MembersAndBases get_derived_derived_derived_info(int8_t)
{
    return
    {
        { { }, { } },
        {
            {
                BaseClass::Create<derived_derived, derived_derived_derived>(),
            }
        }
    };
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<derived_derived_derived>::type = MetaType::RegisterStruct<derived_derived_derived>("derived_derived_derived", 0, &get_derived_derived_derived_info);
namespace
{
TEST(new_meta, derived_three_levels)
{
    derived_derived_derived derived(5, 6, 7, 8, 9);
    MetaReference as_meta(derived);
    ASSERT_TRUE(as_meta.GetType().GetStructInfo());
    const MetaType::StructInfo::AllMemberCollection & all_members = as_meta.GetType().GetStructInfo()->GetAllMembers(as_meta.GetType().GetStructInfo()->GetCurrentHeaders());
    int sum = 0;
    for (const BaseClass::BaseMemberCollection::BaseMember & meta_member : all_members.base_members.members)
    {
        sum += meta_member.Get<int>(as_meta);
    }
    ASSERT_EQ(derived.a + derived.b + derived.c + derived.d + derived.e, sum);
}

struct pointer_to_struct_base
{
    pointer_to_struct_base(int a)
        : a(a)
    {
    }

    pointer_to_struct_base(const pointer_to_struct_base &) = default;
    pointer_to_struct_base & operator=(const pointer_to_struct_base &) = default;
    virtual ~pointer_to_struct_base() = default;

    int a;
};
struct pointer_to_struct_offsetting_base
{
    pointer_to_struct_offsetting_base(int c)
        : c(c)
    {
    }
    int c;
};

struct pointer_to_struct_derived : pointer_to_struct_offsetting_base, pointer_to_struct_base
{
    pointer_to_struct_derived(int a, int b, int c)
        : pointer_to_struct_offsetting_base(c), pointer_to_struct_base(a), b(b)
    {
    }

    int b;
};
static MetaType::StructInfo::MembersAndBases get_pointer_to_struct_base_members(int8_t)
{
    return { { { MetaMember("a", &pointer_to_struct_base::a) }, { } }, { { } } };
}
static MetaType::StructInfo::MembersAndBases get_pointer_to_struct_offsetting_base_members(int8_t)
{
    return { { { MetaMember("c", &pointer_to_struct_offsetting_base::c) }, { } }, { { } } };
}
static MetaType::StructInfo::MembersAndBases get_pointer_to_struct_derived_members(int8_t)
{
    return
    {
        { { MetaMember("b", &pointer_to_struct_derived::b) }, { } },
        {
            {
                BaseClass::Create<pointer_to_struct_offsetting_base, pointer_to_struct_derived>(),
                BaseClass::Create<pointer_to_struct_base, pointer_to_struct_derived>()
            }
        }
    };
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<pointer_to_struct_base>::type = MetaType::RegisterStruct<pointer_to_struct_base>("pointer_to_struct_base", 0, &get_pointer_to_struct_base_members);
template<>
const MetaType MetaType::MetaTypeConstructor<pointer_to_struct_offsetting_base>::type = MetaType::RegisterStruct<pointer_to_struct_offsetting_base>("pointer_to_struct_offsetting_base", 0, &get_pointer_to_struct_offsetting_base_members);
template<>
const MetaType MetaType::MetaTypeConstructor<pointer_to_struct_derived>::type = MetaType::RegisterStruct<pointer_to_struct_derived>("pointer_to_struct_derived", 0, &get_pointer_to_struct_derived_members);
namespace
{
TEST(new_meta, pointer_to_struct)
{
    std::unique_ptr<pointer_to_struct_base> ptr(new pointer_to_struct_derived(5, 6, 7));
    MetaReference as_meta(ptr);
    const MetaType::PointerToStructInfo * info = as_meta.GetType().GetPointerToStructInfo();
    ASSERT_TRUE(info);
    const MetaType::StructInfo * struct_info = info->GetAsPointer(as_meta)->GetType().GetStructInfo();
    ASSERT_TRUE(struct_info);
    const MetaType::StructInfo::AllMemberCollection & members = struct_info->GetAllMembers(struct_info->GetCurrentHeaders());
    int sum = 0;
    for (const MetaMember & member : members.direct_members.members)
    {
        if (member.GetName() == "b")
        {
            ASSERT_EQ(6, member.Get<int>(*info->GetAsPointer(as_meta)));
            sum += member.Get<int>(*info->GetAsPointer(as_meta));
        }
    }
    for (const BaseClass::BaseMemberCollection::BaseMember & member : members.base_members.members)
    {
        if (member.GetName() == "a")
        {
            ASSERT_EQ(5, member.Get<int>(*info->GetAsPointer(as_meta)));
            sum += member.Get<int>(*info->GetAsPointer(as_meta));
        }
        if (member.GetName() == "c")
        {
            ASSERT_EQ(7, member.Get<int>(*info->GetAsPointer(as_meta)));
            sum += member.Get<int>(*info->GetAsPointer(as_meta));
        }
    }
    pointer_to_struct_derived & d = static_cast<pointer_to_struct_derived &>(*ptr);
    ASSERT_EQ(ptr->a + d.b + d.c, sum);
}

struct ATypeErasure : BaseTypeErasure<sizeof(void *), CopyVTable>
{
    using BaseTypeErasure<sizeof(void *), CopyVTable>::BaseTypeErasure;
};
}
namespace metav3
{
template<>
const MetaType MetaType::MetaTypeConstructor<ATypeErasure>::type = MetaType::RegisterTypeErasure<ATypeErasure>();
}
namespace
{
TEST(new_meta, type_erasure)
{
    MetaType::TypeErasureInfo::SupportedType<ATypeErasure, base_struct_a> support;
    ATypeErasure a((base_struct_a(5)));
    MetaReference as_meta(a);
    const MetaType::TypeErasureInfo * info = as_meta.GetType().GetTypeErasureInfo();
    ASSERT_TRUE(info);
    ASSERT_EQ(&GetMetaType<base_struct_a>(), info->TargetType(as_meta));
    MetaReference inner = info->Target(as_meta);
    const MetaType::StructInfo * inner_info = inner.GetType().GetStructInfo();
    ASSERT_TRUE(inner_info);
    ASSERT_EQ(5, inner.Get<base_struct_a>().a);
    MetaReference new_inner = info->AssignNew(as_meta, GetMetaType<base_struct_a>());
    const MetaType::StructInfo * new_inner_info = new_inner.GetType().GetStructInfo();
    ASSERT_TRUE(new_inner_info);
    ASSERT_EQ(0, new_inner.Get<base_struct_a>().a);
}
}

#endif
