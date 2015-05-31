#include "profile.hpp"
#include <iostream>
#include <os/memoryManager.hpp>

Measurer::Measurer()
	: before(std::chrono::high_resolution_clock::now())
	, num_allocations_before(mem::MemoryManager::GetNumAllocations())
{
}
std::chrono::high_resolution_clock::duration Measurer::GetDuration() const
{
	return std::chrono::high_resolution_clock::now() - before;
}
size_t Measurer::GetNumAllocations() const
{
	return mem::MemoryManager::GetNumAllocations() - num_allocations_before;
}

ScopedMeasurer::ScopedMeasurer(const char * name)
	: name(name)
	, before(std::chrono::high_resolution_clock::now())
	, num_allocations_before(mem::MemoryManager::GetNumAllocations())
	, num_frees_before(mem::MemoryManager::GetNumFrees())
{
}
ScopedMeasurer::~ScopedMeasurer()
{
	auto time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - before);
	size_t num_allocations = mem::MemoryManager::GetNumAllocations() - num_allocations_before;
	size_t num_frees = mem::MemoryManager::GetNumFrees() - num_frees_before;
	std::cout << name << ": " << time_spent.count() << " ms. " << num_allocations << " allocations, " << num_frees << " frees" << std::endl;
}

