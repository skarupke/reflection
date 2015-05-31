#include "metav3/serialization/optimistic_binary.hpp"

#include "util/reusable_storage.hpp"
#include <istream>
#include <ostream>
#include "metav3/metav3.hpp"
#include <sstream>

using namespace metav3;

struct BinaryWriter
{
    std::map<const MetaType *, int> type_nums;
    BinaryWriter()
    {
        type_nums =
        {
            { &metav3::GetMetaType<bool>(), 1 },
            { &metav3::GetMetaType<char>(), 2 },
            { &metav3::GetMetaType<int8_t>(), 3 },
            { &metav3::GetMetaType<uint8_t>(), 4 },
            { &metav3::GetMetaType<int16_t>(), 5 },
            { &metav3::GetMetaType<uint16_t>(), 6 },
            { &metav3::GetMetaType<int32_t>(), 7 },
            { &metav3::GetMetaType<uint32_t>(), 8 },
            { &metav3::GetMetaType<int64_t>(), 9 },
            { &metav3::GetMetaType<uint64_t>(), 10 },
            { &metav3::GetMetaType<float>(), 11 },
            { &metav3::GetMetaType<double>(), 12 },
        };
    }

    void simple_to_binary(Range<const unsigned char> data, std::ostream & out)
    {
        out.write(reinterpret_cast<const char *>(data.begin()), data.size());
    }
    template<typename T>
    void simple_to_binary(const T & simple, std::ostream & out)
    {
        out.write(reinterpret_cast<const char *>(&simple), sizeof(T));
    }

    void string_to_binary(Range<const char> str, std::ostream & out)
    {
        simple_to_binary(uint32_t(str.size()), out);
        out.write(str.begin(), str.size());
    }
    void string_to_binary(const MetaReference & ref, std::ostream & out)
    {
        const MetaType::StringInfo * info = ref.GetType().GetStringInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument to string_to_binary"));
        return string_to_binary(info->GetAsRange(ref), out);
    }
    template<typename It>
    void to_binary(It begin, It end, std::ostream & out)
    {
        for (; begin != end; ++begin)
        {
            to_binary(*begin, out);
        }
    }

    void list_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::ListInfo * info = object.GetType().GetListInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument to list_to_binary()"));
        simple_to_binary(uint32_t(info->size(object)), out);
        to_binary(info->begin(object), info->end(object), out);
    }
    void array_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::ArrayInfo * info = object.GetType().GetArrayInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument to array_to_binary"));
        to_binary(info->begin(object), info->end(object), out);
    }
    void set_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::SetInfo * info = object.GetType().GetSetInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument to set_to_binary()"));
        simple_to_binary(uint32_t(info->size(object)), out);
        to_binary(info->begin(object), info->end(object), out);
    }
    void map_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::MapInfo * info = object.GetType().GetMapInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument to map_to_binary()"));
        simple_to_binary(uint32_t(info->size(object)), out);
        auto end = info->end(object);
        for (auto it = info->begin(object); it != end; ++it)
        {
            to_binary(it->first, out);
            to_binary(it->second, out);
        }
    }
    template<typename T>
    void member_to_binary(MetaReference object, const T & member, std::ostream & out)
    {
        to_binary(member.GetReference(object), out);
    }
    template<typename T>
    void conditional_member_to_binary(MetaReference object, const T & member, std::ostream & out)
    {
        if (member.ObjectHasMember(object)) member_to_binary(object, member.GetMember(), out);
    }

    void struct_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::StructInfo * info = object.GetType().GetStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to struct_to_binary"));
        const ClassHeaderList & headers = info->GetCurrentHeaders();
        simple_to_binary(headers.GetMostDerived().GetClassName().get_hash(), out);
        for (const ClassHeader & header : headers)
        {
            simple_to_binary(header.GetVersion(), out);
        }

        const MetaType::StructInfo::AllMemberCollection & all_members = info->GetAllMembers(headers);
        for (const auto & member : all_members.base_members.members) member_to_binary(object, member, out);
        for (const auto & member : all_members.base_members.conditional_members) conditional_member_to_binary(object, member, out);
        for (const auto & member : all_members.direct_members.members) member_to_binary(object, member, out);
        for (const auto & member : all_members.direct_members.conditional_members) conditional_member_to_binary(object, member, out);
    }
    void pointer_to_struct_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::PointerToStructInfo * info = object.GetType().GetPointerToStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to pointer_to_struct_to_binary"));
        MetaPointer pointer = info->GetAsPointer(object);
        if (pointer) struct_to_binary(*pointer, out);
        else simple_to_binary(nullptr, out);
    }
    void type_erasure_to_binary(MetaReference object, std::ostream & out)
    {
        const MetaType::TypeErasureInfo * info = object.GetType().GetTypeErasureInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to type_erasure_to_binary"));
        const MetaType * target_type = info->TargetType(object);
        if (target_type) struct_to_binary(info->Target(object), out);
        else simple_to_binary(nullptr, out);
    }

    void to_binary(const ConstMetaReference & object, std::ostream & out)
    {
        switch (object.GetType().category)
        {
        case MetaType::Bool:
        case MetaType::Char:
        case MetaType::Int8:
        case MetaType::Uint8:
        case MetaType::Int16:
        case MetaType::Uint16:
        case MetaType::Int32:
        case MetaType::Uint32:
        case MetaType::Int64:
        case MetaType::Uint64:
        case MetaType::Float:
        case MetaType::Double:
        case MetaType::Enum:
            return simple_to_binary(object.GetMemory(), out);
        case MetaType::String:
            return string_to_binary(object, out);
        case MetaType::List:
            return list_to_binary(object, out);
        case MetaType::Array:
            return array_to_binary(object, out);
        case MetaType::Set:
            return set_to_binary(object, out);
        case MetaType::Map:
            return map_to_binary(object, out);
        case MetaType::Struct:
            return struct_to_binary(object, out);
        case MetaType::PointerToStruct:
            return pointer_to_struct_to_binary(object, out);
        case MetaType::TypeErasure:
            return type_erasure_to_binary(object, out);
        }
        RAW_THROW(std::runtime_error("unhandled case in the switch above"));
    }

    static constexpr size_t bytes_for_offset = 8;
    std::ostream::pos_type start(std::ostream & /*out*/)
    {
        /*int version_number = 1;
        simple_to_binary(version_number, out);
        char zeros[bytes_for_offset] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        auto result = out.tellp();
        out.write(zeros, bytes_for_offset);
        return result;*/
        return 0;
    }

    void finish(std::ostream::pos_type /*offset_pos*/, std::ostream & /*out*/)
    {
        /*auto pos_now = out.tellp() - std::ostream::pos_type(0);
        out.seekp(offset_pos, std::ios_base::beg);
        static_assert(sizeof(pos_now) <= bytes_for_offset, "has to fit into the eight bytes that I reserve at the beginning of the file");
        out.write(reinterpret_cast<char *>(&pos_now), sizeof(pos_now));
        out.seekp(pos_now, std::ios_base::beg);*/
    }
};

void write_optimistic_binary(ConstMetaReference object, std::ostream & out)
{
    BinaryWriter writer;
    auto start = writer.start(out);
    writer.to_binary(object, out);
    writer.finish(start, out);
}

template<typename In, typename Out>
In copy_to(In in, Out out_begin, Out out_end)
{
    while (out_begin != out_end)
    {
        *out_begin++ = *in++;
    }
    return in;
}

struct BinaryReader
{
    template<typename T>
    static void simple_from_binary(T & object, Range<const unsigned char> & in)
    {
        auto begin = reinterpret_cast<unsigned char *>(std::addressof(object));
        auto end = begin + sizeof(T);
        in = { copy_to(in.begin(), begin, end), in.end() };
    }
    static void simple_from_binary(Range<unsigned char> object, Range<const unsigned char> & in)
    {
        in = { copy_to(in.begin(), object.begin(), object.end()), in.end() };
    }

    void string_from_binary(std::string & str, Range<const unsigned char> & in)
    {
        uint32_t size = 0;
        simple_from_binary(size, in);
        str.resize(size);
        std::copy_n(in.begin(), size, str.begin());
        in = in.subrange(size);
    }
    void string_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::StringInfo * info = object.GetType().GetStringInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to string_from_binary"));
        static thread_local std::string buffer;
        string_from_binary(buffer, in);
        info->SetFromRange(object, buffer);
    }

    void list_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::ListInfo * info = object.GetType().GetListInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for list_from_binary"));
        uint32_t size = 0;
        simple_from_binary(size, in);
        if (!size) return;
        MetaType::allocate_pointer buffer = info->value_type.Allocate();
        for (; size > 0; --size)
        {
            MetaOwningMemory to_read(info->value_type, { buffer.get(), buffer.get() + info->value_type.GetSize() });
            from_binary(static_cast<MetaReference &>(to_read), in);
            info->push_back(object, std::move(to_read));
        }
    }

    void array_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::ArrayInfo * info = object.GetType().GetArrayInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for array_from_binary"));
        auto end = info->end(object);
        for (auto it = info->begin(object); it != end; ++it)
        {
            MetaReference ref = *it;
            from_binary(ref, in);
        }
    }

    void set_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::SetInfo * info = object.GetType().GetSetInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for set_from_binary"));
        uint32_t size = 0;
        simple_from_binary(size, in);
        if (!size) return;
        MetaType::allocate_pointer buffer = info->value_type.Allocate();
        for (; size > 0; --size)
        {
            MetaOwningMemory to_read(info->value_type, { buffer.get(), buffer.get() + info->value_type.GetSize() });
            from_binary(static_cast<MetaReference &>(to_read), in);
            info->insert(object, std::move(to_read));
        }
    }

    void map_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::MapInfo * info = object.GetType().GetMapInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for map_from_binary"));
        uint32_t size = 0;
        simple_from_binary(size, in);
        if (!size) return;
        MetaType::allocate_pointer key_buffer = info->key_type.Allocate();
        MetaType::allocate_pointer value_buffer = info->mapped_type.Allocate();
        for (; size > 0; --size)
        {
            MetaOwningMemory key_to_read(info->key_type, { key_buffer.get(), key_buffer.get() + info->key_type.GetSize() });
            MetaOwningMemory value_to_read(info->mapped_type, { value_buffer.get(), value_buffer.get() + info->mapped_type.GetSize() });
            from_binary(static_cast<MetaReference &>(key_to_read), in);
            from_binary(static_cast<MetaReference &>(value_to_read), in);
            info->insert(object, std::move(key_to_read), std::move(value_to_read));
        }
    }

    ReusableStorage<ClassHeaderList> header_storage;

    ReusableStorage<ClassHeaderList>::Reusable struct_headers_from_binary(Range<const unsigned char> & in, const MetaType * struct_type)
    {
        struct ClassHeaderListBuilder
        {
            void operator()(ClassHeaderList & to_build, const MetaType & struct_type) const
            {
                int16_t version = 0;
                simple_from_binary(version, in);
                const MetaType::StructInfo * info = struct_type.GetStructInfo();
                if (!info) RAW_THROW(std::runtime_error("invalid struct type"));
                to_build.emplace_back(info->GetName(), version);
                for (const BaseClass & base : info->GetDirectBaseClasses(version).bases)
                {
                    (*this)(to_build, base.GetBase());
                }
            }

            Range<const unsigned char> & in;
        };

        ReusableStorage<ClassHeaderList>::Reusable result = header_storage.GetObject();
        uint32_t most_derived = 0;
        simple_from_binary(most_derived, in);
        if (struct_type)
        {
            RAW_ASSERT(struct_type->GetStructInfo()->GetName().get_hash() == most_derived);
        }
        else
        {
            struct_type = &MetaType::GetRegisteredStruct(most_derived);
        }
        ClassHeaderListBuilder{in}(result.object, *struct_type);
        return result;
    }

    template<typename T>
    void member_from_binary(const T & member, MetaReference object, Range<const unsigned char> & in)
    {
        from_binary(member.GetReference(object), in);
    }

    template<typename T>
    void conditional_member_from_binary(const T & member, MetaReference object, Range<const unsigned char> & in)
    {
        if (member.ObjectHasMember(object)) member_from_binary(member.GetMember(), object, in);
    }

    void struct_members_from_binary(MetaReference object, const ClassHeaderList & headers, Range<const unsigned char> & in)
    {
        const MetaType::StructInfo * info = object.GetType().GetStructInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for struct_from_binary"));
        const MetaType::StructInfo::AllMemberCollection & all_members = info->GetAllMembers(headers);
        for (const auto & member : all_members.base_members.members) member_from_binary(member, object, in);
        for (const auto & conditional_member : all_members.base_members.conditional_members) conditional_member_from_binary(conditional_member, object, in);
        for (const auto & member : all_members.direct_members.members) member_from_binary(member, object, in);
        for (const auto & conditional_member : all_members.direct_members.conditional_members) conditional_member_from_binary(conditional_member, object, in);
    }

    void struct_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        struct_members_from_binary(object, struct_headers_from_binary(in, &object.GetType()).object, in);
    }

    void pointer_to_struct_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::PointerToStructInfo * info = object.GetType().GetPointerToStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to pointer_to_struct_from_binary"));
        auto pos = in;
        void * ptr = nullptr;
        simple_from_binary(ptr, in);
        if (!ptr) return;
        in = pos;

        ReusableStorage<ClassHeaderList>::Reusable headers = struct_headers_from_binary(in, nullptr);
        MetaReference new_reference = info->AssignNew(object, MetaType::GetStructType(headers.object.GetMostDerived().GetClassName()));
        return struct_members_from_binary(new_reference, headers.object, in);
    }

    void type_erasure_from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        const MetaType::TypeErasureInfo * info = object.GetType().GetTypeErasureInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to type_erasure_from_binary"));
        auto pos = in;
        void * ptr = nullptr;
        simple_from_binary(ptr, in);
        if (!ptr) return;
        in = pos;

        ReusableStorage<ClassHeaderList>::Reusable headers = struct_headers_from_binary(in, nullptr);
        MetaReference new_reference = info->AssignNew(object, MetaType::GetStructType(headers.object.GetMostDerived().GetClassName()));
        return struct_members_from_binary(new_reference, headers.object, in);
    }

    void from_binary(MetaReference object, Range<const unsigned char> & in)
    {
        switch(object.GetType().category)
        {
        case MetaType::Bool:
        case MetaType::Char:
        case MetaType::Int8:
        case MetaType::Uint8:
        case MetaType::Int16:
        case MetaType::Uint16:
        case MetaType::Int32:
        case MetaType::Uint32:
        case MetaType::Int64:
        case MetaType::Uint64:
        case MetaType::Float:
        case MetaType::Double:
        case MetaType::Enum:
           return simple_from_binary(object.GetMemory(), in);
        case MetaType::String:
           return string_from_binary(object, in);
        case MetaType::List:
           return list_from_binary(object, in);
        case MetaType::Array:
           return array_from_binary(object, in);
        case MetaType::Set:
            return set_from_binary(object, in);
        case MetaType::Map:
           return map_from_binary(object, in);
        case MetaType::Struct:
           return struct_from_binary(object, in);
        case MetaType::PointerToStruct:
           return pointer_to_struct_from_binary(object, in);
        case MetaType::TypeErasure:
            return type_erasure_from_binary(object, in);
        }
        RAW_THROW(std::runtime_error("unhandled case in the switch above"));
    }
};

void read_optimistic_binary(metav3::MetaReference object, Range<const unsigned char> in)
{
    BinaryReader().from_binary(object, in);
}

std::string OptimisticBinarySerializer::serialize(metav3::ConstMetaReference object) const
{
    std::stringstream out;
    write_optimistic_binary(std::move(object), out);
    return out.str();
}
bool OptimisticBinarySerializer::deserialize(metav3::MetaReference object, const std::string & str) const
{
    Range<const unsigned char> in(reinterpret_cast<const unsigned char *>(str.data()), reinterpret_cast<const unsigned char *>(str.data() + str.size()));
    read_optimistic_binary(object, in);
    return true;
}

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include <metav3/serialization/serialization_test.hpp>

INSTANTIATE_TYPED_TEST_CASE_P(optimistic_binary, SerializationTest, OptimisticBinarySerializer);

#endif

