#pragma once

#include <atomic>
#include <cstdlib>

namespace mem
{
struct MemoryManager
{
	static size_t GetNumAllocations();
	static size_t GetNumFrees();
	static size_t GetTotalBytesAllocated();
};
void * AllocAligned(size_t size, size_t alignment);
void FreeAligned(void * ptr);
}
