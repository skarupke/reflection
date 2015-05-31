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
#include "typeTraits.hpp"
#include "util/string/hashed_string.hpp"
#include "util/memoizingMap.hpp"
#include "metafwd.hpp"
#include <typeinfo>
#include "os/memoryManager.hpp"

namespace meta
{
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
struct MetaOwningVariant;
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
    void assign(const MetaReference &);
    void assign(MetaReference &&);
    void assign(const MetaOwningVariant &);
    void assign(MetaOwningVariant &&);

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

    bool operator==(const MetaReference & other) const;
    bool operator!=(const MetaReference & other) const;
    bool operator<(const MetaReference & other) const;
    bool operator<=(const MetaReference & other) const;
    bool operator>(const MetaReference & other) const;
    bool operator>=(const MetaReference & other) const;

private:
    const MetaType * type;
    Range<unsigned char> memory;
};

struct MetaProxy : MetaReference
{
    explicit MetaProxy(MetaReference ref)
        : MetaReference(ref)
    {
    }
    MetaProxy(const MetaProxy &) = default;
    MetaProxy(MetaProxy &&) = default;
    MetaProxy & operator=(const MetaProxy & other)
    {
        assign(other);
        return *this;
    }
    MetaProxy & operator=(const MetaReference & other)
    {
        assign(other);
        return *this;
    }
    MetaProxy & operator=(const MetaOwningVariant & other)
    {
        assign(other);
        return *this;
    }
    MetaProxy & operator=(MetaProxy && other)
    {
        assign(std::move(other));
        return *this;
    }
    MetaProxy & operator=(MetaReference && other)
    {
        assign(std::move(other));
        return *this;
    }
    MetaProxy & operator=(MetaOwningVariant && other)
    {
        assign(std::move(other));
        return *this;
    }
};
void swap(MetaProxy & lhs, MetaProxy & rhs);
void swap(MetaProxy && lhs, MetaProxy && rhs);

struct ConstMetaReference
{
    template<typename T>
    ConstMetaReference(const T & object);
    ConstMetaReference(const MetaType & type, Range<const unsigned char> memory);
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

    bool operator==(const ConstMetaReference & other) const;
    bool operator!=(const ConstMetaReference & other) const;
    bool operator<(const ConstMetaReference & other) const;
    bool operator<=(const ConstMetaReference & other) const;
    bool operator>(const ConstMetaReference & other) const;
    bool operator>=(const ConstMetaReference & other) const;
    bool operator==(const MetaReference & other) const;
    bool operator!=(const MetaReference & other) const;
    bool operator<(const MetaReference & other) const;
    bool operator<=(const MetaReference & other) const;
    bool operator>(const MetaReference & other) const;
    bool operator>=(const MetaReference & other) const;

private:
    MetaReference reference;
};
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

    MetaProxy operator*()
    {
        return MetaProxy(reference);
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

            int unused;
            Derived derived;
        } some_address;
        return reinterpret_cast<unsigned char *>(static_cast<Base *>(&some_address.derived)) - reinterpret_cast<unsigned char *>(&some_address.derived);
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
    void CopyConstruct(Range<unsigned char> memory, Range<const unsigned char> other) const
    {
        general.copy_construct(memory, other);
    }
    void MoveConstruct(Range<unsigned char> memory, Range<unsigned char> && other) const
    {
        general.move_construct(memory, std::move(other));
    }
    void Assign(Range<unsigned char> lhs, Range<const unsigned char> rhs) const
    {
        general.copy_assign(lhs, rhs);
    }
    void Assign(Range<unsigned char> lhs, Range<unsigned char> && rhs) const
    {
        general.move_assign(lhs, std::move(rhs));
    }
    void Destroy(Range<unsigned char> memory) const
    {
        general.destroy(memory);
    }
    bool Equals(Range<const unsigned char> lhs, Range<const unsigned char> rhs) const
    {
        return general.equals(lhs, rhs);
    }
    bool LessThan(Range<const unsigned char> lhs, Range<const unsigned char> rhs) const
    {
        return general.less_than(lhs, rhs);
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
                &CopyConstruct<T, meta::is_copy_constructible<T>::value>::copy,
                &MoveConstruct<T, meta::is_move_constructible<T>::value>::move,
                &CopyAssign<T, meta::is_copy_assignable<T>::value>::copy,
                &MoveAssign<T, std::is_move_assignable<T>::value>::move,
                &Destroy<T>::destroy,
                &EqualityComparer<T, meta::is_equality_comparable<T>::value>::Equals,
                &LessThanComparer<T, meta::is_less_than_comparable<T>::value>::LessThan
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
        template<typename T, bool is_copy_constructible>
        struct CopyConstruct;
        template<typename T>
        struct CopyConstruct<T, true>
        {
            static void copy(Range<unsigned char> memory, Range<const unsigned char> other)
            {
                detail::assert_range_size_and_alignment(memory, sizeof(T), alignof(T));
                new (memory.begin()) T(*reinterpret_cast<const T *>(other.begin()));
            }
        };
        template<typename T>
        struct CopyConstruct<T, false>
        {
            static void copy(Range<unsigned char>, Range<const unsigned char>)
            {
                RAW_ASSERT(false, "the type is not copy constructible");
            }
        };
        template<typename T, bool is_copy_or_move_constructible>
        struct MoveConstruct;
        template<typename T>
        struct MoveConstruct<T, true>
        {
            static void move(Range<unsigned char> memory, Range<unsigned char> && other)
            {
                detail::assert_range_size_and_alignment(memory, sizeof(T), alignof(T));
                new (memory.begin()) T(std::move(*reinterpret_cast<T *>(other.begin())));
            }
        };
        template<typename T>
        struct MoveConstruct<T, false>
        {
            static void move(Range<unsigned char>, Range<unsigned char> &&)
            {
                RAW_ASSERT(false, "the type is not copy or move constructible");
            }
        };
        template<typename T, bool is_copy_assignable>
        struct CopyAssign;
        template<typename T>
        struct CopyAssign<T, true>
        {
            static void copy(Range<unsigned char> lhs, Range<const unsigned char> rhs)
            {
                *reinterpret_cast<T *>(lhs.begin()) = *reinterpret_cast<const T *>(rhs.begin());
            }
        };
        template<typename T>
        struct CopyAssign<T, false>
        {
            static void copy(Range<unsigned char>, Range<const unsigned char>)
            {
                RAW_ASSERT(false, "the type is not copy assignable");
            }
        };
        template<typename T, bool is_copy_or_move_assignable>
        struct MoveAssign;
        template<typename T>
        struct MoveAssign<T, true>
        {
            static void move(Range<unsigned char> lhs, Range<unsigned char> && rhs)
            {
                *reinterpret_cast<T *>(lhs.begin()) = std::move(*reinterpret_cast<T *>(rhs.begin()));
            }
        };
        template<typename T>
        struct MoveAssign<T, false>
        {
            static void move(Range<unsigned char>, Range<unsigned char> &&)
            {
                RAW_ASSERT(false, "the type is not copy or move assignable");
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
        template<typename T, bool is_Comparable>
        struct EqualityComparer;
        template<typename T>
        struct EqualityComparer<T, true>
        {
            static bool Equals(Range<const unsigned char> lhs, Range<const unsigned char> rhs)
            {
                return *reinterpret_cast<const T *>(lhs.begin()) == *reinterpret_cast<const T *>(rhs.begin());
            }
        };
        template<typename T>
        struct EqualityComparer<T, false>
        {
            static bool Equals(Range<const unsigned char>, Range<const unsigned char>)
            {
                return detail::throw_not_equality_comparable();
            }
        };
        template<typename T, bool is_Comparable>
        struct LessThanComparer;
        template<typename T>
        struct LessThanComparer<T, true>
        {
            static bool LessThan(Range<const unsigned char> lhs, Range<const unsigned char> rhs)
            {
                return *reinterpret_cast<const T *>(lhs.begin()) < *reinterpret_cast<const T *>(rhs.begin());
            }
        };
        template<typename T>
        struct LessThanComparer<T, false>
        {
            static bool LessThan(Range<const unsigned char>, Range<const unsigned char>)
            {
                return detail::throw_not_less_than_comparable();
            }
        };

        const std::type_info & type_info;
        size_t size;
        uint32_t alignment;
        allocate_pointer (*allocate)();
        void (*construct)(Range<unsigned char>);
        void (*copy_construct)(Range<unsigned char>, Range<const unsigned char>);
        void (*move_construct)(Range<unsigned char>, Range<unsigned char> &&);
        void (*copy_assign)(Range<unsigned char>, Range<const unsigned char>);
        void (*move_assign)(Range<unsigned char>, Range<unsigned char> &&);
        void (*destroy)(Range<unsigned char>);
        bool (*equals)(Range<const unsigned char> lhs, Range<const unsigned char> rhs);
        bool (*less_than)(Range<const unsigned char> lhs, Range<const unsigned char> rhs);
    };
    const AllTypesThatExist category;
    const AccessType access_type;

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
        void (*pop_back)(MetaReference &);
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
        std::pair<SetIterator, SetIterator> (*equal_range)(const MetaReference & set, const MetaReference & key);
        SetIterator (*erase)(MetaReference & set, SetIterator);
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
                    [](const MetaReference & set, const MetaReference & key) -> std::pair<SetIterator, SetIterator>
                    {
                        return const_cast<T &>(set.Get<T>()).equal_range(key.Get<typename T::key_type>());
                    },
                    [](MetaReference & object, SetIterator it) -> SetIterator
                    {
                        return object.Get<T>().erase(*it.target<typename T::iterator>());
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
        std::pair<MapIterator, MapIterator> (*equal_range)(const MetaReference & map, const MetaReference & key);
        MapIterator (*erase)(MetaReference & map, MapIterator);
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
                    [](const MetaReference & object, const MetaReference & element) -> std::pair<MapIterator, MapIterator>
                    {
                        return const_cast<T &>(object.Get<T>()).equal_range(element.Get<typename T::key_type>());
                    },
                    [](MetaReference & object, MapIterator it) -> MapIterator
                    {
                        return object.Get<T>().erase(*it.target<typename T::iterator>());
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
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "access type mismatch");
            return PointerToStructInfo(GetMetaType<typename meta::remove_pointer<T>::type>(), &templated_get_as_pointer<T>, &templated_assign<T>);
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
            pointer.Get<T>() = T(static_cast<typename meta::remove_pointer<T>::type *>(static_cast<void *>(info.cast_reference(target.GetType(), std::move(target)).GetMemory().begin())));
        }
    };
    struct TypeErasureInfo
    {
        template<typename T>
        static TypeErasureInfo Create()
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "access type mismatch");
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
        return { GeneralInformation::CreateForType<T>(), category, MetaTraits<T>::access_type };
    }

    template<typename T>
    static MetaType RegisterString()
    {
        return { StringInfo::Create<T>(), GeneralInformation::CreateForType<T>(), MetaTraits<T>::access_type };
    }

    template<typename T>
    static MetaType RegisterEnum(std::map<int32_t, HashedString> values)
    {
        static_assert(sizeof(T) == sizeof(int32_t), "I only support enums that are int-sized right now.");
        static_assert(MetaTraits<T>::access_type == PASS_BY_VALUE, "enums have to be cheap to copy");
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
        return  {ArrayInfo::Create<T>(value_type, array_size), GeneralInformation::CreateForType<T>(), MetaTraits<T>::access_type };
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
        return { StructInfo(std::move(name), current_version, get_members, get_bases), GeneralInformation::CreateForType<T>(), MetaTraits<T>::access_type };
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

    MetaType(GeneralInformation, AllTypesThatExist category, AccessType access_type);
    MetaType(StringInfo string_info, GeneralInformation general, AccessType access_type);
    MetaType(GeneralInformation, std::map<int32_t, HashedString> enum_values);
    MetaType(ListInfo list_info, GeneralInformation);
    MetaType(ArrayInfo array_info, GeneralInformation, AccessType access_type);
    MetaType(SetInfo set_info, GeneralInformation);
    MetaType(MapInfo map_info, GeneralInformation);
    MetaType(StructInfo struct_info, GeneralInformation, AccessType access_type);
    MetaType(PointerToStructInfo, GeneralInformation);
    MetaType(TypeErasureInfo, GeneralInformation);
    MetaType(MetaType &&);
};

template<typename T>
const MetaType & GetMetaType()
{
    return MetaType::MetaTypeConstructor<T>::type;
}

template<typename T, typename>
struct MetaTraits
{
    static constexpr const AccessType access_type = PASS_BY_REFERENCE;
};
template<typename T>
struct MetaTraits<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    static constexpr const AccessType access_type = PASS_BY_VALUE;
};
template<typename T>
struct MetaTraits<Range<T> >
{
    static constexpr const AccessType access_type = PASS_BY_VALUE;
};

struct MetaMember
{
    template<typename T, typename S>
    MetaMember(HashedString name, T (*getter)(const S &), MetaOwningVariant (*get_variant)(const MetaReference &), void (*setter)(MetaReference &, MetaReference &))
        : name(std::move(name)), struct_type(&GetMetaType<S>()), type(&GetMetaType<T>()), value_member_access(getter, get_variant, setter)
    {
    }
    MetaMember(HashedString name, MetaReference (*get_ref)(MetaReference &), const MetaType & type, const MetaType & struct_type)
        : name(std::move(name)), struct_type(&struct_type), type(&type), ref_member_access(get_ref)
    {
    }
    MetaMember(HashedString name, ConstMetaReference (*get_ref)(ConstMetaReference &), void (*setter)(MetaReference &, MetaReference &), const MetaType & type, const MetaType & struct_type)
        : name(std::move(name)), struct_type(&struct_type), type(&type), ref_value_member_access(get_ref, setter)
    {
    }

    template<typename T, typename S, T S::* Member>
    static MetaMember CreateFromMemPtr(HashedString name)
    {
        return MemberCreatorSpecialization<T, S, Member, MetaTraits<T>::access_type>()(std::move(name));
    }
    template<typename T, typename S, T (S::*Getter)() const, void (S::*Setter)(T)>
    static MetaMember CreateFromGetterSetter(HashedString name)
    {
        return { std::move(name), &ValueMemberAccess::wrap_member_getter<T, S, Getter>, &ValueMemberAccess::wrap_member_get_variant<T, S, Getter>, &ValueMemberAccess::wrap_member_setter<T, S, Setter> };
    }
    template<typename T, typename S, T (*Getter)(const S &), void (*Setter)(S &, T)>
    static MetaMember CreateFromGetterSetter(HashedString name)
    {
        return { std::move(name), &ValueMemberAccess::wrap_getter<T, S, Getter>, &ValueMemberAccess::wrap_get_variant<T, S, Getter>, &ValueMemberAccess::wrap_setter<T, S, Setter> };
    }
    template<typename T, typename S, const T & (S::*Getter)() const, void (S::*Setter)(T)>
    static MetaMember CreateFromGetterSetter(HashedString name)
    {
        return { std::move(name), &RefValueMemberAccess::member_get_ref<T, S, Getter>, &RefValueMemberAccess::member_set<T, S, Setter>, GetMetaType<T>(), GetMetaType<S>() };
    }
    template<typename T, typename S, T & (S::*GetRef)()>
    static MetaMember CreateFromGetRef(HashedString name)
    {
        return { std::move(name), &RefMemberAccess::member_get_variant_ref<T, S, GetRef>, GetMetaType<T>(), GetMetaType<S>() };
    }
    template<typename T, typename S, T & (*GetRef)(S &)>
    static MetaMember CreateFromGetRef(HashedString name)
    {
        return { std::move(name), &RefMemberAccess::get_variant_ref<T, S, GetRef>, GetMetaType<T>(), GetMetaType<S>() };
    }

    MetaOwningVariant Get(ConstMetaReference object) const;
    void Set(MetaReference object, MetaReference value) const;
    void ValueRefSet(MetaReference object, MetaReference value) const;
    MetaReference GetRef(MetaReference object) const;
    ConstMetaReference GetCRef(ConstMetaReference object) const;

    const HashedString & GetName() const
    {
        return name;
    }
    const MetaType & GetStructType() const
    {
        return *struct_type;
    }
    const MetaType & GetType() const
    {
        return *type;
    }

private:

    template<typename T, typename S, T S::* Member, AccessType AccessType>
    struct MemberCreatorSpecialization;
    template<typename T, typename S, T S::* Member>
    struct MemberCreatorSpecialization<T, S, Member, PASS_BY_VALUE>
    {
        MetaMember operator()(HashedString name) const
        {
            RAW_ASSERT(GetMetaType<T>().access_type == PASS_BY_VALUE);
            return { std::move(name), &ValueMemberAccess::member_ptr_getter<T, S, Member>, &ValueMemberAccess::member_ptr_get_variant<T, S, Member>, &ValueMemberAccess::member_ptr_setter<T, S, Member> };
        }
    };
    template<typename T, typename S, T S::* Member>
    struct MemberCreatorSpecialization<T, S, Member, PASS_BY_REFERENCE>
    {
        MetaMember operator()(HashedString name) const
        {
            RAW_ASSERT(GetMetaType<T>().access_type == PASS_BY_REFERENCE);
            return { std::move(name), &RefMemberAccess::member_ptr_get_variant_ref<T, S, Member>, meta::GetMetaType<T>(), meta::GetMetaType<S>() };
        }
    };
    template<typename T, typename S, T S::* Member>
    struct MemberCreatorSpecialization<T, S, Member, GET_BY_REF_SET_BY_VALUE>
    {
        MetaMember operator()(HashedString name) const
        {
            RAW_ASSERT(GetMetaType<T>().access_type == GET_BY_REF_SET_BY_VALUE);
            return { std::move(name), &RefValueMemberAccess::member_ptr_get_ref<T, S, Member>, &RefValueMemberAccess::member_ptr_set<T, S, Member>, meta::GetMetaType<T>(), meta::GetMetaType<S>() };
        }
    };

    struct ValueMemberAccess
    {
        typedef void * (*GetterFunction)(const void * object);
        typedef MetaOwningVariant (*GetVariantFunction)(const MetaReference &);
        typedef void (*SetterFunction)(MetaReference &, MetaReference &);
        template<typename T, typename S>
        ValueMemberAccess(T (*get)(const S &), GetVariantFunction get_variant, SetterFunction set)
            : get(reinterpret_cast<GetterFunction>(get)), get_variant(get_variant), set(set)
        {
            static_assert(MetaTraits<T>::access_type == PASS_BY_VALUE, "mismatch in the access type");
        }

        GetterFunction get;
        GetVariantFunction get_variant;
        SetterFunction set;

        template<typename T, typename S, T S::* Member>
        static T member_ptr_getter(const S & object)
        {
            return object.*Member;
        }
        template<typename T, typename S, T S::* Member>
        static void member_ptr_setter(MetaReference & object, MetaReference & value)
        {
            object.Get<S>().*Member = value.Get<T>();
        }
        template<typename T, typename S, T (S::*Getter)() const>
        static T wrap_member_getter(const S & object)
        {
            return (object.*Getter)();
        }
        template<typename T, typename S, T (*Getter)(const S &)>
        static T wrap_getter(const S & object)
        {
            return Getter(object);
        }
        template<typename T, typename S, void (S::*Setter)(T)>
        static void wrap_member_setter(MetaReference & object, MetaReference & value)
        {
            (object.Get<S>().*Setter)(value.Get<T>());
        }
        template<typename T, typename S, void (*Setter)(S &, T)>
        static void wrap_setter(MetaReference & object, MetaReference & value)
        {
            Setter(object.Get<S>(), value.Get<T>());
        }
        template<typename T, typename S, T S::* Member>
        static MetaOwningVariant member_ptr_get_variant(const MetaReference & object);
        template<typename T, typename S, T (S::*Getter)() const>
        static MetaOwningVariant wrap_member_get_variant(const MetaReference & object);
        template<typename T, typename S, T (*Getter)(const S &)>
        static MetaOwningVariant wrap_get_variant(const MetaReference & object);
    };
    struct RefMemberAccess
    {
        typedef MetaReference (*GetRefFunction)(MetaReference &);
        RefMemberAccess(GetRefFunction get_ref);

        GetRefFunction get_ref;

        template<typename T, typename S, T S::* Member>
        static MetaReference member_ptr_get_variant_ref(MetaReference & object)
        {
            static_assert(MetaTraits<T>::access_type == PASS_BY_REFERENCE, "mismatch in the access type");
            return { object.Get<S>().*Member };
        }
        template<typename T, typename S, T & (S::*GetRef)()>
        static MetaReference member_get_variant_ref(MetaReference & object)
        {
            static_assert(MetaTraits<T>::access_type == PASS_BY_REFERENCE, "mismatch in the access type");
            return { (object.Get<S>().*GetRef)() };
        }
        template<typename T, typename S, T & (*GetRef)(S &)>
        static MetaReference get_variant_ref(MetaReference & object)
        {
            static_assert(MetaTraits<T>::access_type == PASS_BY_REFERENCE, "mismatch in the access type");
            return { GetRef(object.Get<S>()) };
        }
    };
    struct RefValueMemberAccess
    {
        typedef ConstMetaReference (*GetRefFunction)(ConstMetaReference &);
        typedef void (*SetterFunction)(MetaReference &, MetaReference &);

        RefValueMemberAccess(GetRefFunction get_ref, SetterFunction setter)
            : get_ref(get_ref), setter(setter)
        {
        }

        GetRefFunction get_ref;
        SetterFunction setter;

        template<typename T, typename S, T S::* Member>
        static ConstMetaReference member_ptr_get_ref(ConstMetaReference & object)
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "mismatch in the access type");
            return { object.Get<S>().*Member };
        }
        template<typename T, typename S, T S::*Member>
        static void member_ptr_set(MetaReference & object, MetaReference & value)
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "mismatch in the access type");
            object.Get<S>().*Member = std::move(value.Get<T>());
        }
        template<typename T, typename S, const T & (S::*GetRef)() const>
        static ConstMetaReference member_get_ref(ConstMetaReference & object)
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "mismatch in the access type");
            return { (object.Get<S>().*GetRef)() };
        }
        template<typename T, typename S, void (S::*Set)(T)>
        static void member_set(MetaReference & object, MetaReference & value)
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "mismatch in the access type");
            (object.Get<S>().*Set)(std::move(value.Get<T>()));
        }
        template<typename T, typename S, const T & (*GetRef)(const S &)>
        static ConstMetaReference function_get_ref(ConstMetaReference & object)
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "mismatch in the access type");
            return { GetRef(object.Get<S>()) };
        }
        template<typename T, typename S, void (*Set)(S &, T)>
        static void function_set(MetaReference & object, MetaReference & value)
        {
            static_assert(MetaTraits<T>::access_type == GET_BY_REF_SET_BY_VALUE, "mismatch in the access type");
            Set(object.Get<S>(), std::move(value.Get<T>()));
        }
    };

    HashedString name;
    const MetaType * struct_type;
    const MetaType * type;

    union
    {
        ValueMemberAccess value_member_access;
        RefMemberAccess ref_member_access;
        RefValueMemberAccess ref_value_member_access;
    };
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

struct MetaOwningMemory
{
    MetaOwningMemory(Range<unsigned char> memory, const MetaType & type)
        : type(type), memory(memory)
    {
        type.Construct(this->memory);
    }
    MetaOwningMemory(Range<unsigned char> memory, const MetaReference & other)
        : type(other.GetType()), memory(memory)
    {
        type.CopyConstruct(this->memory, other.GetMemory());
    }
    MetaOwningMemory(Range<unsigned char> memory, MetaReference && other)
        : type(other.GetType()), memory(memory)
    {
        type.MoveConstruct(this->memory, other.GetMemory());
    }

    ~MetaOwningMemory()
    {
        type.Destroy(memory);
    }

    MetaReference GetVariant() const
    {
        return { type, memory };
    }

private:
    const MetaType & type;
    Range<unsigned char> memory;
};

struct MetaOwningVariant
{
    template<typename T, typename Decayed = typename std::decay<T>::type>
    MetaOwningVariant(forwarding_constructor, T && object)
        : uses_local_storage(use_local_storage(sizeof(T), alignof(T)))
        , type(&GetMetaType<Decayed>())
    {
        default_initialize([&](unsigned char * memory)
        {
            new (memory) Decayed(std::forward<T>(object));
        });
    }

    MetaOwningVariant(const MetaReference &);
    MetaOwningVariant(MetaReference &&);
    MetaOwningVariant(const MetaOwningVariant &);
    MetaOwningVariant(MetaOwningVariant &&);
    MetaOwningVariant & operator=(MetaOwningVariant);
    ~MetaOwningVariant();

    MetaReference GetVariant() const;

    bool operator==(const MetaReference & other) const;
    bool operator!=(const MetaReference & other) const;
    bool operator<(const MetaReference & other) const;
    bool operator<=(const MetaReference & other) const;
    bool operator>(const MetaReference & other) const;
    bool operator>=(const MetaReference & other) const;

private:
    static constexpr const size_t Alignment = 16;
    static constexpr const size_t Size = 127;
    union
    {
        alignas(Alignment) unsigned char storage[Size];
        MetaType::allocate_pointer heap_storage;
    };
    bool uses_local_storage;
    const MetaType * type;

    inline static bool use_local_storage(const size_t size, const size_t alignment)
    {
        return size <= Size && (Alignment % alignment) == 0;
    }

    static bool use_local_storage(const MetaType & type);
    MetaType::allocate_pointer & get_ptr_memory();

    template<typename Function>
    void default_initialize(const Function & function)
    {
        if (uses_local_storage)
        {
            function(storage);
        }
        else
        {
            MetaType::allocate_pointer memory = type->Allocate();
            function(memory.get());
            new (&get_ptr_memory()) MetaType::allocate_pointer(std::move(memory));
        }
    }
};
bool operator==(const MetaReference & lhs, const MetaOwningVariant & rhs);
bool operator!=(const MetaReference & lhs, const MetaOwningVariant & rhs);
bool operator<(const MetaReference & lhs, const MetaOwningVariant & rhs);
bool operator<=(const MetaReference & lhs, const MetaOwningVariant & rhs);
bool operator>(const MetaReference & lhs, const MetaOwningVariant & rhs);
bool operator>=(const MetaReference & lhs, const MetaOwningVariant & rhs);

template<typename T, typename S, T S::* Member>
inline MetaOwningVariant MetaMember::ValueMemberAccess::member_ptr_get_variant(const MetaReference & object)
{
    return { forwarding_constructor{}, object.Get<S>().*Member };
}
template<typename T, typename S, T (S::*Getter)() const>
inline MetaOwningVariant MetaMember::ValueMemberAccess::wrap_member_get_variant(const MetaReference & object)
{
    return { forwarding_constructor{}, (object.Get<S>().*Getter)() };
}
template<typename T, typename S, T (*Getter)(const S &)>
inline MetaOwningVariant MetaMember::ValueMemberAccess::wrap_get_variant(const MetaReference & object)
{
    return { forwarding_constructor{}, Getter(object.Get<S>()) };
}

template<>
struct MetaType::MetaTypeConstructor<MetaReference>; // intentionally not defined

template<typename T>
inline MetaReference::MetaReference(T & object)
    : type(&GetMetaType<T>())
    , memory(&reinterpret_cast<unsigned char &>(object), &reinterpret_cast<unsigned char &>(object) + sizeof(T))
{
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

    MetaProxy (*access)(const Storage &, std::ptrdiff_t);
    void (*advance)(Storage &, std::ptrdiff_t);
    std::ptrdiff_t (*distance)(const Storage &, const Storage &);

    template<typename T>
    RandomAccessVtable(T *)
        : CopyVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , RegularVTable<Size, Alignment, Allocator>(static_cast<T *>(nullptr))
        , access([](const Storage & it, std::ptrdiff_t index) -> MetaProxy
        {
            return MetaProxy(it.template get_reference<T>()[index]);
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

struct MetaRandomAccessIterator : std::iterator<std::random_access_iterator_tag, MetaOwningVariant>, RegularTypeErasure<4 * sizeof(void *), detail::RandomAccessVtable>
{
    typedef RegularTypeErasure<4 * sizeof(void *), detail::RandomAccessVtable> Base;
    using Base::Base;

    MetaProxy operator*() const
    {
        return vtable->access(storage, 0);
    }
    MetaPointer operator->() const
    {
        return MetaPointer(**this);
    }
    MetaProxy operator[](std::ptrdiff_t index) const
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
        [](MetaReference & object)
        {
            object.Get<T>().pop_back();
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

        MetaOwningVariant Get(ConstMetaReference object) const
        {
            return member.Get(static_cast_reference(offset, member.GetStructType(), object));
        }
        void Set(MetaReference object, MetaReference value) const
        {
            return member.Set(static_cast_reference(offset, member.GetStructType(), object), value);
        }
        void ValueRefSet(MetaReference object, MetaReference value) const
        {
            return member.ValueRefSet(static_cast_reference(offset, member.GetStructType(), object), value);
        }
        MetaReference GetRef(MetaReference object) const
        {
            return member.GetRef(static_cast_reference(offset, member.GetStructType(), object));
        }
        ConstMetaReference GetCRef(ConstMetaReference object) const
        {
            return member.GetCRef(static_cast_reference(offset, member.GetStructType(), object));
        }

        const HashedString & GetName() const
        {
            return member.GetName();
        }
        const MetaType & GetType() const
        {
            return member.GetType();
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
            return member.ObjectHasMember(static_cast_reference(offset, member.GetMember().GetType(), object));
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

#include "defaultTraits.hpp"
