#pragma once

#include "util/range.hpp"
#include <cstdint>
#include <debug/assert.hpp>

namespace metaf
{

typedef Range<const unsigned char> BinaryInput;

template<typename T, typename Ar>
void reflect_registered_class(Ar & archive, int16_t version);
template<typename T>
int16_t get_current_version();

#define REFLECT_CLASS_START(type, current_version)\
namespace metaf\
{\
template<>\
inline int16_t (get_current_version<type>)() { return current_version; }\
}\
\
template<template<typename> class Ar, typename T>\
struct reflect_ ## type\
{\
    void operator()(Ar<T> & archive, int16_t version) const;\
};\
namespace metaf\
{\
template<>\
void reflect_registered_class<type, ::metaf::OptimisticBinaryDeserializer<type>>(OptimisticBinaryDeserializer<type> & archive, int16_t version)\
{\
    return reflect_ ## type<OptimisticBinaryDeserializer, type>()(archive, version);\
}\
template<>\
void reflect_registered_class<type, ::metaf::OptimisticBinarySerializer<type>>(OptimisticBinarySerializer<type> & archive, int16_t version)\
{\
    return reflect_ ## type<OptimisticBinarySerializer, type>()(archive, version);\
}\
}\
template<template<typename> class Ar, typename T>\
void reflect_ ## type<Ar, T>::operator()(Ar<T> & archive, int16_t version) const\
{\
    static_cast<void>(version);\
    static_cast<void>(archive);
#define REFLECT_MEMBER(member_name) archive.member(#member_name, &T::member_name)
#define REFLECT_BASE(class_name) archive.template base<class_name>()
#define REFLECT_CLASS_END()\
}
#define FRIEND_REFLECT_CLASSES(type) template<template<typename> class, typename> friend struct reflect_ ## type

template<typename T>
struct OptimisticBinaryDeserializer;
template<typename T>
struct OptimisticBinarySerializer;
namespace detail
{
template<typename S>
struct reference_specialization;

template<typename S>
void reference(BinaryInput & input, S & data)
{
    reference_specialization<S>()(input, data);
}
template<typename S>
void reference(std::ostream & output, const S & data)
{
    reference_specialization<S>()(output, data);
}
template<typename S>
void array(BinaryInput & input, S * begin, S * end)
{
    for (; begin != end; ++begin)
    {
        reference(input, *begin);
    }
}
template<typename S>
void array(std::ostream & output, const S * begin, const S * end)
{
    for (; begin != end; ++begin)
    {
        reference(output, *begin);
    }
}

template<typename S>
struct reference_specialization
{
    void operator()(BinaryInput & input, S & data)
    {
        OptimisticBinaryDeserializer<S> deserializer(data, input);
        reflect_registered_class<S>(deserializer, get_current_version<S>());
    }
    void operator()(std::ostream & output, const S & data)
    {
        OptimisticBinarySerializer<S> serializer(data, output);
        reflect_registered_class<S>(serializer, get_current_version<S>());
    }
};

template<typename S, typename A>
struct reference_specialization<std::vector<S, A>>
{
    void operator()(BinaryInput & input, std::vector<S, A> & data)
    {
        size_t size = 0;
        reference(input, size);
        data.resize(size);
        for (S & element : data)
        {
            reference(input, element);
        }
    }
    void operator()(std::ostream & output, const std::vector<S, A> & data)
    {
        reference(output, data.size());
        for (const S & element : data)
        {
            reference(output, element);
        }
    }
};

template<typename S>
inline void memcpy_reference(BinaryInput & input, S & data)
{
    auto end = input.begin() + sizeof(S);
    std::copy(input.begin(), end, reinterpret_cast<unsigned char *>(std::addressof(data)));
    input = { end, input.end() };
}
template<typename S>
inline void memcpy_reference(std::ostream & output, const S & data)
{
    auto begin = reinterpret_cast<const char *>(std::addressof(data));
    std::copy(begin, begin + sizeof(S), std::ostreambuf_iterator<char>(output));
}

#define simple_reference(type)\
template<>\
inline void reference(BinaryInput & input, type & data)\
{\
    memcpy_reference(input, data);\
}\
template<>\
inline void reference(std::ostream & output, const type & data)\
{\
    memcpy_reference(output, data);\
}
simple_reference(bool)
simple_reference(char)
simple_reference(unsigned char)
simple_reference(signed char)
simple_reference(uint16_t)
simple_reference(int16_t)
simple_reference(int32_t)
simple_reference(uint64_t)
simple_reference(int64_t)
simple_reference(double)

//#define COMPRESS_UINT32
//#define STORE_AS_FLOAT8_IF_POSSIBLE
//#define FLOAT8_SUPPORTS_NAN_AND_INFINITY
//#define TRUNCATE_SHORT_FLOATS


#ifdef COMPRESS_UINT32
template<>
inline void reference(BinaryInput & input, uint32_t & data)
{
    constexpr uint8_t top_bit = 0b1000'0000;
    constexpr uint8_t mask = 0b0111'1111;
    data = 0;
    unsigned char read_byte;
    memcpy_reference(input, read_byte);
    data |= read_byte & mask;
    if (read_byte & top_bit)
    {
        memcpy_reference(input, read_byte);
        data |= (read_byte & mask) << 7;
        if (read_byte & top_bit)
        {
            memcpy_reference(input, read_byte);
            data |= (read_byte & mask) << 14;
            if (read_byte & top_bit)
            {
                memcpy_reference(input, read_byte);
                data |= (read_byte & mask) << 21;
                if (read_byte & top_bit)
                {
                    memcpy_reference(input, read_byte);
                    data |= (read_byte & mask) << 28;
                }
            }
        }
    }
}
template<>
inline void reference(std::ostream & output, const uint32_t & data)
{
    constexpr uint8_t top_bit = 0b1000'0000;
    constexpr uint8_t mask = 0b0111'1111;
    uint8_t write_byte = data & mask;
    if (data > mask)
    {
        memcpy_reference(output, write_byte | top_bit);
        uint32_t to_write = (data >> 7);
        write_byte = to_write & mask;
        if (to_write > mask)
        {
            memcpy_reference(output, write_byte | top_bit);
            to_write >>= 7;
            write_byte = to_write & mask;
            if (to_write > mask)
            {
                memcpy_reference(output, write_byte | top_bit);
                to_write >>= 7;
                write_byte = to_write & mask;
                if (to_write > mask)
                {
                    memcpy_reference(output, write_byte | top_bit);
                    to_write >>= 7;
                    write_byte = to_write & mask;
                }
            }
        }
    }
    memcpy_reference(output, write_byte);
}
#else
simple_reference(uint32_t)
#endif

#ifdef TRUNCATE_SHORT_FLOATS
template<>
inline void reference(BinaryInput & input, float & data)
{
    union
    {
        struct
        {
            uint16_t read_two_more_bytes;
            uint16_t read_two_bytes;
        };
        float as_float;
    }
    read_data;
    memcpy_reference(input, read_data.read_two_bytes);
    constexpr uint16_t nan = 0b0111'1111'1000'0000;
    constexpr uint8_t non_nan_bits = 0b0111'1111;
    if ((read_data.read_two_bytes & nan) == nan)
    {
        data = static_cast<float>(read_data.read_two_bytes & non_nan_bits);
    }
    else
    {
        memcpy_reference(input, read_data.read_two_more_bytes);
        data = read_data.as_float;
    }
}
template<>
inline void reference(std::ostream & output, const float & data)
{
    constexpr uint8_t max_supported = 0b0111'1111;
    constexpr uint16_t nan = 0b0111'1111'1000'0000;
    union
    {
        struct
        {
            uint16_t first;
            uint16_t second;
        };
        float as_float;
    }
    write_backwards;
    write_backwards.as_float = data;
    if (data >= 0.0f && data <= max_supported)
    {
        float truncated = trunc(data);
        if (truncated == data)
        {
            uint16_t to_write = nan | static_cast<uint8_t>(truncated);
            memcpy_reference(output, to_write);
        }
        else
        {
            memcpy_reference(output, write_backwards.second);
            memcpy_reference(output, write_backwards.first);
        }
    }
    else
    {
        memcpy_reference(output, write_backwards.second);
        memcpy_reference(output, write_backwards.first);
    }
}
#elif defined(STORE_AS_FLOAT8_IF_POSSIBLE)
struct FloatComponentsCompressed;
union FloatComponents
{
    explicit FloatComponents(float f)
        : f(f)
    {
    }
    explicit FloatComponents(FloatComponentsCompressed);

    static constexpr uint32_t mantissa_bits = 23;

    float f;
    struct
    {
        uint32_t mantissa : mantissa_bits;
        uint32_t exponent : 8;
        uint32_t sign : 1;
    };
    struct
    {
        uint32_t nan_unused_bits : 22;
        uint32_t quiet_nan_bit : 1;
    };

    bool is_nan_or_infinity() const
    {
        return exponent == 0b11111111;
    }
    bool is_infinity() const
    {
        return is_nan_or_infinity() && mantissa == 0;
    }
    bool can_be_compressed() const;
};
static_assert(sizeof(FloatComponents) == sizeof(float), "this should just represent a float");
struct FloatComponentsCompressed
{
    explicit FloatComponentsCompressed(FloatComponents as_float);
    bool is_compressed() const
    {
        return nan_bits == 0b11111111;
    }

    static constexpr uint32_t exponent_bias = 123;
    static constexpr uint32_t mantissa_bits = 4;
#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
    enum SpecialValues
    {
        Zero,
        Infinity,
        SNaN,
        QNaN
    };
#endif

    uint16_t exponent : 3;
    // mantissa comes AFTER the exponent because like this
    // the bits in here are exactly in the same position that
    // they are in in the final float. meaning only the
    // exponent bits will have to be shifted around. the mantissa
    // and sign bit can stay in place. whether the optimizer uses
    // that is another question entirely...
    uint16_t mantissa : mantissa_bits;
    uint16_t nan_bits : 8;
    uint16_t sign : 1;
};
static_assert(sizeof(FloatComponentsCompressed) == sizeof(uint16_t), "this is compressed to a uint16");

inline FloatComponentsCompressed::FloatComponentsCompressed(FloatComponents as_float)
{
    sign = as_float.sign;
    nan_bits = 0b11111111;
#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
    if (as_float.is_nan_or_infinity())
    {
        exponent = 0;
        if (as_float.is_infinity())
            mantissa = Infinity;
        else if (as_float.quiet_nan_bit)
            mantissa = QNaN;
        else
            mantissa = SNaN;
        return;
    }
#endif
    if (as_float.exponent == 0)
    {
        exponent = 0;
        mantissa = 0;
    }
    else
    {
        exponent = as_float.exponent - exponent_bias;
        mantissa = as_float.mantissa >> (FloatComponents::mantissa_bits - mantissa_bits);
    }
}
inline FloatComponents::FloatComponents(FloatComponentsCompressed compressed)
{
    sign = compressed.sign;
    if (compressed.exponent)
    {
        exponent = compressed.exponent + FloatComponentsCompressed::exponent_bias;
        mantissa = compressed.mantissa << (mantissa_bits - FloatComponentsCompressed::mantissa_bits);
    }
    else
    {
#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
        switch(__builtin_expect(compressed.mantissa, FloatComponentsCompressed::Zero))
        {
        case FloatComponentsCompressed::Zero:
            exponent = 0;
            mantissa = 0;
            break;
        case FloatComponentsCompressed::Infinity:
            exponent = 0b1111'1111;
            mantissa = 0;
            break;
        case FloatComponentsCompressed::QNaN:
            f = std::numeric_limits<float>::quiet_NaN();
            break;
        case FloatComponentsCompressed::SNaN:
        default:
            f = std::numeric_limits<float>::signaling_NaN();
            break;
        }
#else
        exponent = 0;
        mantissa = 0;
#endif
    }
}

inline bool FloatComponents::can_be_compressed() const
{
    if (is_nan_or_infinity())
        return true;
    if (exponent == 0)
        return true;
    uint32_t exponent_without_bias = exponent - FloatComponentsCompressed::exponent_bias;
    if (exponent_without_bias == 0)
        return false;
    if ((exponent_without_bias & 0b111) != exponent_without_bias)
        return false;
    uint32_t mantissa_mask = 0b1111 << (mantissa_bits - FloatComponentsCompressed::mantissa_bits);
    return (mantissa & mantissa_mask) == mantissa;
}

template<>
inline void reference(BinaryInput & input, float & data)
{
    union ConversionUnion
    {
        ConversionUnion()
        {
        }

        struct AsBytes
        {
            uint16_t two_more_bytes;
            FloatComponentsCompressed two_bytes;
        }
        as_bytes;
        float as_float;
    }
    read_data;
    memcpy_reference(input, read_data.as_bytes.two_bytes);
    if (read_data.as_bytes.two_bytes.is_compressed())
    {
        data = FloatComponents(read_data.as_bytes.two_bytes).f;
    }
    else
    {
        memcpy_reference(input, read_data.as_bytes.two_more_bytes);
        data = read_data.as_float;
    }
}
template<>
inline void reference(std::ostream & output, const float & data)
{
    FloatComponents components(data);
    if (components.can_be_compressed())
    {
        memcpy_reference(output, FloatComponentsCompressed(components));
    }
    else
    {
        union
        {
            struct
            {
                uint16_t first;
                uint16_t second;
            };
            float as_float;
        }
        write_backwards;
        write_backwards.as_float = data;
        memcpy_reference(output, write_backwards.second);
        memcpy_reference(output, write_backwards.first);
    }
}
#else
simple_reference(float)
#endif
}

template<typename T>
struct OptimisticBinaryDeserializer
{
    OptimisticBinaryDeserializer(T & object, BinaryInput & input)
        : object(object), input(input)
    {
    }

    template<typename M>
    void member(Range<const char>, M T::*m)
    {
        detail::reference(input, object.*m);
    }
    template<typename M, size_t Size>
    void member(Range<const char>, M (T::*m)[Size])
    {
        detail::array(input, object.*m, object.*m + Size);
    }
    template<typename B>
    void base()
    {
        detail::reference(input, static_cast<B &>(object));
    }

private:
    T & object;
    BinaryInput & input;
};
template<typename T>
struct OptimisticBinarySerializer
{
    OptimisticBinarySerializer(const T & object, std::ostream & output)
        : object(object), output(output)
    {
    }

    template<typename M>
    void member(Range<const char>, M T::*m)
    {
        detail::reference(output, object.*m);
    }
    template<typename M, size_t Size>
    void member(Range<const char>, M (T::*m)[Size])
    {
        detail::array(output, object.*m, object.*m + Size);
    }
    template<typename B>
    void base()
    {
        detail::reference(output, static_cast<const B &>(object));
    }

private:
    const T & object;
    std::ostream & output;
};

}
