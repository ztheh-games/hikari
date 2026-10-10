#ifndef HIKARI_CORE_UTIL_TILEDASSETS
#define HIKARI_CORE_UTIL_TILEDASSETS

#include "hikari/core/game/map/TileTransform.hpp"
#include "hikari/core/game/map/RoomTransition.hpp"
#include "hikari/client/game/objects/BlockSequenceDescriptor.hpp"
#include "hikari/core/math/Vector2.hpp"
#include <json/value.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hikari {

    struct TiledAnimationFrame {
        int tile;
        int milliseconds;
    };

    struct TiledTilesetData {
        std::string role;
        std::string image;
        int tileSize = 0;
        std::vector<Rectangle2D<int>> tiles;
        std::map<int, std::vector<TiledAnimationFrame>> animations;
        std::vector<int> attributes;
    };

    struct TiledSpawnerData {
        bool enemy = false;
        std::string entity;
        Point2D<int> position;
        std::string direction = "None";
        unsigned int spawnLimit = 1;
        float spawnRate = 1.0f;
        bool continuous = false;
        Json::Value config;
    };

    struct TiledForceData {
        Rectangle2D<int> bounds;
        Vector2<float> velocity;
    };

    struct TiledRoomData {
        int id = 0;
        int boundsObject = 0;
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        int backgroundColor = 0;
        std::string bossEntity = "None";
        Point2D<int> heroSpawn;
        Rectangle2D<int> cameraBounds;
        std::vector<int> tiles;
        std::vector<int> attributes;
        std::vector<TileTransform> transforms;
        std::vector<TiledSpawnerData> spawners;
        std::vector<TiledForceData> forces;
        std::vector<RoomTransition> transitions;
        std::vector<BlockSequenceDescriptor> blockSequences;
        std::optional<Rectangle2D<int>> entranceDoor;
        std::optional<Rectangle2D<int>> exitDoor;
    };

    struct TiledMapData {
        std::string tileset;
        int gridSize = 0;
        std::string musicName = "None";
        std::string bossEntity = "None";
        int starting = 0;
        int midpoint = 0;
        int bossCorridor = 0;
        int bossChamber = 0;
        std::vector<TiledRoomData> rooms;
    };

    std::string resolveTiledPath(const std::string &document, const std::string &relative);
    TiledTilesetData readTiledTileset(const std::string &fileName);
    TiledMapData readTiledMap(const std::string &fileName);

}

#endif
