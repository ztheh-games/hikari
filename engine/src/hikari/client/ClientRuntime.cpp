#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/client/ClientRuntime.hpp"
#include "hikari/client/ClientConfig.hpp"
#include "hikari/client/audio/AudioService.hpp"
#include "hikari/client/game/GameProgress.hpp"
#include "hikari/client/game/KeyboardInput.hpp"
#include "hikari/client/game/ScreenEffectsService.hpp"
#include "hikari/client/game/WeaponTable.hpp"
#include "hikari/client/game/DamageTable.hpp"
#include "hikari/client/game/events/EventBusImpl.hpp"
#include "hikari/client/game/objects/ItemFactory.hpp"
#include "hikari/client/game/objects/EnemyFactory.hpp"
#include "hikari/client/game/objects/ProjectileFactory.hpp"
#include "hikari/client/game/objects/ParticleFactory.hpp"
#include "hikari/client/gui/GuiService.hpp"
#include "hikari/client/scripting/SquirrelService.hpp"
#include "hikari/client/scripting/AudioServiceScriptProxy.hpp"
#include "hikari/client/scripting/GameProgressScriptProxy.hpp"
#include "hikari/client/scripting/GamePlayStateScriptProxy.hpp"
#include "hikari/core/game/AnimationLoader.hpp"
#include "hikari/core/game/Movable.hpp"
#include "hikari/core/game/map/TilesetLoader.hpp"
#include "hikari/core/game/map/MapLoader.hpp"
#include "hikari/core/util/ImageCache.hpp"
#include "hikari/core/util/AnimationSetCache.hpp"
#include "hikari/core/util/TilesetCache.hpp"
#include <json/value.h>

namespace hikari {

    ClientRuntime::ClientRuntime(const ClientConfig & config,
            const Json::Value & gameConfig, hikari::gfx::RenderTexture & screenBuffer)
        : input(std::make_shared<KeyboardInput>())
        , events(std::make_shared<EventBusImpl>("GlobalEvents", true))
        , images(std::make_shared<ImageCache>(ImageCache::NO_SMOOTHING, ImageCache::USE_MASKING))
        , animationLoader(std::make_shared<AnimationLoader>(*images))
        , animations(std::make_shared<AnimationSetCache>(*animationLoader))
        , tilesetLoader(std::make_shared<TilesetLoader>(*images, *animationLoader))
        , tilesets(std::make_shared<TilesetCache>(*tilesetLoader))
        , maps(std::make_shared<MapLoader>(*animations, *tilesets))
        , audio(std::make_shared<AudioService>(gameConfig["assets"]["audio"]))
        , progress(std::make_shared<GameProgress>())
        , scripting(std::make_shared<SquirrelService>(config.getScriptingStackSize()))
        , gui(std::make_shared<GuiService>(gameConfig, *images, screenBuffer))
        , screenEffects(std::make_shared<ScreenEffectsService>(screenBuffer.getSize().x, screenBuffer.getSize().y))
        , items(std::make_shared<ItemFactory>())
        , enemies(std::make_shared<EnemyFactory>())
        , projectiles(std::make_shared<ProjectileFactory>())
        , particles(std::make_shared<ParticleFactory>())
        , weapons(std::make_shared<WeaponTable>())
        , damage(std::make_shared<DamageTable>())
    {
        AudioServiceScriptProxy::setWrappedService(audio);
        GameProgressScriptProxy::setWrappedService(progress);
    }

    ClientRuntime::~ClientRuntime() {
        GamePlayStateScriptProxy::setWrappedService({});
        AudioServiceScriptProxy::setWrappedService({});
        GameProgressScriptProxy::setWrappedService({});
        Movable::setCollisionResolver(nullptr);
    }

}
