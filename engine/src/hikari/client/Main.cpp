#include "hikari/client/Main.hpp"
#include "hikari/client/Client.hpp"
#include "hikari/core/util/Log.hpp"
#include <SDL3/SDL_main.h>
#include <exception>
#include <guichan/exception.hpp>

int main(int argc, char** argv) {
    try {
        return hikari::Client(argc, argv).run();
    } catch(const std::exception & error) {
        HIKARI_LOG(hikari::error) << "Hikari startup/runtime failure: " << error.what();
        return 1;
    } catch(const gcn::Exception & error) {
        HIKARI_LOG(hikari::error) << "Hikari GUI failure: " << error.getMessage();
        return 1;
    }
}
