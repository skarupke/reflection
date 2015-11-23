#pragma once

#include "util/view.hpp"
#include <memory>

struct UnixFile
{
    UnixFile(StringView<const char> filename, int flags);
    UnixFile(StringView<const char> filename, int flags, int mode);
    ~UnixFile();

    bool is_valid() const
    {
        return file_descriptor >= 0;
    }

    void evict_from_os_cache();

    int file_descriptor = -1;
};

struct MMappedFileRead
{
    MMappedFileRead(StringView<const char> filename);
    ~MMappedFileRead();

    ArrayView<const unsigned char> get_bytes() const;

    void close_and_evict_from_os_cache();

private:
    struct Internals;
    std::unique_ptr<Internals> internals;
};
