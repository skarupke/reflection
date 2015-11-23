#pragma once

#include "util/view.hpp"
#include <cstdint>
#include <debug/assert.hpp>
#include "metafast/metafast_type_traits.hpp"
#include <ostream>
#include <iterator>
#include "util/stl_memory_forward.hpp"
#include "metav3/metav3.hpp"
#include "util/pp_concat.hpp"

namespace metaf
{

typedef unsigned char byte;

struct BinaryInput
{
    BinaryInput(ArrayView<const byte> input)
        : input(input)
    {
    }

    template<typename T>
    void memcpy(T & output)
    {
        auto end = input.begin() + sizeof(T);
        std::copy(input.begin(), end, reinterpret_cast<byte *>(std::addressof(output)));
        input = { end, input.end() };
    }
    template<typename T>
    T read_memcpy()
    {
        T value;
        memcpy(value);
        return value;
    }

    template<typename T>
    void step_back()
    {
        input = { input.begin() - sizeof(T), input.end() };
    }
    template<typename T>
    void step_back(const T &)
    {
        step_back<T>();
    }

    ArrayView<const byte> input;
};

struct BinaryOutput
{
    BinaryOutput(std::ostream & output)
        : output(output)
    {
    }

    template<typename T>
    void memcpy(const T & data)
    {
        auto begin = reinterpret_cast<const char *>(std::addressof(data));
        std::copy(begin, begin + sizeof(T), std::ostreambuf_iterator<char>(output));
    }
    typedef std::ostream::pos_type pos_type;
    pos_type current_position() const
    {
        return output.tellp();
    }
    void go_to_position(pos_type position)
    {
        output.seekp(position);
    }

private:
    std::ostream & output;
};


template<typename T, typename Ar>
void reflect_registered_class(Ar & archive, int8_t version);
template<typename T>
struct OptimisticBinaryDeserializer;
template<typename T>
struct OptimisticBinarySerializer;
template<typename T>
void optimistic_reflect_registered_class(OptimisticBinaryDeserializer<T> & archive);
template<typename T>
struct reflect_registered_class_any_archive;

#define SKIP_DEFAULT_MEMBERS

#ifdef SKIP_DEFAULT_MEMBERS
#define SPECIALIZE_OPTIMISTIC_REFLECTION(type_to_register, current_version)\
template<>\
void optimistic_reflect_registered_class<type_to_register>(OptimisticBinaryDeserializer<type_to_register> & archive)\
{\
    MemberCounter<type_to_register> count_members;\
    reflect_registered_class_any_archive<type_to_register>()(count_members, current_version);\
    archive.member_count = count_members.get_count();\
    return reflect_registered_class_any_archive<type_to_register>()(archive, current_version);\
}\
template<>\
void reflect_registered_class<type_to_register, OptimisticBinarySerializer<type_to_register>>(OptimisticBinarySerializer<type_to_register> & archive, int8_t version)\
{\
    MemberCounter<type_to_register> count_members;\
    reflect_registered_class_any_archive<type_to_register>()(count_members, version);\
    archive.member_count = count_members.get_count();\
    return reflect_registered_class_any_archive<type_to_register>()(archive, version);\
}
#else
#define SPECIALIZE_OPTIMISTIC_REFLECTION(type_to_register, current_version)\
template<>\
void optimistic_reflect_registered_class<type_to_register>(OptimisticBinaryDeserializer<type_to_register> & archive)\
{\
    return reflect_registered_class_any_archive<type_to_register>()(archive, current_version);\
}\
template<>\
void reflect_registered_class<type_to_register, OptimisticBinarySerializer<type_to_register>>(OptimisticBinarySerializer<type_to_register> & archive, int8_t version)\
{\
    return reflect_registered_class_any_archive<type_to_register>()(archive, version);\
}
#endif

#define REFLECT_ABSTRACT_CLASS_START(type_to_register, current_version)\
namespace metav3\
{\
template<>\
const MetaType MetaType::MetaTypeConstructor<type_to_register>::type = MetaType::RegisterStruct<type_to_register>(#type_to_register, current_version, &metaf::detail::GetBoth<type_to_register>);\
}\
namespace metaf\
{\
template<>\
struct reflect_registered_class_any_archive<type_to_register>\
{\
    template<template<typename> class Ar, typename T>\
    inline void operator()(Ar<T> & archive, int8_t version) const;\
};\
SPECIALIZE_OPTIMISTIC_REFLECTION(type_to_register, current_version)\
template<>\
void reflect_registered_class<type_to_register, detail::MetaV3Bridge<type_to_register>>(detail::MetaV3Bridge<type_to_register> & archive, int8_t version)\
{\
    return reflect_registered_class_any_archive<type_to_register>()(archive, version);\
}\
}\
template<template<typename> class Ar, typename T>\
inline void metaf::reflect_registered_class_any_archive<type_to_register>::operator()(Ar<T> & archive, int8_t version) const\
{\
    archive.begin(version);
#define REFLECT_CLASS_START(type_to_register, current_version)\
namespace metaf\
{\
static detail::RegisterDeserializeStruct<type_to_register> PP_CONCAT(register_deserialize_, __LINE__);\
}\
REFLECT_ABSTRACT_CLASS_START(type_to_register, current_version)
#define REFLECT_MEMBER(member_name) archive.member(#member_name, &T::member_name)
#define REFLECT_BASE(class_name) archive.template base<class_name>()
#define REFLECT_CLASS_END() archive.finish();\
}
#define FRIEND_REFLECT_CLASSES(type) template<typename> friend struct metaf::reflect_registered_class_any_archive

template<typename T>
struct OptimisticBinaryDeserializer;
template<typename T>
struct OptimisticBinarySerializer;
namespace detail
{

template<typename T>
struct MetaV3Bridge
{
    void begin(int8_t)
    {
    }
    template<typename M>
    void member(StringView<const char> name, M T::*m)
    {
        members.push_back(metav3::MetaMember(name.str(), m));
    }
    template<typename B>
    void base()
    {
        bases.push_back(metav3::BaseClass::Create<B, T>());
    }
    void finish()
    {
    }

    std::vector<metav3::MetaMember> members;
    std::vector<metav3::BaseClass> bases;
};
template<typename T>
metav3::MetaType::StructInfo::MembersAndBases GetBoth(int8_t version)
{
    MetaV3Bridge<T> bridge;
    reflect_registered_class<T, MetaV3Bridge<T>>(bridge, version);
    return { { std::move(bridge.members), {} }, { std::move(bridge.bases) } };
}

template<typename S>
inline void deserialize_struct(BinaryInput & input, S & data);
template<typename S>
inline void serialize_struct(BinaryOutput & output, const S & data);
template<typename S, typename = void>
struct reference_specialization
{
    void operator()(BinaryInput & input, S & data)
    {
        deserialize_struct(input, data);
    }
    void operator()(BinaryOutput & output, const S & data)
    {
        serialize_struct(output, data);
    }
};

template<typename S>
inline void reference(BinaryInput & input, S & data)
{
    reference_specialization<S>()(input, data);
}
template<typename S>
void reference(BinaryOutput & output, const S & data)
{
    reference_specialization<S>()(output, data);
}
template<typename S>
inline void array(BinaryInput & input, S * begin, S * end)
{
    for (; begin != end; ++begin)
    {
        reference(input, *begin);
    }
}
template<typename S>
void array(BinaryOutput & output, const S * begin, const S * end)
{
    for (; begin != end; ++begin)
    {
        reference(output, *begin);
    }
}

template<typename S>
inline void deserialize_struct(BinaryInput & input, S & data)
{
    OptimisticBinaryDeserializer<S> deserializer(data, input);
    optimistic_reflect_registered_class<S>(deserializer);
}
void serialize_struct(BinaryOutput & output, metav3::ConstMetaReference ref);
void deserialize_struct(BinaryInput & input, metav3::MetaReference ref);
void register_type_erased_functions(const metav3::MetaType &, void (*)(BinaryOutput &, metav3::ConstMetaReference), void (*)(BinaryInput &, metav3::MetaReference));
std::pair<void (*)(BinaryOutput &, metav3::ConstMetaReference), void (*)(BinaryInput &, metav3::MetaReference)> get_type_erased_functions(const metav3::MetaType &);
template<typename S>
struct RegisterDeserializeStruct
{
    RegisterDeserializeStruct()
    {
        register_type_erased_functions(metav3::GetMetaType<S>(), &serialize, &deserialize);
    }

    static void serialize(BinaryOutput & output, metav3::ConstMetaReference ref)
    {
        return serialize_struct(output, ref.Get<S>());
    }
    static void deserialize(BinaryInput & input, metav3::MetaReference ref)
    {
        return deserialize_struct(input, ref.Get<S>());
    }
};

#ifdef SKIP_DEFAULT_MEMBERS
template<typename S>
const S & get_default_values()
{
    static const S default_initialized{};
    return default_initialized;
}
template<typename T, typename = void>
struct default_comparer
{
    bool operator()(const T & object, const T & defaults) const
    {
        return object == defaults;
    }
};
template<typename T>
struct default_comparer<T, typename std::enable_if<!metaf::is_equality_comparable<T>::value>::type>
{
    bool operator()(const T &, const T &) const
    {
        return false;
    }
};
template<typename T, size_t Size>
struct default_comparer<T[Size], void>
{
    bool operator()(const T (&object)[Size], const T (&defaults)[Size]) const
    {
        return std::equal(object, object + Size, defaults);
    }
};

template<typename T>
inline bool is_default(const T & object, const T & defaults)
{
    return default_comparer<T>()(object, defaults);
}
template<typename S, typename M>
inline bool is_default(const S & object, M S::*member, const S & defaults)
{
    return is_default(object.*member, defaults.*member);
}

template<typename S>
inline void serialize_struct(BinaryOutput & output, const S & data, const S & defaults)
{
    OptimisticBinarySerializer<S> serializer(data, output, defaults);
    reflect_registered_class<S>(serializer, metav3::GetMetaType<S>().GetStructInfo()->GetCurrentVersion());
}
#endif
template<typename S>
inline void serialize_struct(BinaryOutput & output, const S & data)
{
#ifdef SKIP_DEFAULT_MEMBERS
    serialize_struct(output, data, get_default_values<S>());
#else
    OptimisticBinarySerializer<S> serializer(data, output);
    reflect_registered_class<S>(serializer, metav3::GetMetaType<S>().GetStructInfo()->GetCurrentVersion());
#endif
}

template<typename S>
inline void memcpy_reference(BinaryInput & input, S & data)
{
    input.memcpy(data);
}
template<typename S>
inline void memcpy_reference(BinaryOutput & output, const S & data)
{
    output.memcpy(data);
}

template<typename T>
struct reference_specialization<T, typename std::enable_if<std::is_enum<T>::value>::type>
{
    void operator()(BinaryInput & input, T & data)
    {
        if (sizeof(T) == sizeof(uint64_t))
            reference(input, reinterpret_cast<uint64_t &>(data));
        else if (sizeof(T) == sizeof(uint32_t))
            reference(input, reinterpret_cast<uint32_t &>(data));
        else
            memcpy_reference(input, data);
    }
    void operator()(BinaryOutput & output, const T & data)
    {
        if (sizeof(T) == sizeof(uint64_t))
            reference(output, reinterpret_cast<const uint64_t &>(data));
        else if (sizeof(T) == sizeof(uint32_t))
            reference(output, reinterpret_cast<const uint32_t &>(data));
        else
            memcpy_reference(output, data);
    }
};

template<typename T>
struct pointer_to_base_specialization
{
    void operator()(BinaryInput & input, T & data)
    {
        uint32_t class_hash = input.read_memcpy<uint32_t>();
        if (!class_hash)
        {
            data = T();
            return;
        }
        const metav3::MetaType & struct_type = metav3::MetaType::GetRegisteredStruct(class_hash);
        metav3::MetaReference ref = metav3::GetMetaType<T>().GetPointerToStructInfo()->AssignNew(data, struct_type);
        deserialize_struct(input, ref);
    }
    void operator()(BinaryOutput & output, const T & data)
    {
        if (!data)
        {
            memcpy_reference(output, uint32_t());
            return;
        }
        const metav3::MetaType & struct_type = metav3::MetaType::GetStructType(typeid(*data));
        memcpy_reference(output, struct_type.GetStructInfo()->GetName().get_hash());
        const metav3::MetaType & pointer_type = metav3::GetMetaType<T>();
        metav3::ConstMetaReference ref = *pointer_type.GetPointerToStructInfo()->GetAsPointer(data);
        serialize_struct(output, ref);
    }
};
template<typename T, typename D>
struct reference_specialization<std::unique_ptr<T, D>>
    : pointer_to_base_specialization<std::unique_ptr<T, D>>
{
};
}

#ifdef SKIP_DEFAULT_MEMBERS
template<typename T>
struct MemberCounter
{
    void begin(int8_t)
    {
    }
    template<typename M>
    void member(StringView<const char>, M T::*)
    {
        increment_count();
    }
    template<typename B>
    void base()
    {
        increment_count();
    }
    void finish()
    {
    }

    uint8_t get_count() const
    {
        return count;
    }

private:
    uint8_t count = 0;

    void increment_count()
    {
        ++count;
#ifndef FINAL
        if (count > 64)
            assert_count();
#endif
    }

    // this noinline hackery is needed because clang would actually
    // inline this assert four times in a struct with four members.
    // no idea why it would do that, but now we check twice. once in
    // increment_count and once again in here. this rejiggers the
    // optimizer and now it does the right thing. now this assert
    // will never even be generated in clang because it can know at
    // compile time that a struct with four members will never have
    // a count > 64
    __attribute__((noinline)) void assert_count()
    {
        RAW_ASSERT(count <= 64, "Serialization only supports structs with up to 64 members. This is required to only use one bit of overhead per member. Can you move some members to a nested struct?");
    }
};
#endif

template<typename T>
struct OptimisticBinaryDeserializer
{
    OptimisticBinaryDeserializer(T & object, BinaryInput & input)
        : object(object), input(input)
    {
    }

#ifdef SKIP_DEFAULT_MEMBERS
    void begin(int8_t)
    {
        if (member_count == 0)
            return;
        else if (member_count <= 8)
            member_flags = input.read_memcpy<uint8_t>();
        else if (member_count <= 16)
            member_flags = input.read_memcpy<uint16_t>();
        else if (member_count <= 32)
            member_flags = input.read_memcpy<uint32_t>();
        else
            input.memcpy(member_flags);
    }

    template<typename M>
    void member(StringView<const char>, M T::*m)
    {
        if (should_read_member())
        {
            read_member(m);
        }
        ++current_member;
    }
    template<typename B>
    void base()
    {
        if (should_read_member())
        {
            detail::reference(input, static_cast<B &>(object));
        }
        ++current_member;
    }
    void finish()
    {
    }

    uint8_t member_count;
#else
    void begin(int8_t)
    {
    }

    template<typename M>
    void member(StringView<const char>, M T::*m)
    {
        read_member(m);
    }
    template<typename B>
    void base()
    {
        detail::reference(input, static_cast<B &>(object));
    }
    void finish()
    {
    }
#endif

private:
    T & object;
    BinaryInput & input;
#ifdef SKIP_DEFAULT_MEMBERS
    uint8_t current_member = 0;
    uint64_t member_flags = 0;

    bool should_read_member()
    {
        return member_flags & (1ull << current_member);
    }
#endif

    template<typename M>
    void read_member(M T::*m)
    {
        detail::reference(input, object.*m);
    }
    template<typename M, size_t Size>
    void read_member(M (T::*m)[Size])
    {
        detail::array(input, object.*m, object.*m + Size);
    }
};
template<typename T>
struct OptimisticBinarySerializer
{
#ifdef SKIP_DEFAULT_MEMBERS
    OptimisticBinarySerializer(const T & object, BinaryOutput & output, const T & defaults)
        : object(object), output(output), defaults(defaults)
        , position_to_write_flag(output.current_position())
    {
    }
#else
    OptimisticBinarySerializer(const T & object, BinaryOutput & output)
        : object(object), output(output)
    {
    }
#endif

#ifdef SKIP_DEFAULT_MEMBERS
    void begin(int8_t)
    {
        if (member_count != 0)
            write_flag();
    }

    template<typename M>
    void member(StringView<const char>, M T::*m)
    {
        if (!detail::is_default(object, m, defaults))
        {
            write_member_start();
            write_member(m);
        }
        after_member();
    }
    template<typename B>
    void base()
    {
        if (!detail::is_default(static_cast<const B &>(object), static_cast<const B &>(defaults)))
        {
            write_member_start();
            detail::serialize_struct(output, static_cast<const B &>(object), static_cast<const B &>(defaults));
        }
        after_member();
    }
    void finish()
    {
        if (member_count == 0)
            return;
        BinaryOutput::pos_type position_now = output.current_position();
        output.go_to_position(position_to_write_flag);
        write_flag();
        output.go_to_position(position_now);
    }

    uint8_t member_count;
#else
    void begin(int8_t)
    {
    }

    template<typename M>
    void member(StringView<const char>, M T::*m)
    {
        write_member(m);
    }
    template<typename B>
    void base()
    {
        detail::serialize_struct(output, static_cast<const B &>(object));
    }
    void finish()
    {
    }
#endif

private:
    const T & object;
    BinaryOutput & output;
#ifdef SKIP_DEFAULT_MEMBERS
    const T & defaults;
    BinaryOutput::pos_type position_to_write_flag;
    uint8_t current_member = 0;
    uint64_t flag_to_write = 0;

    void write_flag()
    {
        if (member_count <= 8)
            detail::memcpy_reference(output, uint8_t(flag_to_write));
        else if (member_count <= 16)
            detail::memcpy_reference(output, uint16_t(flag_to_write));
        else if (member_count <= 32)
            detail::memcpy_reference(output, uint32_t(flag_to_write));
        else
            detail::memcpy_reference(output, flag_to_write);
    }

    void write_member_start()
    {
        flag_to_write |= 1ull << current_member;
    }
    void after_member()
    {
        ++current_member;
    }
#endif

    template<typename M>
    void write_member(M T::*m)
    {
        detail::reference(output, object.*m);
    }
    template<typename M, size_t Size>
    void write_member(M (T::*m)[Size])
    {
        detail::array(output, object.*m, object.*m + Size);
    }
};
template<typename T>
void read_binary(BinaryInput & input, T & to_fill)
{
    detail::reference(input, to_fill);
}
template<typename T>
void write_binary(BinaryOutput & output, const T & to_write)
{
    detail::reference(output, to_write);
}
}

#include "metafast/metafast_simple_types.hpp"
#include "metafast/metafast_stl.hpp"
