#include "hikari/core/game/Movable.hpp"
#include "hikari/core/game/CollisionResolver.hpp"
#include "hikari/core/geom/Point2D.hpp"
#include "hikari/core/util/Log.hpp"
#include "hikari/core/math/MathUtils.hpp"

namespace hikari {

    // Static
    float Movable::maxYVelocity = 7.0f;
    float Movable::maxXVelocity = 16.0f;
    float Movable::minYVelocity = -16.0f;
    float Movable::minXVelocity = -maxXVelocity;

    std::shared_ptr<CollisionResolver> Movable::collisionResolver = nullptr;
    float Movable::gravity = 0.0f;

    void Movable::setGravity(const float& gravity) {
        Movable::gravity = gravity;
    }

    const float Movable::getGravity() {
        return Movable::gravity;
    }

    Movable::Movable()
        : gravityApplicationCounter(0u)
        , gravityApplicationThreshold(1u)
        , onGroundNow(false)
        , onGroundLastFrame(false)
        , topBlockedFlag(false)
        , rightBlockedFlag(false)
        , bottomBlockedFlag(false)
        , leftBlockedFlag(false)
        , affectedByGravity(true)
        , collidesWithWorld(true)
        , treatPlatformAsGround(true)
        , applyHorizontalVelocity(true)
        , applyVerticalVelocity(true)
        , supportObstacleId(-1)
        , ambientVelocity(0.0f, 0.0f)
        , velocity(0.0f, 0.0f)
        , frameDisplacement(0.0f, 0.0f)
        , boundingBox(0.0f, 0.0f, 0.0f, 0.0f)
        , collisionInfo()
        , landingCallback()
        , collisionCallback()
        , crushCallback()
    {

    }

    Movable::Movable(float width, float height)
        : gravityApplicationCounter(0u)
        , gravityApplicationThreshold(1u)
        , onGroundNow(false)
        , onGroundLastFrame(false)
        , topBlockedFlag(false)
        , rightBlockedFlag(false)
        , bottomBlockedFlag(false)
        , leftBlockedFlag(false)
        , affectedByGravity(true)
        , collidesWithWorld(true)
        , treatPlatformAsGround(true)
        , applyHorizontalVelocity(true)
        , applyVerticalVelocity(true)
        , supportObstacleId(-1)
        , ambientVelocity(0.0f, 0.0f)
        , velocity(0.0f, 0.0f)
        , frameDisplacement(0.0f, 0.0f)
        , boundingBox(0.0f, 0.0f, width, height)
        , collisionInfo()
        , landingCallback()
        , collisionCallback()
        , crushCallback()
    {

    }

    Movable::Movable(const Movable& proto)
        : gravityApplicationCounter(proto.gravityApplicationCounter)
        , gravityApplicationThreshold(proto.gravityApplicationThreshold)
        , onGroundNow(proto.onGroundNow)
        , onGroundLastFrame(proto.onGroundLastFrame)
        , topBlockedFlag(proto.topBlockedFlag)
        , rightBlockedFlag(proto.rightBlockedFlag)
        , bottomBlockedFlag(proto.bottomBlockedFlag)
        , leftBlockedFlag(proto.leftBlockedFlag)
        , affectedByGravity(proto.affectedByGravity)
        , collidesWithWorld(proto.collidesWithWorld)
        , treatPlatformAsGround(proto.treatPlatformAsGround)
        , applyHorizontalVelocity(proto.applyHorizontalVelocity)
        , applyVerticalVelocity(proto.applyVerticalVelocity)
        , supportObstacleId(-1)
        , ambientVelocity(proto.ambientVelocity)
        , velocity(proto.velocity)
        , frameDisplacement(0.0f, 0.0f)
        , boundingBox(proto.boundingBox)
        , collisionInfo(proto.collisionInfo)
        , landingCallback(proto.landingCallback)        // TODO: Cloning the callbacks is almost 100% wrong
        , collisionCallback(proto.collisionCallback)    // You should create new ones instead
        , crushCallback(proto.crushCallback)
    {

    }

    Movable::~Movable() {

    }

    const std::shared_ptr<CollisionResolver> & Movable::getCollisionResolver() {
        return collisionResolver;
    }

    void Movable::setCollisionResolver(std::shared_ptr<CollisionResolver> resolver) {
        collisionResolver = resolver;
    }

    bool Movable::isOnGround() const {
        return isOnGroundNow();
    }

    bool Movable::isTopBlocked() const {
        return topBlockedFlag;
    }

    bool Movable::isRightBlocked() const {
        return rightBlockedFlag;
    }

    bool Movable::isBottomBlocked() const {
        return bottomBlockedFlag;
    }

    bool Movable::isLeftBlocked() const {
        return leftBlockedFlag;
    }

    const Vector2<float>& Movable::getPosition() const {
        return boundingBox.getPosition();
    }

    const Vector2<float>& Movable::getAmbientVelocity() const {
        return ambientVelocity;
    }

    const Vector2<float>& Movable::getVelocity() const {
        return velocity;
    }

    const Vector2<float>& Movable::getFrameDisplacement() const {
        return frameDisplacement;
    }

    const BoundingBoxF& Movable::getBoundingBox() const {
        return boundingBox;
    }

    int Movable::getSupportObstacleId() const {
        return supportObstacleId;
    }

    const bool& Movable::isOnGroundNow() const {
        return onGroundNow;
    }

    const bool& Movable::wasOnGroundLastFrame() const {
        return onGroundLastFrame;
    }

    const bool& Movable::isGravitated() const {
        return affectedByGravity;
    }

    const bool& Movable::doesCollideWithWorld() const {
        return collidesWithWorld;
    }

    void Movable::setOnGround(const bool & bypassCallback = false) {
        onGroundNow = true;

        if(bypassCallback) {
            onGroundLastFrame = true;
        }
    }

    void Movable::setGravitated(const bool& isAffected) {
        affectedByGravity = isAffected;
    }

    void Movable::setHasWorldCollision(const bool& hasCollision) {
        collidesWithWorld = hasCollision;
    }

    void Movable::preCheckCollision() {
        onGroundLastFrame = onGroundNow;
        onGroundNow = false;
        topBlockedFlag = false;
        rightBlockedFlag = false;
        bottomBlockedFlag = false;
        leftBlockedFlag = false;
    }

    Vector2<float> Movable::checkCollision(const float& dt) {
        Vector2<float> translation = getVelocity() + getAmbientVelocity();

        collisionInfo.clear();
        collisionInfo.treatPlatformAsGround = this->treatPlatformAsGround;

        preCheckCollision();

        // Check horizontal directions first
        if(translation.getX() < 0) {
            // Moving left

            // We subtract 1 here because getBottom() represents the first pixel outside of the bounding box.
            getCollisionResolver()->checkHorizontalEdge(
                static_cast<int>(boundingBox.getLeft() + translation.getX()/* * dt */),
                static_cast<int>(boundingBox.getTop()),
                static_cast<int>(boundingBox.getBottom() - 1),
                Directions::Left,
                collisionInfo
            );

            if(collisionInfo.isCollisionX) {
                boundingBox.setLeft(static_cast<float>(collisionInfo.correctedX));
                velocity.setX(0.0f);
                translation.setX(0.0f);
                leftBlockedFlag = true;

                if(collisionCallback) {
                    collisionCallback(*this, collisionInfo);
                }
            }

        } else if(translation.getX() > 0) {
            // Moving right

            // We subtract 1 here because getBottom() represents the first pixel outside of the bounding box.
            getCollisionResolver()->checkHorizontalEdge(
                static_cast<int>(boundingBox.getRight() + translation.getX()/* * dt */),
                static_cast<int>(boundingBox.getTop()),
                static_cast<int>(boundingBox.getBottom() - 1),
                Directions::Right,
                collisionInfo
            );

            if(collisionInfo.isCollisionX) {
                boundingBox.setRight(static_cast<float>(collisionInfo.correctedX));
                velocity.setX(0.0f);
                translation.setX(0.0f);
                rightBlockedFlag = true;

                if(collisionCallback) {
                    collisionCallback(*this, collisionInfo);
                }
            }
        }

        // Check vertical directions second
        if(translation.getY() < 0) {
            // Moving up

            // We subtract 1 here because getRight() represents the first pixel outside of the bounding box.
            getCollisionResolver()->checkVerticalEdge(
                static_cast<int>(boundingBox.getTop() + translation.getY()/* * dt */),
                static_cast<int>(boundingBox.getLeft()),
                static_cast<int>(boundingBox.getRight() - 1),
                Directions::Up,
                collisionInfo
            );

            if(collisionInfo.isCollisionY) {
                boundingBox.setTop(static_cast<float>(collisionInfo.correctedY));
                velocity.setY(0.0f);
                translation.setY(0.0f);
                topBlockedFlag = true;

                if(collisionCallback) {
                    collisionCallback(*this, collisionInfo);
                }
            }

        } else if(translation.getY() > 0) {
            // Moving down

            // We subtract 1 here because getRight() represents the first pixel outside of the bounding box.
            getCollisionResolver()->checkVerticalEdge(
                static_cast<int>(std::ceil(boundingBox.getBottom() + translation.getY()/* * dt */)),
                static_cast<int>(boundingBox.getLeft()),
                static_cast<int>(boundingBox.getRight() - 1),
                Directions::Down,
                collisionInfo
            );

            if(collisionInfo.isCollisionY) {
                boundingBox.setBottom(static_cast<float>(collisionInfo.correctedY));
                velocity.setY(velocity.getY() - std::floor(velocity.getY()));
                translation.setY(velocity.getY());
                onGroundNow = true;
                bottomBlockedFlag = true;
                supportObstacleId = collisionInfo.obstacleId;

                if(collisionCallback) {
                    collisionCallback(*this, collisionInfo);
                }

                if(isGravitated() && !wasOnGroundLastFrame()) {
                    onLanding();
                }
            }
        }

        postCheckCollision();

        return translation;
    }

    void Movable::postCheckCollision() {

    }

    void Movable::onLanding() {
        if(landingCallback) {
            landingCallback(*this, collisionInfo);
        }
    }

    void Movable::setPosition(const Vector2<float>& position) {
        boundingBox.setPosition(position);
    }

    void Movable::setPosition(const float& x, const float& y) {
        boundingBox.setPosition(x, y);
    }

    void Movable::setBottom(float newBottom) {
        boundingBox.setBottom(newBottom);
    }

    void Movable::setAmbientVelocity(const Vector2<float>& velocity) {
        ambientVelocity = velocity;
    }

    void Movable::setVelocity(const Vector2<float>& velocity) {
        this->velocity = velocity;
    }

    void Movable::setVelocity(const float& x, const float& y) {
        this->velocity.setX(x).setY(y);
    }

    void Movable::setBoundingBox(const BoundingBoxF& boundingBox) {
        this->boundingBox = boundingBox;
    }

    void Movable::setTreatPlatformAsGround(const bool& treatAsGround) {
        this->treatPlatformAsGround = treatAsGround;
    }

    void Movable::setApplyHorizontalVelocity(const bool& applyVelocity) {
        this->applyHorizontalVelocity = applyVelocity;
    }

    void Movable::setApplyVerticalVelocity(const bool& applyVelocity) {
        this->applyVerticalVelocity = applyVelocity;
    }

    void Movable::move(const Vector2<float>& position) {
        setPosition(getPosition() + position);
    }

    void Movable::move(const float& x, const float& y) {
        const Vector2<float>& position = getPosition();
        setPosition(position.getX() + x, position.getY() + y);
    }

    void Movable::setLandingCallback(const CollisionCallback& callback) {
        this->landingCallback = callback;
    }

    void Movable::setCollisionCallback(const CollisionCallback& callback) {
        this->collisionCallback = callback;
    }

    void Movable::setCrushCallback(const CollisionCallback& callback) {
        this->crushCallback = callback;
    }

    void Movable::clearSupport() {
        supportObstacleId = -1;
    }

    bool Movable::applySupportDisplacement() {
        if(supportObstacleId < 0 || velocity.getY() < 0.0f || !getCollisionResolver()) {
            clearSupport();
            return true;
        }

        BoundingBoxF supportBounds(0.0f, 0.0f, 0.0f, 0.0f);
        Vector2<float> supportDisplacement;

        if(!getCollisionResolver()->getObstacleState(supportObstacleId, supportBounds, supportDisplacement)) {
            clearSupport();
            return true;
        }

        BoundingBoxF previousSupportBounds(supportBounds);
        previousSupportBounds.setPosition(supportBounds.getPosition() - supportDisplacement);

        const bool overlapsPreviousSupport =
            boundingBox.getRight() > previousSupportBounds.getLeft() &&
            boundingBox.getLeft() < previousSupportBounds.getRight();
        const bool wasOnTop =
            std::abs(boundingBox.getBottom() - previousSupportBounds.getTop()) <= 1.0f;

        if(!overlapsPreviousSupport || !wasOnTop) {
            clearSupport();
            return true;
        }

        CollisionInfo carrierCollision;
        carrierCollision.treatPlatformAsGround = false;

        const float dx = supportDisplacement.getX();
        if(dx != 0.0f) {
            const Direction direction = dx < 0.0f ? Directions::Left : Directions::Right;
            const float edge = dx < 0.0f ? boundingBox.getLeft() : boundingBox.getRight();

            getCollisionResolver()->checkHorizontalEdge(
                static_cast<int>(edge + dx),
                static_cast<int>(boundingBox.getTop()),
                static_cast<int>(boundingBox.getBottom() - 1.0f),
                direction,
                carrierCollision,
                supportObstacleId
            );

            if(carrierCollision.isCollisionX) {
                if(direction == Directions::Left) {
                    boundingBox.setLeft(static_cast<float>(carrierCollision.correctedX));
                } else {
                    boundingBox.setRight(static_cast<float>(carrierCollision.correctedX));
                }

                clearSupport();
                if(crushCallback) {
                    crushCallback(*this, carrierCollision);
                }
                return false;
            }

            boundingBox.setPosition(boundingBox.getPosition().getX() + dx, boundingBox.getPosition().getY());
        }

        const float dy = supportDisplacement.getY();
        if(dy != 0.0f) {
            const Direction direction = dy < 0.0f ? Directions::Up : Directions::Down;
            const float edge = dy < 0.0f ? boundingBox.getTop() : boundingBox.getBottom();
            const int destinationEdge = direction == Directions::Down
                ? static_cast<int>(std::ceil(edge + dy))
                : static_cast<int>(edge + dy);

            carrierCollision.clear();
            carrierCollision.treatPlatformAsGround = false;
            getCollisionResolver()->checkVerticalEdge(
                destinationEdge,
                static_cast<int>(boundingBox.getLeft()),
                static_cast<int>(boundingBox.getRight() - 1.0f),
                direction,
                carrierCollision,
                supportObstacleId
            );

            if(carrierCollision.isCollisionY) {
                if(direction == Directions::Up) {
                    boundingBox.setTop(static_cast<float>(carrierCollision.correctedY));
                } else {
                    boundingBox.setBottom(static_cast<float>(carrierCollision.correctedY));
                }

                clearSupport();
                if(crushCallback) {
                    crushCallback(*this, carrierCollision);
                }
                return false;
            }

            boundingBox.setPosition(boundingBox.getPosition().getX(), boundingBox.getPosition().getY() + dy);
        }

        return true;
    }

    void Movable::update(float dt) {
        const Vector2<float> startingPosition = getPosition();

        if(!applySupportDisplacement()) {
            frameDisplacement = getPosition() - startingPosition;
            return;
        }

        clearSupport();
        Vector2<float> translation;

        if(isGravitated()) {
            if(isOnGround()) {
                velocity.setY(velocity.getY() + getGravity());
                gravityApplicationCounter = 0;
            } else {
                gravityApplicationCounter++;

                if(gravityApplicationCounter >= getGravityApplicationThreshold()) {
                    velocity.setY(velocity.getY() + getGravity());
                    gravityApplicationCounter = 0;
                }
            }
        }

        velocity.setY(math::clamp(velocity.getY(), minYVelocity, maxYVelocity));

        if(doesCollideWithWorld()) {
            translation = checkCollision(dt);
        } else {
            translation = velocity + getAmbientVelocity();
        }

        // Integrate the position
        setPosition(
            getPosition().getX() + (applyHorizontalVelocity ? translation.getX() : 0.0f)/* * dt */,
            getPosition().getY() + (applyVerticalVelocity   ? translation.getY() : 0.0f)/* * dt */);

        if(supportObstacleId >= 0) {
            BoundingBoxF supportBounds(0.0f, 0.0f, 0.0f, 0.0f);
            Vector2<float> supportDisplacement;
            const bool supportIsAvailable =
                getCollisionResolver() &&
                getCollisionResolver()->getObstacleState(supportObstacleId, supportBounds, supportDisplacement);
            const bool overlapsSupport =
                supportIsAvailable &&
                boundingBox.getRight() > supportBounds.getLeft() &&
                boundingBox.getLeft() < supportBounds.getRight();

            if(!overlapsSupport) {
                clearSupport();
                onGroundNow = false;
                bottomBlockedFlag = false;
            }
        }

        frameDisplacement = getPosition() - startingPosition;
    }

    unsigned int Movable::getGravityApplicationThreshold() const {
        return gravityApplicationThreshold;
    }

    void Movable::setGravityApplicationThreshold(unsigned int threshold) {
        gravityApplicationThreshold = threshold;
    }

} // hikari
