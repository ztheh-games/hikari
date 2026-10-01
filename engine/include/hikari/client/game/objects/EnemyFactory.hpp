#ifndef HIKARI_CLIENT_GAME_OBJECT_ENEMYFACTORY
#define HIKARI_CLIENT_GAME_OBJECT_ENEMYFACTORY

#include <memory>
#include <string>
#include <unordered_map>


namespace hikari {
    class Enemy;

    class EnemyFactory {
    private:
        //
        // Fields
        //
        std::unordered_map<std::string, std::shared_ptr<Enemy>> prototypeRegistry;

    public:
        //
        // Constructor
        //
        EnemyFactory();
        virtual ~EnemyFactory();

        //
        // Methods
        //
        std::unique_ptr<Enemy> create(const std::string& enemyType);

        void registerPrototype(const std::string & prototypeName, const std::shared_ptr<Enemy> & instancee);
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_OBJECT_ENEMYFACTORY