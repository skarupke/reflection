#pragma once

#include "metafast/metafast.hpp"

namespace metaf
{
namespace detail
{

#define simple_reference(type)\
template<>\
inline void reference(BinaryInput & input, type & data)\
{\
    memcpy_reference(input, data);\
}\
template<>\
inline void reference(BinaryOutput & output, const type & data)\
{\
    memcpy_reference(output, data);\
}
simple_reference(bool)
simple_reference(char)
simple_reference(unsigned char)
simple_reference(signed char)
simple_reference(wchar_t)
simple_reference(char16_t)
simple_reference(char32_t)
simple_reference(unsigned short)
simple_reference(short)
simple_reference(double)

#define COMPRESS_INT
#define STORE_AS_FLOAT8_IF_POSSIBLE
//#define FLOAT8_SUPPORTS_NAN_AND_INFINITY


#ifdef COMPRESS_INT
struct CompressedIntByte
{
    CompressedIntByte()
        : as_byte()
    {
    }
    CompressedIntByte(unsigned data)
        : CompressedIntByte(static_cast<unsigned long long>(data))
    {
    }
    CompressedIntByte(unsigned long data)
        : CompressedIntByte(static_cast<unsigned long long>(data))
    {
    }
    CompressedIntByte(unsigned long long data)
        : unsigned_data(data)
        , has_more(data > max_value)
    {
    }

    union
    {
        struct
        {
            uint8_t unsigned_data : 7;
            bool has_more : 1;
        };
        int8_t signed_data : 7;
        uint8_t as_byte;
    };

    static constexpr uint8_t max_value = 0b1111111;
};
static_assert(sizeof(CompressedIntByte) == sizeof(uint8_t), "shouldn't add any extra bits");

template<typename T>
struct unsigned_int_reference_specialization
{
    void operator()(BinaryInput & input, T & data)
    {
        CompressedIntByte read_byte;
        memcpy_reference(input, read_byte);
        data = read_byte.unsigned_data;
        for (unsigned i = 1; read_byte.has_more && i <= sizeof(T); ++i)
        {
            memcpy_reference(input, read_byte);
            uint64_t to_add = i == sizeof(T) ? read_byte.as_byte : read_byte.unsigned_data;
            data |= to_add << (7 * i);
        }
    }
    void operator()(BinaryOutput & output, const T & data)
    {
        CompressedIntByte to_write(data);
        memcpy_reference(output, to_write);
        for (unsigned i = 1; to_write.has_more && i <= sizeof(T); ++i)
        {
            to_write = data >> (7 * i);
            memcpy_reference(output, to_write);
        }
    }
};
template<>
struct reference_specialization<unsigned>
    : unsigned_int_reference_specialization<unsigned>
{
};
template<>
struct reference_specialization<unsigned long>
    : unsigned_int_reference_specialization<unsigned long>
{
};
template<>
struct reference_specialization<unsigned long long>
    : unsigned_int_reference_specialization<unsigned long long>
{
};

template<typename T>
struct signed_int_reference_specialization
{
    void operator()(BinaryInput & input, T & data)
    {
        CompressedIntByte read_byte = input.read_memcpy<CompressedIntByte>();
        if (!read_byte.has_more)
        {
            data = read_byte.signed_data;
            return;
        }

        uint64_t as_unsigned = read_byte.unsigned_data;
        for (unsigned i = sizeof(T) - 1, to_shift = 7;; --i, to_shift += 7)
        {
            if (i == 0)
            {
                data = T(input.read_memcpy<int8_t>()) << to_shift;
                break;
            }
            input.memcpy(read_byte);
            if (!read_byte.has_more)
            {
                data = T(read_byte.signed_data) << to_shift;
                break;
            }
            as_unsigned |= uint64_t(read_byte.unsigned_data) << to_shift;
        }
        data |= as_unsigned;
    }

    void operator()(BinaryOutput & output, const T & data)
    {
        T bit_compare_copy = data < 0 ? ~data : data;
        T copy = data;
        constexpr uint64_t mask = ~0b111111ull;
        unsigned num_extra_bytes = 0;
        for (; (bit_compare_copy & mask) && num_extra_bytes < sizeof(T); ++num_extra_bytes)
        {
            memcpy_reference(output, uint8_t(copy | 0b10000000));
            copy >>= 7;
            bit_compare_copy >>= 7;
        }
        if (num_extra_bytes != sizeof(T))
            copy &= 0b01111111;
        memcpy_reference(output, uint8_t(copy));
    }
};

template<>
struct reference_specialization<int>
    : signed_int_reference_specialization<int>
{
};
template<>
struct reference_specialization<long>
    : signed_int_reference_specialization<long>
{
};
template<>
struct reference_specialization<long long>
    : signed_int_reference_specialization<long long>
{
};

#else
simple_reference(int)
simple_reference(unsigned)
simple_reference(long)
simple_reference(unsigned long)
simple_reference(long long)
simple_reference(unsigned long long)
#endif

#ifdef STORE_AS_FLOAT8_IF_POSSIBLE
union FloatComponents;
struct FloatComponentsCompressed
{
    explicit FloatComponentsCompressed(FloatComponents as_float);
    bool is_compressed() const
    {
        return nan_bits == 0b11111111;
    }

    static constexpr uint32_t exponent_bias = 123;
    static constexpr uint32_t exponent_bits = 3;
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

    uint16_t exponent : exponent_bits;
    // mantissa comes AFTER the exponent because like this
    // the bits in here are exactly in the same position that
    // they are in in the final float. meaning only the
    // exponent bits will have to be shifted around. the mantissa
    // and sign bit can stay in place
    uint16_t mantissa : mantissa_bits;
    uint16_t nan_bits : 8;
    uint16_t sign : 1;
};
static_assert(sizeof(FloatComponentsCompressed) == sizeof(uint16_t), "this is compressed to a uint16");

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
        uint32_t : 22;
        uint32_t quiet_nan_bit : 1;
    };
    struct Compressed_conversion
    {
        uint16_t : 16;
        FloatComponentsCompressed compressed;
    }
    compressed_conversion;
    // these bits will always be zero when converting from
    // a compressed float
    uint32_t conversion_zero_mantissa : 16 + FloatComponentsCompressed::exponent_bits;

    bool is_nan_or_infinity() const
    {
        return exponent == 0b11111111;
    }
    bool is_infinity() const
    {
        return is_nan_or_infinity() && mantissa == 0;
    }
    bool can_be_compressed() const;
    bool is_denormal() const
    {
        return exponent == 0 && mantissa != 0;
    }
};
static_assert(sizeof(FloatComponents) == sizeof(float), "this should just represent a float");

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
    if (compressed.exponent)
    {
        compressed_conversion.compressed = compressed;
        conversion_zero_mantissa = 0;
        exponent = compressed.exponent + FloatComponentsCompressed::exponent_bias;
    }
    else
    {
        sign = compressed.sign;
#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
        switch(__builtin_expect(compressed.mantissa, FloatComponentsCompressed::Zero))
        {
        case FloatComponentsCompressed::Zero:
            exponent = 0;
            mantissa = 0;
            break;
        case FloatComponentsCompressed::Infinity:
            exponent = 0b11111111;
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
#ifdef FLOAT8_SUPPORTS_NAN_AND_INFINITY
    if (is_nan_or_infinity())
        return true;
#else
    // don't support nan or infinity
    RAW_ASSERT(!is_nan_or_infinity());
#endif
    if (exponent == 0)
    {
        return !mantissa;
    }
    uint32_t exponent_without_bias = exponent - FloatComponentsCompressed::exponent_bias;
    if (exponent_without_bias == 0)
        return false;
    if ((exponent_without_bias & 0b111) != exponent_without_bias)
        return false;
    constexpr uint32_t mantissa_mask = 0b1111 << (mantissa_bits - FloatComponentsCompressed::mantissa_bits);
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
inline void reference(BinaryOutput & output, const float & data)
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
}

#undef simple_reference

