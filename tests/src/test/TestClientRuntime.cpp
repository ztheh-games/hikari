#include <catch.hpp>
#include "hikari/client/ClientRuntime.hpp"
#include "hikari/client/ClientConfig.hpp"
#include "hikari/client/game/GameConfig.hpp"
#include "hikari/client/game/GameOverState.hpp"
#include "hikari/client/game/GamePlayState.hpp"
#include "hikari/client/game/GameProgress.hpp"
#include "hikari/client/game/GameWorld.hpp"
#include "hikari/client/game/KeyboardInput.hpp"
#include "hikari/client/game/OptionsState.hpp"
#include "hikari/client/game/PasswordState.hpp"
#include "hikari/client/game/RefillHealthTask.hpp"
#include "hikari/client/game/StageSelectState.hpp"
#include "hikari/client/game/TitleState.hpp"
#include "hikari/client/game/WeaponGetState.hpp"
#include "hikari/client/game/objects/FactoryHelpers.hpp"
#include "hikari/client/game/objects/Enemy.hpp"
#include "hikari/client/game/objects/CollectableItem.hpp"
#include "hikari/client/game/objects/EnemyFactory.hpp"
#include "hikari/client/game/objects/ItemFactory.hpp"
#include "hikari/client/game/objects/ParticleFactory.hpp"
#include "hikari/client/game/objects/ProjectileFactory.hpp"
#include "hikari/client/game/objects/brains/ScriptedEnemyBrain.hpp"
#include "hikari/client/game/objects/controllers/CutSceneHeroActionController.hpp"
#include "hikari/client/game/objects/effects/ScriptedEffect.hpp"
#include "hikari/client/gui/GuiService.hpp"
#include "hikari/client/scripting/SquirrelService.hpp"
#include "hikari/client/scripting/AudioServiceScriptProxy.hpp"
#include "hikari/client/scripting/GameProgressScriptProxy.hpp"
#include "hikari/client/scripting/GamePlayStateScriptProxy.hpp"
#include "hikari/core/game/AnimationLoader.hpp"
#include "hikari/core/game/AnimationSet.hpp"
#include "hikari/core/game/Movable.hpp"
#include "hikari/core/game/WorldCollisionResolver.hpp"
#include "hikari/core/util/FileSystemSession.hpp"
#include "hikari/core/util/ImageCache.hpp"
#include "hikari/core/util/JsonUtils.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include <SFML/Graphics/RenderTexture.hpp>
#include <guichan/gui.hpp>
#include <guichan/image.hpp>
#include <guichan/widget.hpp>
#include <guichan/widgets/container.hpp>
#include <stdexcept>
#include <type_traits>

namespace {

    struct StateLifetimeProbe : hikari::GameState {
        hikari::ClientRuntime & runtime;
        bool & destroyedSafely;
        std::string name = "probe";

        StateLifetimeProbe(hikari::ClientRuntime & runtime, bool & destroyedSafely)
            : runtime(runtime), destroyedSafely(destroyedSafely) { }

        ~StateLifetimeProbe() override {
            destroyedSafely = PhysFS::isInit()
                && Sqrat::DefaultVM::Get() == runtime.scripting->getVmInstance()
                && hikari::AudioServiceScriptProxy::getWrappedService().expired()
                && hikari::GameProgressScriptProxy::getWrappedService().expired()
                && hikari::GamePlayStateScriptProxy::getWrappedService().expired()
                && !hikari::Movable::getCollisionResolver();
        }

        void handleEvent(sf::Event &) override { }
        void render(sf::RenderTarget &) override { }
        bool update(float) override { return false; }
        void onEnter() override { }
        void onExit() override { }
        const std::string & getName() const override { return name; }
    };

    Json::Value loadRuntimeConfig() {
        PhysFS::addToSearchPath(HIKARI_TEST_CONTENT_DIR);
        return hikari::JsonUtils::loadJson("game.json");
    }

}

TEST_CASE("Filesystem lifetime covers failed startup", "[runtime][lifetime]") {
    REQUIRE_FALSE(PhysFS::isInit());
    REQUIRE_THROWS_AS(([&] {
        hikari::FileSystemSession filesystem("runtime-tests");
        REQUIRE(PhysFS::isInit());
        throw std::runtime_error("startup failure");
    }()), std::runtime_error);
    REQUIRE_FALSE(PhysFS::isInit());
}

TEST_CASE("Runtime destroys states and script objects before their dependencies", "[runtime][lifetime]") {
    hikari::FileSystemSession filesystem("runtime-tests");
    const auto config = loadRuntimeConfig();
    sf::RenderTexture screen({256, 240});
    bool destroyedSafely = false;
    std::weak_ptr<hikari::SquirrelService> scripting;
    std::weak_ptr<hikari::GuiService> gui;
    gcn::Container fontProbe;
    auto defaultFont = fontProbe.getFont();

    {
        hikari::ClientRuntime runtime(hikari::ClientConfig(), config, screen);
        scripting = runtime.scripting;
        gui = runtime.gui;
        runtime.controller.addState("probe", std::make_shared<StateLifetimeProbe>(runtime, destroyedSafely));
        hikari::Movable::setCollisionResolver(std::make_shared<hikari::WorldCollisionResolver>());

        runtime.scripting->runScriptString(
            "class LifetimeBrain { constructor(config) {} }\n"
            "class LifetimeEffect { constructor(config) {} function applyEffect() {} function unapplyEffect() {} }");
        auto enemy = std::make_shared<hikari::Enemy>(1, nullptr);
        enemy->setBrain(std::make_shared<hikari::ScriptedEnemyBrain>(*runtime.scripting, "LifetimeBrain"));
        runtime.enemies->registerPrototype("probe", enemy);

        auto effect = std::make_shared<hikari::ScriptedEffect>(*runtime.scripting, "LifetimeEffect");
        REQUIRE(effect->clone());
        runtime.items->registerPrototype("probe",
            std::make_shared<hikari::CollectableItem>(2, nullptr, effect));
    }

    REQUIRE(destroyedSafely);
    REQUIRE(scripting.expired());
    REQUIRE(gui.expired());
    REQUIRE(Sqrat::DefaultVM::Get() == nullptr);
    REQUIRE(gcn::Image::getImageLoader() == nullptr);
    REQUIRE(fontProbe.getFont() == defaultFont);
}

TEST_CASE("Runtime can be constructed again after teardown", "[runtime][lifetime]") {
    hikari::FileSystemSession filesystem("runtime-tests");
    const auto config = loadRuntimeConfig();
    sf::RenderTexture screen({256, 240});

    for(int i = 0; i < 2; ++i) {
        hikari::ClientRuntime runtime(hikari::ClientConfig(), config, screen);
        REQUIRE(runtime.gui->getGui().getTop() != nullptr);
        REQUIRE_FALSE(hikari::AudioServiceScriptProxy::getWrappedService().expired());
    }
    REQUIRE(Sqrat::DefaultVM::Get() == nullptr);
}

TEST_CASE("Partial runtime construction clears GUI and VM registrations", "[runtime][lifetime]") {
    hikari::FileSystemSession filesystem("runtime-tests");
    auto config = loadRuntimeConfig();
    config["gui"]["fonts"]["default"]["glyphSize"] = Json::Value(Json::arrayValue);
    sf::RenderTexture screen({256, 240});

    // Force a failure after the VM exists and the GUI has installed its image loader.
    REQUIRE_THROWS(hikari::ClientRuntime(hikari::ClientConfig(), config, screen));
    REQUIRE(Sqrat::DefaultVM::Get() == nullptr);
    REQUIRE(gcn::Image::getImageLoader() == nullptr);
    REQUIRE(hikari::AudioServiceScriptProxy::getWrappedService().expired());
}

TEST_CASE("Factories and cut-scene input need no service dependencies", "[runtime][injection]") {
    static_assert(std::is_default_constructible<hikari::ItemFactory>::value, "ItemFactory needs no services");
    static_assert(std::is_default_constructible<hikari::EnemyFactory>::value, "EnemyFactory needs no services");
    static_assert(std::is_default_constructible<hikari::ParticleFactory>::value, "ParticleFactory needs no services");
    static_assert(std::is_default_constructible<hikari::ProjectileFactory>::value, "ProjectileFactory needs no services");

    hikari::CutSceneHeroActionController controller;
    REQUIRE_FALSE(controller.shouldMoveRight());
    controller.moveRight();
    controller.superJump();
    REQUIRE(controller.shouldMoveRight());
    REQUIRE(controller.shouldSuperJump());
    controller.stopMoving();
    controller.stopJumping();
    REQUIRE_FALSE(controller.shouldMoveRight());
    REQUIRE_FALSE(controller.shouldSuperJump());

    hikari::ItemFactory items;
    hikari::EnemyFactory enemies;
    hikari::ParticleFactory particles;
    hikari::ProjectileFactory projectiles;
    enemies.registerPrototype("probe", std::make_shared<hikari::Enemy>(2, nullptr));
    hikari::GameWorld world(items, enemies, particles, projectiles);
    auto enemy = world.spawnEnemy("probe");
    REQUIRE(enemy);
    REQUIRE(enemy->getId() != 2);
}

TEST_CASE("Refill tasks use the injected progress instance", "[runtime][injection]") {
    hikari::FileSystemSession filesystem("runtime-tests");
    const auto config = loadRuntimeConfig();
    sf::RenderTexture screen({256, 240});
    hikari::ClientRuntime runtime(hikari::ClientConfig(), config, screen);
    runtime.progress->setPlayerEnergy(7);
    hikari::GameProgress progress;
    auto type = hikari::RefillHealthTask::PLAYER_ENERGY;

    SECTION("Player energy is capped") {
        progress.setPlayerEnergy(26);
    }
    SECTION("Weapon energy is capped") {
        type = hikari::RefillHealthTask::WEAPON_ENERGY;
        progress.setCurrentWeapon(1);
        progress.setWeaponEnergy(1, 26);
    }
    SECTION("Boss energy is capped") {
        type = hikari::RefillHealthTask::BOSS_ENERGY;
        progress.setBossMaxEnergy(16);
        progress.setBossEnergy(14);
    }

    hikari::RefillHealthTask task(type, 100, *runtime.audio, progress);

    for(int i = 0; i < 30 && !task.isComplete(); ++i) {
        task.update(1.0f / 60.0f);
    }
    REQUIRE(task.isComplete());
    if(type == hikari::RefillHealthTask::PLAYER_ENERGY) {
        REQUIRE(progress.getPlayerEnergy() == 28);
    } else if(type == hikari::RefillHealthTask::WEAPON_ENERGY) {
        REQUIRE(progress.getWeaponEnergy(1) == 28);
    } else {
        REQUIRE(progress.getBossEnergy() == 16);
    }
    REQUIRE(runtime.progress->getPlayerEnergy() == 7);
}

TEST_CASE("Animation loaders use their own injected image cache", "[runtime][injection]") {
    hikari::FileSystemSession filesystem("runtime-tests");
    loadRuntimeConfig();
    hikari::ImageCache firstImages(hikari::ImageCache::NO_SMOOTHING, hikari::ImageCache::USE_MASKING);
    hikari::ImageCache secondImages(hikari::ImageCache::NO_SMOOTHING, hikari::ImageCache::USE_MASKING);
    hikari::AnimationLoader firstLoader(firstImages);
    hikari::AnimationLoader secondLoader(secondImages);
    const auto firstSet = firstLoader.loadSet("assets/animations/cursor.json");
    const auto secondSet = secondLoader.loadSet("assets/animations/cursor.json");

    REQUIRE(firstSet);
    REQUIRE(secondSet);
    REQUIRE(firstSet->getTexture() == firstImages.get(firstSet->getImageFileName()));
    REQUIRE(secondSet->getTexture() == secondImages.get(secondSet->getImageFileName()));
    REQUIRE(firstSet->getTexture() != secondSet->getTexture());
}

TEST_CASE("Production states use injected content and survive queued-work teardown", "[runtime][injection][states]") {
    hikari::FileSystemSession filesystem("runtime-tests");
    const auto config = loadRuntimeConfig();
    const hikari::GameConfig gameConfig(config);
    sf::RenderTexture screen({256, 240});
    std::weak_ptr<hikari::GamePlayState> gameplayObserver;

    {
        hikari::ClientRuntime runtime(hikari::ClientConfig(), config, screen);
        for(const auto & script : gameConfig.getStartUpScripts()) {
            runtime.scripting->runScriptFile(script);
        }
        hikari::FactoryHelpers::populateCollectableItemFactory(gameConfig.getItemTemplatePath(),
            *runtime.items, *runtime.images, *runtime.animations, *runtime.scripting);
        hikari::FactoryHelpers::populateEnemyFactory(gameConfig.getEnemyTemplatePath(),
            *runtime.enemies, *runtime.images, *runtime.animations, *runtime.scripting);
        hikari::FactoryHelpers::populateParticleFactory(gameConfig.getParticleTemplatePath(),
            *runtime.particles, *runtime.images, *runtime.animations);
        hikari::FactoryHelpers::populateProjectileFactory(gameConfig.getProjectileTemplatePath(),
            *runtime.projectiles, *runtime.images, *runtime.animations);
        hikari::FactoryHelpers::populateWeaponTable(gameConfig.getWeaponTemplatePath(), *runtime.weapons);

        auto & controller = runtime.controller;
        const hikari::GamePlayDependencies dependencies{
            gameConfig, *runtime.audio, *runtime.gui, *runtime.weapons,
            *runtime.damage, *runtime.progress, *runtime.screenEffects, *runtime.maps,
            *runtime.animations, *runtime.items, *runtime.enemies, *runtime.particles, *runtime.projectiles
        };
        auto gameplay = std::make_shared<hikari::GamePlayState>("gameplay", controller, config, dependencies);
        gameplayObserver = gameplay;
        hikari::GamePlayStateScriptProxy::setWrappedService(gameplay);
        controller.addState("gameplay", gameplay);
        controller.addState("stageselect", std::make_shared<hikari::StageSelectState>(
            "stageselect", config["states"]["select"], hikari::StageSelectStateConfig(config["states"]["select"]),
            controller, *runtime.gui, *runtime.audio, *runtime.progress,
            *runtime.screenEffects, *runtime.images, *runtime.animations));
        controller.addState("title", std::make_shared<hikari::TitleState>(
            "title", config, controller, *runtime.gui, *runtime.audio, *runtime.events));
        controller.addState("options", std::make_shared<hikari::OptionsState>(
            "options", config, controller, *runtime.gui));
        controller.addState("password", std::make_shared<hikari::PasswordState>(
            "password", config, controller, *runtime.gui, *runtime.audio, *runtime.input));
        controller.addState("gameover", std::make_shared<hikari::GameOverState>(
            "gameover", config, controller, *runtime.gui, *runtime.audio, *runtime.input));
        controller.addState("weaponget", std::make_shared<hikari::WeaponGetState>(
            "weaponget", controller, gameConfig, *runtime.gui, *runtime.audio, *runtime.progress, *runtime.input));

        for(const auto * state : {"title", "options", "password", "gameover", "weaponget", "stageselect", "gameplay"}) {
            REQUIRE_NOTHROW(controller.setState(state));
            REQUIRE_NOTHROW(controller.update(1.0f / 60.0f));
            REQUIRE_NOTHROW(controller.render(screen));
        }

        REQUIRE(hikari::Movable::getCollisionResolver());
        runtime.progress->setPlayerEnergy(1);
        gameplay->refillPlayerEnergy(10);
        controller.update(1.0f / 60.0f);
        controller.requestStateChange("title");
        controller.update(1.0f / 60.0f);
    }

    REQUIRE(gameplayObserver.expired());
    REQUIRE_FALSE(hikari::Movable::getCollisionResolver());
    REQUIRE(hikari::GamePlayStateScriptProxy::getWrappedService().expired());
    REQUIRE(Sqrat::DefaultVM::Get() == nullptr);
    REQUIRE(gcn::Image::getImageLoader() == nullptr);
}
