#ifndef HIKARI_CLIENT
#define HIKARI_CLIENT

#include "hikari/client/ClientConfig.hpp"
#include "hikari/client/game/GameConfig.hpp"
#include "hikari/core/util/NonCopyable.hpp"
#include <json/value.h>
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

namespace hikari {

    class ClientRuntime;
    class FileSystemSession;

    class Client : private NonCopyable {
    private:
        struct SharedGraphicsResources;

        static const std::string APP_TITLE;
        static const std::string PATH_CONTENT;
        static const std::string PATH_CUSTOM_CONTENT;
        static const std::string PATH_CONFIG_FILE;
        static const std::string PATH_GAME_CONFIG_FILE;
        static const std::string PATH_DAMAGE_FILE;
        static const unsigned int SCREEN_WIDTH;
        static const unsigned int SCREEN_HEIGHT;
        static const unsigned int SCREEN_BITS_PER_PIXEL;

        void initConfig();
        void initEventBus();
        void initFileSystem(int argc, char** argv);
        void initGame();
        void initLogging(int argc, char** argv);
        void initServices();
        void initWindow();
        void loadPalettes();
        void loadScriptingEnvironment();
        void loadObjectTemplates();
        void loadDamageTable();
        void loop();

        // Dependencies precede their consumers so teardown works on every exit path.
        std::unique_ptr<FileSystemSession> filesystem;
        Json::Value gameConfigJson;
        ClientConfig clientConfig;
        std::shared_ptr<GameConfig> gameConfig;
        sf::VideoMode videoMode;
        sf::RenderWindow window;
        sf::RenderTexture screenBuffer;
        sf::View screenBufferView;
        bool quitGame;
        std::unique_ptr<SharedGraphicsResources> graphicsResources;
        std::unique_ptr<ClientRuntime> runtime;

    public:
        Client(int argc, char** argv);
        ~Client();
        int run();
    };

}

#endif
