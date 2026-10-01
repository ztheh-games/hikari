#ifndef HIKARI_CLIENT_CLIENTRUNTIME
#define HIKARI_CLIENT_CLIENTRUNTIME

#include "hikari/core/game/GameController.hpp"
#include "hikari/core/util/NonCopyable.hpp"
#include <memory>

namespace Json {
    class Value;
}

namespace sf {
    class RenderTexture;
}

namespace hikari {

    class ClientConfig;
    class KeyboardInput;
    class EventBus;
    class ImageCache;
    class AnimationLoader;
    class AnimationSetCache;
    class TilesetLoader;
    class TilesetCache;
    class MapLoader;
    class AudioService;
    class GameProgress;
    class SquirrelService;
    class GuiService;
    class ItemFactory;
    class EnemyFactory;
    class ProjectileFactory;
    class ParticleFactory;
    class WeaponTable;
    class DamageTable;
    class ScreenEffectsService;

    // Composition-only owner. Consumers receive individual dependencies, not this object.
    struct ClientRuntime : private NonCopyable {
        std::shared_ptr<KeyboardInput> input;
        std::shared_ptr<EventBus> events;
        std::shared_ptr<ImageCache> images;
        std::shared_ptr<AnimationLoader> animationLoader;
        std::shared_ptr<AnimationSetCache> animations;
        std::shared_ptr<TilesetLoader> tilesetLoader;
        std::shared_ptr<TilesetCache> tilesets;
        std::shared_ptr<MapLoader> maps;
        std::shared_ptr<AudioService> audio;
        std::shared_ptr<GameProgress> progress;
        std::shared_ptr<SquirrelService> scripting;
        std::shared_ptr<GuiService> gui;
        std::shared_ptr<ScreenEffectsService> screenEffects;
        // Prototypes may contain VM objects; factories must be destroyed before scripting.
        std::shared_ptr<ItemFactory> items;
        std::shared_ptr<EnemyFactory> enemies;
        std::shared_ptr<ProjectileFactory> projectiles;
        std::shared_ptr<ParticleFactory> particles;
        std::shared_ptr<WeaponTable> weapons;
        std::shared_ptr<DamageTable> damage;
        GameController controller;

        ClientRuntime(const ClientConfig & config, const Json::Value & gameConfig,
            sf::RenderTexture & screenBuffer);
        ~ClientRuntime();
    };

}

#endif
