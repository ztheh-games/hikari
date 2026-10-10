#include "hikari/core/game/map/MapLoader.hpp"
#include "hikari/core/game/map/Map.hpp"
#include "hikari/core/game/map/Room.hpp"
#include "hikari/core/game/map/Door.hpp"
#include "hikari/core/game/map/Force.hpp"
#include "hikari/client/game/objects/EnemySpawner.hpp"
#include "hikari/client/game/objects/ItemSpawner.hpp"
#include "hikari/client/scripting/SquirrelUtils.hpp"
#include "hikari/core/util/TiledAssets.hpp"
#include "hikari/core/util/AnimationSetCache.hpp"
#include "hikari/core/util/TilesetCache.hpp"
#include <stdexcept>

namespace hikari {

    MapLoader::MapLoader(AnimationSetCache &animations, TilesetCache &tilesets)
        : animationSetCache(animations), tilesetCache(tilesets) { }

    MapLoader::~MapLoader() = default;

    MapPtr MapLoader::load(const std::string &fileName) const {
        const auto data = readTiledMap(fileName);
        const auto tileset = tilesetCache.get(data.tileset);
        std::vector<RoomPtr> rooms;
        for(const auto &source : data.rooms) {
            std::vector<std::shared_ptr<Spawner>> spawners;
            for(const auto &spawn : source.spawners) {
                std::shared_ptr<Spawner> spawner;
                if(spawn.enemy) {
                    auto enemy = std::make_shared<EnemySpawner>(spawn.entity, spawn.spawnLimit, spawn.spawnRate);
                    enemy->setContinuous(spawn.continuous);
                    if(!spawn.config.isNull())
                        enemy->setInstanceConfig(SquirrelUtils::jsonToSquirrel(Sqrat::DefaultVM::Get(), spawn.config));
                    spawner = enemy;
                } else spawner = std::make_shared<ItemSpawner>(spawn.entity);
                const auto &direction = spawn.direction;
                spawner->setDirection(direction == "Up" ? Directions::Up
                    : direction == "Right" ? Directions::Right
                    : direction == "Down" ? Directions::Down
                    : direction == "Left" ? Directions::Left : Directions::None);
                spawner->setPosition({static_cast<float>(spawn.position.getX()), static_cast<float>(spawn.position.getY())});
                spawners.push_back(std::move(spawner));
            }
            std::vector<std::shared_ptr<Force>> forces;
            for(const auto &force : source.forces) {
                forces.push_back(std::make_shared<Force>(BoundingBox<float>(
                    static_cast<float>(force.bounds.getX()), static_cast<float>(force.bounds.getY()),
                    static_cast<float>(force.bounds.getWidth()), static_cast<float>(force.bounds.getHeight())),
                    force.velocity));
            }
            const auto makeDoor = [&](const std::optional<Rectangle2D<int>> &bounds) -> std::shared_ptr<Door> {
                if(!bounds) return {};
                auto door = std::make_shared<Door>(bounds->getX(), bounds->getY(), bounds->getWidth(), bounds->getHeight());
                door->setAnimationSet(animationSetCache.get("assets/animations/door.json"));
                return door;
            };
            rooms.push_back(std::make_shared<Room>(
                source.id, source.x, source.y, source.width, source.height, data.gridSize,
                source.backgroundColor, source.heroSpawn, source.cameraBounds,
                source.tiles, source.attributes, source.transitions, spawners, forces,
                source.blockSequences, makeDoor(source.entranceDoor), makeDoor(source.exitDoor),
                source.bossEntity, source.transforms));
        }
        return std::make_shared<Map>(tileset, data.gridSize, data.musicName, data.bossEntity, rooms,
                                    data.starting, data.midpoint,
                                    static_cast<unsigned int>(data.bossCorridor),
                                    static_cast<unsigned int>(data.bossChamber));
    }

}
