#ifndef HIKARI_CLIENT_GAME_OBJECT_PROJECTILEFACTORY
#define HIKARI_CLIENT_GAME_OBJECT_PROJECTILEFACTORY

#include <memory>
#include <string>
#include <unordered_map>


namespace hikari {
    class Projectile;

    class ProjectileFactory {
    private:
        //
        // Fields
        //
        std::unordered_map<std::string, std::shared_ptr<Projectile>> prototypeRegistry;

    public:
        //
        // Constructor
        //
        ProjectileFactory();
        virtual ~ProjectileFactory();

        //
        // Methods
        //
        std::unique_ptr<Projectile> create(const std::string& enemyType);

        void registerPrototype(const std::string & prototypeName, const std::shared_ptr<Projectile> & instancee);
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_OBJECT_PROJECTILEFACTORY