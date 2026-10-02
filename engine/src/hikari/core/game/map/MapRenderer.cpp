#include "hikari/core/graphics/Graphics.hpp"
#include "hikari/core/game/map/MapRenderer.hpp"
#include "hikari/core/game/map/Door.hpp"
#include "hikari/core/game/map/Force.hpp"
#include "hikari/core/game/map/Room.hpp"
#include "hikari/core/game/map/Tileset.hpp"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include <iostream>
#endif

namespace hikari {

    bool MapRenderer::isDebugLadderRenderingEnabled = false;
    bool MapRenderer::isDebugDoorRenderingEnabled = false;
    bool MapRenderer::isDebugForceRenderingEnabled = false;
    const int MapRenderer::TILE_OVERDRAW = 1;

    MapRenderer::MapRenderer(const RoomPtr &room, const TileDataPtr &tileData)
        : room(room)
        , tileData(tileData)
        , tileSprite()
        , backgroundShape()
        , debugLadderShape()
        , debugForceShape()
        , visibleScreenArea()
        , visibleTileArea()
    {
        if(tileData) {
            tileSprite.setTexture(*(tileData->getTexture()));
        }

        debugLadderShape.setFillColor(hikari::gfx::Color(128, 128, 0, 96));
        debugLadderShape.setOutlineColor(hikari::gfx::Color(255, 255, 255, 128));
        debugLadderShape.setOutlineThickness(1.0f);

        debugForceShape.setFillColor(hikari::gfx::Color(128, 0, 0, 96));
        debugForceShape.setOutlineColor(hikari::gfx::Color(255, 255, 255, 128));
        debugForceShape.setOutlineThickness(1.0f);
    }

    void MapRenderer::setRoom(const RoomPtr &room) {
        if(this->room != room) {
            this->room = room;
            buildBackgroundRectangle();
            cullTiles();
        }
    }

    const RoomPtr & MapRenderer::getRoom() const {
        return room;
    }

    void MapRenderer::setTileData(const TileDataPtr &tileData) {
        this->tileData = tileData;
        tileSprite.setTexture(*(tileData->getTexture()));
        cullTiles();
    }

    const TileDataPtr & MapRenderer::getTileData() const {
        return tileData;
    }

    void MapRenderer::renderBackground(hikari::gfx::RenderTarget &target) {
        target.draw(backgroundShape);
    }

    void MapRenderer::renderForeground(hikari::gfx::RenderTarget &target) {
        int tileIndex = Room::NO_TILE;
        int tileAttributes = TileAttribute::NO_ATTRIBUTES;

        for(int y = visibleTileArea.getY(); y < visibleTileArea.getHeight(); ++y) {
            for(int x = visibleTileArea.getX(); x < visibleTileArea.getWidth(); ++x) {
                tileIndex = room->getTileAt(x, y);
                tileAttributes = room->getAttributeAt(x, y);

                if(tileIndex == Room::NO_TILE) {
                    continue;
                }

                applyTileToSprite(tileIndex, x, y);
                applyTileAttributes(tileAttributes);

                // TODO: Not sure about this... it may make the jittering MORE obvious...?
                const auto & unQuantizedPosition = tileSprite.getPosition();
                tileSprite.setPosition({std::ceil(unQuantizedPosition.x), std::ceil(unQuantizedPosition.y)});

                target.draw(tileSprite);
            }
        }

        // Render ladder outlines if debug rendering is enabled
        if(isDebugLadderRenderingEnabled) {
            std::for_each(std::begin(room->getLadders()), std::end(room->getLadders()), [this, &target](const BoundingBox<float> & ladder) {
                debugLadderShape.setPosition(hikari::gfx::Vector2f(ladder.getLeft(), ladder.getTop()));
                debugLadderShape.setSize(hikari::gfx::Vector2f(ladder.getWidth(), ladder.getHeight()));

                target.draw(debugLadderShape);
            });
        }

        // Render force outlines if debug rendering is enabled
        if(isDebugForceRenderingEnabled) {
            std::for_each(std::begin(room->getForces()), std::end(room->getForces()), [this, &target](const std::shared_ptr<Force> & force) {
                const auto & bounds = force->getBounds();
                debugForceShape.setPosition(hikari::gfx::Vector2f(bounds.getLeft(), bounds.getTop()));
                debugForceShape.setSize(hikari::gfx::Vector2f(bounds.getWidth(), bounds.getHeight()));

                target.draw(debugForceShape);
            });
        }

        const auto & entranceDoor = room->getEntranceDoor();
        const auto & exitDoor = room->getExitDoor();

        if(entranceDoor || exitDoor) {
            if(entranceDoor) {
                entranceDoor->render(target);
            }

            if(exitDoor) {
                exitDoor->render(target);
            }

            if(isDebugDoorRenderingEnabled) {
                hikari::gfx::RectangleShape doorRect;
                doorRect.setOutlineColor(hikari::gfx::Color(255, 255, 255, 192));
                doorRect.setOutlineThickness(1.0f);

                if(entranceDoor) {
                    doorRect.setFillColor(hikari::gfx::Color(0, 192, 32, 96));
                    doorRect.setPosition(
                        hikari::gfx::Vector2f(entranceDoor->getX() * 16.0f, entranceDoor->getY() * 16.0f)
                    );
                    doorRect.setSize(
                        hikari::gfx::Vector2f(entranceDoor->getWidth() * 16.0f, entranceDoor->getHeight() * 16.0f)
                    );

                    target.draw(doorRect);
                }

                if(exitDoor) {
                    doorRect.setFillColor(hikari::gfx::Color(192, 0, 32, 96));
                    doorRect.setPosition(
                        hikari::gfx::Vector2f(exitDoor->getX() * 16.0f, exitDoor->getY() * 16.0f)
                    );
                    doorRect.setSize(
                        hikari::gfx::Vector2f(exitDoor->getWidth() * 16.0f, exitDoor->getHeight() * 16.0f)
                    );

                    target.draw(doorRect);
                }
            }
        }
    }

    void MapRenderer::render(hikari::gfx::RenderTarget &target) {
        renderBackground(target);
        renderForeground(target);
    }

    inline void MapRenderer::buildBackgroundRectangle() {
        if(room) {
            const auto & widthInTiles = room->getWidth();
            const auto & heightInTiles = room->getHeight();
            const auto & gridSize = room->getGridSize();
            const auto & bgColor = room->getBackgroundColor();

            backgroundShape.setSize(
                hikari::gfx::Vector2f(
                    widthInTiles * gridSize,
                    heightInTiles * gridSize
                )
            );

            backgroundShape.setPosition(
                hikari::gfx::Vector2f(room->getX() * gridSize, room->getY() * gridSize)
            );

            backgroundShape.setFillColor(
                hikari::gfx::Color(
                    ((bgColor >> 16) & 0xFF), // Extract Red
                    ((bgColor >>  8) & 0xFF), // Extract Green
                    ( bgColor        & 0xFF), // Extract Blue
                    255
                )
            );
        }
    }

    inline void MapRenderer::cullTiles() {
        if(tileData != nullptr && room != nullptr) {
            const int tileSize = tileData->getTileSize();

            int x = visibleScreenArea.getX() / tileSize;
            int width = visibleScreenArea.getWidth() / tileSize;
            int y = visibleScreenArea.getY() / tileSize;
            int height = visibleScreenArea.getHeight() / tileSize;

            // Find minimum and maximum tile rendering ranges
            x = std::max(x, room->getX());
            width = std::min(x + width + TILE_OVERDRAW, room->getWidth() + room->getX());
            y = std::max(y, room->getY());
            height = std::min(y + height + TILE_OVERDRAW, room->getHeight() + room->getY());

            visibleTileArea.setX(x);
            visibleTileArea.setWidth(width);
            visibleTileArea.setY(y);
            visibleTileArea.setHeight(height);
        }
    }

    void MapRenderer::setCullRegion(const Rectangle2D<int> &cullRegion) {
        visibleScreenArea = cullRegion;
        cullTiles();
    }

    inline void MapRenderer::applyTileToSprite(const int &tileIndex, const int &x, const int &y) {
        const hikari::gfx::IntRect& rect = tileData->getTileRect(tileIndex);

        tileSprite.setTextureRect(rect);
        tileSprite.setPosition({
            static_cast<float>(x * tileData->getTileSize()),
            static_cast<float>(y * tileData->getTileSize())
        });
    }

    inline void MapRenderer::applyTileAttributes(const int &attributes) {
        //tileSprite.FlipX(false);
        //tileSprite.FlipY(false);
        tileSprite.setRotation(hikari::gfx::degrees(0.0f));
        tileSprite.setOrigin({0.0f, 0.0f});

        if((attributes & TileAttribute::FLIP_HORIZONTAL) == TileAttribute::FLIP_HORIZONTAL) {
            //tileSprite.FlipX(true);
        }

        if((attributes & TileAttribute::FLIP_VERTICAL) == TileAttribute::FLIP_VERTICAL) {
            //tileSprite.FlipY(true);
        }

        if((attributes & TileAttribute::ROTATE_BY_90) == TileAttribute::ROTATE_BY_90) {
            float tileRotationOffset = static_cast<float>(tileData->getTileSize() * 0.5f);
            tileSprite.setOrigin({tileRotationOffset, tileRotationOffset});
            tileSprite.setRotation(hikari::gfx::degrees(90.0f));
            tileSprite.move({tileRotationOffset, tileRotationOffset});
        }
    }

} // hikari
