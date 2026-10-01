#ifndef HIKARI_CORE_GAME_COLLISIONSELECTION
#define HIKARI_CORE_GAME_COLLISIONSELECTION

#include "hikari/core/game/Direction.hpp"
#include "hikari/core/geom/BoundingBox.hpp"

namespace hikari {
namespace CollisionSelection {

    inline bool isPreferredVerticalObstacle(
        const Direction& direction,
        const BoundingBoxF& candidateBounds,
        int candidateId,
        const BoundingBoxF& currentBounds,
        int currentId
    ) {
        if(direction == Directions::Down) {
            if(candidateBounds.getTop() != currentBounds.getTop()) {
                return candidateBounds.getTop() < currentBounds.getTop();
            }
        } else if(direction == Directions::Up) {
            if(candidateBounds.getBottom() != currentBounds.getBottom()) {
                return candidateBounds.getBottom() > currentBounds.getBottom();
            }
        }

        return candidateId < currentId;
    }

} // CollisionSelection
} // hikari

#endif // HIKARI_CORE_GAME_COLLISIONSELECTION
