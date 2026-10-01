#include "catch.hpp"

#include "hikari/core/game/CollisionSelection.hpp"

TEST_CASE("Vertical obstacle selection prefers the visually highest platform when moving down") {
    const hikari::BoundingBoxF higherPlatform(16.0f, 20.0f, 16.0f, 8.0f);
    const hikari::BoundingBoxF lowerPlatform(0.0f, 22.0f, 16.0f, 8.0f);

    REQUIRE(hikari::CollisionSelection::isPreferredVerticalObstacle(
        hikari::Directions::Down,
        higherPlatform,
        20,
        lowerPlatform,
        10
    ));
    REQUIRE_FALSE(hikari::CollisionSelection::isPreferredVerticalObstacle(
        hikari::Directions::Down,
        lowerPlatform,
        10,
        higherPlatform,
        20
    ));
}

TEST_CASE("Vertical obstacle selection uses object ID as a stable tie-breaker") {
    const hikari::BoundingBoxF leftPlatform(0.0f, 20.0f, 16.0f, 8.0f);
    const hikari::BoundingBoxF rightPlatform(16.0f, 20.0f, 16.0f, 8.0f);

    REQUIRE(hikari::CollisionSelection::isPreferredVerticalObstacle(
        hikari::Directions::Down,
        leftPlatform,
        10,
        rightPlatform,
        20
    ));
    REQUIRE_FALSE(hikari::CollisionSelection::isPreferredVerticalObstacle(
        hikari::Directions::Down,
        rightPlatform,
        20,
        leftPlatform,
        10
    ));
}
