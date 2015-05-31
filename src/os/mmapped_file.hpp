#pragma once

#include "util/range.hpp"
#include <memory>

struct MMappedFileRead
{
    MMappedFileRead(Range<const char> filename);
    ~MMappedFileRead();

    Range<const unsigned char> get_bytes() const;

    void clear_and_evict_from_os_cache();

private:
    struct Internals;
    std::unique_ptr<Internals> internals;
};
