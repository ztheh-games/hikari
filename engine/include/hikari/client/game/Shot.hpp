#ifndef HIKARI_CLIENT_GAME_SHOT
#define HIKARI_CLIENT_GAME_SHOT

#include <memory>
#include <vector>

namespace hikari {

    class GameObject;

    class Shot {
    private:
        std::vector<std::weak_ptr<GameObject>> trackedObjects;

    public:
        explicit Shot(std::vector<std::weak_ptr<GameObject>> trackedObjects);

        /**
         * Returns whether the shot is active or not.
         * 
         * @return true if the shot is active, false otherwise
         */
        bool isActive() const;
    };
} // hikari

#endif // HIKARI_CLIENT_GAME_SHOT