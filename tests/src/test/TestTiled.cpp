#include <catch.hpp>
#include "hikari/core/util/TiledAssets.hpp"
#include "hikari/core/util/FileSystemSession.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/core/util/PhysFS.hpp"
#include "hikari/core/util/JsonUtils.hpp"
#include "hikari/core/util/ImageCache.hpp"
#include "hikari/core/util/AnimationSetCache.hpp"
#include "hikari/core/util/TilesetCache.hpp"
#include "hikari/core/game/AnimationLoader.hpp"
#include "hikari/core/game/map/MapLoader.hpp"
#include "hikari/core/game/map/Map.hpp"
#include "hikari/core/game/map/Room.hpp"
#include "hikari/core/game/map/MapRenderer.hpp"
#include "hikari/core/game/map/Tileset.hpp"
#include "hikari/core/game/map/TilesetLoader.hpp"
#include "hikari/client/scripting/SquirrelService.hpp"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {

    struct TiledFiles {
        hikari::FileSystemSession session{"tiled-tests"};
        std::filesystem::path scratch = std::filesystem::current_path() /
            ("tiled-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        TiledFiles() {
            const auto content = std::getenv("HIKARI_TEST_TILED_CONTENT");
            PhysFS::addToSearchPath(content ? content : HIKARI_TEST_CONTENT_DIR);
            PhysFS::addToSearchPath(HIKARI_TEST_FIXTURE_DIR);
            std::filesystem::create_directories(scratch / "fixtures" / "tiled");
            PhysFS::addToSearchPath(scratch.string(), false);
        }
        ~TiledFiles() {
            PhysFS::removeFromSearchPath(scratch.string());
            std::filesystem::remove_all(scratch);
        }
        std::string change(const std::string &source, const std::string &from,
                           const std::string &to, const std::string &destination = "changed.tmx") {
            auto text = hikari::FileSystem::readFileAsString("fixtures/tiled/" + source);
            const auto index = text.find(from);
            if(index == std::string::npos) throw std::runtime_error("Test mutation does not match fixture");
            text.replace(index, from.size(), to);
            std::ofstream out(scratch / "fixtures" / "tiled" / destination);
            out << text;
            out.close();
            if(!out) throw std::runtime_error("Cannot write test fixture");
            return "fixtures/tiled/" + destination;
        }
    };

    void rectangle(const hikari::Rectangle2D<int> &actual, int x, int y, int w, int h) {
        REQUIRE(actual.getX() == x);
        REQUIRE(actual.getY() == y);
        REQUIRE(actual.getWidth() == w);
        REQUIRE(actual.getHeight() == h);
    }

    struct RuntimeMaps {
        hikari::ImageCache images{hikari::ImageCache::NO_SMOOTHING, hikari::ImageCache::USE_MASKING};
        hikari::AnimationLoader animations{images};
        hikari::AnimationSetCache sets{animations};
        hikari::TilesetLoader tilesets{images};
        hikari::TilesetCache cache{tilesets};
        hikari::MapLoader loader{sets, cache};
    };

}

TEST_CASE("Native Tiled objects retain units, references and configuration", "[tiled]") {
    TiledFiles files;
    const auto map = hikari::readTiledMap("fixtures/tiled/minimal.tmx");
    REQUIRE(map.rooms.size() == 1);
    REQUIRE(map.starting == 0);
    const auto &room = map.rooms[0];
    REQUIRE(room.x == 2);
    REQUIRE(room.y == 1);
    REQUIRE(room.heroSpawn.getX() == 8);
    REQUIRE(room.heroSpawn.getY() == 9);
    rectangle(room.cameraBounds, 32, 16, 64, 32);
    REQUIRE(room.attributes[1] == 1);
    REQUIRE(room.spawners.size() == 2);
    const auto &enemy = room.spawners[0];
    REQUIRE(enemy.enemy);
    REQUIRE(enemy.position.getX() == 40);
    REQUIRE(enemy.position.getY() == 32);
    REQUIRE(enemy.spawnLimit == 2);
    REQUIRE(enemy.spawnRate == Approx(0.5));
    REQUIRE(enemy.continuous);
    REQUIRE(enemy.config["velocityX"].asDouble() == Approx(1.25));
    REQUIRE(room.transitions[0].getToRegion() == 0);
    REQUIRE(room.transitions[0].getX() == 2);
    REQUIRE(room.transitions[0].getY() == 1);
    REQUIRE(room.transitions[0].isLadderOnly());
    REQUIRE(room.exitDoor);
    rectangle(*room.exitDoor, 5, 1, 1, 2);
    rectangle(room.forces[0].bounds, 35, 20, 8, 12);
    REQUIRE(room.forces[0].velocity.getX() == Approx(-0.875));
    const auto &sequence = room.blockSequences[0];
    rectangle(sequence.getBounds(), 32, 16, 64, 32);
    REQUIRE(sequence.getBlockPositions()[1].getX() == 48);
    REQUIRE(sequence.getBlockPositions()[1].getY() == 32);
    REQUIRE((sequence.getTiming()[2].getBlockIndicies() == std::vector<int>{0, 1}));
}

TEST_CASE("GID flags implement all eight Tiled transforms and reset sprite state", "[tiled]") {
    TiledFiles files;
    const auto data = hikari::readTiledMap("fixtures/tiled/minimal.tmx");
    hikari::SquirrelService scripting;
    RuntimeMaps resources;
    const auto map = resources.loader.load("fixtures/tiled/minimal.tmx");
    const auto room = map->getRoom(0);
    hikari::MapRenderer renderer(room, map->getTileset());
    renderer.setRoom(room);
    renderer.setCullRegion({32, 16, 64, 32});
    hikari::gfx::RenderTarget target({128, 64});
    renderer.renderForeground(target);
    REQUIRE(target.getDraws().size() >= 8);
    for(int flags = 0; flags < 8; ++flags) {
        REQUIRE(static_cast<int>(data.rooms[0].transforms[flags]) == flags);
        const int x = 32 + (flags % 4) * 16;
        const int y = 16 + (flags / 4) * 16;
        const auto &draw = target.getDraws()[flags];
        const int corners[4][2] = {{0, 0}, {16, 0}, {16, 16}, {0, 16}};
        for(int i = 0; i < 4; ++i) {
            int cx = corners[i][0], cy = corners[i][1];
            if(flags & 4) std::swap(cx, cy);
            if(flags & 1) cx = 16 - cx;
            if(flags & 2) cy = 16 - cy;
            REQUIRE(std::abs(draw.vertices[i].x - (x + cx)) < 0.0001);
            REQUIRE(std::abs(draw.vertices[i].y - (y + cy)) < 0.0001);
        }
    }
    // Render identity after the final transformed cell using the same renderer.
    target.clear(hikari::gfx::Color::Black);
    renderer.setCullRegion({32, 16, 1, 1});
    renderer.renderForeground(target);
    REQUIRE(target.getDraws()[0].vertices[0].x == Approx(32));
    REQUIRE(target.getDraws()[0].vertices[0].y == Approx(16));
}

TEST_CASE("Empty GIDs and the unused hex flag never alias the first tile", "[tiled]") {
    TiledFiles files;
    const auto path = files.change("minimal.tmx", ">1,2147483649,", ">0,268435457,");
    const auto map = hikari::readTiledMap(path);
    REQUIRE(map.rooms[0].tiles[0] == hikari::Room::NO_TILE);
    REQUIRE(map.rooms[0].tiles[1] == 0);
    REQUIRE(map.rooms[0].transforms[1] == hikari::TileTransform::None);
}

TEST_CASE("Native Tiled input failures have contextual diagnostics", "[tiled]") {
    TiledFiles files;
    const std::vector<std::pair<std::string, std::string>> changes = {
        {"orientation=\"orthogonal\"", "orientation=\"hexagonal\""},
        {"infinite=\"0\"", "infinite=\"1\""},
        {"encoding=\"csv\"", "encoding=\"base64\""},
        {"encoding=\"csv\"", "encoding=\"csv\" compression=\"zlib\""},
        {">1,2147483649,", ">99,2147483649,"},
        {">1,2147483649,", ">-1,2147483649,"},
        {">1,2147483649,", ">4294967296,2147483649,"},
        {",3758096385</data>", "</data>"},
        {",3758096385</data>", ",3758096385,1</data>"},
        {",3758096385</data>", ",3758096385,</data>"},
        {">1,2147483649,", ">1,,2147483649,"},
        {"firstgid=\"17\"", "firstgid=\"2\""},
        {"name=\"Tiles\" width=\"4\"", "name=\"Tiles\" width=\"3\""},
        {"name=\"Tiles\" width=\"4\"", "name=\"Tiles\" mode=\"multiply\" width=\"4\""},
        {"name=\"Tiles\" width=\"4\"", "name=\"Tiles\" parallaxx=\"0.5\" width=\"4\""},
        {"name=\"to\" type=\"object\" value=\"1\"", "name=\"to\" type=\"object\" value=\"999\""},
        {"name=\"to\" type=\"object\" value=\"1\"", "name=\"to\" type=\"int\" value=\"1\""},
        {"name=\"roomId\" type=\"int\" value=\"0\"", "name=\"roomId\" type=\"int\" value=\"1\""},
        {"name=\"hikari.role\" value=\"attributes\"", "name=\"hikari.role\" value=\"tiles\""},
        {"name=\"direction\" value=\"Left\"", "name=\"direction\" value=\"Invalid\""},
        {"name=\"sequence\">0;1;0,1", "name=\"sequence\">0;5;0,1"},
        {"name=\"blockIndex\" type=\"int\" value=\"1\"", "name=\"blockIndex\" type=\"int\" value=\"0\""},
        {"name=\"spawnRate\" type=\"float\" value=\"0.5\"", "name=\"spawnRate\" type=\"float\" value=\"-1\""},
        {"name=\"spawnRate\" type=\"float\" value=\"0.5\"", "name=\"spawnRate\" type=\"float\" value=\"nan\""},
        {"offsetx=\"32\"", "offsetx=\"33\""},
        {"offsetx=\"32\"", "offsetx=\"2147483648\""},
        {"name=\"hikari.role\" value=\"force\"", "name=\"hikari.role\" value=\"unknown\""},
        {"<object id=\"11\"", "<object id=\"10\""},
        {"</objectgroup>", "<object id=\"12\" x=\"0\" y=\"0\" width=\"16\" height=\"16\"><properties><property name=\"hikari.role\" value=\"exitDoor\"/></properties></object></objectgroup>"},
        {"<map version=", "<broken version="}
    };
    for(const auto &change : changes) {
        INFO(change.first);
        const auto path = files.change("minimal.tmx", change.first, change.second);
        REQUIRE_THROWS_AS(hikari::readTiledMap(path), std::runtime_error);
        try {
            hikari::readTiledMap(path);
        } catch(const std::runtime_error &error) {
            REQUIRE(std::string(error.what()).find("fixtures/tiled/changed.tmx") != std::string::npos);
        }
    }
    REQUIRE_THROWS_AS(hikari::readTiledMap("missing.tmx"), std::runtime_error);
    REQUIRE_THROWS_AS(hikari::resolveTiledPath("map.tmx", "../escape.tsx"), std::runtime_error);
    REQUIRE_THROWS_AS(hikari::resolveTiledPath("map.tmx", "C:/host.tsx"), std::runtime_error);
    REQUIRE(hikari::resolveTiledPath("assets/stages/map.tmx", "../tilesets/./tiles.tsx") == "assets/tilesets/tiles.tsx");
}

TEST_CASE("TSX validates atlases, animation frames, spacing and margins", "[tiled]") {
    TiledFiles files;
    auto path = files.change("visual.tsx", "tilecount=\"16\" columns=\"4\"",
                             "tilecount=\"9\" columns=\"3\" margin=\"1\" spacing=\"2\"", "changed.tsx");
    const auto data = hikari::readTiledTileset(path);
    rectangle(data.tiles[8], 37, 37, 16, 16);
    REQUIRE(data.animations.at(5)[1].milliseconds == 83);
    for(const auto &change : std::vector<std::pair<std::string, std::string>>{
            {"tilecount=\"16\"", "tilecount=\"0\""},
            {"columns=\"4\"", "columns=\"0\""},
            {"tileid=\"0\"", "tileid=\"99\""},
            {"duration=\"100\"", "duration=\"0\""},
            {"tileheight=\"16\"", "tileheight=\"8\""},
            {"<tile id=\"5\">", "<tile id=\"5\" width=\"8\">"},
            {"<tile id=\"5\">", "<tile id=\"5\" height=\"8\">"},
            {"<frame tileid=\"0\"", "<frame tileid=\"-1\""}}) {
        path = files.change("visual.tsx", change.first, change.second, "changed.tsx");
        REQUIRE_THROWS_AS(hikari::readTiledTileset(path), std::runtime_error);
    }
}

TEST_CASE("Nested Tiled offsets translate every room surface consistently", "[tiled]") {
    TiledFiles files;
    const auto original = hikari::FileSystem::readFileAsString("fixtures/tiled/minimal.tmx");
    const auto begin = original.find("  <layer");
    const auto end = original.find(" </group>", begin);
    REQUIRE(begin != std::string::npos);
    REQUIRE(end != std::string::npos);
    const auto children = original.substr(begin, end - begin);
    const auto path = files.change("minimal.tmx", children,
        "<group id=\"5\" offsetx=\"16\" offsety=\"32\">" + children + "</group>\n");
    const auto data = hikari::readTiledMap(path);
    const auto &room = data.rooms[0];
    REQUIRE(room.x == 3);
    REQUIRE(room.y == 3);
    REQUIRE(room.heroSpawn.getX() == 8);
    REQUIRE(room.heroSpawn.getY() == 9);
    rectangle(room.cameraBounds, 48, 48, 64, 32);
    rectangle(*room.exitDoor, 6, 3, 1, 2);
    rectangle(room.forces[0].bounds, 51, 52, 8, 12);
    REQUIRE(room.spawners[0].position.getX() == 56);
    REQUIRE(room.spawners[0].position.getY() == 64);
    REQUIRE(room.transitions[0].getX() == 3);
    REQUIRE(room.transitions[0].getY() == 3);
    rectangle(room.blockSequences[0].getBounds(), 48, 48, 64, 32);
    REQUIRE(room.blockSequences[0].getBlockPositions()[1].getX() == 64);
    REQUIRE(room.blockSequences[0].getBlockPositions()[1].getY() == 64);
    REQUIRE(room.tiles[0] == 0);
    REQUIRE(room.attributes[1] == 1);
}

TEST_CASE("Tiled room and object references survive group reordering", "[tiled]") {
    TiledFiles files;
    const auto original = hikari::FileSystem::readFileAsString("assets/stages/map-pearl.tmx");
    const auto begin = original.find(" <group");
    REQUIRE(begin != std::string::npos);
    const auto end = original.find("</group>", begin) + std::string("</group>").size();
    REQUIRE(end < original.size());
    auto reordered = original.substr(0, begin) + original.substr(end);
    reordered.insert(reordered.find("</map>"), original.substr(begin, end - begin));
    std::filesystem::create_directories(files.scratch / "assets" / "stages");
    std::ofstream out(files.scratch / "assets" / "stages" / "map-pearl.tmx");
    out << reordered;
    out.close();
    const auto map = hikari::readTiledMap("assets/stages/map-pearl.tmx");
    REQUIRE(map.rooms[0].id == 0);
    REQUIRE(map.rooms[0].width == 80);
    REQUIRE(map.rooms[0].transitions[0].getToRegion() == 1);
    REQUIRE(map.bossChamber == -1);
}

TEST_CASE("Native TMX, TSX and atlas images load from PhysicsFS archives", "[tiled]") {
    TiledFiles files;
    const auto content = std::getenv("HIKARI_TEST_TILED_CONTENT");
    PhysFS::removeFromSearchPath(content ? content : HIKARI_TEST_CONTENT_DIR);
    PhysFS::removeFromSearchPath(HIKARI_TEST_FIXTURE_DIR);
    PhysFS::addToSearchPath(std::string(HIKARI_TEST_FIXTURE_DIR) + "/fixtures/tiled/packed.zip");
    const auto data = hikari::readTiledMap("fixtures/tiled/minimal.tmx");
    REQUIRE(data.rooms.size() == 1);
    hikari::ImageCache images(hikari::ImageCache::NO_SMOOTHING, hikari::ImageCache::NO_MASKING);
    hikari::TilesetLoader loader(images);
    const auto tileset = loader.load(data.tileset);
    REQUIRE(tileset->getTexture()->getSize().x == 64);
    tileset->update(0.01f);
    REQUIRE(tileset->getTileRect(5).position.x == 0);
    REQUIRE(tileset->getTileRect(5).position.y == 0);
    REQUIRE_THROWS_AS(tileset->getTileRect(99), std::out_of_range);
}

TEST_CASE("Runtime map construction accepts every shipped native stage", "[tiled][content]") {
    TiledFiles files;
    hikari::SquirrelService scripting;
    RuntimeMaps resources;
    const auto baseline = hikari::JsonUtils::loadJson("fixtures/tiled/legacy-baseline.json");
    for(const auto &name : baseline["maps"].getMemberNames()) {
        INFO(name);
        const auto map = resources.loader.load(name.substr(0, name.size() - 5) + ".tmx");
        REQUIRE(map->getRoomCount() == baseline["maps"][name]["rooms"].size());
        REQUIRE(map->getStartingRoom());
        for(unsigned int i = 0; i < map->getRoomCount(); ++i) {
            const auto room = map->getRoom(i);
            const auto &old = baseline["maps"][name]["rooms"][i];
            REQUIRE(room->getSpawners().size() == old["enemies"].size() + old["items"].size());
            REQUIRE(room->getBlockSequences().size() == old["blockSequences"].size());
            REQUIRE(room->getForces().size() == old["forces"].size());
        }
        map->getTileset()->update(0.02f);
    }
}

TEST_CASE("All migrated stages preserve legacy gameplay and effective rendering", "[tiled][content]") {
    TiledFiles files;
    const auto baseline = hikari::JsonUtils::loadJson("fixtures/tiled/legacy-baseline.json");
    REQUIRE(baseline["maps"].size() == 15);
    REQUIRE(baseline["tilesets"].size() == 7);
    unsigned int rooms = 0;
    for(const auto &name : baseline["maps"].getMemberNames()) {
        INFO(name);
        const auto &old = baseline["maps"][name];
        const auto map = hikari::readTiledMap(name.substr(0, name.size() - 5) + ".tmx");
        const auto &oldTileset = baseline["tilesets"][old["tileset"].asString()];
        const auto tileset = hikari::readTiledTileset(map.tileset);
        const int grid = old["gridsize"].asInt();
        REQUIRE(map.gridSize == grid);
        REQUIRE(map.musicName == old.get("musicName", "None").asString());
        REQUIRE(map.bossEntity == old.get("bossEntity", "None").asString());
        const auto specials = old["specialRooms"];
        REQUIRE(map.starting == specials.get("starting", 0).asInt());
        REQUIRE(map.midpoint == specials.get("midpoint", 0).asInt());
        const auto corridor = specials.get("bossCorridor", 0).asUInt();
        const auto chamber = specials.get("bossChamber", 0).asUInt();
        REQUIRE(map.bossCorridor == (corridor < old["rooms"].size() ? static_cast<int>(corridor) : -1));
        REQUIRE(map.bossChamber == (chamber < old["rooms"].size() ? static_cast<int>(chamber) : -1));
        REQUIRE(map.rooms.size() == old["rooms"].size());
        rooms += static_cast<unsigned int>(map.rooms.size());
        for(std::size_t r = 0; r < map.rooms.size(); ++r) {
            INFO(r);
            const auto &room = map.rooms[r];
            const auto &previous = old["rooms"][static_cast<Json::Value::ArrayIndex>(r)];
            const int ox = previous["x"].asInt() * grid, oy = previous["y"].asInt() * grid;
            REQUIRE(room.id == previous["id"].asInt());
            REQUIRE(room.x == previous["x"].asInt());
            REQUIRE(room.y == previous["y"].asInt());
            REQUIRE(room.width == previous["width"].asInt());
            REQUIRE(room.height == previous["height"].asInt());
            REQUIRE(room.backgroundColor == previous.get("backgroundColor", 0).asInt());
            REQUIRE(room.heroSpawn.getX() == previous.get("heroSpawnX", 0).asInt());
            REQUIRE(room.heroSpawn.getY() == previous.get("heroSpawnY", 0).asInt());
            REQUIRE(room.bossEntity == previous.get("bossEntity", "None").asString());
            const auto camera = previous["cameraBounds"];
            rectangle(room.cameraBounds, ox + camera["x"].asInt() * grid, oy + camera["y"].asInt() * grid,
                      camera["width"].asInt() * grid, camera["height"].asInt() * grid);
            REQUIRE(room.tiles.size() == previous["tile"].size());
            for(std::size_t i = 0; i < room.tiles.size(); ++i) {
                const auto j = static_cast<Json::Value::ArrayIndex>(i);
                const int tile = previous["tile"][j].asInt(), mask = previous["attr"][j].asInt();
                REQUIRE(room.attributes[i] == (mask & 0x11f));
                if(tile == -1) REQUIRE(room.tiles[i] == -1);
                else {
                    const auto &rect = oldTileset["tiles"][tile];
                    rectangle(tileset.tiles.at(room.tiles[i]), rect["x"].asInt(), rect["y"].asInt(), grid, grid);
                    REQUIRE(static_cast<int>(room.transforms[i]) == (mask & 128 ? 5 : 0));
                }
            }
            REQUIRE(room.spawners.size() == previous["enemies"].size() + previous["items"].size());
            std::size_t spawnIndex = 0;
            for(const auto kind : {"enemies", "items"}) {
                for(const auto &spawn : previous[kind]) {
                    const auto &actual = room.spawners.at(spawnIndex++);
                    REQUIRE(actual.enemy == (std::string(kind) == "enemies"));
                    REQUIRE(actual.entity == spawn["type"].asString());
                    REQUIRE(actual.position.getX() == ox + spawn["x"].asInt());
                    REQUIRE(actual.position.getY() == oy + spawn["y"].asInt());
                    const auto direction = spawn.get("direction", "None").asString();
                    REQUIRE(actual.direction == (direction == "Up" || direction == "Right" ||
                        direction == "Down" || direction == "Left" ? direction : "None"));
                    REQUIRE(actual.spawnLimit == spawn.get("spawnLimit", 1).asUInt());
                    REQUIRE(actual.spawnRate == Approx(spawn.get("spawnRate", 1.0).asDouble()));
                    REQUIRE(actual.continuous == spawn.get("continuous", false).asBool());
                    REQUIRE(actual.config == spawn["config"]);
                }
            }
            REQUIRE(room.transitions.size() == previous["transitions"].size());
            for(std::size_t i = 0; i < room.transitions.size(); ++i) {
                const auto &actual = room.transitions[i];
                const auto &expected = previous["transitions"][static_cast<Json::Value::ArrayIndex>(i)];
                REQUIRE(actual.getToRegion() == expected["to"].asInt());
                REQUIRE(actual.getX() == expected["x"].asInt());
                REQUIRE(actual.getY() == expected["y"].asInt());
                REQUIRE(actual.getWidth() == expected["width"].asInt());
                REQUIRE(actual.getHeight() == expected["height"].asInt());
                REQUIRE(actual.isDoor() == expected.get("door", false).asBool());
                REQUIRE(actual.isLadderOnly() == expected.get("ladderOnly", false).asBool());
                const auto dir = expected["direction"].asString();
                REQUIRE(actual.getDirection() == (dir == "forward" ? hikari::RoomTransition::DirectionForward :
                    dir == "backward" ? hikari::RoomTransition::DirectionBackward :
                    dir == "up" ? hikari::RoomTransition::DirectionUp :
                    dir == "down" ? hikari::RoomTransition::DirectionDown : hikari::RoomTransition::DirectionTeleport));
            }
            for(const auto role : {"entrance", "exit"}) {
                const auto &actual = std::string(role) == "entrance" ? room.entranceDoor : room.exitDoor;
                const auto doors = previous["doors"];
                const auto expected = doors.isObject() ? doors[role] : Json::Value();
                REQUIRE(actual.has_value() == !expected.isNull());
                if(actual) rectangle(*actual, room.x + expected["x"].asInt(), room.y + expected["y"].asInt(),
                                     expected["width"].asInt(), expected["height"].asInt());
            }
            REQUIRE(room.forces.size() == previous["forces"].size());
            for(std::size_t i = 0; i < room.forces.size(); ++i) {
                const auto &actual = room.forces[i];
                const auto &expected = previous["forces"][static_cast<Json::Value::ArrayIndex>(i)];
                rectangle(actual.bounds, expected["x"].asInt(), expected["y"].asInt(),
                          expected["width"].asInt(), expected["height"].asInt());
                REQUIRE(actual.velocity.getX() == Approx(expected.get("velocityX", 0.0).asDouble()));
                REQUIRE(actual.velocity.getY() == Approx(expected.get("velocityY", 0.0).asDouble()));
            }
            REQUIRE(room.blockSequences.size() == previous["blockSequences"].size());
            for(std::size_t i = 0; i < room.blockSequences.size(); ++i) {
                const auto &actual = room.blockSequences[i];
                const auto &expected = previous["blockSequences"][static_cast<Json::Value::ArrayIndex>(i)];
                const int x = ox + expected["x"].asInt() * grid, y = oy + expected["y"].asInt() * grid;
                rectangle(actual.getBounds(), x, y, expected["width"].asInt() * grid, expected["height"].asInt() * grid);
                REQUIRE(actual.getBlockPositions().size() == expected["blocks"].size());
                for(std::size_t b = 0; b < actual.getBlockPositions().size(); ++b) {
                    const auto block = expected["blocks"][static_cast<Json::Value::ArrayIndex>(b)];
                    REQUIRE(actual.getBlockPositions()[b].getX() == x + block["x"].asInt() * grid);
                    REQUIRE(actual.getBlockPositions()[b].getY() == y + block["y"].asInt() * grid);
                }
                REQUIRE(actual.getTiming().size() == expected["sequence"].size());
                for(std::size_t step = 0; step < actual.getTiming().size(); ++step) {
                    std::vector<int> indexes;
                    for(const auto &value : expected["sequence"][static_cast<Json::Value::ArrayIndex>(step)])
                        indexes.push_back(value.asInt());
                    REQUIRE(actual.getTiming()[step].getBlockIndicies() == indexes);
                }
                REQUIRE(actual.getSpawnInterval() == Approx(expected.get("interval", 1.0).asDouble()));
                REQUIRE(actual.getMaximumBlockAge() == Approx(expected.get("maximumBlockAge", 106.0 / 60.0).asDouble()));
                REQUIRE(actual.getEntityName() == expected.get("entity", "Appearing Block (Red)").asString());
                REQUIRE(actual.getSoundName() == expected.get("spawnSound", "Disappearing Block").asString());
            }
        }
    }
    REQUIRE(rooms == 85);
    for(const auto &name : baseline["tilesets"].getMemberNames()) {
        const auto &old = baseline["tilesets"][name];
        const auto data = hikari::readTiledTileset(name.substr(0, name.size() - 5) + ".tsx");
        for(const auto &tile : old["tiles"]) {
            if(!tile.isMember("animation")) continue;
            const auto it = std::find_if(data.tiles.begin(), data.tiles.end(), [&](const auto &rect) {
                return rect.getX() == tile["x"].asInt() && rect.getY() == tile["y"].asInt();
            });
            REQUIRE(it != data.tiles.end());
            const auto &frames = data.animations.at(static_cast<int>(it - data.tiles.begin()));
            const auto previous = tile["animation"]["frames"];
            REQUIRE(frames.size() == previous.size());
            for(std::size_t i = 0; i < frames.size(); ++i) {
                const auto frame = previous[static_cast<Json::Value::ArrayIndex>(i)];
                rectangle(data.tiles.at(frames[i].tile), frame["x"].asInt(), frame["y"].asInt(),
                          frame["width"].asInt(), frame["height"].asInt());
                REQUIRE(std::abs(frames[i].milliseconds / 1000.0 - frame["length"].asDouble()) <= 0.000500001);
            }
        }
    }
}
