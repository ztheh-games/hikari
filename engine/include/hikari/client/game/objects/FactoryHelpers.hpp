#ifndef HIKARI_CLIENT_GAME_OBJECTS_FACTORYHELPERS
#define HIKARI_CLIENT_GAME_OBJECTS_FACTORYHELPERS

#include <string>

namespace hikari {

    //
    // Forward declarations
    //
    class ImageCache;
    class AnimationSetCache;
    class SquirrelService;
    class EnemyFactory;
    class ItemFactory;
    class ProjectileFactory;
    class ParticleFactory;
    class WeaponTable;

namespace FactoryHelpers {

    /**
     * Populates an ItemFactory with prototype instances loaded from a descriptor file.
     *
     * This function loads a descriptor file and creates instances of items to 
     * be used as prototypes. These prototypes are then injected into a specified
     * factory.
     * 
     * @param descriptorFilePath the path to the descriptor file
     * @param factory            the factory to populate
     */
    void populateCollectableItemFactory(
        const std::string & descriptorFilePath,
        ItemFactory & factory,
        ImageCache & imageCache,
        AnimationSetCache & animationSetCache,
        SquirrelService & squirrel
    );

    /**
     * Populates an EnemyFactory with prototype instances loaded from a 
     * descriptor file.
     *
     * This function loads a descriptor file and creates instances of enemies
     * to be used as prototypes. These prototypes are then injected into a 
     * specified factory.
     *
     * @param descriptorFilePath the path to the descriptor file
     * @param factory            the factory to populate
     */
    void populateEnemyFactory(
        const std::string & descriptorFilePath,
        EnemyFactory & factory,
        ImageCache & imageCache,
        AnimationSetCache & animationSetCache,
        SquirrelService & squirrel
    );

    /**
     * Populates an ParticleFactory with prototype instances loaded from a 
     * descriptor file.
     *
     * This function loads a descriptor file and creates instances of 
     * particles to be used as prototypes. These prototypes are then injected
     * into a specified factory.
     *
     * @param descriptorFilePath the path to the descriptor file
     * @param factory            the factory to populate
     */
    void populateParticleFactory(
        const std::string & descriptorFilePath,
        ParticleFactory & factory,
        ImageCache & imageCache,
        AnimationSetCache & animationSetCache
    );

    /**
     * Populates an ProjectileFactory with prototype instances loaded from a 
     * descriptor file.
     *
     * This function loads a descriptor file and creates instances of 
     * projectiles to be used as prototypes. These prototypes are then injected
     * into a specified factory.
     *
     * @param descriptorFilePath the path to the descriptor file
     * @param factory            the factory to populate
     */
    void populateProjectileFactory(
        const std::string & descriptorFilePath,
        ProjectileFactory & factory,
        ImageCache & imageCache,
        AnimationSetCache & animationSetCache
    );

    /**
     * Populates a WeaponTable with instances of Weapons by parsing and 
     * instantiating the Weapon instances from a descriptor file.
     *
     * @param descriptorFilePath the path to the descriptor file
     * @param weaponTable        a weapon table to populate
     */
    void populateWeaponTable(
        const std::string & descriptorFilePath,
        WeaponTable & weaponTable
    );

} // hikari::FactoryHelpers
} // hikari

#endif // HIKARI_CLIENT_GAME_OBJECTS_FACTORYHELPERS