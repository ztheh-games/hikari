#include "hikari/core/util/FileSystemSession.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include "hikari/core/util/Log.hpp"

namespace hikari {

    FileSystemSession::FileSystemSession(const char * executable) {
        PhysFS::init(executable);
    }

    FileSystemSession::~FileSystemSession() {
        try {
            PhysFS::deinit();
        } catch(const PhysFS::Exception & ex) {
            HIKARI_LOG(error) << "Failed to shut down PhysicsFS: " << ex.what();
        }
    }

}
