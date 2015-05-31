#include "memoryManager.hpp"
#include <cstdlib>
#include "debug/assert.hpp"
#include <algorithm>
#include <cstddef>

namespace mem
{

std::atomic<size_t> num_allocations(0);
std::atomic<size_t> num_frees(0);
std::atomic<size_t> bytes_allocated(0);

size_t MemoryManager::GetNumAllocations()
{
	return num_allocations;
}

size_t MemoryManager::GetNumFrees()
{
	return num_frees;
}

size_t MemoryManager::GetTotalBytesAllocated()
{
	return bytes_allocated;
}

void * AllocAligned(size_t size, size_t alignment)
{
	unsigned char * allocated = static_cast<unsigned char *>(::operator new(size + alignment));
	unsigned char * result = allocated + alignment - reinterpret_cast<uintptr_t>(allocated) % alignment;
	*reinterpret_cast<void **>(result - sizeof(void *)) = allocated;
	return result;
}

void FreeAligned(void * ptr)
{
	if (!ptr) return;
	::operator delete(*reinterpret_cast<void **>(static_cast<unsigned char *>(ptr) - sizeof(void *)));
}
}

#if defined(__has_feature)
#	if __has_feature(address_sanitizer) || __has_feature(memory_sanitizer) || __has_feature(thread_sanitizer)
#		define NO_CUSTOM_OPERATOR_NEW
#	endif
#endif
#ifndef NO_CUSTOM_OPERATOR_NEW
void * operator new(size_t num_bytes)
{
	++mem::num_allocations;
	mem::bytes_allocated += num_bytes;
	return malloc(num_bytes);
}
void * operator new[](size_t num_bytes)
{
	++mem::num_allocations;
	mem::bytes_allocated += num_bytes;
	return malloc(num_bytes);
}

void operator delete(void * ptr) noexcept
{
    if (ptr)
    {
        ++mem::num_frees;
        free(ptr);
    }
}
void operator delete(void * ptr, size_t) noexcept
{
    ::operator delete(ptr);
}
void operator delete[](void * ptr) noexcept
{
    ::operator delete(ptr);
}
void operator delete[](void * ptr, size_t) noexcept
{
    ::operator delete[](ptr);
}

#endif
