#include "load_fast.pb.h"
#include "os/mmapped_file.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "debug/assert.hpp"

#ifndef DISABLE_TESTS
#include <benchmark/benchmark.h>
void ProtobufReading(benchmark::State & state)
{
    test::Array array;
    for (int i = 0; i < 100000; ++i)
    {
        test::LoadFastTest * test = array.add_array();
        test->mutable_vec()->set_f0(0.0f);
        test->mutable_vec()->set_f1(0.0f);
        test->mutable_vec()->set_f2(0.0f);
        test->mutable_vec()->set_f3(0.0f);
        test->set_f(float(i));
        test->set_i(i % 5);
    }
    StringView<const char> tmp_filename = "/tmp/protobuf_test";
    {
        UnixFile file(tmp_filename, O_RDWR | O_CREAT, 0644);
        array.SerializePartialToFileDescriptor(file.file_descriptor);
    }
    while (state.KeepRunning())
    {
        UnixFile file(tmp_filename, O_RDONLY);
        test::Array copy;
        copy.ParseFromFileDescriptor(file.file_descriptor);
        for (int i = 0; i < 100000; ++i)
        {
            const test::LoadFastTest & test = copy.array(i);
            RAW_ASSERT(0.0f == test.vec().f0());
            RAW_ASSERT(0.0f == test.vec().f1());
            RAW_ASSERT(0.0f == test.vec().f2());
            RAW_ASSERT(0.0f == test.vec().f3());
            RAW_ASSERT(float(i) == test.f());
            RAW_ASSERT(i % 5 == test.i());
        }
        file.evict_from_os_cache();
    }
}
BENCHMARK(ProtobufReading);
#endif
