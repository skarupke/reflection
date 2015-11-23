#include "simpleBinary.hpp"

#include <istream>
#include <ostream>
#include "meta/meta.hpp"
#include "meta/serialization/parser.hpp"
#include <sstream>

using namespace meta;

struct BinaryWriter
{
	template<typename T>
	void simple_to_binary(const T & simple, std::ostream & out)
	{
		out.write(reinterpret_cast<const char *>(&simple), sizeof(T));
	}

	void string_to_binary(StringView<const char> str, std::ostream & out)
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

	void enum_to_binary(const MetaReference & object, std::ostream & out)
	{
		const MetaType::EnumInfo * info = object.GetType().GetEnumInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument to enum_to_binary()"));
		simple_to_binary(info->GetAsInt(object), out);
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
		switch(member.GetType().access_type)
		{
        case PASS_BY_VALUE:
			to_binary(member.Get(object).GetVariant(), out);
			return;
		case PASS_BY_REFERENCE:
			to_binary(member.GetRef(object), out);
			return;
        case GET_BY_REF_SET_BY_VALUE:
            to_binary(const_cast<MetaReference &>(static_cast<const MetaReference &>(member.GetCRef(object))), out);
			return;
		}
        RAW_THROW(std::runtime_error("unhandled case in the switch above"));
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

	void to_binary(const MetaReference & object, std::ostream & out)
	{
		switch (object.GetType().category)
		{
		case MetaType::Bool:
			return simple_to_binary(object.Get<bool>(), out);
		case MetaType::Char:
			return simple_to_binary(object.Get<char>(), out);
		case MetaType::Int8:
			return simple_to_binary(object.Get<int8_t>(), out);
		case MetaType::Uint8:
			return simple_to_binary(object.Get<uint8_t>(), out);
		case MetaType::Int16:
			return simple_to_binary(object.Get<int16_t>(), out);
		case MetaType::Uint16:
			return simple_to_binary(object.Get<uint16_t>(), out);
		case MetaType::Int32:
			return simple_to_binary(object.Get<int32_t>(), out);
		case MetaType::Uint32:
			return simple_to_binary(object.Get<uint32_t>(), out);
		case MetaType::Int64:
			return simple_to_binary(object.Get<int64_t>(), out);
		case MetaType::Uint64:
			return simple_to_binary(object.Get<uint64_t>(), out);
		case MetaType::Float:
			return simple_to_binary(object.Get<float>(), out);
		case MetaType::Double:
			return simple_to_binary(object.Get<double>(), out);
		case MetaType::String:
			return string_to_binary(object, out);
		case MetaType::Enum:
			return enum_to_binary(object, out);
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
};

void write_binary(meta::ConstMetaReference object, std::ostream & out)
{
	BinaryWriter().to_binary(object, out);
}

struct BinaryReader
{
	template<typename T>
	static void simple_from_binary(T & object, std::istream & in)
	{
		in.read(reinterpret_cast<char *>(&object), sizeof(T));
	}

	void string_from_binary(std::string & str, std::istream & in)
	{
		uint32_t size = 0;
		simple_from_binary(size, in);
		str.resize(size);
		in.read(const_cast<char *>(str.data()), size);
	}
	void string_from_binary(MetaReference & object, std::istream & in)
	{
		const MetaType::StringInfo * info = object.GetType().GetStringInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to string_from_binary"));
		static thread_local std::string buffer;
		string_from_binary(buffer, in);
		info->SetFromRange(object, buffer);
	}

	void enum_from_binary(MetaReference & object, std::istream & in)
	{
		const MetaType::EnumInfo * info = object.GetType().GetEnumInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for enum_from_binary"));
		simple_from_binary(*reinterpret_cast<int32_t *>(object.GetMemory().begin()), in);
	}

	void list_from_binary(MetaReference & object, std::istream & in)
	{
		const MetaType::ListInfo * info = object.GetType().GetListInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for list_from_binary"));
		uint32_t size = 0;
		simple_from_binary(size, in);
        if (!size) return;
		MetaType::allocate_pointer buffer = info->value_type.Allocate();
		for (; size > 0; --size)
		{
			MetaOwningMemory to_read({ buffer.get(), buffer.get() + info->value_type.GetSize() }, info->value_type);
			MetaReference reference = to_read.GetVariant();
			from_binary(reference, in);
			info->push_back(object, std::move(reference));
		}
	}

	void array_from_binary(MetaReference & object, std::istream & in)
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

	void set_from_binary(MetaReference & object, std::istream & in)
	{
		const MetaType::SetInfo * info = object.GetType().GetSetInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for set_from_binary"));
		uint32_t size = 0;
		simple_from_binary(size, in);
        if (!size) return;
		MetaType::allocate_pointer buffer = info->value_type.Allocate();
		for (; size > 0; --size)
		{
			MetaOwningMemory to_read({ buffer.get(), buffer.get() + info->value_type.GetSize() }, info->value_type);
			MetaReference reference = to_read.GetVariant();
			from_binary(reference, in);
			info->insert(object, std::move(reference));
		}
	}

	void map_from_binary(MetaReference & object, std::istream & in)
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
			MetaOwningMemory key_to_read({ key_buffer.get(), key_buffer.get() + info->key_type.GetSize() }, info->key_type);
			MetaReference key_reference = key_to_read.GetVariant();
			MetaOwningMemory value_to_read({ value_buffer.get(), value_buffer.get() + info->mapped_type.GetSize() }, info->mapped_type);
			MetaReference value_reference = value_to_read.GetVariant();
			from_binary(key_reference, in);
			from_binary(value_reference, in);
			info->insert(object, std::move(key_reference), std::move(value_reference));
		}
	}

    ReusableStorage<ClassHeaderList> header_storage;

    ReusableStorage<ClassHeaderList>::Reusable struct_headers_from_binary(std::istream & in, const MetaType * struct_type)
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

			std::istream & in;
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
	void member_from_binary(const T & member, MetaReference & object, std::istream & in)
	{
		switch(member.GetType().access_type)
		{
        case PASS_BY_VALUE:
		{
			MetaOwningVariant variant = member.Get(object);
			MetaReference as_reference = variant.GetVariant();
			from_binary(as_reference, in);
			member.Set(object, as_reference);
			return;
		}
		case PASS_BY_REFERENCE:
		{
			MetaReference as_reference = member.GetRef(object);
			from_binary(as_reference, in);
			return;
		}
        case GET_BY_REF_SET_BY_VALUE:
		{
			unsigned char * memory = static_cast<unsigned char *>(alloca(member.GetType().GetSize()));
			MetaOwningMemory temp(ArrayView<unsigned char>(memory, memory + member.GetType().GetSize()), member.GetType());
			MetaReference as_reference = temp.GetVariant();
			from_binary(as_reference, in);
            member.ValueRefSet(object, as_reference);
			return;
		}
		}
        RAW_ASSERT(false, "missing case in the switch statement above");
	}

	template<typename T>
	void conditional_member_from_binary(const T & member, MetaReference & object, std::istream & in)
	{
		if (member.ObjectHasMember(object)) member_from_binary(member.GetMember(), object, in);
	}

    void struct_members_from_binary(MetaReference & object, const ClassHeaderList & headers, std::istream & in)
	{
		const MetaType::StructInfo * info = object.GetType().GetStructInfo();
        if (!info) RAW_THROW(std::runtime_error("wrong argument for struct_from_binary"));
		const MetaType::StructInfo::AllMemberCollection & all_members = info->GetAllMembers(headers);
		for (const auto & member : all_members.base_members.members) member_from_binary(member, object, in);
		for (const auto & conditional_member : all_members.base_members.conditional_members) conditional_member_from_binary(conditional_member, object, in);
		for (const auto & member : all_members.direct_members.members) member_from_binary(member, object, in);
		for (const auto & conditional_member : all_members.direct_members.conditional_members) conditional_member_from_binary(conditional_member, object, in);
	}

	void struct_from_binary(MetaReference & object, std::istream & in)
	{
        struct_members_from_binary(object, struct_headers_from_binary(in, &object.GetType()).object, in);
	}

	void pointer_to_struct_from_binary(MetaReference & object, std::istream & in)
	{
		const MetaType::PointerToStructInfo * info = object.GetType().GetPointerToStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to pointer_to_struct_from_binary"));
		auto pos = in.tellg();
		void * ptr = nullptr;
		simple_from_binary(ptr, in);
		if (!ptr) return;
		in.seekg(pos);

        ReusableStorage<ClassHeaderList>::Reusable headers = struct_headers_from_binary(in, nullptr);
		MetaReference new_reference = info->AssignNew(object, MetaType::GetStructType(headers.object.GetMostDerived().GetClassName()));
        return struct_members_from_binary(new_reference, headers.object, in);
	}

	void type_erasure_from_binary(MetaReference & object, std::istream & in)
	{
		const MetaType::TypeErasureInfo * info = object.GetType().GetTypeErasureInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to type_erasure_from_binary"));
		auto pos = in.tellg();
		void * ptr = nullptr;
		simple_from_binary(ptr, in);
		if (!ptr) return;
		in.seekg(pos);

        ReusableStorage<ClassHeaderList>::Reusable headers = struct_headers_from_binary(in, nullptr);
		MetaReference new_reference = info->AssignNew(object, MetaType::GetStructType(headers.object.GetMostDerived().GetClassName()));
        return struct_members_from_binary(new_reference, headers.object, in);
	}

	void from_binary(MetaReference & object, std::istream & in)
	{
		switch(object.GetType().category)
		{
		case MetaType::Bool:
		   return simple_from_binary(object.Get<bool>(), in);
		case MetaType::Char:
		   return simple_from_binary(object.Get<char>(), in);
		case MetaType::Int8:
		   return simple_from_binary(object.Get<int8_t>(), in);
		case MetaType::Uint8:
		   return simple_from_binary(object.Get<uint8_t>(), in);
		case MetaType::Int16:
		   return simple_from_binary(object.Get<int16_t>(), in);
		case MetaType::Uint16:
		   return simple_from_binary(object.Get<uint16_t>(), in);
		case MetaType::Int32:
		   return simple_from_binary(object.Get<int32_t>(), in);
		case MetaType::Uint32:
		   return simple_from_binary(object.Get<uint32_t>(), in);
		case MetaType::Int64:
		   return simple_from_binary(object.Get<int64_t>(), in);
		case MetaType::Uint64:
		   return simple_from_binary(object.Get<uint64_t>(), in);
		case MetaType::Float:
		   return simple_from_binary(object.Get<float>(), in);
		case MetaType::Double:
		   return simple_from_binary(object.Get<double>(), in);
		case MetaType::String:
		   return string_from_binary(object, in);
		case MetaType::Enum:
		   return enum_from_binary(object, in);
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

void read_binary(meta::MetaReference object, std::istream & in)
{
	BinaryReader().from_binary(object, in);
}

std::string BinarySerializer::serialize(meta::ConstMetaReference object) const
{
	std::stringstream out;
	write_binary(std::move(object), out);
	return out.str();
}
bool BinarySerializer::deserialize(meta::MetaReference object, const std::string & str) const
{
	std::istringstream in(str);
	read_binary(object, in);
	return true;
}

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include <meta/serialization/serializationTest.hpp>

INSTANTIATE_TYPED_TEST_CASE_P(binary, SerializationTest, BinarySerializer);

#endif
