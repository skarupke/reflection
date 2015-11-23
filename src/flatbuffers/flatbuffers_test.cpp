#include "test_generated.h"

#include "os/mmapped_file.hpp"
#include "debug/assert.hpp"

#ifndef DISABLE_TESTS
#include <benchmark/benchmark.h>
#include <fstream>
void FlatbuffersReading(benchmark::State & state)
{
    namespace fb = flatbuffers;
    namespace tfb = test_flatbuffers;
    std::string filename = "/tmp/flatbuffers_test";
    {
        fb::FlatBufferBuilder builder;
        std::vector<fb::Offset<tfb::Monster>> monster_array;
        for (int i = 0; i < 100000; ++i)
        {
            tfb::Vec4 vec(0.0f, 0.0f, 0.0f, 0.0f);
            monster_array.push_back(test_flatbuffers::CreateMonster(builder, &vec, i % 5, float(i)));
        }
        auto vec = builder.CreateVector(monster_array);
        tfb::ArrayBuilder array(builder);
        array.add_array(vec);
        builder.Finish(array.Finish());
        std::ofstream file(filename);
        file.write(reinterpret_cast<const char *>(builder.GetBufferPointer()), builder.GetSize());
    }

    while (state.KeepRunning())
    {
        MMappedFileRead file(filename);
        const tfb::Array * read_array = tfb::GetArray(file.get_bytes().begin());
        auto array = read_array->array();
        int count = 0;
        for (auto it = array->begin(), end = array->end(); it != end; ++it)
        {
            RAW_ASSERT(it->vec()->x() == 0.0f);
            RAW_ASSERT(it->vec()->y() == 0.0f);
            RAW_ASSERT(it->vec()->z() == 0.0f);
            RAW_ASSERT(it->vec()->w() == 0.0f);
            RAW_ASSERT(it->f() == float(count));
            RAW_ASSERT(it->i() == count % 5);
            ++count;
        }
        RAW_ASSERT(count == 100000);
        file.close_and_evict_from_os_cache();
    }
}
BENCHMARK(FlatbuffersReading);

#endif
