#pragma once
#include <string>
#include <chrono>

struct Measurer
{
	Measurer();

	std::chrono::high_resolution_clock::duration GetDuration() const;
	size_t GetNumAllocations() const;

private:
	std::chrono::high_resolution_clock::time_point before;
	size_t num_allocations_before;
};

struct ScopedMeasurer
{
	ScopedMeasurer(const char * name);
	~ScopedMeasurer();
	const char * name;
	std::chrono::high_resolution_clock::time_point before;
	size_t num_allocations_before;
	size_t num_frees_before;
};
