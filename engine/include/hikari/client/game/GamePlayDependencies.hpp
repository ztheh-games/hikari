#ifndef HIKARI_CLIENT_GAME_GAMEPLAYDEPENDENCIES
#define HIKARI_CLIENT_GAME_GAMEPLAYDEPENDENCIES

namespace hikari {

    class GameConfig;
    class AudioService;
    class GuiService;
    class WeaponTable;
    class DamageTable;
    class GameProgress;
    class ScreenEffectsService;
    class MapLoader;
    class AnimationSetCache;
    class ItemFactory;
    class EnemyFactory;
    class ParticleFactory;
    class ProjectileFactory;

    struct GamePlayDependencies {
        const GameConfig & config;
        AudioService & audio;
        GuiService & gui;
        WeaponTable & weapons;
        DamageTable & damage;
        GameProgress & progress;
        ScreenEffectsService & screenEffects;
        MapLoader & maps;
        AnimationSetCache & animations;
        ItemFactory & items;
        EnemyFactory & enemies;
        ParticleFactory & particles;
        ProjectileFactory & projectiles;
    };

}

#endif
