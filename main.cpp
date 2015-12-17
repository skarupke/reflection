#include "metav3/metav3.hpp"
#include <fstream>
#include "metav3/serialization/optimistic_binary.hpp"
#include "metav3/serialization/json.hpp"
#include "debug/profile.hpp"
#include "metav3/metav3_stl.hpp"
#include "metafast/metafast.hpp"
#include <benchmark/benchmark.h>
#include <sstream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include "os/mmapped_file.hpp"
#include <cstring> // for memcpy
#include "util/compressed_buffer.hpp"

using namespace metav3;
struct memcpy_speed_comparison
{
    float vec[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    int i = 0;
    float f = 0.0f;

    template<typename Ar>
    void serialize(Ar & archive, int)
    {
        archive & vec;
        archive & i;
        archive & f;
    }
};

REFLECT_CLASS_START(memcpy_speed_comparison, 0)
    REFLECT_MEMBER(vec);
    REFLECT_MEMBER(i);
    REFLECT_MEMBER(f);
REFLECT_CLASS_END()

bool operator==(const memcpy_speed_comparison & lhs, const memcpy_speed_comparison & rhs)
{
    return std::equal(lhs.vec, lhs.vec + 4, rhs.vec) && lhs.i == rhs.i && lhs.f == rhs.f;
}
bool operator!=(const memcpy_speed_comparison & lhs, const memcpy_speed_comparison & rhs)
{
    return !(lhs == rhs);
}

std::vector<memcpy_speed_comparison> test_read_serialization(const std::string & filename)
{
    MMappedFileRead file(filename);
    std::vector<memcpy_speed_comparison> result;
    read_optimistic_binary(result, file.get_bytes());
    return result;
}

std::vector<memcpy_speed_comparison> generate_comparison_data()
{
    std::vector<memcpy_speed_comparison> elements(100000);
    std::mt19937_64 engine(5);
    std::geometric_distribution<int> distribution(1 / 100.0f);
    std::uniform_int_distribution<int> negative_int(0, 1);
    auto random_int = [&]{ return negative_int(engine) ? -distribution(engine) : distribution(engine); };
    std::binomial_distribution<int> float_distribution(256, 0.5f);
    auto random_float = [&]{ return (float_distribution(engine) - 128) / 8.0f; };
    std::uniform_int_distribution<int> unchanged_distribution(0, 2);
    auto should_set = []{ return true; };//[&]{ return !unchanged_distribution(engine); };
    for (size_t i = 0; i < elements.size(); ++i)
    {
        //if (should_set())
        //    std::generate(elements[i].vec, elements[i].vec + 4, random_float);
        if (should_set())
            elements[i].f = random_float();
        if (should_set())
            elements[i].i = random_int();
        //elements[i].f = float(i);
        //elements[i].i = i % 5;
    }
    return elements;
}

void MemcpyInMemory(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    while (state.KeepRunning())
    {
        std::vector<memcpy_speed_comparison> comparison = elements;
        RAW_ASSERT(comparison == elements);
    }
}
BENCHMARK(MemcpyInMemory);

void test_write_memcpy(std::ostream & out, const std::vector<memcpy_speed_comparison> & elements)
{
    size_t size = elements.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    out.write(reinterpret_cast<const char *>(elements.data()), sizeof(memcpy_speed_comparison) * elements.size());
}

void test_write_memcpy(const std::string & filename, const std::vector<memcpy_speed_comparison> & elements)
{
    std::ofstream file(filename, std::ios::binary);
    test_write_memcpy(file, elements);
}
std::vector<memcpy_speed_comparison> memcpy_from_bytes(ArrayView<const unsigned char> bytes)
{
    size_t size = *reinterpret_cast<const size_t *>(bytes.begin());
    ArrayView<const unsigned char> content = { bytes.begin() + sizeof(size), bytes.end() };
    std::vector<memcpy_speed_comparison> elements(size);
    memcpy(elements.data(), content.begin(), content.size());
    return elements;
}

void MemcpyReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string memcpy_filename = "/tmp/memcpy_test";
    test_write_memcpy(memcpy_filename, elements);
    while (state.KeepRunning())
    {
        MMappedFileRead file(memcpy_filename);
        std::vector<memcpy_speed_comparison> comparison = memcpy_from_bytes(file.get_bytes());
        file.close_and_evict_from_os_cache();
        RAW_ASSERT(comparison == elements);
    }
}
BENCHMARK(MemcpyReading);

void MemcpyFileReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string memcpy_filename = "/tmp/memcpy_test";
    test_write_memcpy(memcpy_filename, elements);
    while (state.KeepRunning())
    {
        UnixFile file(memcpy_filename, UnixFile::RDONLY);
        size_t size = file.size();
        std::unique_ptr<unsigned char[]> bytes(new unsigned char[size]);
        ArrayView<unsigned char> as_range(bytes.get(), bytes.get() + size);
        file.read(as_range);
        std::vector<memcpy_speed_comparison> comparison = memcpy_from_bytes(as_range);
        file.evict_from_os_cache();
        RAW_ASSERT(comparison == elements);
    }
}
BENCHMARK(MemcpyFileReading);

void MemcpyDirectReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string memcpy_filename = "/tmp/memcpy_test";
    test_write_memcpy(memcpy_filename, elements);
    while (state.KeepRunning())
    {
        UnixFile file(memcpy_filename, UnixFile::RDONLY);
        size_t vec_size = 0;
        file.read({ reinterpret_cast<unsigned char *>(&vec_size), reinterpret_cast<unsigned char *>(&vec_size) + sizeof(vec_size) });
        std::vector<memcpy_speed_comparison> comparison(vec_size);
        file.read({ reinterpret_cast<unsigned char *>(comparison.data()), reinterpret_cast<unsigned char*>(comparison.data() + vec_size) });
        file.evict_from_os_cache();
        RAW_ASSERT(comparison == elements);
    }
}
BENCHMARK(MemcpyDirectReading);

void MemcpyCompressedReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string memcpy_filename = "/tmp/memcpy_compressed_test";
    {
        std::stringstream uncompressed;
        test_write_memcpy(uncompressed, elements);
        std::string as_string = uncompressed.str();
        std::ofstream out(memcpy_filename);
        CompressedBuffer compressed(as_string);
        out.write(reinterpret_cast<const char *>(compressed.get_bytes().begin()), compressed.get_bytes().size());
    }
    while (state.KeepRunning())
    {
        MMappedFileRead file(memcpy_filename);
        UncompressedBuffer uncompressed(file.get_bytes());
        std::vector<memcpy_speed_comparison> comparison = memcpy_from_bytes(uncompressed.get_bytes());
        file.close_and_evict_from_os_cache();
        RAW_ASSERT(comparison == elements);
    }
}
BENCHMARK(MemcpyCompressedReading);

void ReflectionInMemory(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::stringstream buffer;
    metaf::BinaryOutput output(buffer);
    metaf::write_binary(output, elements);
    std::string in_memory = buffer.str();
    while (state.KeepRunning())
    {
        metaf::BinaryInput input({ reinterpret_cast<const unsigned char *>(in_memory.data()), reinterpret_cast<const unsigned char *>(in_memory.data() + in_memory.size()) });
        std::vector<memcpy_speed_comparison> comparison;
        metaf::read_binary(input, comparison);
        RAW_ASSERT(comparison == elements);
    }
}
BENCHMARK(ReflectionInMemory);

void ReflectionReading(benchmark::State & state)
{
    std::string serialization_filename_fast = "/tmp/serialization_test_fast";
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    {
        std::ofstream file(serialization_filename_fast);
        metaf::BinaryOutput output(file);
        metaf::write_binary(output, elements);
    }
    while (state.KeepRunning())
    {
        MMappedFileRead file(serialization_filename_fast);
        metaf::BinaryInput input = file.get_bytes();
        std::vector<memcpy_speed_comparison> comparison;
        metaf::read_binary(input, comparison);
        RAW_ASSERT(comparison == elements);
        file.close_and_evict_from_os_cache();
    }
}
BENCHMARK(ReflectionReading);

void ReflectionCompressedReading(benchmark::State & state)
{
    std::string serialization_filename_fast = "/tmp/serialization_test_fast_compressed";
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    {
        std::stringstream uncompressed;
        metaf::BinaryOutput output(uncompressed);
        metaf::write_binary(output, elements);
        std::ofstream file(serialization_filename_fast);
        std::string as_string = uncompressed.str();
        CompressedBuffer compressed(as_string);
        file.write(reinterpret_cast<const char *>(compressed.get_bytes().begin()), compressed.get_bytes().size());
    }
    while (state.KeepRunning())
    {
        MMappedFileRead file(serialization_filename_fast);
        UncompressedBuffer uncompressed(file.get_bytes());
        metaf::BinaryInput input(uncompressed.get_bytes());
        std::vector<memcpy_speed_comparison> comparison;
        metaf::read_binary(input, comparison);
        RAW_ASSERT(comparison == elements);
        file.close_and_evict_from_os_cache();
    }
}
BENCHMARK(ReflectionCompressedReading);

void SlowReflectionReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string serialization_filename = "/tmp/serialization_test";
    {
        std::ofstream file(serialization_filename);
        write_optimistic_binary(elements, file);
    }
    while (state.KeepRunning())
    {
        std::vector<memcpy_speed_comparison> comparison = test_read_serialization(serialization_filename);
        RAW_ASSERT(comparison == elements);
    }
}
//BENCHMARK(SlowReflectionReading);

void JsonReflectionReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string serialization_filename = "/tmp/json_serialization_test";
    {
        std::string text = JsonSerializer().serialize(elements);
        std::ofstream file(serialization_filename);
        file.write(text.c_str(), text.size());
    }
    while (state.KeepRunning())
    {
        MMappedFileRead read(serialization_filename);
        std::vector<memcpy_speed_comparison> comparison;
        JsonSerializer().deserialize(comparison, { reinterpret_cast<const char *>(read.get_bytes().begin()), reinterpret_cast<const char *>(read.get_bytes().end()) });
        RAW_ASSERT(comparison == elements);
    }
}
//BENCHMARK(JsonReflectionReading);

void BoostSerializationReading(benchmark::State & state)
{
    std::string boost_filename = "/tmp/boost_serialization_test";
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    {
        std::ofstream file_out(boost_filename);
        boost::archive::binary_oarchive out(file_out);
        out & elements;
    }
    while (state.KeepRunning())
    {
        std::vector<memcpy_speed_comparison> comparison;
        std::ifstream file_in(boost_filename);
        boost::archive::binary_iarchive in(file_in);
        in & comparison;
        RAW_ASSERT(comparison == elements);
        file_in.close();
        UnixFile(boost_filename, 0).evict_from_os_cache();
    }
}
//BENCHMARK(BoostSerializationReading);

#include <gtest/gtest.h>

int main(int argc, char * argv[])
{
    int result = 0;
#ifndef DISABLE_TESTS
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();
#endif
    if (result == 0)
    {
        ::benchmark::Initialize(&argc, argv);
        ::benchmark::RunSpecifiedBenchmarks();
    }
    return result;
}
