#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "debug/assert.hpp"
#include <map>
#include "util/forwarding_constructor.hpp"
#include "util/range.hpp"
#include "util/pointer.hpp"
#include "util/shared_ptr.hpp"
#include "util/type_erasure.hpp"
#include <cstddef>
#include <cstdint>
#include <iterator>
#include "util/string/hashed_string.hpp"
#include "util/memoizingMap.hpp"
#include <typeinfo>
#include "metav3/type_traits.hpp"
#include "os/memoryManager.hpp"

namespace metav3
{
struct MetaReference;
struct ConstMetaReference;

struct MetaType;
template<typename T>
const MetaType & GetMetaType();

template<typename T, typename D = std::default_delete<T> >
using unique_ptr_type = dunique_ptr<T, D>;

namespace detail
{
bool throw_not_equality_comparable();
bool throw_not_less_than_comparable();
void assert_range_size_and_alignment(Range<unsigned char> memory, size_t size, size_t alignment);
}

struct MetaMember;
struct MetaConditionalMember;
struct MetaType;
struct MetaRandomAccessIterator;
struct ConstMetaReference;

struct MetaReference
{
    template<typename T>
    MetaReference(T & object);
    MetaReference(const MetaType & type, Range<unsigned char> memory);
    MetaReference(const MetaReference &) = default;
    MetaReference(MetaReference &&) = default;
    MetaReference(MetaReference & other)
        : MetaReference(const_cast<const MetaReference &>(other))
    {
    }
    MetaReference & operator=(const MetaReference &) = default;
    MetaReference & operator=(MetaReference &&) = default;

    operator ConstMetaReference() const;

    const MetaType & GetType() const
    {
        return *type;
    }
    Range<unsigned char> GetMemory() const
    {
        return memory;
    }

    template<typename T>
    T & Get();
    template<typename T>
    const T & Get() const;

private:
    const MetaType * type;
    Range<unsigned char> memory;
};

struct ConstMetaReference
{
    template<typename T>
    ConstMetaReference(const T & object);
    ConstMetaReference(const MetaType & type, Range<const unsigned char> memory)
        : reference(type, Range<unsigned char>(const_cast<unsigned char *>(memory.begin()), const_cast<unsigned char *>(memory.end())))
    {
    }
    ConstMetaReference(const ConstMetaReference &) = default;
    ConstMetaReference(ConstMetaReference &&) = default;
    ConstMetaReference & operator=(const ConstMetaReference &) = delete;
    ConstMetaReference & operator=(ConstMetaReference &&) = delete;

    operator const MetaReference &() const
    {
        return reference;
    }

    const MetaType & GetType() const
    {
        return reference.GetType();
    }
    Range<const unsigned char> GetMemory() const
    {
        return reference.GetMemory();
    }

    template<typename T>
    const T & Get() const;

private:
    MetaReference reference;
};
inline MetaReference::operator ConstMetaReference() const
{
    return { *type, memory };
}
struct MetaPointer
{
    MetaPointer(const MetaType & type, std::nullptr_t);

    explicit MetaPointer(MetaReference reference)
        : reference(reference)
    {
    }

    explicit operator bool() const
    {
        return reference.GetMemory().begin() != nullptr;
    }

    MetaReference operator*()
    {
        return reference;
    }
    ConstMetaReference operator*() const
    {
        return reference;
    }

    MetaReference * operator->()
    {
        return &reference;
    }
    const MetaReference * operator->() const
    {
        return &reference;
    }

private:
    MetaReference reference;
};
struct ConstMetaPointer
{
    ConstMetaPointer(const MetaType & type, std::nullptr_t);
    explicit ConstMetaPointer(ConstMetaReference reference)
        : reference(reference)
    {
    }

    explicit operator bool() const
    {
        return reference.GetMemory().begin() != nullptr;
    }

    ConstMetaReference & operator*()
    {
        return reference;
    }
    const ConstMetaReference & operator*() const
    {
        return reference;
    }

    ConstMetaReference * operator->()
    {
        return &reference;
    }
    const ConstMetaReference * operator->() const
    {
        return &reference;
    }

private:
    ConstMetaReference reference;
};

struct ClassHeader
{
    ClassHeader(HashedString, int16_t);

    bool operator<(const ClassHeader & other) const;
    bool operator<=(const ClassHeader & other) const;
    bool operator>(const ClassHeader & other) const;
    bool operator>=(const ClassHeader & other) const;
    bool operator==(const ClassHeader & other) const;
    bool operator!=(const ClassHeader & other) const;

    const HashedString & GetClassName() const
    {
        return class_name;
    }
    int16_t GetVersion() const
    {
        return version;
    }
private:
    HashedString class_name;
    int16_t version;
};
struct ClassHeaderList : std::vector<ClassHeader>
{
    using std::vector<ClassHeader>::vector;

    inline const ClassHeader & GetMostDerived() const
    {
        return front();
    }

    iterator find(const ClassHeader & header);
    iterator find(const HashedString & class_name);
    const_iterator find(const ClassHeader & header) const;
    const_iterator find(const HashedString & class_name) const;
};

struct BaseClass
{
    template<typename Base, typename Derived>
    static BaseClass Create();

    static BaseClass Combine(const BaseClass & basebase, const BaseClass & base);

    const MetaType & GetBase() const
    {
        return *base;
    }
    const MetaType & GetDerived() const
    {
        return *derived;
    }
    ptrdiff_t GetOffset() const
    {
        return offset;
    }

    struct BaseMemberCollection;
    const BaseMemberCollection & GetMembers(const ClassHeaderList & versions) const;

private:
    BaseClass(const MetaType & base, const MetaType & derived, ptrdiff_t offset);

    const MetaType * base;
    const MetaType * derived;
    ptrdiff_t offset;
    memoizing_map<ClassHeaderList, BaseMemberCollection> members;

    template<typename Base, typename Derived>
    static ptrdiff_t calculate_offset()
    {
        union SomeMemory
        {
            SomeMemory() {}
            ~SomeMemory() {}

            Derived derived;
        } some_address;
        return reinterpret_cast<unsigned char *>(static_cast<Base *>(std::addressof(some_address.derived))) - reinterpret_cast<unsigned char *>(std::addressof(some_address.derived));
    }
};

struct MetaType
{
    enum AllTypesThatExist
    {
        Bool,
        Char,
        Int8,
        Uint8,
        Int16,
        Uint16,
        Int32,
        Uint32,
        Int64,
        Uint64,
        Float,
        Double,
        Enum,
        String, // Utf8 text in a Range<const char>
        List, // dynamic size Array
        Array, // fixed size List
        Set,
        Map,
        Struct,
        PointerToStruct,
        TypeErasure
    };

    size_t GetSize() const noexcept
    {
        return general.size;
    }
    uint32_t GetAlignment() const noexcept
    {
        return general.alignment;
    }
    const std::type_info & GetTypeInfo() const noexcept
    {
        return general.type_info;
    }

    typedef unique_ptr_type<unsigned char, void (*)(void *)> allocate_pointer;
    allocate_pointer Allocate() const
    {
        return general.allocate();
    }
    void Construct(Range<unsigned char> memory) const
    {
        general.construct(memory);
    }
    void Destroy(Range<unsigned char> memory) const
    {
        general.destroy(memory);
    }

    struct GeneralInformation
    {
        template<typename T>
        static GeneralInformation CreateForType()
        {
            return
            {
                typeid(T), sizeof(T), alignof(T),
                &Allocate<T>::allocate,
                &PlacementConstruct<T, std::is_default_constructible<T>::value>::construct,
                &Destroy<T>::destroy,
            };
        }

        template<typename T>
        struct Allocate
        {
            static allocate_pointer allocate()
            {
                if (16 % alignof(T) != 0) return allocate_pointer(static_cast<unsigned char *>(mem::AllocAligned(sizeof(T), alignof(T))), &mem::FreeAligned);
                else return allocate_pointer(static_cast<unsigned char *>(::operator new(sizeof(T))), &::operator delete);
            }
        };

        template<typename T, bool is_default_constructible>
        struct PlacementConstruct;
        template<typename T>
        struct PlacementConstruct<T, true>
        {
            static void construct(Range<unsigned char> memory)
            {
                detail::assert_range_size_and_alignment(memory, sizeof(T), alignof(T));
                new (memory.begin()) T();
            }
        };
        template<typename T>
        struct PlacementConstruct<T, false>
        {
            static void construct(Range<unsigned char>)
            {
                RAW_ASSERT(false, "the type is not default constructible");
            }
        };
        template<typename T>
        struct Destroy
        {
            static void destroy(Range<unsigned char> object)
            {
                reinterpret_cast<T *>(object.begin())->~T();
            }
        };
        // need a special case for this because the above function
        // won't compile for arrays
        template<typename T, size_t Size>
        struct Destroy<T[Size]>
        {
            static void destroy(Range<unsigned char> object)
            {
                for (size_t i = 0; i < Size; ++i)
                    (*reinterpret_cast<T (*)[Size]>(object.begin()))[i].~T();
            }
        };

        const std::type_info & type_info;
        size_t size;
        uint32_t alignment;
        allocate_pointer (*allocate)();
        void (*construct)(Range<unsigned char>);
        void (*destroy)(Range<unsigned char>);
    };
    const AllTypesThatExist category;

    template<typename T>
    struct MetaTypeConstructor
    {
        static const MetaType type;
    };

    static const MetaType & GetStructType(const std::type_info &);
    static const MetaType & GetStructType(const HashedString &);
    static const HashedString & GetRegisteredStructName(Range<const char> name);
    static const MetaType & GetRegisteredStruct(uint32_t hash);

    struct SimpleInfo
    {
    };
    struct StringInfo
    {
        template<typename T>
        static StringInfo Create()
        {
            return StringInfo(&Specialization<T>::GetAsRange, &Specialization<T>::SetFromRange);
        }

        Range<const char> GetAsRange(const MetaReference & ref) const
        {
            return get_as_range(ref);
        }
        void SetFromRange(MetaReference & ref, Range<const char> range) const
        {
            return set_from_range(ref, range);
        }

    private:
        Range<const char> (*get_as_range)(const MetaReference &);
        void (*set_from_range)(MetaReference &, Range<const char>);
        StringInfo(Range<const char> (*get_as_range)(const MetaReference &), void (*set_from_range)(MetaReference &, Range<const char>));

        template<typename T, typename = void>
        struct Specialization
        {
            static Range<const char> GetAsRange(const MetaReference & ref)
            {
                return ref.Get<T>();
            }
            static void SetFromRange(MetaReference & ref, Range<const char> range)
            {
                ref.Get<T>() = range;
            }
        };
    };
    struct EnumInfo
    {
        EnumInfo(std::map<int32_t, HashedString> values);

        int32_t GetAsInt(const MetaReference &) const;
        const HashedString & GetAsHashedString(const MetaReference &) const;
        void SetFromString(MetaReference & to_set, const std::string & value) const;

        std::map<int32_t, HashedString> values;

    private:
        std::map<std::string, int32_t> int_values;
    };
    struct ListInfo
    {
        template<typename T>
        struct Creator
        {
            static ListInfo Create();
        };

        const MetaType & value_type;

        size_t (*size)(const MetaReference &);
        void (*push_back)(MetaReference &, MetaReference && value);
        MetaRandomAccessIterator (*begin)(MetaReference &);
        MetaRandomAccessIterator (*end)(MetaReference &);
    };
    struct ArrayInfo
    {
        template<typename T>
        static ArrayInfo Create(const MetaType & value_type, size_t array_size)
        {
            return { value_type, array_size, &BeginSpecialization<T>::begin };
        }

        ArrayInfo(const MetaType & value_type, size_t array_size, MetaRandomAccessIterator (*begin)(MetaReference & object));

        MetaRandomAccessIterator (*begin)(MetaReference & object);
        MetaRandomAccessIterator end(MetaReference & object) const;

        const MetaType & value_type;
        size_t array_size;

        size_t size(const MetaReference &) const
        {
            return array_size;
        }

    private:
        template<typename T>
        struct BeginSpecialization
        {
            static MetaRandomAccessIterator begin(MetaReference & object);
        };
    };
    struct SetInfo
    {
        struct SetIterator;

        const MetaType & value_type;
        size_t (*size)(const MetaReference & set);
        std::pair<SetIterator, bool> (*insert)(MetaReference & set, MetaReference && value);
        SetIterator (*begin)(const MetaReference & set);
        SetIterator (*end)(const MetaReference & set);

        template<typename T>
        struct Creator
        {
            static SetInfo Create()
            {
                return
                {
                    GetMetaType<typename T::value_type>(),
                    [](const MetaReference & set) -> size_t
                    {
                        return set.Get<T>().size();
                    },
                    [](MetaReference & object, MetaReference && value) -> std::pair<SetIterator, bool>
                    {
                        return object.Get<T>().insert(std::move(value.Get<typename T::value_type>()));
                    },
                    [](const MetaReference & object) -> SetIterator
                    {
                        return const_cast<T &>(object.Get<T>()).begin();
                    },
                    [](const MetaReference & object) -> SetIterator
                    {
                        return const_cast<T &>(object.Get<T>()).end();
                    }
                };
            }
        };

        template<size_t Size, size_t Alignment, typename Allocator>
        struct IteratorVTable : CopyVTable<Size, Alignment, Allocator>, EqualityVTable<Size, Alignment, Allocator>
        {
            typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;

            ConstMetaReference (*dereference)(const Storage &);
            void (*increment)(Storage &);

            template<typename T>
            IteratorVTable(T *)
                : CopyVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
                , EqualityVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
                , dereference([](const Storage & object) -> ConstMetaReference
                {
                    return *object.template get_reference<T>();
                })
                , increment([](Storage & object)
                {
                    ++object.template get_reference<T>();
                })
            {
            }
        };

        struct SetIterator : std::iterator<std::forward_iterator_tag, ConstMetaReference>, EqualityTypeErasure<sizeof(void *) * 4, IteratorVTable>
        {
            typedef EqualityTypeErasure<sizeof(void *) * 4, IteratorVTable> Base;
            using Base::Base;

            ConstMetaReference operator*() const
            {
                return vtable->dereference(storage);
            }
            ConstMetaPointer operator->() const
            {
                return ConstMetaPointer(**this);
            }
            SetIterator & operator++()
            {
                vtable->increment(storage);
                return *this;
            }
            SetIterator operator++(int)
            {
                SetIterator copy(*this);
                ++*this;
                return copy;
            }
        };
    };
    struct MapInfo
    {
        const MetaType & key_type;
        const MetaType & mapped_type;

        struct MapIterator;
        size_t (*size)(const MetaReference & map);
        std::pair<MapIterator, bool> (*insert)(MetaReference & map, MetaReference && key, MetaReference && value);
        MapIterator (*begin)(const MetaReference & map);
        MapIterator (*end)(const MetaReference & map);

        template<typename T>
        struct Creator
        {
            static MapInfo Create()
            {
                return
                {
                    GetMetaType<typename T::key_type>(),
                    GetMetaType<typename T::mapped_type>(),
                    [](const MetaReference & object) -> size_t
                    {
                        return object.Get<T>().size();
                    },
                    [](MetaReference & object, MetaReference && key, MetaReference && value) -> std::pair<MapIterator, bool>
                    {
                        return object.Get<T>().insert(std::make_pair(std::move(key.Get<typename T::key_type>()), std::move(value.Get<typename T::mapped_type>())));
                    },
                    [](const MetaReference & object) -> MapIterator
                    {
                        return const_cast<T &>(object.Get<T>()).begin();
                    },
                    [](const MetaReference & object) -> MapIterator
                    {
                        return const_cast<T &>(object.Get<T>()).end();
                    }
                };
            }
        };

        template<size_t Size, size_t Alignment, typename Allocator>
        struct IteratorVTable : CopyVTable<Size, Alignment, Allocator>, EqualityVTable<Size, Alignment, Allocator>
        {
            typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;

            std::pair<ConstMetaReference, MetaReference> (*dereference)(const Storage &);
            void (*increment)(Storage &);

            template<typename T>
            IteratorVTable(T *)
                : CopyVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
                , EqualityVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
                , dereference([](const Storage & object) -> std::pair<ConstMetaReference, MetaReference>
                {
                    const T & it = object.template get_reference<T>();
                    return { ConstMetaReference(it->first), MetaReference(it->second) };
                })
                , increment([](Storage & object)
                {
                    ++object.template get_reference<T>();
                })
            {
            }
        };

        typedef std::pair<ConstMetaReference, MetaReference> IteratorValueType;
        struct MapIterator : std::iterator<std::forward_iterator_tag, IteratorValueType>, EqualityTypeErasure<4 * sizeof(void *), IteratorVTable>
        {
            typedef EqualityTypeErasure<4 * sizeof(void *), IteratorVTable> Base;
            using Base::Base;

            IteratorValueType operator*() const
            {
                return vtable->dereference(storage);
            }
            struct PointerWrapper
            {
                PointerWrapper(IteratorValueType to_wrap)
                    : wrapped(std::move(to_wrap))
                {
                }
                IteratorValueType * operator->()
                {
                    return &wrapped;
                }
                const IteratorValueType * operator->() const
                {
                    return &wrapped;
                }

            private:
                IteratorValueType wrapped;
            };

            PointerWrapper operator->() const
            {
                return **this;
            }
            MapIterator & operator++()
            {
                vtable->increment(storage);
                return *this;
            }
            MapIterator operator++(int)
            {
                MapIterator copy(*this);
                ++*this;
                return copy;
            }
        };
    };
    struct StructInfo
    {
        struct MemberCollection
        {
            MemberCollection(std::vector<MetaMember> members, std::vector<MetaConditionalMember> conditional_members);
            std::vector<MetaMember> members;
            std::vector<MetaConditionalMember> conditional_members;
        };
        struct BaseClassCollection
        {
            BaseClassCollection(std::vector<BaseClass> bases);

            std::vector<BaseClass> bases;
        };
        struct AllMemberCollection;

        typedef StructInfo::MemberCollection (*GetMembersFunction)(int16_t version);
        typedef StructInfo::BaseClassCollection (*GetBaseClassesFunction)(int16_t version);
        StructInfo(HashedString name, int16_t current_version, GetMembersFunction get_members, GetBaseClassesFunction get_bases);

        const HashedString & GetName() const noexcept { return name; }
        int16_t GetCurrentVersion() const noexcept { return current_version; }
        const ClassHeaderList & GetCurrentHeaders() const;
        const MemberCollection & GetDirectMembers(int16_t version) const;
        const AllMemberCollection & GetAllMembers(const ClassHeaderList & versions) const;
        const BaseClassCollection & GetDirectBaseClasses(int16_t version) const;
        const BaseClassCollection & GetAllBaseClasses(const ClassHeaderList & versions) const;

        static BaseClassCollection NoBaseClasses(int16_t);

    private:
        HashedString name;
        int16_t current_version;
        lazy_initialize<ClassHeaderList> current_headers;
        GetMembersFunction get_members;
        GetBaseClassesFunction get_bases;
        memoizing_map<int16_t, MemberCollection> direct_members;
        memoizing_map<ClassHeaderList, AllMemberCollection> all_members;
        memoizing_map<ClassHeaderList, BaseClassCollection> all_bases;
        memoizing_map<int16_t, BaseClassCollection> direct_bases;
    };
    struct PointerToStructInfo
    {
        template<typename T>
        static PointerToStructInfo Create()
        {
            return PointerToStructInfo(GetMetaType<typename metav3::remove_pointer<T>::type>(), &templated_get_as_pointer<T>, &templated_assign<T>);
        }

        MetaPointer GetAsPointer(const MetaReference & reference) const
        {
            return get_as_pointer(*this, reference);
        }

        void Assign(MetaReference & pointer, MetaReference & target) const
        {
            return assign(*this, pointer, target);
        }
        MetaReference AssignNew(MetaReference & pointer, const MetaType & type) const;

        const MetaType & GetTargetType() const
        {
            return target_type;
        }

    private:
        const MetaType & target_type;
        typedef MetaPointer (*pointer_function)(const PointerToStructInfo &, const MetaReference &);
        typedef void (*assign_function)(const PointerToStructInfo &, MetaReference &, MetaReference &);
        PointerToStructInfo(const MetaType & target_type, pointer_function get_as_pointer, assign_function assign);
        pointer_function get_as_pointer;
        assign_function assign;
        memoizing_map<const MetaType *, ptrdiff_t> cached_offsets;

        template<typename T>
        static MetaPointer templated_get_as_pointer(const PointerToStructInfo & info, const MetaReference & ref)
        {
            const T & ptr = ref.Get<T>();
            if (ptr)
                return MetaPointer(info.cast_reference(GetStructType(typeid(*ptr)), MetaReference(*ptr)));
            else
                return MetaPointer(ref.GetType().GetPointerToStructInfo()->target_type, nullptr);
        }
        MetaReference cast_reference(const MetaType & struct_type, MetaReference to_cast) const;
        ptrdiff_t get_offset_for_struct(const MetaType &) const;
        template<typename T>
        static void templated_assign(const PointerToStructInfo & info, MetaReference & pointer, MetaReference & target)
        {
            pointer.Get<T>() = T(static_cast<typename metav3::remove_pointer<T>::type *>(static_cast<void *>(info.cast_reference(target.GetType(), std::move(target)).GetMemory().begin())));
        }
    };
    struct TypeErasureInfo
    {
        template<typename T>
        static TypeErasureInfo Create()
        {
            return TypeErasureInfo(GetMetaType<T>(), &templated_target_type<T>);
        }
        template<typename TypeErasure, typename CapturedType>
        struct SupportedType
        {
            SupportedType()
            {
                GetSupportedTypes().emplace(compute_index(), RegisteredSupportedType{ &templated_target<TypeErasure, CapturedType>, &templated_assign<TypeErasure, CapturedType> });
            }
            ~SupportedType()
            {
                GetSupportedTypes().erase(compute_index());
            }

        private:
            std::pair<const MetaType *, const MetaType *> compute_index() const
            {
                return { &GetMetaType<TypeErasure>(), &GetMetaType<CapturedType>() };
            }
        };

        const MetaType * TargetType(ConstMetaReference reference) const;
        MetaReference Target(MetaReference reference) const;
        MetaReference Assign(MetaReference type_erasure, MetaReference && target) const;
        MetaReference AssignNew(MetaReference & pointer, const MetaType & type) const;

    private:
        typedef const MetaType * (*target_type_function)(ConstMetaReference);
        typedef MetaReference (*target_function)(MetaReference);
        typedef MetaReference (*assign_function)(MetaReference, MetaReference &&);
        struct RegisteredSupportedType
        {
            target_function target;
            assign_function assign;
        };
        static std::map<std::pair<const MetaType *, const MetaType *>, RegisteredSupportedType> & GetSupportedTypes();
        TypeErasureInfo(const MetaType & self, target_type_function target_type);
        const MetaType * self;
        target_type_function target_type;

        template<typename T>
        static const MetaType * templated_target_type(ConstMetaReference ref)
        {
            const std::type_info & info = ref.Get<T>().target_type();
            if (info == typeid(void)) return nullptr;
            else return &GetStructType(info);
        }
        template<typename TypeErasure, typename CapturedType>
        static MetaReference templated_target(MetaReference ref)
        {
            CapturedType * target = ref.Get<TypeErasure>().template target<CapturedType>();
            RAW_ASSERT(target);
            return *target;
        }
        template<typename TypeErasure, typename CapturedType>
        static MetaReference templated_assign(MetaReference pointer, MetaReference && target)
        {
            TypeErasure & object = pointer.Get<TypeErasure>();
            object = std::move(target.Get<CapturedType>());
            return *object.template target<CapturedType>();
        }
    };

    template<typename T>
    static MetaType RegisterSimple(AllTypesThatExist category)
    {
        return { GeneralInformation::CreateForType<T>(), category };
    }

    template<typename T>
    static MetaType RegisterString()
    {
        return { StringInfo::Create<T>(), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterEnum(std::map<int32_t, HashedString> values)
    {
        static_assert(sizeof(T) == sizeof(int32_t), "I only support enums that are int-sized right now.");
        return { GeneralInformation::CreateForType<T>(), std::move(values) };
    }

    template<typename T>
    static MetaType RegisterList()
    {
        return { ListInfo::Creator<T>::Create(), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterArray(const MetaType & value_type, size_t array_size)
    {
        return  {ArrayInfo::Create<T>(value_type, array_size), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterSet()
    {
        return { SetInfo::Creator<T>::Create(), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterMap()
    {
        return { MapInfo::Creator<T>::Create(), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterStruct(HashedString name, int16_t current_version, StructInfo::GetMembersFunction get_members, StructInfo::GetBaseClassesFunction get_bases = &StructInfo::NoBaseClasses)
    {
        return { StructInfo(std::move(name), current_version, get_members, get_bases), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterPointerToStruct()
    {
        return { PointerToStructInfo::Create<T>(), GeneralInformation::CreateForType<T>() };
    }

    template<typename T>
    static MetaType RegisterTypeErasure()
    {
        return { TypeErasureInfo::Create<T>(), GeneralInformation::CreateForType<T>() };
    }

    const SimpleInfo * GetSimpleInfo() const;
    const StringInfo * GetStringInfo() const;
    const EnumInfo * GetEnumInfo() const;
    const ListInfo * GetListInfo() const;
    const ArrayInfo * GetArrayInfo() const;
    const SetInfo * GetSetInfo() const;
    const MapInfo * GetMapInfo() const;
    const StructInfo * GetStructInfo() const;
    const PointerToStructInfo * GetPointerToStructInfo() const;
    const TypeErasureInfo * GetTypeErasureInfo() const;

    ~MetaType();

private:
    union
    {
        SimpleInfo simple_info;
        StringInfo string_info;
        EnumInfo enum_info;
        ListInfo list_info;
        ArrayInfo array_info;
        SetInfo set_info;
        MapInfo map_info;
        StructInfo struct_info;
        PointerToStructInfo pointer_to_struct_info;
        TypeErasureInfo type_erasure_info;
    };

    GeneralInformation general;

    MetaType(GeneralInformation, AllTypesThatExist category);
    MetaType(StringInfo string_info, GeneralInformation general);
    MetaType(GeneralInformation, std::map<int32_t, HashedString> enum_values);
    MetaType(ListInfo list_info, GeneralInformation);
    MetaType(ArrayInfo array_info, GeneralInformation);
    MetaType(SetInfo set_info, GeneralInformation);
    MetaType(MapInfo map_info, GeneralInformation);
    MetaType(StructInfo struct_info, GeneralInformation);
    MetaType(PointerToStructInfo, GeneralInformation);
    MetaType(TypeErasureInfo, GeneralInformation);
    MetaType(MetaType &&);
};

template<typename T>
const MetaType & GetMetaType()
{
    return MetaType::MetaTypeConstructor<T>::type;
}


struct MetaMember
{
    template<typename T, typename S>
    MetaMember(HashedString name, T S::*member)
        : offset(offset_for(member))
        , name(std::move(name))
        , struct_type(&metav3::GetMetaType<S>())
        , member_type(&metav3::GetMetaType<T>())
    {
    }

    template<typename T>
    T & Get(MetaReference owner) const
    {
        return GetReference(owner).Get<T>();
    }
    template<typename T>
    const T & Get(ConstMetaReference owner) const
    {
        return GetReference(owner).Get<T>();
    }
    MetaReference GetReference(MetaReference owner) const
    {
        RAW_ASSERT(&owner.GetType() == struct_type);
        return MetaReference(GetMemberType(), owner.GetMemory().subrange(offset, member_type->GetSize()));
    }
    ConstMetaReference GetReference(ConstMetaReference owner) const
    {
        RAW_ASSERT(&owner.GetType() == struct_type);
        return ConstMetaReference(GetMemberType(), owner.GetMemory().subrange(offset, member_type->GetSize()));
    }

    ptrdiff_t GetOffset() const
    {
        return offset;
    }
    const HashedString & GetName() const
    {
        return name;
    }
    const MetaType & GetStructType() const
    {
        return *struct_type;
    }
    const MetaType & GetMemberType() const
    {
        return *member_type;
    }

private:
    ptrdiff_t offset;
    HashedString name;
    const MetaType * struct_type;
    const MetaType * member_type;


    template<typename T, typename S>
    static ptrdiff_t offset_for(T S::*member)
    {
        union SomeMemory
        {
            SomeMemory() {}
            ~SomeMemory() {}

            S value;
        } some_address;
        return reinterpret_cast<unsigned char *>(std::addressof(some_address.value.*member)) - reinterpret_cast<unsigned char *>(std::addressof(some_address.value));
    }
};

struct MetaConditionalMember
{
    MetaConditionalMember(MetaMember member, bool (*condition)(const MetaReference & object));

    template<typename S, bool (S::*condition)() const>
    static MetaConditionalMember CreateFromMemFun(MetaMember member)
    {
        return { std::move(member), &Condition::mem_fun_wrapper<S, condition> };
    }
    template<typename S, bool S::*condition>
    static MetaConditionalMember CreateFromMember(MetaMember member)
    {
        return { std::move(member), &Condition::member_wrapper<S, condition> };
    }
    template<typename S, bool (*condition)(const S &)>
    static MetaConditionalMember CreateFromFunction(MetaMember member)
    {
        return { std::move(member), &Condition::function_wrapper<S, condition> };
    }
    template<typename S>
    bool ObjectHasMember(const S & object) const
    {
        return condition.condition(MetaReference(object));
    }
    bool ObjectHasMember(const MetaReference & object) const
    {
        return condition.condition(object);
    }

    const MetaMember & GetMember() const
    {
        return member;
    }

private:
    MetaMember member;

    struct Condition
    {
        typedef bool (*ConditionFunction)(const MetaReference & object);

        Condition(ConditionFunction condition);
        ConditionFunction condition;

        template<typename S, bool (S::*condition_function)() const>
        static bool mem_fun_wrapper(const MetaReference & object)
        {
            return (object.Get<S>().*condition_function)();
        }
        template<typename S, bool S::*condition_member>
        static bool member_wrapper(const MetaReference & object)
        {
            return object.Get<S>().*condition_member;
        }
        template<typename S, bool (*condition_function)(const S &)>
        static bool function_wrapper(const MetaReference & object)
        {
            return condition_function(object.Get<S>());
        }
    };

    Condition condition;
};

struct MetaOwningMemory : MetaReference
{
    MetaOwningMemory(const MetaType & type, Range<unsigned char> memory)
        : MetaReference(type, memory)
    {
        GetType().Construct(GetMemory());
    }
    ~MetaOwningMemory()
    {
        GetType().Destroy(GetMemory());
    }
};

template<>
struct MetaType::MetaTypeConstructor<MetaReference>; // intentionally not defined
template<>
struct MetaType::MetaTypeConstructor<MetaOwningMemory>; // intentionally not defined

template<typename T>
inline MetaReference::MetaReference(T & object)
    : type(&GetMetaType<T>())
    , memory(&reinterpret_cast<unsigned char &>(object), &reinterpret_cast<unsigned char &>(object) + sizeof(T))
{
}
inline MetaReference::MetaReference(const MetaType & type, Range<unsigned char> memory)
    : type(&type), memory(memory)
{
    RAW_ASSERT(memory.size() == type.GetSize());
}
template<typename T>
inline T & MetaReference::Get()
{
    RAW_ASSERT(type == &GetMetaType<T>());
    return *reinterpret_cast<T *>(memory.begin());
}
template<typename T>
inline const T & MetaReference::Get() const
{
    RAW_ASSERT(type == &GetMetaType<T>());
    return *reinterpret_cast<const T *>(memory.begin());
}

template<>
struct MetaType::MetaTypeConstructor<ConstMetaReference>; // intentionally not defined

template<typename T>
inline ConstMetaReference::ConstMetaReference(const T & object)
    : reference(const_cast<T &>(object))
{
}
template<typename T>
const T & ConstMetaReference::Get() const
{
    return reference.Get<T>();
}

inline MetaPointer::MetaPointer(const MetaType &type, std::nullptr_t)
    : reference(type, { static_cast<unsigned char *>(nullptr), static_cast<unsigned char *>(nullptr) + type.GetSize() })
{
}
inline ConstMetaPointer::ConstMetaPointer(const MetaType &type, std::nullptr_t)
    : reference(type, { static_cast<unsigned char *>(nullptr), static_cast<unsigned char *>(nullptr) + type.GetSize() })
{
}


template<typename Base, typename Derived>
BaseClass BaseClass::Create()
{
    return { GetMetaType<Base>(), GetMetaType<Derived>(), calculate_offset<Base, Derived>() };
}

namespace detail
{
template<size_t Size, size_t Alignment, typename Allocator>
struct RandomAccessVtable : CopyVTable<Size, Alignment, Allocator>, RegularVTable<Size, Alignment, Allocator>
{
    typedef TypeErasureStorage<Size, Alignment, Allocator> Storage;

    MetaReference (*access)(const Storage &, std::ptrdiff_t);
    void (*advance)(Storage &, std::ptrdiff_t);
    std::ptrdiff_t (*distance)(const Storage &, const Storage &);

    template<typename T>
    RandomAccessVtable(T *)
        : CopyVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , RegularVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , access([](const Storage & it, std::ptrdiff_t index) -> MetaReference
        {
            return MetaReference(it.template get_reference<T>()[index]);
        })
        , advance([](Storage & it, std::ptrdiff_t distance)
        {
            it.template get_reference<T>() += distance;
        })
        , distance([](const Storage & lhs, const Storage & rhs) -> std::ptrdiff_t
        {
            return rhs.template get_reference<T>() - lhs.template get_reference<T>();
        })
    {
    }
};
}

struct MetaRandomAccessIterator : std::iterator<std::random_access_iterator_tag, MetaReference>, RegularTypeErasure<4 * sizeof(void *), detail::RandomAccessVtable>
{
    typedef RegularTypeErasure<4 * sizeof(void *), detail::RandomAccessVtable> Base;
    using Base::Base;

    MetaReference operator*() const
    {
        return vtable->access(storage, 0);
    }
    MetaPointer operator->() const
    {
        return MetaPointer(**this);
    }
    MetaReference operator[](std::ptrdiff_t index) const
    {
        return vtable->access(storage, index);
    }
    MetaRandomAccessIterator & operator++()
    {
        vtable->advance(storage, 1);
        return *this;
    }
    MetaRandomAccessIterator operator++(int)
    {
        MetaRandomAccessIterator copy(*this);
        ++*this;
        return copy;
    }
    MetaRandomAccessIterator & operator--()
    {
        vtable->advance(storage, -1);
        return *this;
    }
    MetaRandomAccessIterator operator--(int)
    {
        MetaRandomAccessIterator copy(*this);
        --*this;
        return copy;
    }
    MetaRandomAccessIterator operator+(std::ptrdiff_t offset) const
    {
        return MetaRandomAccessIterator(*this) += offset;
    }
    MetaRandomAccessIterator operator-(std::ptrdiff_t offset) const
    {
        return MetaRandomAccessIterator(*this) -= offset;
    }
    std::ptrdiff_t operator-(const MetaRandomAccessIterator & other) const
    {
        return vtable->distance(other.storage, storage);
    }
    MetaRandomAccessIterator & operator+=(std::ptrdiff_t offset)
    {
        vtable->advance(storage, offset);
        return *this;
    }
    MetaRandomAccessIterator & operator-=(std::ptrdiff_t offset)
    {
        vtable->advance(storage, -offset);
        return *this;
    }
};
template<typename T>
MetaType::ListInfo MetaType::ListInfo::Creator<T>::Create()
{
    return
    {
        GetMetaType<typename T::value_type>(),
        [](const MetaReference & object) -> size_t
        {
            return object.Get<T>().size();
        },
        [](MetaReference & object, MetaReference && value)
        {
            object.Get<T>().push_back(std::move(value.Get<typename T::value_type>()));
        },
        [](MetaReference & object) -> MetaRandomAccessIterator
        {
            return object.Get<T>().begin();
        },
        [](MetaReference & object) -> MetaRandomAccessIterator
        {
            return object.Get<T>().end();
        }
    };
}
inline MetaRandomAccessIterator MetaType::ArrayInfo::end(MetaReference & object) const
{
    return begin(object) + array_size;
}
template<typename T>
inline MetaRandomAccessIterator MetaType::ArrayInfo::BeginSpecialization<T>::begin(MetaReference & object)
{
    using std::begin;
    return begin(object.Get<T>());
}

struct BaseClass::BaseMemberCollection
{
    static MetaReference static_cast_reference(ptrdiff_t offset, const MetaType & new_type, const MetaReference & reference)
    {
        unsigned char * new_begin = reference.GetMemory().begin() + offset;
        unsigned char * new_end = new_begin + new_type.GetSize();
        return MetaReference(new_type, { new_begin, new_end });
    }
    static ConstMetaReference static_cast_reference(ptrdiff_t offset, const MetaType & new_type, ConstMetaReference & reference)
    {
        return static_cast_reference(offset, new_type, const_cast<MetaReference &>(static_cast<const MetaReference &>(reference)));
    }

    struct BaseMember
    {
        BaseMember(MetaMember member, ptrdiff_t offset)
            : offset(offset), member(std::move(member))
        {
        }

        ptrdiff_t GetOffset() const
        {
            return member.GetOffset() + offset;
        }
        const HashedString & GetName() const
        {
            return member.GetName();
        }
        const MetaType & GetMemberType() const
        {
            return member.GetMemberType();
        }
        template<typename T>
        T & Get(MetaReference owner) const
        {
            return GetReference(owner).Get<T>();
        }
        template<typename T>
        const T & Get(ConstMetaReference owner) const
        {
            return GetReference(owner).Get<T>();
        }
        MetaReference GetReference(MetaReference owner) const
        {
            return MetaReference(GetMemberType(), owner.GetMemory().subrange(GetOffset(), member.GetMemberType().GetSize()));
        }
        ConstMetaReference GetReference(ConstMetaReference owner) const
        {
            return ConstMetaReference(GetMemberType(), owner.GetMemory().subrange(GetOffset(), member.GetMemberType().GetSize()));
        }

    private:
        friend BaseClass;
        ptrdiff_t offset;
        MetaMember member;
    };
    struct BaseConditionalMember
    {
        BaseConditionalMember(MetaConditionalMember member, ptrdiff_t offset)
            : offset(offset), member(std::move(member))
        {
        }

        bool ObjectHasMember(const MetaReference & object) const
        {
            return member.ObjectHasMember(static_cast_reference(offset, member.GetMember().GetMemberType(), object));
        }
        BaseMember GetMember() const
        {
            return { member.GetMember(), offset };
        }

    private:
        friend BaseClass;
        ptrdiff_t offset;
        MetaConditionalMember member;
    };

    std::vector<BaseMember> members;
    std::vector<BaseConditionalMember> conditional_members;
};


struct MetaType::StructInfo::AllMemberCollection
{
    AllMemberCollection(MemberCollection direct_members, BaseClass::BaseMemberCollection base_members);
    MemberCollection direct_members;
    BaseClass::BaseMemberCollection base_members;
};
} // end namespace meta

#include "metav3/default_types.hpp"

