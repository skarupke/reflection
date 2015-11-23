
#include "json.hpp"

#include <iterator>
#include "metav3/metav3.hpp"
#include <algorithm>
#include "util/algorithm.hpp"
#include "meta/serialization/parser.hpp"
#include <sstream>
#include <clocale>

using namespace metav3;

namespace
{
template<size_t Size>
void copy_string(const char * input, char (&output)[Size])
{
    auto it = std::begin(output), end = std::end(output);
    for (const char * c = input; *c; *it++ = *c++)
    {
        if (it == end) RAW_THROW(std::runtime_error("overflow in locale"));
    }
    *it++ = 0;
}

struct ScopedChangeLocale
{
    ScopedChangeLocale(const char * locale)
    {
        copy_string(setlocale(LC_NUMERIC, nullptr), old_locale);
        setlocale(LC_NUMERIC, locale);
    }
    ~ScopedChangeLocale()
    {
        setlocale(LC_NUMERIC, old_locale);
    }

private:
    char old_locale[1024];
};
}

template<typename T>
struct to_bool
{
    bool operator()(const T & object) const
    {
        return object;
    }
};

template<size_t Size, typename Out>
Out copy_string(const char (&constant)[Size], Out out)
{
    return std::copy(constant, constant + Size - 1, out);
}

template<typename It>
It to_decimal(int64_t i, It out)
{
    char buffer[32];
    sprintf(buffer, "%li", i);
    return copy_while(buffer, out, to_bool<char>());
}

template<typename It>
It to_decimal(uint64_t i, It out)
{
    char buffer[32];
    sprintf(buffer, "%lu", i);
    return copy_while(buffer, out, to_bool<char>());
}

struct JsonWriter
{
    template<typename It>
    It simple_to_json(bool object, It out)
    {
        if (object) return copy_string("true", out);
        else return copy_string("false", out);
    }

    template<typename It>
    It simple_to_json(char object, It out)
    {
        *out++ = '\'';
        switch(object)
        {
        case '\\':
            *out++ = '\\';
            *out++ = '\\';
            break;
        case '\n':
            *out++ = '\\';
            *out++ = 'n';
            break;
        case '\t':
            *out++ = '\\';
            *out++ = 't';
            break;
        case '\'':
            *out++ = '\\';
            *out++ = '\'';
            break;
        default:
            *out++ = object;
            break;
        }
        *out++ = '\'';
        return out;
    }

    #define SIGNED_TO_JSON(type) \
    template<typename It>\
    It simple_to_json(type object, It out)\
    {\
        return to_decimal(int64_t(object), out);\
    }
    #define UNSIGNED_TO_JSON(type) \
    template<typename It>\
    It simple_to_json(type object, It out)\
    {\
        return to_decimal(uint64_t(object), out);\
    }
    SIGNED_TO_JSON(int8_t)
    UNSIGNED_TO_JSON(uint8_t)
    SIGNED_TO_JSON(int16_t)
    UNSIGNED_TO_JSON(uint16_t)
    SIGNED_TO_JSON(int32_t)
    UNSIGNED_TO_JSON(uint32_t)
    SIGNED_TO_JSON(int64_t)
    UNSIGNED_TO_JSON(uint64_t)
    #undef SIGNED_TO_JSON
    #undef UNSIGNED_TO_JSON

    template<typename It>
    It simple_to_json(float object, It out)
    {
        char buffer[32];
        ScopedChangeLocale change_locale("C");
        sprintf(buffer, "%g", object);
        return copy_while(buffer, out, to_bool<char>());
    }
    template<typename It>
    It simple_to_json(double object, It out)
    {
        char buffer[32];
        ScopedChangeLocale change_locale("C");
        sprintf(buffer, "%g", object);
        return copy_while(buffer, out, to_bool<char>());
    }
    template<typename It>
    It string_to_json(const MetaReference & object, It out)
    {
        const MetaType::StringInfo * info = object.GetType().GetStringInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to string_to_json"));
        return string_to_json(info->GetAsRange(object), out);
    }
    template<typename It>
    It string_to_json(StringView<const char> str, It out)
    {
        *out++ = '"';
        for (char c : str)
        {
            switch(c)
            {
            case '\\':
                *out++ = '\\';
                *out++ = '\\';
                break;
            case '\n':
                *out++ = '\\';
                *out++ = 'n';
                break;
            case '\t':
                *out++ = '\\';
                *out++ = 't';
                break;
            case '"':
                *out++ = '\\';
                *out++ = '"';
                break;
            default:
                *out++ = c;
                break;
            }
        }
        *out++ = '"';
        return out;
    }
    // this function can be used if you know that no characters need
    // to be escaped
    template<typename It>
    static It write_simple_string(const std::string & object, It out)
    {
        *out++ = '"';
        out = std::copy(object.begin(), object.end(), out);
        *out++ = '"';
        return out;
    }

    template<typename It>
    It enum_to_json(const MetaReference & object, It out)
    {
        const MetaType::EnumInfo * info = object.GetType().GetEnumInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument for enum_to_json"));
        return string_to_json(StringView<const char>(info->GetAsString(object)), out);
    }

    template<typename In, typename Out>
    Out write_json_list(In begin, In end, Out out)
    {
        out = begin_scope('[', out);
        bool first = true;
        out = std::accumulate(begin, end, out, [&](Out out, const MetaReference & child)
        {
            if (first) first = false;
            else *out++ = ',';
            return to_json(child, new_line(out));
        });
        return end_scope(']', out);
    }

    template<typename It>
    It list_to_json(const MetaReference & object, It out)
    {
        const MetaType::ListInfo * info = object.GetType().GetListInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to list_to_json"));
        return write_json_list(info->begin(const_cast<MetaReference &>(object)), info->end(const_cast<MetaReference &>(object)), out);
    }

    template<typename It>
    It array_to_json(const MetaReference & object, It out)
    {
        const MetaType::ArrayInfo * info = object.GetType().GetArrayInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to array_to_json"));
        switch (info->value_type.category)
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
        {
            *out++ = '[';
            bool first = true;
            out = std::accumulate(info->begin(const_cast<MetaReference &>(object)), info->end(const_cast<MetaReference &>(object)), out, [&](It out, const MetaReference & child)
            {
                if (first) first = false;
                else out = copy_string(", ", out);
                return to_json(child, out);
            });
            *out++ = ']';
            return out;
        }
        default:
            return write_json_list(info->begin(const_cast<MetaReference &>(object)), info->end(const_cast<MetaReference &>(object)), out);
        }
    }

    template<typename It>
    It set_to_json(const MetaReference & object, It out)
    {
        const MetaType::SetInfo * info = object.GetType().GetSetInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to set_to_json"));
        return write_json_list(info->begin(const_cast<MetaReference &>(object)), info->end(const_cast<MetaReference &>(object)), out);
    }

    template<typename It>
    It map_to_json(const MetaReference & object, It out)
    {
        const MetaType::MapInfo * info = object.GetType().GetMapInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to map_to_json"));
        out = begin_scope('{', out);
        bool first = true;
        out = std::accumulate(info->begin(const_cast<MetaReference &>(object)), info->end(const_cast<MetaReference &>(object)), out,
                [&](It out, const std::pair<ConstMetaReference, MetaReference> & child)
        {
            if (first) first = false;
            else *out++ = ',';
            out = new_line(out);
            out = to_json(child.first, out);
            out = copy_string(" : ", out);
            return to_json(child.second, out);
        });
        return end_scope('}', out);
    }

    struct MemberPrinter
    {
        MemberPrinter(const MetaReference & object, JsonWriter & writer)
            : object(object), writer(writer)
        {
        }

        template<typename It, typename T>
        void operator()(It & lhs, const T & rhs) const
        {
            *lhs++ = ',';
            lhs = writer.new_line(lhs);
            lhs = write_simple_string(rhs.GetName().get(), lhs);
            lhs = copy_string(" : ", lhs);
            lhs = writer.to_json(rhs.GetReference(const_cast<MetaReference &>(object)), lhs);
        }

    private:
        const MetaReference & object;
        JsonWriter & writer;
    };

    struct ConditionalMemberPrinter
    {
        ConditionalMemberPrinter(const MetaReference & object, JsonWriter & writer)
            : object(object), writer(writer)
        {
        }

        template<typename It, typename T>
        void operator()(It & lhs, const T & rhs) const
        {
            if (rhs.ObjectHasMember(object)) MemberPrinter(object, writer)(lhs, rhs.GetMember());
        }

    private:
        const MetaReference & object;
        JsonWriter & writer;
    };

    template<typename It>
    It struct_to_json(const MetaReference & object, It out)
    {
        const MetaType::StructInfo * info = object.GetType().GetStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to struct_to_json"));
        out = begin_scope('{', out);
        bool first = true;
        for (const ClassHeader & header : info->GetCurrentHeaders())
        {
            if (first) first = false;
            else *out++ = ',';
            out = new_line(out);
            out = std::copy(header.GetClassName().get().begin(), header.GetClassName().get().end(), out);
            out = copy_string(" : ", out);
            out = to_decimal(int64_t(header.GetVersion()), out);
        }

        const MetaType::StructInfo::AllMemberCollection & all_members = info->GetAllMembers(info->GetCurrentHeaders());
        MemberPrinter printer(object, *this);
        ConditionalMemberPrinter conditional_printer(object, *this);
        out = foldl(all_members.base_members.members, out, printer);
        out = foldl(all_members.base_members.conditional_members, out, conditional_printer);
        out = foldl(all_members.direct_members.members, out, printer);
        out = foldl(all_members.direct_members.conditional_members, out, conditional_printer);
        return end_scope('}', out);
    }

    template<typename It>
    It pointer_to_struct_to_json(const MetaReference & object, It out)
    {
        const MetaType::PointerToStructInfo * info = object.GetType().GetPointerToStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to pointer_to_struct_to_json"));
        MetaPointer pointer = info->GetAsPointer(object);
        if (pointer) return struct_to_json(*pointer, out);
        else return copy_string("null", out);
    }

    template<typename It>
    It type_erasure_to_json(const MetaReference & object, It out)
    {
        const MetaType::TypeErasureInfo * info = object.GetType().GetTypeErasureInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to type_erasure_to_json"));
        const MetaType * target_type = info->TargetType(object);
        if (target_type) return struct_to_json(info->Target(object), out);
        else return copy_string("null", out);
    }

    template<typename It>
    It to_json(const MetaReference & object, It out)
    {
        switch (object.GetType().category)
        {
        case MetaType::Bool:
            return simple_to_json(object.Get<bool>(), out);
        case MetaType::Char:
            return simple_to_json(object.Get<char>(), out);
        case MetaType::Int8:
            return simple_to_json(object.Get<int8_t>(), out);
        case MetaType::Uint8:
            return simple_to_json(object.Get<uint8_t>(), out);
        case MetaType::Int16:
            return simple_to_json(object.Get<int16_t>(), out);
        case MetaType::Uint16:
            return simple_to_json(object.Get<uint16_t>(), out);
        case MetaType::Int32:
            return simple_to_json(object.Get<int32_t>(), out);
        case MetaType::Uint32:
            return simple_to_json(object.Get<uint32_t>(), out);
        case MetaType::Int64:
            return simple_to_json(object.Get<int64_t>(), out);
        case MetaType::Uint64:
            return simple_to_json(object.Get<uint64_t>(), out);
        case MetaType::Float:
            return simple_to_json(object.Get<float>(), out);
        case MetaType::Double:
            return simple_to_json(object.Get<double>(), out);
        case MetaType::String:
            return string_to_json(object, out);
        case MetaType::Enum:
            return enum_to_json(object, out);
        case MetaType::List:
            return list_to_json(object, out);
        case MetaType::Array:
            return array_to_json(object, out);
        case MetaType::Set:
            return set_to_json(object, out);
        case MetaType::Map:
            return map_to_json(object, out);
        case MetaType::Struct:
            return struct_to_json(object, out);
        case MetaType::PointerToStruct:
            return pointer_to_struct_to_json(object, out);
        case MetaType::TypeErasure:
            return type_erasure_to_json(object, out);
        }
        RAW_THROW(std::runtime_error("unhandled case in the switch above"));
    }

private:
    template<typename It>
    It begin_scope(char opener, It out)
    {
        *out++ = opener;
        ++indentation;
        return out;
    }
    template<typename It>
    It end_scope(char closer, It out)
    {
        --indentation;
        out = new_line(out);
        *out++ = closer;
        return out;
    }
    template<typename It>
    It new_line(It out) const
    {
        *out++ = '\n';
        return std::fill_n(out, 2 * indentation, ' ');
    }

    unsigned indentation = 0;
};

std::string JsonSerializer::serialize(ConstMetaReference object) const
{
    std::stringstream result;
    JsonWriter writer;
    writer.to_json(object, std::ostreambuf_iterator<char>(result));
    return result.str();
}

ParseResult<char> parse_escaped_character(ParseState state)
{
    if (state.text.empty()) return ParseErrorMessage("unexpected end of text", state);
    else if (state.text[0] == '\\')
    {
        auto next = state.text.begin() + 1;
        if (next == state.text.end()) return ParseErrorMessage("unexpected end of text", state.NewText({ next, state.text.end() }));
        auto return_success = [&](char c)
        {
            return ParseSuccess<char>(c, state.NewText({ next + 1, state.text.end() }));
        };
        switch(*next)
        {
        case '\'':
            return return_success('\'');
        case '\\':
            return return_success('\\');
        case 'n':
            return return_success('\n');
        case 't':
            return return_success('\t');
        default:
            return ParseErrorMessage("unknown escape character", state.NewText({ next, state.text.end() }));
        }
    }
    else return ParseSuccess<char>(state.text[0], state.NewText({ state.text.begin() + 1, state.text.end() }));
}

ParseResult<char> parse_json_character(ParseState state)
{
    return parse_char(state, '\'').thenKeepRhs(&parse_escaped_character).thenKeepLhs([](ParseState state)
    {
        return parse_char(state, '\'');
    });
}

template<typename T>
ParseResult<T> templatized_parse(ParseState state);
template<>
ParseResult<bool> templatized_parse(ParseState state)
{
    return parse_bool(state);
}
template<>
ParseResult<char> templatized_parse(ParseState state)
{
    return parse_json_character(state);
}
#define PARSE_UNSIGNED(type) \
    template<>\
    ParseResult<type> templatized_parse(ParseState state)\
    {\
        return parse_unsigned<type>(state);\
    }
    PARSE_UNSIGNED(uint8_t)
    PARSE_UNSIGNED(uint16_t)
    PARSE_UNSIGNED(uint32_t)
    PARSE_UNSIGNED(uint64_t)
#undef PARSE_UNSIGNED
#define PARSE_SIGNED(type) \
    template<>\
    ParseResult<type> templatized_parse(ParseState state)\
    {\
        return parse_signed<type>(state);\
    }
    PARSE_SIGNED(int8_t)
    PARSE_SIGNED(int16_t)
    PARSE_SIGNED(int32_t)
    PARSE_SIGNED(int64_t)
#undef PARSE_SIGNED
#define PARSE_FLOAT(type) \
    template<>\
    ParseResult<type> templatized_parse(ParseState state)\
    {\
        return FloatParser::parse_float<type>(state);\
    }
    PARSE_FLOAT(float)
    PARSE_FLOAT(double)
#undef PARSE_FLOAT

struct MemberComparer
{
    MemberComparer(const std::string & str)
        : str(str)
    {
    }

    template<typename T>
    bool operator()(const T & member) const
    {
        return member.GetName() == str;
    }

private:
    const std::string & str;
};

struct ConditionalMemberComparer : MemberComparer
{
    ConditionalMemberComparer(const std::string & str)
        : MemberComparer(str)
    {
    }
    template<typename T>
    bool operator()(const T & member) const
    {
        return MemberComparer::operator()(member.GetMember());
    }
};

struct JsonParser
{
    template<typename T>
    ParseResult<void> simple_from_json(T & to_fill, ParseState state)
    {
        return templatized_parse<T>(state) >>= [&](ParseSuccess<T> success) -> ParseResult<void>
        {
            to_fill = std::move(success.result);
            return ParseSuccess<void>(success.new_state);
        };
    }

    ParseResult<void> string_from_json(MetaReference & to_fill, ParseState state)
    {
        const MetaType::StringInfo * info = to_fill.GetType().GetStringInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to string_from_json"));
        static thread_local std::string buffer;
        return string_from_json(buffer, state) >>= [&](ParseSuccess<void> success) -> ParseResult<void>
        {
            info->SetFromRange(to_fill, buffer);
            return std::move(success);
        };
    }
    ParseResult<void> string_from_json(std::string & to_fill, ParseState state)
    {
        return parse_string(state) >>= [&](ParseSuccess<ReusableStorage<std::string>::Reusable> success) -> ParseResult<void>
        {
            to_fill = std::move(success.result.object);
            return ParseSuccess<void>(success.new_state);
        };
    }

    ParseResult<void> enum_from_json(MetaReference & object, ParseState state)
    {
        const MetaType::EnumInfo * info = object.GetType().GetEnumInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to enum_from_json"));
        static thread_local std::string buffer;
        return string_from_json(buffer, state) >>= [&](ParseSuccess<void> success) -> ParseResult<void>
        {
            info->SetFromString(object, buffer);
            return std::move(success);
        };
    }

    ParseResult<void> skip_json_object(ParseState)
    {
        RAW_THROW(std::runtime_error("not yet implemented"));
    }

    template<typename Function>
    ParseResult<void> parse_list_content(ParseState state, const Function & function)
    {
        auto parse_comma = [](ParseState state) { return parse_char(state, ','); };
        return function(state).thenKeepRhs([&](ParseState state)
        {
            return parse_zero_or_more(state, [&](ParseState state)
            {
                return skip_whitespace(state).thenKeepRhs(parse_comma).thenKeepRhs(&skip_whitespace).thenKeepRhs(function);
            });
        }).thenKeepLhs(&skip_whitespace);
    }

    template<char opening_paren, char closing_paren, typename Function>
    ParseResult<void> parse_list(ParseState state, const Function & function)
    {
        auto parse_closing_paren = [](ParseState state) { return parse_char(state, closing_paren); };
        return parse_char(state, opening_paren).thenKeepRhs(&skip_whitespace).thenKeepRhs([&](ParseState state)
        {
            return parse_list_content(state, function).thenKeepLhs(parse_closing_paren).orParse(parse_closing_paren, state);
        });
    }

    template<typename Function>
    ParseResult<void> parse_json_list(ParseState state, const Function & function)
    {
        return parse_list<'[', ']'>(state, function);
    }

    ParseResult<void> list_from_json(MetaReference & object, ParseState state)
    {
        const MetaType::ListInfo * info = object.GetType().GetListInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to list_from_json"));
        MetaType::allocate_pointer buffer = info->value_type.Allocate();
        return parse_json_list(state, [&](ParseState state) -> ParseResult<void>
        {
            MetaOwningMemory to_read(info->value_type, { buffer.get(), buffer.get() + info->value_type.GetSize() });
            return from_json(to_read, state) >>= [&](ParseSuccess<void> success) -> ParseResult<void>
            {
                info->push_back(object, std::move(to_read));
                return std::move(success);
            };
        });
    }

    ParseResult<void> array_from_json(MetaReference & object, ParseState state)
    {
        const MetaType::ArrayInfo * info = object.GetType().GetArrayInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to list_from_json"));
        auto it = info->begin(object);
        auto end = info->end(object);
        ParseResult<void> result = parse_json_list(state, [&](ParseState state) -> ParseResult<void>
        {
            if (it == end) return ParseErrorMessage("expected end of array", state);
            MetaReference reference = *it++;
            return from_json(reference, state);
        });
        return std::move(result) >>= [&](ParseSuccess<void> success) -> ParseResult<void>
        {
            if (it != end) return ParseErrorMessage("didn't parse the right number of elements", success.new_state);
            else return std::move(success);
        };
    }

    ParseResult<void> set_from_json(MetaReference & object, ParseState state)
    {
        const MetaType::SetInfo * info = object.GetType().GetSetInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to set_from_json"));
        MetaType::allocate_pointer buffer = info->value_type.Allocate();
        return parse_json_list(state, [&](ParseState state) -> ParseResult<void>
        {
            MetaOwningMemory to_read(info->value_type, { buffer.get(), buffer.get() + info->value_type.GetSize() });
            return from_json(to_read, state) >>= [&](ParseSuccess<void> success) -> ParseResult<void>
            {
                info->insert(object, std::move(to_read));
                return std::move(success);
            };
        });
    }

    template<typename KeyFunction, typename ValueFunction>
    ParseResult<void> parse_json_map_element(ParseState state, const KeyFunction & key_function, const ValueFunction & value_function)
    {
        typedef typename monad::BindType<typename std::result_of<KeyFunction(ParseState)>::type>::type key_success_type;
        auto parse_colon = [](ParseState state) { return parse_char(state, ':'); };
        return key_function(state) >>= [&](key_success_type key_success)
        {
            return skip_whitespace(key_success.new_state)
                    .thenKeepRhs(parse_colon)
                    .thenKeepRhs(&skip_whitespace)
                    .thenKeepRhs([&](ParseState state)
            {
                return value_function(state, std::move(key_success));
            });
        };
    }

    template<typename KeyFunction, typename ValueFunction>
    ParseResult<void> parse_json_map_content(ParseState state, const KeyFunction & key_function, const ValueFunction & value_function)
    {
        return parse_list_content(state, [&](ParseState state)
        {
            return parse_json_map_element(state, key_function, value_function);
        });
    }

    template<typename KeyFunction, typename ValueFunction>
    ParseResult<void> parse_json_map(ParseState state, const KeyFunction & key_function, const ValueFunction & value_function)
    {
        return parse_list<'{', '}'>(state, [&](ParseState state)
        {
            return parse_json_map_element(state, key_function, value_function);
        });
    }

    ParseResult<void> map_from_json(MetaReference & object, ParseState state)
    {
        const MetaType::MapInfo * info = object.GetType().GetMapInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to set_from_json"));
        MetaType::allocate_pointer key_buffer = info->key_type.Allocate();
        MetaOwningMemory key(info->key_type, { key_buffer.get(), key_buffer.get() + info->key_type.GetSize() });
        MetaType::allocate_pointer value_buffer = info->mapped_type.Allocate();
        return parse_json_map(state, [&](ParseState state) -> ParseResult<MetaReference>
        {
            return from_json(key, state) >>= [&](ParseSuccess<void> success) -> ParseResult<MetaReference>
            {
                return ParseSuccess<MetaReference>(std::move(key), success.new_state);
            };
        },
        [&](ParseState state, ParseSuccess<MetaReference> key_success) -> ParseResult<void>
        {
            MetaOwningMemory value(info->mapped_type, { value_buffer.get(), value_buffer.get() + info->mapped_type.GetSize() });
            return from_json(value, state) >>= [&](ParseSuccess<void> success) -> ParseResult<void>
            {
                info->insert(object, std::move(key_success.result), std::move(value));
                return std::move(success);
            };
        });
    }

    template<typename MemberType>
    ParseResult<void> parse_member(ParseState state, MetaReference & object, const MemberType & member)
    {
        MetaReference as_reference = member.GetReference(object);
        return from_json(as_reference, state);
    }
    template<typename MemberType>
    ParseResult<void> parse_conditional_member(ParseState state, MetaReference & object, const MemberType & member)
    {
        if (!member.ObjectHasMember(object)) return skip_json_object(state);
        else return parse_member(state, object, member.GetMember());
    }

    ReusableStorage<ClassHeaderList> header_storage;

    ParseResult<ReusableStorage<ClassHeaderList>::Reusable> parse_struct_begin(ParseState state)
    {
        ReusableStorage<ClassHeaderList>::Reusable result = header_storage.GetObject();
        return parse_char(state, '{').thenKeepRhs(&skip_whitespace).thenKeepRhs([&](ParseState state)
        {
            return parse_json_map_content(state, &parse_identifier, [&](ParseState state, ParseSuccess<StringView<const char> > identifier_success)
            {
                return parse_signed<int>(state) >>= [&](ParseSuccess<int> version_success) -> ParseResult<void>
                {
                    result.object.emplace_back(MetaType::GetRegisteredStructName(identifier_success.result), version_success.result);
                    return ParseSuccess<void>(version_success.new_state);
                };
            }) >>= [&](ParseSuccess<void> success) -> ParseResult<ReusableStorage<ClassHeaderList>::Reusable>
            {
                if (result.object.empty()) return ParseErrorMessage("expected struct type and version number", success.new_state);
                else return ParseSuccess<ReusableStorage<ClassHeaderList>::Reusable>(std::move(result), success.new_state);
            };
        });
    }

    static ParseResult<ReusableStorage<std::string>::Reusable> parse_simple_string(ParseState state)
    {
        if (state.text.empty() || *state.text.begin() != '"') return ParseErrorMessage("expected string", state);
        auto end = std::find(state.text.begin() + 1, state.text.end(), '"');
        if (end == state.text.end()) return ParseErrorMessage("string without closing quote \"", state);
        ReusableStorage<std::string>::Reusable result = state.string_storage.get().GetObject();
        result.object.insert(result.object.begin(), state.text.begin() + 1, end);
        return ParseSuccess<ReusableStorage<std::string>::Reusable>(std::move(result), state.NewText({ end + 1, state.text.end() }));
    }

    ParseResult<void> parse_struct_after_headers(MetaReference & object, const ClassHeaderList & headers, ParseState state)
    {
        const MetaType::StructInfo * info = object.GetType().GetStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to parse_struct_after_headers"));
        const MetaType::StructInfo::AllMemberCollection & all_members = info->GetAllMembers(headers);
        auto parse_closing_paren = [](ParseState state) { return parse_char(state, '}'); };
        auto parse_comma = [](ParseState state) { return parse_char(state, ','); };
        return skip_whitespace(state).thenKeepRhs(parse_comma).thenKeepRhs(&skip_whitespace).thenKeepRhs([&](ParseState state)
        {
            return parse_json_map_content(state, &parse_simple_string,
                [&](ParseState state, ParseSuccess<ReusableStorage<std::string>::Reusable> key_success) -> ParseResult<void>
            {
                auto found = std::find_if(all_members.direct_members.members.begin(), all_members.direct_members.members.end(), MemberComparer(key_success.result.object));
                if (found != all_members.direct_members.members.end()) return parse_member(state, object, *found);
                auto found_base = std::find_if(all_members.base_members.members.begin(), all_members.base_members.members.end(), MemberComparer(key_success.result.object));
                if (found_base != all_members.base_members.members.end()) return parse_member(state, object, *found_base);
                auto found_conditional = std::find_if(all_members.direct_members.conditional_members.begin(), all_members.direct_members.conditional_members.end(), ConditionalMemberComparer(key_success.result.object));
                if (found_conditional != all_members.direct_members.conditional_members.end()) return parse_conditional_member(state, object, *found_conditional);
                auto found_base_conditional = std::find_if(all_members.base_members.conditional_members.begin(), all_members.base_members.conditional_members.end(), ConditionalMemberComparer(key_success.result.object));
                if (found_base_conditional != all_members.base_members.conditional_members.end()) return parse_conditional_member(state, object, *found_base_conditional);
                return ParseErrorMessage("didn't recognize the member name", state);
            });
        }).thenKeepLhs(&skip_whitespace).thenKeepLhs(parse_closing_paren);
    }

    ParseResult<void> struct_from_json(MetaReference & object, ParseState state)
    {
        return parse_struct_begin(state) >>= [&](ParseSuccess<ReusableStorage<ClassHeaderList>::Reusable> header_success)
        {
            return parse_struct_after_headers(object, header_success.result.object, header_success.new_state);
        };
    }

    ParseResult<void> pointer_to_struct_from_json(MetaReference & object, ParseState state)
    {
        const MetaType::PointerToStructInfo * info = object.GetType().GetPointerToStructInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to pointer_to_struct_from_json"));
        return parse_word(state, "null").orParse([&](ParseState state)
        {
            return parse_struct_begin(state) >>= [&](ParseSuccess<ReusableStorage<ClassHeaderList>::Reusable> header_success) -> ParseResult<void>
            {
                MetaReference new_reference = info->AssignNew(object, MetaType::GetStructType(header_success.result.object.GetMostDerived().GetClassName()));
                return parse_struct_after_headers(new_reference, header_success.result.object, header_success.new_state);
            };
        }, state);
    }

    ParseResult<void> type_erasure_from_json(MetaReference object, ParseState state)
    {
        const MetaType::TypeErasureInfo * info = object.GetType().GetTypeErasureInfo();
        if (!info) RAW_THROW(std::runtime_error("invalid argument to type_erasure_from_json"));
        return parse_word(state, "null").orParse([&](ParseState state)
        {
            return parse_struct_begin(state) >>= [&](ParseSuccess<ReusableStorage<ClassHeaderList>::Reusable> header_success) -> ParseResult<void>
            {
                MetaReference new_reference = info->AssignNew(object, MetaType::GetStructType(header_success.result.object.GetMostDerived().GetClassName()));
                return parse_struct_after_headers(new_reference, header_success.result.object, header_success.new_state);
            };
        }, state);
    }

    ParseResult<void> from_json(MetaReference & object, ParseState state)
    {
        state = skip_whitespace(state).GetResult().GetSuccess().new_state;
        switch(object.GetType().category)
        {
        case MetaType::Bool:
           return simple_from_json(object.Get<bool>(), state);
        case MetaType::Char:
           return simple_from_json(object.Get<char>(), state);
        case MetaType::Int8:
           return simple_from_json(object.Get<int8_t>(), state);
        case MetaType::Uint8:
           return simple_from_json(object.Get<uint8_t>(), state);
        case MetaType::Int16:
           return simple_from_json(object.Get<int16_t>(), state);
        case MetaType::Uint16:
           return simple_from_json(object.Get<uint16_t>(), state);
        case MetaType::Int32:
           return simple_from_json(object.Get<int32_t>(), state);
        case MetaType::Uint32:
           return simple_from_json(object.Get<uint32_t>(), state);
        case MetaType::Int64:
           return simple_from_json(object.Get<int64_t>(), state);
        case MetaType::Uint64:
           return simple_from_json(object.Get<uint64_t>(), state);
        case MetaType::Float:
           return simple_from_json(object.Get<float>(), state);
        case MetaType::Double:
           return simple_from_json(object.Get<double>(), state);
        case MetaType::String:
           return string_from_json(object, state);
        case MetaType::Enum:
           return enum_from_json(object, state);
        case MetaType::List:
           return list_from_json(object, state);
        case MetaType::Array:
           return array_from_json(object, state);
        case MetaType::Set:
            return set_from_json(object, state);
        case MetaType::Map:
           return map_from_json(object, state);
        case MetaType::Struct:
           return struct_from_json(object, state);
        case MetaType::PointerToStruct:
           return pointer_to_struct_from_json(object, state);
        case MetaType::TypeErasure:
            return type_erasure_from_json(object, state);
        }
        RAW_THROW(std::runtime_error("unhandled case in the switch above"));
    }
};

bool JsonSerializer::deserialize(MetaReference to_fill, StringView<const char> json_text) const
{
    ReusableStorage<std::string> storage;
    ParseState state(json_text, storage);
    ParseResult<void> result = JsonParser().from_json(to_fill, state);
    if (result.GetResult().IsFailure() || !result.GetResult().GetSuccess().new_state.text.empty()) return false;
    else return true;
}

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
#include "metav3/serialization/serialization_test.hpp"

TEST(json, simple)
{
#define SIMPLE_TEST(type, value)\
    {\
        type object(value);\
        EXPECT_EQ(#value, JsonSerializer().serialize(MetaReference(object)));\
    }
    SIMPLE_TEST(uint8_t, 0);
    SIMPLE_TEST(int, 15);
    SIMPLE_TEST(int, -10001);
    SIMPLE_TEST(float, -1.5);
    SIMPLE_TEST(float, 0.346);
    SIMPLE_TEST(double, 700.2);
    {
        // test that locale doesn't mess with the writing
        ScopedChangeLocale change_locale("de_DE.UTF-8");
        SIMPLE_TEST(float, -1.5);
        SIMPLE_TEST(float, 0.346);
        SIMPLE_TEST(double, 700.2);
    }
    SIMPLE_TEST(bool, true);
    SIMPLE_TEST(bool, false);
    SIMPLE_TEST(char, 'a');
    SIMPLE_TEST(std::string, "foo");
    SIMPLE_TEST(std::string, "");
#undef SIMPLE_TEST
}

TEST(json, string)
{
    ASSERT_EQ("'\\n'", JsonSerializer().serialize(ConstMetaReference('\n')));
}

INSTANTIATE_TYPED_TEST_CASE_P(json, SerializationTest, JsonSerializer);

#endif
