#include "metav3/metav3.hpp"
#include <fstream>
#include "metav3/serialization/optimistic_binary.hpp"
#include "debug/profile.hpp"
#include "metav3/metav3_stl.hpp"
#include "metafast/metafast.hpp"
#include <benchmark/benchmark.h>
#include <sstream>

namespace
{
using namespace metav3;
struct memcpy_speed_comparison
{
    float vec[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    unsigned i = 0;
    float f = 0.0f;

    bool operator==(const memcpy_speed_comparison & other) const
    {
        return std::equal(vec, vec + 4, other.vec) && i == other.i && f == other.f;
    }
    bool operator!=(const memcpy_speed_comparison & other) const
    {
        return !(*this == other);
    }
};
static MetaType::StructInfo::MemberCollection get_memcpy_speed_comparison_members(int16_t)
{
    return
    {
        {
            MetaMember("vec", &memcpy_speed_comparison::vec),
            MetaMember("i", &memcpy_speed_comparison::i),
            MetaMember("f", &memcpy_speed_comparison::f),
        },
        {
        }
    };
}
}
template<>
const MetaType MetaType::MetaTypeConstructor<memcpy_speed_comparison>::type = MetaType::RegisterStruct<memcpy_speed_comparison>("memcpy_speed_comparison", 0, &get_memcpy_speed_comparison_members);

REFLECT_CLASS_START(memcpy_speed_comparison, 0)
    REFLECT_MEMBER(vec);
    REFLECT_MEMBER(i);
    REFLECT_MEMBER(f);
REFLECT_CLASS_END()

void test_write_memcpy(const std::string & filename, const std::vector<memcpy_speed_comparison> & elements)
{
    std::ofstream file(filename, std::ios::binary);
    size_t size = elements.size();
    file.write(reinterpret_cast<const char *>(&size), sizeof(size));
    file.write(reinterpret_cast<const char *>(elements.data()), sizeof(memcpy_speed_comparison) * elements.size());
}
#include "os/mmapped_file.hpp"
#include <cstring> // for memcpy
std::vector<memcpy_speed_comparison> memcpy_from_bytes(Range<const unsigned char> bytes)
{
    size_t size = *reinterpret_cast<const size_t *>(bytes.begin());
    Range<const unsigned char> content = { bytes.begin() + sizeof(size), bytes.end() };
    std::vector<memcpy_speed_comparison> elements;
    elements.resize(size);
    memcpy(elements.data(), content.begin(), content.size());
    return elements;
}

void test_write_serialization(const std::string & filename, const std::vector<memcpy_speed_comparison> & elements)
{
    std::ofstream file(filename);
    write_optimistic_binary(elements, file);
}
std::vector<memcpy_speed_comparison> test_read_serialization(const std::string & filename)
{
    MMappedFileRead file(filename);
    std::vector<memcpy_speed_comparison> result;
    read_optimistic_binary(result, file.get_bytes());
    return result;
}
void test_write_serialization_fast(const std::string & filename, const std::vector<memcpy_speed_comparison> & elements)
{
    std::ofstream file(filename);
    metaf::detail::reference(file, elements);
}

std::vector<memcpy_speed_comparison> generate_comparison_data()
{
    std::vector<memcpy_speed_comparison> elements(100000);
    for (size_t i = 0; i < elements.size(); ++i)
    {
         elements[i].f = float(i);
         elements[i].i = i % 5;
    }
    return elements;
}

void MemcpyInMemory(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::vector<memcpy_speed_comparison> comparison;
    while (state.KeepRunning())
    {
        comparison = elements;
    }
    RAW_ASSERT(comparison == elements);
}
BENCHMARK(MemcpyInMemory);

void MemcpyReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string memcpy_filename = "/tmp/memcpy_test";
    test_write_memcpy(memcpy_filename, elements);
    std::vector<memcpy_speed_comparison> comparison;
    while (state.KeepRunning())
    {
        MMappedFileRead file(memcpy_filename);
        comparison = memcpy_from_bytes(file.get_bytes());
        file.clear_and_evict_from_os_cache();
    }
    RAW_ASSERT(comparison == elements);
}
BENCHMARK(MemcpyReading);

void ReflectionInMemory(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::stringstream buffer;
    metaf::detail::reference(buffer, elements);
    std::string in_memory = buffer.str();
    std::vector<memcpy_speed_comparison> comparison;
    while (state.KeepRunning())
    {
        metaf::BinaryInput input({ reinterpret_cast<const unsigned char *>(in_memory.data()), reinterpret_cast<const unsigned char *>(in_memory.data() + in_memory.size()) });
        metaf::detail::reference(input, comparison);
    }
    auto mismatch = std::mismatch(comparison.begin(), comparison.end(), elements.begin());
    static_cast<void>(mismatch);
    RAW_ASSERT(comparison == elements);
}
BENCHMARK(ReflectionInMemory);

void ReflectionReading(benchmark::State & state)
{
    std::string serialization_filename_fast = "/tmp/serialization_test_fast";
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    test_write_serialization_fast(serialization_filename_fast, elements);
    std::vector<memcpy_speed_comparison> comparison;
    while (state.KeepRunning())
    {
        MMappedFileRead file(serialization_filename_fast);
        metaf::BinaryInput input = file.get_bytes();
        metaf::detail::reference(input, comparison);
        file.clear_and_evict_from_os_cache();
    }
    RAW_ASSERT(comparison == elements);
}
BENCHMARK(ReflectionReading);

void SlowReflectionReading(benchmark::State & state)
{
    std::vector<memcpy_speed_comparison> elements = generate_comparison_data();
    std::string serialization_filename = "/tmp/serialization_test";
    test_write_serialization(serialization_filename, elements);
    std::vector<memcpy_speed_comparison> comparison;
    while (state.KeepRunning())
    {
        comparison = test_read_serialization(serialization_filename);
    }
    RAW_ASSERT(comparison == elements);
}
//BENCHMARK(SlowReflectionReading);

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
