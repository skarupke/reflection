#include "util/compressed_buffer.hpp"
#include "lz4.h"
#include "lz4hc.h"

CompressedBuffer::CompressedBuffer(ArrayView<const unsigned char> bytes, int lz4_compression_level)
{
    int compressed_bound = LZ4_compressBound(int(bytes.size()));
    int size_extra_space = sizeof(uint64_t);

    buffer.reset(new unsigned char[compressed_bound + size_extra_space]);
    *reinterpret_cast<uint64_t *>(buffer.get()) = bytes.size();
    size = size_extra_space;

    size += LZ4_compress_HC(reinterpret_cast<const char *>(bytes.begin()), reinterpret_cast<char *>(buffer.get() + size_extra_space), bytes.size(), compressed_bound, lz4_compression_level);
}
CompressedBuffer::CompressedBuffer(StringView<const char> text, int lz4_compression_level)
    : CompressedBuffer(ArrayView<const unsigned char>(reinterpret_cast<const unsigned char *>(text.begin()), reinterpret_cast<const unsigned char *>(text.end())), lz4_compression_level)
{
}

void CompressedBuffer::shrink_to_fit()
{
    std::unique_ptr<unsigned char[]> replacement(new unsigned char[size]);
    std::copy(buffer.get(), buffer.get() + size, replacement.get());
    buffer = std::move(replacement);
}
size_t CompressedBuffer::shrink_to_fit_savings() const
{
    return LZ4_compressBound(*reinterpret_cast<uint64_t *>(buffer.get())) + sizeof(uint64_t) - size;
}

UncompressedBuffer::UncompressedBuffer(ArrayView<const unsigned char> compressed_bytes)
    : size(*reinterpret_cast<const uint64_t *>(compressed_bytes.begin()))
{
    buffer.reset(new unsigned char[size]);
    LZ4_decompress_safe(reinterpret_cast<const char *>(compressed_bytes.begin() + sizeof(uint64_t)), reinterpret_cast<char *>(buffer.get()), compressed_bytes.size() - sizeof(uint64_t), size);
}

#ifndef DISABLE_GTEST
#include <gtest/gtest.h>
TEST(compressed_buffer, roundtrip)
{
    unsigned char bytes[] = { 1, 2, 3, 4, 1, 2, 3, 4 };
    ArrayView<unsigned char> as_view = bytes;
    CompressedBuffer compressed(as_view);
    UncompressedBuffer uncompressed(compressed.get_bytes());
    ASSERT_EQ(as_view, uncompressed.get_bytes());
}
TEST(compressed_buffer, string_roundtrip)
{
    std::string text = "hello, world!";
    CompressedBuffer compressed(text);
    UncompressedBuffer uncompressed(compressed.get_bytes());
    ASSERT_EQ(text, uncompressed.get_text());
}
TEST(compressed_buffer, shrink_to_fit)
{
    unsigned char bytes[] = { 1, 2, 3, 4, 5, 1, 2, 3, 4 };
    ArrayView<unsigned char> as_view = bytes;
    CompressedBuffer compressed(as_view);
    ASSERT_GT(compressed.shrink_to_fit_savings(), 0u);
    compressed.shrink_to_fit();
    UncompressedBuffer uncompressed(compressed.get_bytes());
    ASSERT_EQ(as_view, uncompressed.get_bytes());
}

#endif

