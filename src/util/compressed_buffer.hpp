#pragma once

#include <memory>
#include "util/view.hpp"
#include "util/stl_container_forward.hpp"

struct CompressedBuffer
{
    // lz4_compression_level goes from 0 to 16, with 1 being the least compressed, 16 being
    // the most compressed, and 0 being a special value meaning "use default value."
    // I'm not sure what it actually means. looking at the source code it looks like it
    // doubles some "number of attempts" with every increase. meaning when it's 10, that
    // number is twice as high as when compression_level 9. so maybe the worst case takes twice
    // as long every time that you double this...
    // lz4 recommends values between 4 and 9, and currently 0 makes it default to 9
    CompressedBuffer(ArrayView<const unsigned char> bytes, int lz4_compression_level = 0);
    CompressedBuffer(StringView<const char> bytes, int lz4_compression_level = 0);

    // the above functions will allocate too much. this function
    // will reallocate the buffer to the size that will be returned by get_bytes()
    void shrink_to_fit();
    // indicates how much smaller the internal buffer would be after calling shrink_to_fit
    // compared to the buffer originally allocated
    size_t shrink_to_fit_savings() const;

    ArrayView<const unsigned char> get_bytes() const
    {
        return { buffer.get(), buffer.get() + size };
    }

private:
    std::unique_ptr<unsigned char[]> buffer;
    size_t size;
};

struct UncompressedBuffer
{
    UncompressedBuffer(ArrayView<const unsigned char> compressed_bytes);

    ArrayView<const unsigned char> get_bytes() const
    {
        return { buffer.get(), buffer.get() + size };
    }
    ArrayView<unsigned char> get_bytes()
    {
        return { buffer.get(), buffer.get() + size };
    }
    StringView<const char> get_text() const
    {
        const char * begin = reinterpret_cast<const char *>(buffer.get());
        return { begin, begin + size };
    }
    StringView<char> get_text()
    {
        char * begin = reinterpret_cast<char *>(buffer.get());
        return { begin, begin + size };
    }

private:
    std::unique_ptr<unsigned char[]> buffer;
    uint64_t size;
};
