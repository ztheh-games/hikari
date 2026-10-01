#include "catch.hpp"

#include "hikari/core/game/CollisionInfo.hpp"
#include "hikari/core/game/CollisionResolver.hpp"
#include "hikari/core/game/Movable.hpp"

#include <memory>

namespace {

    class PlatformCollisionResolver : public hikari::CollisionResolver {
    public:
        int obstacleId = 42;
        bool obstacleAvailable = true;
        bool ceilingEnabled = false;
        bool wallEnabled = false;
        hikari::BoundingBoxF obstacleBounds = hikari::BoundingBoxF(0.0f, 10.0f, 20.0f, 5.0f);
        hikari::Vector2<float> obstacleDisplacement;

        virtual void checkHorizontalEdge(
            const int& x,
            const int&,
            const int&,
            const hikari::Direction& directionX,
            hikari::CollisionInfo& collisionInfo,
            int
        ) {
            collisionInfo.isCollisionX = false;

            if(wallEnabled && directionX == hikari::Directions::Right && x > 5) {
                collisionInfo.isCollisionX = true;
                collisionInfo.directionX = directionX;
                collisionInfo.correctedX = 5;
                collisionInfo.obstacleId = -1;
            }
        }

        virtual void checkVerticalEdge(
            const int& y,
            const int& xMin,
            const int& xMax,
            const hikari::Direction& directionY,
            hikari::CollisionInfo& collisionInfo,
            int ignoredObstacleId
        ) {
            collisionInfo.isCollisionY = false;

            if(ceilingEnabled && directionY == hikari::Directions::Up && y < 0) {
                collisionInfo.isCollisionY = true;
                collisionInfo.directionY = directionY;
                collisionInfo.correctedY = 0;
                collisionInfo.obstacleId = -1;
                return;
            }

            const bool overlapsObstacle =
                static_cast<float>(xMax) >= obstacleBounds.getLeft() &&
                static_cast<float>(xMin) < obstacleBounds.getRight();
            const bool intersectsObstacle =
                static_cast<float>(y) >= obstacleBounds.getTop() &&
                static_cast<float>(y) <= obstacleBounds.getBottom();

            if(obstacleAvailable &&
                ignoredObstacleId != obstacleId &&
                directionY == hikari::Directions::Down &&
                overlapsObstacle &&
                intersectsObstacle
            ) {
                collisionInfo.isCollisionY = true;
                collisionInfo.directionY = directionY;
                collisionInfo.correctedY = static_cast<int>(obstacleBounds.getTop());
                collisionInfo.obstacleId = obstacleId;
            }
        }

        virtual bool getObstacleState(
            int requestedObstacleId,
            hikari::BoundingBoxF& bounds,
            hikari::Vector2<float>& displacement
        ) const {
            if(!obstacleAvailable || requestedObstacleId != obstacleId) {
                return false;
            }

            bounds = obstacleBounds;
            displacement = obstacleDisplacement;
            return true;
        }
    };

    void landRider(hikari::Movable& rider, const std::shared_ptr<PlatformCollisionResolver>& resolver) {
        hikari::Movable::setCollisionResolver(resolver);
        hikari::Movable::setGravity(0.0f);

        rider.setBoundingBox(hikari::BoundingBoxF(0.0f, 0.0f, 5.0f, 10.0f));
        rider.setGravitated(false);
        rider.setPosition(0.0f, 0.0f);
        rider.setVelocity(0.0f, 0.25f);
        rider.update(1.0f);

        REQUIRE(rider.getSupportObstacleId() == resolver->obstacleId);
    }

}

TEST_CASE("Movable acquires obstacle support on a downward collision") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);

    REQUIRE(rider.isOnGround());
    REQUIRE(rider.getSupportObstacleId() == resolver->obstacleId);
}

TEST_CASE("Movable follows the supporting obstacle's actual frame displacement") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);

    resolver->obstacleDisplacement = hikari::Vector2<float>(2.0f, -2.0f);
    resolver->obstacleBounds.setPosition(2.0f, 8.0f);
    const auto previousPosition = rider.getPosition();

    rider.update(1.0f);

    REQUIRE(rider.getPosition().getX() == Approx(previousPosition.getX() + 2.0f));
    REQUIRE(rider.getPosition().getY() == Approx(previousPosition.getY() - 2.0f));
    REQUIRE(rider.getFrameDisplacement().getX() == Approx(2.0f));
    REQUIRE(rider.getFrameDisplacement().getY() == Approx(-2.0f));
    REQUIRE(rider.getSupportObstacleId() == resolver->obstacleId);
}

TEST_CASE("Movable detaches immediately when jumping") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);

    resolver->obstacleDisplacement = hikari::Vector2<float>(3.0f, 0.0f);
    resolver->obstacleBounds.setPosition(3.0f, 10.0f);
    rider.setVelocity(0.0f, -2.0f);
    const float previousX = rider.getPosition().getX();

    rider.update(1.0f);

    REQUIRE(rider.getPosition().getX() == Approx(previousX));
    REQUIRE(rider.getSupportObstacleId() == -1);
}

TEST_CASE("Movable does not retain platform motion after walking off") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);

    resolver->obstacleDisplacement = hikari::Vector2<float>(2.0f, 0.0f);
    resolver->obstacleBounds.setPosition(2.0f, 10.0f);
    rider.setVelocity(30.0f, 0.25f);
    rider.update(1.0f);

    REQUIRE(rider.getSupportObstacleId() == -1);
    const float detachedX = rider.getPosition().getX();

    resolver->obstacleBounds.setPosition(4.0f, 10.0f);
    rider.setVelocity(0.0f, 0.25f);
    rider.update(1.0f);

    REQUIRE(rider.getPosition().getX() == Approx(detachedX));
}

TEST_CASE("Movable clears support when the obstacle is unavailable") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);

    resolver->obstacleAvailable = false;
    const auto previousPosition = rider.getPosition();
    rider.update(1.0f);

    REQUIRE(rider.getSupportObstacleId() == -1);
    REQUIRE(rider.getPosition().getX() == Approx(previousPosition.getX()));
}

TEST_CASE("Movable reports a crush when support motion is blocked") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);
    int crushCount = 0;

    rider.setCrushCallback(
        [&crushCount](hikari::Movable&, hikari::CollisionInfo&) {
            ++crushCount;
        }
    );

    resolver->ceilingEnabled = true;
    resolver->obstacleDisplacement = hikari::Vector2<float>(0.0f, -2.0f);
    resolver->obstacleBounds.setPosition(0.0f, 8.0f);
    rider.update(1.0f);

    REQUIRE(crushCount == 1);
    REQUIRE(rider.getSupportObstacleId() == -1);
}

TEST_CASE("Movable reports a crush when horizontal support motion is blocked") {
    auto resolver = std::make_shared<PlatformCollisionResolver>();
    hikari::Movable rider;
    landRider(rider, resolver);
    int crushCount = 0;

    rider.setCrushCallback(
        [&crushCount](hikari::Movable&, hikari::CollisionInfo&) {
            ++crushCount;
        }
    );

    resolver->wallEnabled = true;
    resolver->obstacleDisplacement = hikari::Vector2<float>(2.0f, 0.0f);
    resolver->obstacleBounds.setPosition(2.0f, 10.0f);
    rider.update(1.0f);

    REQUIRE(crushCount == 1);
    REQUIRE(rider.getSupportObstacleId() == -1);
}
