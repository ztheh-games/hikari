#ifndef HIKARI_CLIENT_GAME_OBJECT_PARTICLEFACTORY
#define HIKARI_CLIENT_GAME_OBJECT_PARTICLEFACTORY

#include <memory>
#include <string>
#include <unordered_map>


namespace hikari {
    class Particle;

    class ParticleFactory {
    private:
        //
        // Fields
        //
        std::unordered_map<std::string, std::shared_ptr<Particle>> prototypeRegistry;

    public:
        //
        // Constructor
        //
        ParticleFactory();
        virtual ~ParticleFactory();

        //
        // Methods
        //
        std::unique_ptr<Particle> create(const std::string& enemyType);

        void registerPrototype(const std::string & prototypeName, const std::shared_ptr<Particle> & instancee);
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_OBJECT_PARTICLEFACTORY