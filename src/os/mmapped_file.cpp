#include "os/mmapped_file.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

UnixFile::UnixFile(StringView<const char> filename, int flags)
    : file_descriptor(open(filename.begin(), flags))
{
}
UnixFile::UnixFile(StringView<const char> filename, int flags, int mode)
    : file_descriptor(open(filename.begin(), flags, mode))
{
}
UnixFile::~UnixFile()
{
    if (file_descriptor >= 0)
        close(file_descriptor);
}
void UnixFile::evict_from_os_cache()
{
    fdatasync(file_descriptor);
    posix_fadvise(file_descriptor, 0, 0, POSIX_FADV_DONTNEED);
}

struct MMappedFileRead::Internals
{
    Internals(StringView<const char> filename)
        : file(filename, O_RDONLY)
    {
        if (!file.is_valid())
            return;
        struct stat file_info;
        if (fstat(file.file_descriptor, &file_info) == -1 || !file_info.st_size)
            return;
        void * mapped = mmap(nullptr, file_info.st_size, PROT_READ, MAP_PRIVATE, file.file_descriptor, 0);
        if (!mapped)
            return;
        if (file_info.st_size == 0)
        {
            munmap(mapped, 0);
            return;
        }
        file_contents = { static_cast<unsigned char *>(mapped), static_cast<unsigned char *>(mapped) + file_info.st_size };
    }
    ~Internals()
    {
        if (!file_contents.empty())
            munmap(file_contents.begin(), file_contents.size());
    }

    UnixFile file;
    ArrayView<unsigned char> file_contents;
};

MMappedFileRead::MMappedFileRead(StringView<const char> filename)
    : internals(new Internals(filename))
{
}
MMappedFileRead::~MMappedFileRead() = default;

ArrayView<const unsigned char> MMappedFileRead::get_bytes() const
{
    return internals->file_contents;
}

void MMappedFileRead::close_and_evict_from_os_cache()
{
    if (!internals->file_contents.empty())
    {
        munmap(internals->file_contents.begin(), internals->file_contents.size());
        internals->file_contents = {};
    }
    if (internals->file.is_valid())
        internals->file.evict_from_os_cache();
}

