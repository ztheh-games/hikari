#ifndef HIKARI_CORE_UTIL_FILESYSTEMSESSION
#define HIKARI_CORE_UTIL_FILESYSTEMSESSION

#include "hikari/core/util/NonCopyable.hpp"

namespace hikari {

    class FileSystemSession : private NonCopyable {
    public:
        explicit FileSystemSession(const char * executable);
        ~FileSystemSession();
    };

}

#endif
