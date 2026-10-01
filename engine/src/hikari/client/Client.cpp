#include "hikari/client/Client.hpp"
#include "hikari/client/ClientRuntime.hpp"
#include "hikari/client/audio/AudioService.hpp"
#include "hikari/client/game/GameProgress.hpp"
#include "hikari/client/game/StageSelectState.hpp"
#include "hikari/client/game/StageSelectStateConfig.hpp"
#include "hikari/client/game/GamePlayState.hpp"
#include "hikari/client/game/GameOverState.hpp"
#include "hikari/client/game/KeyboardInput.hpp"
#include "hikari/client/game/Input.hpp"
#include "hikari/client/game/ScreenEffectsService.hpp"
#include "hikari/client/game/events/EventBus.hpp"
#include "hikari/client/game/events/EventBusImpl.hpp"
#include "hikari/client/game/events/EventListenerDelegate.hpp"
#include "hikari/client/game/events/GameQuitEventData.hpp"

#include "hikari/client/game/PasswordState.hpp"
#include "hikari/client/game/WeaponGetState.hpp"
#include "hikari/client/game/TitleState.hpp"
#include "hikari/client/game/OptionsState.hpp"
#include "hikari/client/game/WeaponTable.hpp"
#include "hikari/client/game/DamageTable.hpp"
#include "hikari/client/game/PaletteHelpers.hpp"
#include "hikari/client/game/objects/EnemyFactory.hpp"
#include "hikari/client/game/objects/FactoryHelpers.hpp"
#include "hikari/client/game/objects/ItemFactory.hpp"
#include "hikari/client/game/objects/ProjectileFactory.hpp"
#include "hikari/client/game/objects/ParticleFactory.hpp"
#include "hikari/client/game/objects/PalettedAnimatedSprite.hpp"
#include "hikari/client/gui/GuiService.hpp"
#include "hikari/client/scripting/SquirrelService.hpp"
#include "hikari/client/scripting/AudioServiceScriptProxy.hpp"
#include "hikari/client/scripting/GameProgressScriptProxy.hpp"
#include "hikari/client/scripting/GamePlayStateScriptProxy.hpp"
#include "hikari/client/game/objects/Enemy.hpp"


#include "hikari/core/game/AnimationLoader.hpp"
#include "hikari/core/game/SliceStateTransition.hpp"
#include "hikari/core/game/map/MapLoader.hpp"
#include "hikari/core/game/map/TilesetLoader.hpp"
#include "hikari/core/util/AnimationSetCache.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/FileSystemSession.hpp"
#include "hikari/core/util/exception/HikariException.hpp"
#include "hikari/core/util/ImageCache.hpp"
#include "hikari/core/util/Log.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include "hikari/core/util/TilesetCache.hpp"

#include <squirrel.h>
#include <sqrat.h>

#include <SFML/Graphics/Color.hpp>

#include <guichan/gui.hpp>
#include <guichan/exception.hpp>

#include <json/reader.h>

namespace hikari {

    struct Client::SharedGraphicsResources {
        ~SharedGraphicsResources() {
            PalettedAnimatedSprite::destroySharedResources();
            SliceStateTransition::destroySharedTextures();
        }
    };

    const std::string Client::APP_TITLE             = "hikari";
    const std::string Client::PATH_CONTENT          = "content.zip";
    const std::string Client::PATH_CUSTOM_CONTENT   = "custom.zip";
    const std::string Client::PATH_CONFIG_FILE      = "conf.json";
    const std::string Client::PATH_GAME_CONFIG_FILE = "game.json";
    const std::string Client::PATH_DAMAGE_FILE      = "damage.json";

    const unsigned int Client::SCREEN_WIDTH          = 256;
    const unsigned int Client::SCREEN_HEIGHT         = 240;
    const unsigned int Client::SCREEN_BITS_PER_PIXEL = 32;

    Client::Client(int argc, char** argv)
        : filesystem()
        , gameConfigJson()
        , clientConfig()
        , gameConfig()
        , videoMode({SCREEN_WIDTH, SCREEN_HEIGHT}, SCREEN_BITS_PER_PIXEL)
        , window()
        , screenBuffer()
        , quitGame(false)
    {
        initLogging(argc, argv);
        initFileSystem(argc, argv);
        initConfig();
    }

    Client::~Client() = default;

    void Client::initConfig() {
        // Load client config first
        if(FileSystem::exists(PATH_CONFIG_FILE)) {
            auto fs = FileSystem::openFileRead(PATH_CONFIG_FILE);

            Json::Reader reader;
            Json::Value value;

            bool success = reader.parse(*fs, value, false);

            if(!success) {
                HIKARI_LOG(info) << "Configuration file could not be found or was corrupt, using defaults.";
            } else {
                clientConfig = ClientConfig(value);
            }
        } else {
            HIKARI_LOG(fatal) << "Couldn't find configuration file '" << PATH_CONFIG_FILE << "'";
        }

        // Then load the game config
        if(FileSystem::exists(PATH_GAME_CONFIG_FILE)) {
            auto fs = FileSystem::openFileRead(PATH_GAME_CONFIG_FILE);

            Json::Reader reader;
            Json::Value value;

            bool success = reader.parse(*fs, value, false);

            if(!success) {
                throw HikariException("Game configuration file could not be found or was corrupt.");
            } else {
                gameConfigJson = value;
                gameConfig = std::make_shared<GameConfig>(value);
            }
        } else {
            throw HikariException("Couldn't find game configuration file '" + PATH_GAME_CONFIG_FILE + "'");
        }
    }

    void Client::initEventBus() {
        EventListenerDelegate quitRequestDelegate([&](const EventDataPtr & evt) {
            HIKARI_LOG(info) << "Quit requested!";
            quitGame = true;
        });

        runtime->events->addListener(quitRequestDelegate, GameQuitEventData::Type);
    }

    void Client::initFileSystem(int argc, char** argv) {
        // Create virtual file system
        filesystem = std::make_unique<FileSystemSession>(argv[0]);
        PhysFS::addToSearchPath(PhysFS::getBaseDir());

        // Load standard content
        try {
            PhysFS::addToSearchPath(PhysFS::getBaseDir() + PATH_CONTENT);
        } catch(PhysFS::Exception & ex) {
            HIKARI_LOG(error) << "Couldn't load content package. Reason: " << ex.what();
        }

        // Load custom content (if it exists)
        try {
            PhysFS::addToSearchPath(PhysFS::getBaseDir() + PATH_CUSTOM_CONTENT);
        } catch(PhysFS::Exception & ex) {
            HIKARI_LOG(warning) << "(This is normal) Couldn't load custom content package. Reason: " << ex.what();
        }

        PhysFS::setWriteDir(PhysFS::getBaseDir());
    }

    void Client::initGame() {
        loadPalettes();
        loadScriptingEnvironment();
        loadObjectTemplates();
        loadDamageTable();

        auto & controller = runtime->controller;
        const GamePlayDependencies dependencies{
            *gameConfig, *runtime->audio, *runtime->gui, *runtime->weapons,
            *runtime->damage, *runtime->progress, *runtime->screenEffects, *runtime->maps,
            *runtime->animations, *runtime->items, *runtime->enemies,
            *runtime->particles, *runtime->projectiles
        };
        auto gamePlayState = std::make_shared<GamePlayState>("gameplay", controller, gameConfigJson, dependencies);
        GamePlayStateScriptProxy::setWrappedService(gamePlayState);

        // Create controller and game states
        StageSelectStateConfig stageSelectConfig(gameConfigJson["states"]["select"]);

        StatePtr stageSelectState(new StageSelectState("stageselect", gameConfigJson["states"]["select"], stageSelectConfig, controller,
            *runtime->gui, *runtime->audio, *runtime->progress, *runtime->screenEffects, *runtime->images, *runtime->animations));
        StatePtr gameOverState(new GameOverState("gameover", gameConfigJson, controller, *runtime->gui, *runtime->audio, *runtime->input));
        StatePtr passwordState(new PasswordState("password", gameConfigJson, controller, *runtime->gui, *runtime->audio, *runtime->input));
        StatePtr weaponGetState(new WeaponGetState("weaponget", controller, *gameConfig, *runtime->gui, *runtime->audio, *runtime->progress, *runtime->input));
        StatePtr titleState(new TitleState("title", gameConfigJson, controller, *runtime->gui, *runtime->audio, *runtime->events));
        StatePtr optionsState(new OptionsState("options", gameConfigJson, controller, *runtime->gui));

        controller.addState(stageSelectState->getName(), stageSelectState);
        controller.addState(gamePlayState->getName(), gamePlayState);
        controller.addState(gameOverState->getName(), gameOverState);
        controller.addState(passwordState->getName(), passwordState);
        controller.addState(weaponGetState->getName(), weaponGetState);
        controller.addState(titleState->getName(), titleState);
        controller.addState(optionsState->getName(), optionsState);

        controller.setState(gameConfig->getInitialState());
    }

    void Client::initLogging(int argc, char** argv) {
        // #ifdef DEBUG
        ::hikari::Log::setReportingLevel(debug4);
        // #else
        //::hikari::Log::setReportingLevel(warning);
        // #endif
    }

    void Client::initServices() {
        runtime = std::make_unique<ClientRuntime>(clientConfig, gameConfigJson, screenBuffer);
        initEventBus();
    }

    void Client::initWindow() {
        std::string videoScale = clientConfig.getVideoMode();
        bool enabledFullScreen = false;
        unsigned int screenScaler = 1;

        if(videoScale == ClientConfig::VIDEO_SCALE_1X) {
            screenScaler = 1;
        } else if(videoScale == ClientConfig::VIDEO_SCALE_2X) {
            screenScaler = 2;
        } else if(videoScale == ClientConfig::VIDEO_SCALE_3X) {
            screenScaler = 3;
        } else {
            if(videoScale == ClientConfig::VIDEO_SCALE_FULL) {
                enabledFullScreen = true;
            }
        }

        videoMode.size = {SCREEN_WIDTH * screenScaler, SCREEN_HEIGHT * screenScaler};

        // Due to some weirdness between OSX, Windows, and Linux, the window
        // needs to be created before anything serious can be done.
        window.create(
            videoMode,
            APP_TITLE,
            enabledFullScreen ? sf::State::Fullscreen : sf::State::Windowed);
        window.setActive(true);
        window.setVerticalSyncEnabled(clientConfig.isVsyncEnabled());
        window.setKeyRepeatEnabled(false);

        // Screen buffer is hard-coded to be the size of the render area, in
        // other words, it doesn't scale with the window size. When it is
        // rendered it will be stretched to fit the window. This makes it retain
        // its (desired) pixelated quality.
        screenBuffer.resize({SCREEN_WIDTH, SCREEN_HEIGHT});

        screenBufferView.setSize({
            static_cast<float>(SCREEN_WIDTH),
            static_cast<float>(SCREEN_HEIGHT)});

        screenBufferView.setCenter({
            static_cast<float>(SCREEN_WIDTH / 2),
            static_cast<float>(SCREEN_HEIGHT / 2)});

        graphicsResources = std::make_unique<SharedGraphicsResources>();
        SliceStateTransition::createSharedTextures();
    }

    void Client::loadPalettes() {
        PalettedAnimatedSprite::setShaderFile("assets/shaders/palette.frag");
        PalettedAnimatedSprite::createColorTable(
            PaletteHelpers::loadPaletteFile("assets/palettes.json"));
    }

    void Client::loadScriptingEnvironment() {
        for(const auto & scriptPath : gameConfig->getStartUpScripts()) {
            runtime->scripting->runScriptFile(scriptPath);
        }
    }

    void Client::loadObjectTemplates() {
        FactoryHelpers::populateCollectableItemFactory(gameConfig->getItemTemplatePath(),
            *runtime->items, *runtime->images, *runtime->animations, *runtime->scripting);
        FactoryHelpers::populateEnemyFactory(gameConfig->getEnemyTemplatePath(),
            *runtime->enemies, *runtime->images, *runtime->animations, *runtime->scripting);
        FactoryHelpers::populateParticleFactory(gameConfig->getParticleTemplatePath(),
            *runtime->particles, *runtime->images, *runtime->animations);
        FactoryHelpers::populateProjectileFactory(gameConfig->getProjectileTemplatePath(),
            *runtime->projectiles, *runtime->images, *runtime->animations);
        FactoryHelpers::populateWeaponTable(gameConfig->getWeaponTemplatePath(), *runtime->weapons);
    }

    void Client::loadDamageTable() {
        auto & damageTable = *runtime->damage;
        auto fs = FileSystem::openFileRead(PATH_DAMAGE_FILE);

        Json::Reader reader;
        Json::Value value;

        bool success = reader.parse(*fs, value, false);

        if(success) {
            const auto & damageArray = value["damage"];

            for(unsigned int i = 0, length = damageArray.size(); i < length; i++) {
                const auto & damageEntry = damageArray[i];
                const int damageId = damageEntry["id"].asInt();
                const float damageAmount = static_cast<float>(damageEntry["amount"].asDouble());

                damageTable.addEntry(damageId, damageAmount);
            }

            const auto & buffsArray = value["buffs"];

            for(unsigned int i = 0, length = buffsArray.size(); i < length; i++) {
                // const auto & damageEntry = buffsArray[i];
                // TODO: Add buffs to damage table
            }
        } else {
            HIKARI_LOG(info) << "Damage table file could not be found or was corrupt, using defaults.";
            damageTable.addEntry(0, 0.0f);
            damageTable.addEntry(1, 1.0f);
        }
    }

    void Client::loop() {
        sf::Clock clock;
        quitGame = false;

        const float dt = 1.0f/60.0f;
        float totalRuntime = 0.0f;
        float speedMultiplier = 1.0f;

        sf::Time currentTime = clock.getElapsedTime();
        float accumulator = 0.0f;

        auto & guiService = runtime->gui;
        auto & audioService = runtime->audio;
        auto & screenEffectsService = runtime->screenEffects;
        auto & controller = runtime->controller;

        gcn::Gui & gui = guiService->getGui();

        while(!quitGame) {

            //
            // Logic
            //

            sf::Time newTime = clock.getElapsedTime();
            float frameTime = newTime.asSeconds() - currentTime.asSeconds();
            currentTime = newTime;
            accumulator += frameTime;
            // float fps = 1.0f / frameTime;

            while(accumulator >= dt) {
                while(auto event = window.pollEvent()) {
                    if(event->is<sf::Event::Closed>()) {
                        quitGame = true;
                    }

                    const sf::Event::KeyPressed* keyPressed = event->getIf<sf::Event::KeyPressed>();
                    const sf::Event::KeyReleased* keyReleased = event->getIf<sf::Event::KeyReleased>();
                    if(keyPressed || keyReleased) {
                        const sf::Keyboard::Key keyCode = keyPressed ? keyPressed->code : keyReleased->code;

                        // Audio tweaking code
                        if(keyCode == sf::Keyboard::Key::Y) {
                            audioService->setMusicVolume(audioService->getMusicVolume() + 10.0f);
                        }
                        if(keyCode == sf::Keyboard::Key::U) {
                            audioService->setMusicVolume(audioService->getMusicVolume() - 10.0f);
                        }
                        if(keyCode == sf::Keyboard::Key::H) {
                            audioService->mute();
                        }
                        if(keyCode == sf::Keyboard::Key::J) {
                            audioService->unmute();
                        }

                        runtime->input->processEvent(*event);
                        controller.handleEvent(*event);
                    }

                    guiService->processEvent(*event);
                }

                // Process queued GUI input once per tick, after all events are polled.
                if(gui.getTop()) {
                    try {
                        gui.logic();
                    } catch(gcn::Exception & gex) {
                        HIKARI_LOG(error) << "Uncaught exception from GUI: " << gex.getMessage()
                            << "\n\tFile: " << gex.getFilename()
                            << "\n\tFunction: " << gex.getFunction()
                            << "\n\tLine: " << gex.getLine();
                    }
                }

                controller.update(dt * speedMultiplier);
                screenEffectsService->update(dt * speedMultiplier);

                // Update input after the game controller so you don't accidentally
                // skip an event that took place.
                runtime->input->update(dt);

                accumulator -= dt;
                totalRuntime += dt;
            }

            //
            // Rendering
            //

            window.clear(sf::Color::Blue);
            screenBuffer.clear(sf::Color::Magenta);
            controller.render(screenBuffer);

            // This is weird because we call display() twice on screenBuffer.
            // We need to do this in order to render it to a buffer, and then
            // that buffer gets rendered back to screenBuffer, hence the need
            // to call display() again.
            screenBuffer.display();
            screenEffectsService->setInputTexture(screenBuffer);
            screenEffectsService->render(screenBuffer);

            window.setView(screenBufferView);
            guiService->renderHudContainer();

            screenBuffer.display();

            sf::Sprite renderSprite(screenBuffer.getTexture());

            window.draw(renderSprite);
            window.display();
        }

        HIKARI_LOG(debug) << "Quitting; total run time = " << totalRuntime << " seconds.";
    }

    int Client::run() {
        initWindow();
        initServices();
        initGame();

        loop();

        return 0;
    }

} // hikari
