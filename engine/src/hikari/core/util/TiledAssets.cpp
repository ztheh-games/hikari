#include "hikari/core/util/TiledAssets.hpp"
#include "hikari/core/util/FileSystem.hpp"
#include "hikari/client/game/objects/BlockTiming.hpp"
#include <tinyxml2.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace hikari {
namespace {

    using Element = tinyxml2::XMLElement;
    constexpr std::uint32_t GID_MASK = 0x0fffffff;
    constexpr int COLLISION_MASK = 0x11f;

    [[noreturn]] void fail(const std::string &file, const Element *element,
                          const std::string &message) {
        std::ostringstream out;
        out << file;
        if(element) {
            out << ":" << element->GetLineNum() << " <" << element->Name();
            if(element->Attribute("name")) out << " name=\"" << element->Attribute("name") << "\"";
            if(element->Attribute("id")) out << " id=\"" << element->Attribute("id") << "\"";
            out << ">";
        }
        throw std::runtime_error(out.str() + ": " + message);
    }

    void loadXml(tinyxml2::XMLDocument &document, const std::string &file) {
        auto stream = FileSystem::openFileRead(file);
        if(!stream->good()) fail(file, nullptr, "cannot open Tiled asset");
        std::ostringstream text;
        text << stream->rdbuf();
        if(stream->bad() || text.bad()) fail(file, nullptr, "failed reading Tiled asset");
        const auto bytes = text.str();
        if(document.Parse(bytes.data(), bytes.size()) != tinyxml2::XML_SUCCESS)
            fail(file, nullptr, std::string("XML error: ") + document.ErrorStr());
    }

    std::string trim(std::string value) {
        const auto begin = value.find_first_not_of(" \t\r\n");
        if(begin == std::string::npos) return {};
        return value.substr(begin, value.find_last_not_of(" \t\r\n") - begin + 1);
    }

    template<class T>
    T number(const std::string &text, const std::string &file, const Element *element) {
        const auto value = trim(text);
        T result{};
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
        if(value.empty() || parsed.ec != std::errc() || parsed.ptr != value.data() + value.size())
            fail(file, element, "invalid number \"" + text + "\"");
        if constexpr(std::is_floating_point<T>::value) {
            if(!std::isfinite(result)) fail(file, element, "non-finite number");
        }
        return result;
    }

    int checked(std::int64_t value, const std::string &file, const Element *element) {
        if(value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
            fail(file, element, "coordinate/dimension overflow");
        return static_cast<int>(value);
    }

    int attribute(const Element *element, const char *key, const std::string &file,
                  std::optional<int> fallback = std::nullopt) {
        const char *text = element->Attribute(key);
        if(!text) {
            if(fallback) return *fallback;
            fail(file, element, std::string("missing attribute ") + key);
        }
        const auto value = number<double>(text, file, element);
        if(std::trunc(value) != value || value < std::numeric_limits<int>::min() ||
           value > std::numeric_limits<int>::max())
            fail(file, element, std::string(key) + " must be an integer");
        return static_cast<int>(value);
    }

    struct Property {
        std::string type;
        Json::Value value;
    };
    using Properties = std::map<std::string, Property>;

    Properties properties(const Element *element, const std::string &file) {
        Properties result;
        const auto container = element->FirstChildElement("properties");
        if(!container) return result;
        if(container->NextSiblingElement("properties")) fail(file, element, "duplicate properties container");
        for(auto node = container->FirstChildElement("property"); node;
            node = node->NextSiblingElement("property")) {
            const auto name = node->Attribute("name");
            if(!name || !*name) fail(file, node, "property has no name");
            const std::string type = node->Attribute("type") ? node->Attribute("type") : "string";
            std::string text = node->Attribute("value") ? node->Attribute("value")
                : (node->GetText() ? node->GetText() : "");
            if(!node->Attribute("value") && !node->GetText()) {
                if(type == "int" || type == "object" || type == "float") text = "0";
                else if(type == "bool") text = "false";
            }
            Json::Value value;
            if(type == "int" || type == "object") value = number<int>(text, file, node);
            else if(type == "float") value = number<double>(text, file, node);
            else if(type == "bool") {
                if(text != "true" && text != "false") fail(file, node, "invalid boolean property");
                value = text == "true";
            } else if(type == "string" || type == "color" || type == "file") value = text;
            else value = text;
            if(!result.emplace(name, Property{type, value}).second)
                fail(file, node, std::string("duplicate property ") + name);
        }
        return result;
    }

    const Property *property(const Properties &values, const char *name, const char *type,
                             const std::string &file, const Element *element) {
        const auto found = values.find(name);
        if(found == values.end()) return nullptr;
        if(found->second.type != type)
            fail(file, element, std::string("property ") + name + " must have type " + type);
        return &found->second;
    }

    std::string stringProperty(const Properties &values, const char *name,
                               const std::string &file, const Element *element,
                               const std::string &fallback = {}) {
        const auto value = property(values, name, "string", file, element);
        return value ? value->value.asString() : fallback;
    }

    int intProperty(const Properties &values, const char *name, const std::string &file,
                    const Element *element, std::optional<int> fallback = std::nullopt,
                    const char *type = "int") {
        const auto value = property(values, name, type, file, element);
        if(value) return value->value.asInt();
        if(fallback) return *fallback;
        fail(file, element, std::string("missing property ") + name);
    }

    float floatProperty(const Properties &values, const char *name, const std::string &file,
                        const Element *element, float fallback) {
        const auto value = property(values, name, "float", file, element);
        const double result = value ? value->value.asDouble() : fallback;
        if(std::abs(result) > std::numeric_limits<float>::max())
            fail(file, element, std::string("property overflow: ") + name);
        return static_cast<float>(result);
    }

    bool boolProperty(const Properties &values, const char *name, const std::string &file,
                      const Element *element, bool fallback = false) {
        const auto value = property(values, name, "bool", file, element);
        return value ? value->value.asBool() : fallback;
    }

    int color(const std::string &value, const std::string &file, const Element *element) {
        if(value.size() != 7 && value.size() != 9) fail(file, element, "invalid color");
        if(value[0] != '#') fail(file, element, "color must start with #");
        std::uint32_t result = 0;
        const auto parsed = std::from_chars(value.data() + 1, value.data() + value.size(), result, 16);
        if(parsed.ec != std::errc() || parsed.ptr != value.data() + value.size())
            fail(file, element, "invalid color");
        if(value.size() == 9 && (result >> 24) != 255)
            fail(file, element, "translucent room backgrounds are unsupported");
        return static_cast<int>(result & 0xffffff);
    }

    void presentation(const Element *element, const std::string &file) {
        for(const auto key : {"opacity", "parallaxx", "parallaxy"}) {
            if(element->Attribute(key) && number<double>(element->Attribute(key), file, element) != 1)
                fail(file, element, std::string("unsupported ") + key);
        }
        if(element->Attribute("tintcolor") ||
           (element->Attribute("mode") && std::string(element->Attribute("mode")) != "normal"))
            fail(file, element, "tint/blend modes are unsupported");
    }

    int tiles(int pixels, int grid, const std::string &file, const Element *element) {
        if(pixels % grid) fail(file, element, "geometry must align with the tile grid");
        return pixels / grid;
    }

    std::size_t cellCount(int width, int height, const std::string &file, const Element *element) {
        if(width <= 0 || height <= 0) fail(file, element, "dimensions must be positive");
        const auto count = static_cast<std::int64_t>(width) * height;
        if(count > std::numeric_limits<int>::max()) fail(file, element, "tile array is too large");
        return static_cast<std::size_t>(count);
    }

    struct Offset {
        int x = 0;
        int y = 0;
    };

    Offset offset(const Element *element, Offset parent, const std::string &file) {
        if(attribute(element, "x", file, 0) || attribute(element, "y", file, 0))
            fail(file, element, "deprecated layer x/y offsets are unsupported; use offsetx/offsety");
        return {checked(static_cast<std::int64_t>(parent.x) + attribute(element, "offsetx", file, 0), file, element),
                checked(static_cast<std::int64_t>(parent.y) + attribute(element, "offsety", file, 0), file, element)};
    }

    struct Object {
        const Element *element;
        Properties props;
        std::string role;
        int id;
        Rectangle2D<int> bounds;
    };

    struct Layer {
        const Element *element;
        std::string role;
        Offset origin;
    };

    struct RoomSource {
        const Element *element;
        Properties props;
        std::vector<Object> objects;
        std::vector<Layer> layers;
    };

    void gatherRoom(const Element *parent, Offset origin, RoomSource &source,
                    std::set<int> &objectIds, const std::string &file) {
        for(auto node = parent->FirstChildElement(); node; node = node->NextSiblingElement()) {
            const std::string tag = node->Name();
            if(tag == "group") {
                presentation(node, file);
                if(!stringProperty(properties(node, file), "hikari.role", file, node).empty())
                    fail(file, node, "nested gameplay room groups are unsupported");
                gatherRoom(node, offset(node, origin, file), source, objectIds, file);
            } else if(tag == "layer") {
                const auto role = stringProperty(properties(node, file), "hikari.role", file, node);
                if(role != "tiles" && role != "attributes") fail(file, node, "unrecognized tile layer role");
                if(role == "tiles") presentation(node, file);
                source.layers.push_back({node, role, offset(node, origin, file)});
            } else if(tag == "objectgroup") {
                const auto position = offset(node, origin, file);
                for(auto obj = node->FirstChildElement("object"); obj; obj = obj->NextSiblingElement("object")) {
                    const int id = attribute(obj, "id", file);
                    if(id <= 0 || !objectIds.insert(id).second) fail(file, obj, "invalid or duplicate object ID");
                    if(obj->Attribute("gid") || obj->Attribute("template") || attribute(obj, "rotation", file, 0))
                        fail(file, obj, "tile objects, templates and object rotation are unsupported");
                    for(auto shape = obj->FirstChildElement(); shape; shape = shape->NextSiblingElement()) {
                        if(std::string(shape->Name()) != "point" && std::string(shape->Name()) != "properties")
                            fail(file, shape, "unsupported object shape");
                    }
                    auto props = properties(obj, file);
                    const auto role = stringProperty(props, "hikari.role", file, obj);
                    if(role.empty()) fail(file, obj, "missing object role");
                    const int x = checked(static_cast<std::int64_t>(position.x) + attribute(obj, "x", file, 0), file, obj);
                    const int y = checked(static_cast<std::int64_t>(position.y) + attribute(obj, "y", file, 0), file, obj);
                    const int w = attribute(obj, "width", file, 0);
                    const int h = attribute(obj, "height", file, 0);
                    if(w < 0 || h < 0) fail(file, obj, "negative object dimensions");
                    checked(static_cast<std::int64_t>(x) + w, file, obj);
                    checked(static_cast<std::int64_t>(y) + h, file, obj);
                    source.objects.push_back({obj, std::move(props), role, id, {x, y, w, h}});
                }
            } else if(tag == "imagelayer") fail(file, node, "image layers are unsupported");
        }
    }

    struct TilesetReference {
        std::uint32_t first;
        std::string path;
        TiledTilesetData data;
    };

    std::vector<std::uint32_t> csv(const Element *layer, int width, int height,
                                   const std::string &file) {
        if(attribute(layer, "width", file) != width || attribute(layer, "height", file) != height)
            fail(file, layer, "finite layer dimensions must match the map");
        const auto data = layer->FirstChildElement("data");
        if(!data || data->NextSiblingElement("data") || !data->Attribute("encoding") || std::string(data->Attribute("encoding")) != "csv" ||
           data->Attribute("compression") || data->FirstChildElement())
            fail(file, layer, "only uncompressed CSV tile data is supported");
        std::vector<std::uint32_t> result;
        const auto expected = cellCount(width, height, file, layer);
        result.reserve(expected);
        const auto text = trim(data->GetText() ? data->GetText() : "");
        if(text.empty() || text.back() == ',') fail(file, layer, "empty CSV cell");
        std::istringstream stream(text);
        std::string token;
        while(std::getline(stream, token, ',')) {
            if(result.size() == expected) fail(file, layer, "too many CSV cells");
            result.push_back(number<std::uint32_t>(token, file, layer));
        }
        if(result.size() != expected) fail(file, layer, "incorrect CSV cell count");
        return result;
    }

    const TilesetReference &reference(std::uint32_t gid, const std::vector<TilesetReference> &sets,
                                      const std::string &file, const Element *element) {
        for(auto it = sets.rbegin(); it != sets.rend(); ++it) {
            if(gid >= it->first) {
                if(gid - it->first >= it->data.tiles.size()) fail(file, element, "GID outside tileset range");
                return *it;
            }
        }
        fail(file, element, "GID has no tileset");
    }

    Rectangle2D<int> gridRectangle(const Object &obj, int grid, const std::string &file) {
        if(obj.element->FirstChildElement("point") || obj.bounds.getWidth() <= 0 || obj.bounds.getHeight() <= 0)
            fail(file, obj.element, "rectangle dimensions must be positive");
        return {tiles(obj.bounds.getX(), grid, file, obj.element),
                tiles(obj.bounds.getY(), grid, file, obj.element),
                tiles(obj.bounds.getWidth(), grid, file, obj.element),
                tiles(obj.bounds.getHeight(), grid, file, obj.element)};
    }

    int roomReference(const Properties &props, const char *key, const std::map<int, int> &rooms,
                      const std::string &file, const Element *element, bool optional = false,
                      bool allowDisabled = false) {
        const int id = intProperty(props, key, file, element, optional ? std::optional<int>(0) : std::nullopt, "object");
        if(allowDisabled && id == 0 && props.count(key)) return -1;
        if(optional && id == 0) return 0;
        const auto target = rooms.find(id);
        if(target == rooms.end()) fail(file, element, std::string(key) + " must reference a roomBounds object");
        return target->second;
    }

    RoomTransition::Direction transitionDirection(const std::string &direction,
                                                  const std::string &file, const Element *element) {
        if(direction == "forward") return RoomTransition::DirectionForward;
        if(direction == "backward") return RoomTransition::DirectionBackward;
        if(direction == "up") return RoomTransition::DirectionUp;
        if(direction == "down") return RoomTransition::DirectionDown;
        if(direction == "transport" || direction == "teleport") return RoomTransition::DirectionTeleport;
        fail(file, element, "invalid transition direction " + direction);
    }

} // namespace

    std::string resolveTiledPath(const std::string &document, const std::string &relative) {
        if(relative.empty() || relative.front() == '/' || relative.find('\\') != std::string::npos ||
           relative.find(':') != std::string::npos)
            fail(document, nullptr, "asset reference must be a relative virtual path: " + relative);
        const auto slash = document.find_last_of('/');
        std::string path = (slash == std::string::npos ? "" : document.substr(0, slash + 1)) + relative;
        std::vector<std::string> parts;
        std::istringstream stream(path);
        std::string part;
        while(std::getline(stream, part, '/')) {
            if(part.empty() || part == ".") continue;
            if(part == "..") {
                if(parts.empty()) fail(document, nullptr, "asset reference escapes the virtual filesystem");
                parts.pop_back();
            } else parts.push_back(part);
        }
        std::string result;
        for(const auto &value : parts) {
            if(!result.empty()) result += '/';
            result += value;
        }
        if(result.empty()) fail(document, nullptr, "empty asset path");
        return result;
    }

    TiledTilesetData readTiledTileset(const std::string &fileName) {
        tinyxml2::XMLDocument document;
        loadXml(document, fileName);
        const auto root = document.RootElement();
        if(!root || root->NextSiblingElement() || std::string(root->Name()) != "tileset") fail(fileName, root, "expected one TSX tileset");
        TiledTilesetData result;
        const auto props = properties(root, fileName);
        result.role = stringProperty(props, "hikari.role", fileName, root);
        if(result.role != "visual" && result.role != "attributes") fail(fileName, root, "invalid tileset role");
        result.tileSize = attribute(root, "tilewidth", fileName);
        if(result.tileSize <= 0 || attribute(root, "tileheight", fileName) != result.tileSize)
            fail(fileName, root, "only square positive tile sizes are supported");
        const int count = attribute(root, "tilecount", fileName);
        const int columns = attribute(root, "columns", fileName);
        cellCount(count, 1, fileName, root);
        if(columns <= 0) fail(fileName, root, "image-collection tilesets are unsupported");
        const int spacing = attribute(root, "spacing", fileName, 0);
        const int margin = attribute(root, "margin", fileName, 0);
        if(spacing < 0 || margin < 0) fail(fileName, root, "negative spacing or margin");
        const auto image = root->FirstChildElement("image");
        if(!image || !image->Attribute("source") || image->Attribute("trans") || image->FirstChildElement())
            fail(fileName, root, "a single external atlas image is required");
        if(image->NextSiblingElement("image")) fail(fileName, root, "multiple atlas images are unsupported");
        result.image = resolveTiledPath(fileName, image->Attribute("source"));
        const int width = attribute(image, "width", fileName);
        const int height = attribute(image, "height", fileName);
        const auto stride = static_cast<std::int64_t>(result.tileSize) + spacing;
        const auto usableWidth = static_cast<std::int64_t>(width) - 2LL * margin;
        const auto usableHeight = static_cast<std::int64_t>(height) - 2LL * margin;
        const auto rows = (usableHeight + spacing) / stride;
        if(usableWidth < result.tileSize || usableHeight < result.tileSize ||
           (usableWidth + spacing) / stride != columns || static_cast<std::int64_t>(count) > columns * rows)
            fail(fileName, root, "atlas dimensions do not contain the declared tiles");
        if(const auto tileOffset = root->FirstChildElement("tileoffset")) {
            if(attribute(tileOffset, "x", fileName, 0) || attribute(tileOffset, "y", fileName, 0))
                fail(fileName, tileOffset, "tileset drawing offsets are unsupported");
        }
        for(int i = 0; i < count; ++i) {
            result.tiles.emplace_back(checked(margin + (i % columns) * stride, fileName, root),
                                      checked(margin + (i / columns) * stride, fileName, root),
                                      result.tileSize, result.tileSize);
        }
        result.attributes.resize(count, -1);
        std::set<int> ids;
        for(auto tile = root->FirstChildElement("tile"); tile; tile = tile->NextSiblingElement("tile")) {
            const int id = attribute(tile, "id", fileName);
            if(id < 0 || id >= count || !ids.insert(id).second) fail(fileName, tile, "invalid or duplicate tile ID");
            if(tile->FirstChildElement("image") || tile->FirstChildElement("objectgroup") ||
               tile->Attribute("x") || tile->Attribute("y") ||
               tile->Attribute("width") || tile->Attribute("height"))
                fail(fileName, tile, "per-tile images, subrectangles and polygon collision are unsupported");
            const auto values = properties(tile, fileName);
            if(result.role == "attributes") {
                const int mask = intProperty(values, "attributes", fileName, tile);
                if(mask < 0 || (mask & ~COLLISION_MASK)) fail(fileName, tile, "unknown collision attribute bits");
                result.attributes[id] = mask;
            }
            if(const auto animation = tile->FirstChildElement("animation")) {
                if(animation->NextSiblingElement("animation")) fail(fileName, tile, "duplicate tile animation");
                if(result.role != "visual") fail(fileName, tile, "attribute tiles cannot animate");
                auto &frames = result.animations[id];
                for(auto frame = animation->FirstChildElement("frame"); frame; frame = frame->NextSiblingElement("frame")) {
                    const int index = attribute(frame, "tileid", fileName);
                    const int duration = attribute(frame, "duration", fileName);
                    if(index < 0 || index >= count || duration <= 0) fail(fileName, frame, "invalid animation frame");
                    frames.push_back({index, duration});
                }
                if(frames.empty()) fail(fileName, animation, "animation has no frames");
            }
        }
        return result;
    }

    TiledMapData readTiledMap(const std::string &fileName) {
        tinyxml2::XMLDocument document;
        loadXml(document, fileName);
        const auto root = document.RootElement();
        if(!root || root->NextSiblingElement() || std::string(root->Name()) != "map") fail(fileName, root, "expected one TMX map");
        if(!root->Attribute("orientation") || std::string(root->Attribute("orientation")) != "orthogonal" ||
           attribute(root, "infinite", fileName, 0))
            fail(fileName, root, "only finite orthogonal maps are supported");
        if(root->Attribute("renderorder") && std::string(root->Attribute("renderorder")) != "right-down")
            fail(fileName, root, "only right-down render order is supported");
        presentation(root, fileName);
        TiledMapData result;
        result.gridSize = attribute(root, "tilewidth", fileName);
        if(result.gridSize <= 0 || attribute(root, "tileheight", fileName) != result.gridSize)
            fail(fileName, root, "only square positive tile sizes are supported");
        const int width = attribute(root, "width", fileName);
        const int height = attribute(root, "height", fileName);
        cellCount(width, height, fileName, root);
        checked(static_cast<std::int64_t>(width) * result.gridSize, fileName, root);
        checked(static_cast<std::int64_t>(height) * result.gridSize, fileName, root);
        const auto props = properties(root, fileName);
        result.musicName = stringProperty(props, "musicName", fileName, root, "None");
        result.bossEntity = stringProperty(props, "bossEntity", fileName, root, "None");
        std::vector<TilesetReference> sets;
        std::set<std::string> roles;
        for(auto node = root->FirstChildElement("tileset"); node; node = node->NextSiblingElement("tileset")) {
            if(!node->Attribute("source")) fail(fileName, node, "external TSX tilesets are required");
            const auto first = number<std::uint32_t>(
                node->Attribute("firstgid") ? node->Attribute("firstgid") : "", fileName, node);
            const auto path = resolveTiledPath(fileName, node->Attribute("source"));
            auto data = readTiledTileset(path);
            if(first == 0 || first > GID_MASK ||
               data.tiles.size() > static_cast<std::size_t>(GID_MASK) - first + 1 ||
               (!sets.empty() && first < sets.back().first + sets.back().data.tiles.size()))
                fail(fileName, node, "invalid or overlapping firstgid ranges");
            if(sets.empty() && first != 1) fail(fileName, node, "the first tileset must start at GID 1");
            if(!roles.insert(data.role).second) fail(fileName, node, "multiple visual/attribute tilesets are unsupported");
            if(data.tileSize != result.gridSize) fail(fileName, node, "tileset size must match the map grid");
            if(data.role == "visual") result.tileset = path;
            sets.push_back({first, path, std::move(data)});
        }
        if(roles != std::set<std::string>{"visual", "attributes"})
            fail(fileName, root, "one visual and one attribute tileset are required");

        std::set<int> objectIds;
        std::vector<RoomSource> sources;
        for(auto node = root->FirstChildElement(); node; node = node->NextSiblingElement()) {
            const std::string tag = node->Name();
            if(tag == "group") {
                auto values = properties(node, fileName);
                if(stringProperty(values, "hikari.role", fileName, node) != "room")
                    fail(fileName, node, "top-level groups must have the room role");
                presentation(node, fileName);
                RoomSource source{node, std::move(values), {}, {}};
                gatherRoom(node, offset(node, {}, fileName), source, objectIds, fileName);
                sources.push_back(std::move(source));
            } else if(tag == "layer" || tag == "objectgroup" || tag == "imagelayer")
                fail(fileName, node, "gameplay layers must belong to a room group");
        }
        if(sources.empty()) fail(fileName, root, "map has no rooms");
        std::sort(sources.begin(), sources.end(), [&](const RoomSource &a, const RoomSource &b) {
            return intProperty(a.props, "roomId", fileName, a.element)
                < intProperty(b.props, "roomId", fileName, b.element);
        });
        result.rooms.resize(sources.size());
        std::map<int, int> roomObjects;
        for(std::size_t index = 0; index < sources.size(); ++index) {
            const auto &source = sources[index];
            auto &room = result.rooms[index];
            room.id = intProperty(source.props, "roomId", fileName, source.element);
            if(room.id != static_cast<int>(index)) fail(fileName, source.element, "room IDs must be unique and contiguous from zero");
            const Object *bounds = nullptr;
            for(const auto &obj : source.objects) {
                if(obj.role == "roomBounds") {
                    if(bounds) fail(fileName, obj.element, "duplicate room bounds");
                    bounds = &obj;
                }
            }
            if(!bounds) fail(fileName, source.element, "room has no roomBounds rectangle");
            const auto rect = gridRectangle(*bounds, result.gridSize, fileName);
            room.boundsObject = bounds->id;
            roomObjects.emplace(bounds->id, room.id);
            room.x = rect.getX(); room.y = rect.getY();
            room.width = rect.getWidth(); room.height = rect.getHeight();
            const auto count = cellCount(room.width, room.height, fileName, bounds->element);
            room.tiles.resize(count, -1);
            room.attributes.resize(count, 0);
            room.transforms.resize(count, TileTransform::None);
            const auto bg = property(source.props, "backgroundColor", "color", fileName, source.element);
            room.backgroundColor = bg ? color(bg->value.asString(), fileName, source.element)
                : (root->Attribute("backgroundcolor") ? color(root->Attribute("backgroundcolor"), fileName, root) : 0);
            room.bossEntity = stringProperty(source.props, "bossEntity", fileName, source.element, "None");
            std::set<std::string> layerRoles;
            for(const auto &layer : source.layers) {
                if(!layerRoles.insert(layer.role).second) fail(fileName, layer.element, "duplicate room tile layer");
                const auto data = csv(layer.element, width, height, fileName);
                const int x0 = tiles(checked(static_cast<std::int64_t>(bounds->bounds.getX()) - layer.origin.x,
                                            fileName, layer.element), result.gridSize, fileName, layer.element);
                const int y0 = tiles(checked(static_cast<std::int64_t>(bounds->bounds.getY()) - layer.origin.y,
                                            fileName, layer.element), result.gridSize, fileName, layer.element);
                if(x0 < 0 || y0 < 0 || static_cast<std::int64_t>(x0) + room.width > width ||
                   static_cast<std::int64_t>(y0) + room.height > height)
                    fail(fileName, layer.element, "room bounds are outside the tile layer");
                for(int y = 0; y < height; ++y) {
                    for(int x = 0; x < width; ++x) {
                        const auto raw = data[x + y * width];
                        const auto gid = raw & GID_MASK;
                        const bool inside = x >= x0 && x < x0 + room.width && y >= y0 && y < y0 + room.height;
                        if(!inside) {
                            if(gid) fail(fileName, layer.element, "nonempty cells outside room bounds");
                            continue;
                        }
                        const int index = x - x0 + (y - y0) * room.width;
                        if(!gid) continue;
                        const auto &set = reference(gid, sets, fileName, layer.element);
                        const auto tile = gid - set.first;
                        if(layer.role == "tiles") {
                            if(set.data.role != "visual") fail(fileName, layer.element, "attribute GID on visual layer");
                            room.tiles[index] = static_cast<int>(tile);
                            const auto flags = static_cast<std::uint8_t>(
                                ((raw >> 31) & 1) | ((raw >> 29) & 2) | ((raw >> 27) & 4));
                            room.transforms[index] = static_cast<TileTransform>(flags);
                        } else {
                            if(set.data.role != "attributes" || (raw & 0xe0000000))
                                fail(fileName, layer.element, "visual/transformed GID on attribute layer");
                            if(set.data.attributes[tile] < 0) fail(fileName, layer.element, "palette tile has no collision mask");
                            room.attributes[index] = set.data.attributes[tile];
                        }
                    }
                }
            }
            if(layerRoles != std::set<std::string>{"tiles", "attributes"})
                fail(fileName, source.element, "room requires visual and attribute layers");
        }
        result.starting = roomReference(props, "starting", roomObjects, fileName, root, true);
        result.midpoint = roomReference(props, "midpoint", roomObjects, fileName, root, true);
        result.bossCorridor = roomReference(props, "bossCorridor", roomObjects, fileName, root, true, true);
        result.bossChamber = roomReference(props, "bossChamber", roomObjects, fileName, root, true, true);

        for(std::size_t index = 0; index < sources.size(); ++index) {
            const auto &source = sources[index];
            auto &room = result.rooms[index];
            const int ox = checked(static_cast<std::int64_t>(room.x) * result.gridSize, fileName, source.element);
            const int oy = checked(static_cast<std::int64_t>(room.y) * result.gridSize, fileName, source.element);
            bool hasCamera = false, hasSpawn = false;
            std::map<int, const Object *> sequences;
            std::map<int, std::map<int, const Object *>> blocks;
            for(const auto &obj : source.objects) {
                const auto el = obj.element;
                const auto &values = obj.props;
                const int x = obj.bounds.getX(), y = obj.bounds.getY();
                if(obj.role == "roomBounds") continue;
                if(obj.role == "heroSpawn" || obj.role == "enemy" || obj.role == "item" || obj.role == "block") {
                    if(!el->FirstChildElement("point")) fail(fileName, el, "this role requires a point object");
                } else if(obj.bounds.getWidth() <= 0 || obj.bounds.getHeight() <= 0 ||
                          el->FirstChildElement("point"))
                    fail(fileName, el, "this role requires a positive rectangle");
                if(obj.role == "heroSpawn") {
                    if(hasSpawn) fail(fileName, el, "duplicate hero spawn");
                    hasSpawn = true;
                    room.heroSpawn = {checked(static_cast<std::int64_t>(x) - ox, fileName, el),
                                      checked(static_cast<std::int64_t>(y) - oy, fileName, el)};
                } else if(obj.role == "cameraBounds") {
                    if(hasCamera) fail(fileName, el, "duplicate camera bounds");
                    hasCamera = true;
                    gridRectangle(obj, result.gridSize, fileName);
                    room.cameraBounds = obj.bounds;
                } else if(obj.role == "enemy" || obj.role == "item") {
                    TiledSpawnerData spawn;
                    spawn.enemy = obj.role == "enemy";
                    spawn.entity = stringProperty(values, "entity", fileName, el);
                    if(spawn.entity.empty()) fail(fileName, el, "spawner requires an entity name");
                    spawn.position = {x, y};
                    spawn.direction = stringProperty(values, "direction", fileName, el, "None");
                    if(spawn.direction != "None" && spawn.direction != "Up" && spawn.direction != "Right" &&
                       spawn.direction != "Down" && spawn.direction != "Left")
                        fail(fileName, el, "invalid spawner direction");
                    const int limit = intProperty(values, "spawnLimit", fileName, el, 1);
                    spawn.spawnRate = floatProperty(values, "spawnRate", fileName, el, 1);
                    if(limit <= 0 || spawn.spawnRate < 0) fail(fileName, el, "invalid spawn limit/rate");
                    spawn.spawnLimit = static_cast<unsigned int>(limit);
                    spawn.continuous = boolProperty(values, "continuous", fileName, el);
                    for(const auto &entry : values) {
                        if(entry.first.compare(0, 7, "config.") == 0) {
                            if(entry.first.size() == 7 ||
                               (entry.second.type != "bool" && entry.second.type != "string" &&
                                entry.second.type != "int" && entry.second.type != "float"))
                                fail(fileName, el, "config members must be named scalar properties");
                            spawn.config[entry.first.substr(7)] = entry.second.value;
                        }
                    }
                    if(!spawn.enemy && (!spawn.config.isNull() || values.count("continuous") ||
                                        values.count("spawnLimit") || values.count("spawnRate")))
                        fail(fileName, el, "enemy-only properties on item spawner");
                    room.spawners.push_back(std::move(spawn));
                } else if(obj.role == "transition") {
                    const auto rect = gridRectangle(obj, result.gridSize, fileName);
                    const auto target = roomReference(values, "to", roomObjects, fileName, el);
                    const auto direction = transitionDirection(stringProperty(values, "direction", fileName, el), fileName, el);
                    room.transitions.emplace_back(-1, target, rect.getWidth(), rect.getHeight(),
                        rect.getX(), rect.getY(), direction, boolProperty(values, "door", fileName, el),
                        boolProperty(values, "ladderOnly", fileName, el));
                } else if(obj.role == "entranceDoor" || obj.role == "exitDoor") {
                    auto &door = obj.role == "entranceDoor" ? room.entranceDoor : room.exitDoor;
                    if(door) fail(fileName, el, "duplicate door");
                    door = gridRectangle(obj, result.gridSize, fileName);
                } else if(obj.role == "force") {
                    room.forces.push_back({obj.bounds, {
                        floatProperty(values, "velocityX", fileName, el, 0),
                        floatProperty(values, "velocityY", fileName, el, 0)}});
                } else if(obj.role == "blockSequence") {
                    gridRectangle(obj, result.gridSize, fileName);
                    sequences.emplace(obj.id, &obj);
                } else if(obj.role == "block") {
                    const int sequence = intProperty(values, "sequenceObject", fileName, el, std::nullopt, "object");
                    const int blockIndex = intProperty(values, "blockIndex", fileName, el);
                    tiles(x, result.gridSize, fileName, el);
                    tiles(y, result.gridSize, fileName, el);
                    if(blockIndex < 0 || !blocks[sequence].emplace(blockIndex, &obj).second)
                        fail(fileName, el, "invalid or duplicate block index");
                } else fail(fileName, el, "unsupported gameplay role " + obj.role);
            }
            if(!hasCamera) fail(fileName, source.element, "room requires cameraBounds");
            for(const auto &entry : blocks) {
                if(!sequences.count(entry.first)) fail(fileName, source.element, "block references a sequence outside its room");
            }
            for(const auto &entry : sequences) {
                const auto &obj = *entry.second;
                const auto &values = obj.props;
                const auto el = obj.element;
                std::vector<Point2D<int>> positions;
                for(const auto &block : blocks[entry.first]) {
                    if(block.first != static_cast<int>(positions.size())) fail(fileName, block.second->element, "block indexes must be contiguous from zero");
                    positions.emplace_back(block.second->bounds.getX(), block.second->bounds.getY());
                }
                if(positions.empty()) fail(fileName, el, "block sequence has no blocks");
                const auto description = stringProperty(values, "sequence", fileName, el);
                if(description.empty()) fail(fileName, el, "block sequence has no timing steps");
                std::vector<BlockTiming> timing;
                std::size_t start = 0;
                do {
                    const auto end = description.find(';', start);
                    const auto step = description.substr(start, end == std::string::npos ? end : end - start);
                    std::vector<int> indexes;
                    if(!trim(step).empty()) {
                        std::istringstream stream(step);
                        std::string token;
                        while(std::getline(stream, token, ',')) {
                            const int block = number<int>(token, fileName, el);
                            if(block < 0 || block >= static_cast<int>(positions.size())) fail(fileName, el, "timing references an invalid block index");
                            indexes.push_back(block);
                        }
                        if(step.back() == ',') fail(fileName, el, "empty block index in timing");
                    }
                    timing.emplace_back(indexes);
                    if(end == std::string::npos) break;
                    start = end + 1;
                } while(start <= description.size());
                const auto interval = floatProperty(values, "interval", fileName, el, BlockSequenceDescriptor::DEFAULT_SPAWN_INTERVAL);
                const auto age = floatProperty(values, "maximumBlockAge", fileName, el, BlockSequenceDescriptor::DEFAULT_MAXIMUM_BLOCK_AGE);
                if(interval <= 0 || age <= 0) fail(fileName, el, "block timing durations must be positive");
                room.blockSequences.emplace_back(obj.bounds, positions, timing, interval, age,
                    stringProperty(values, "entity", fileName, el, BlockSequenceDescriptor::DEFAULT_BLOCK_ENTITY),
                    stringProperty(values, "spawnSound", fileName, el, BlockSequenceDescriptor::DEFAULT_SPAWN_SOUND));
            }
        }
        return result;
    }

} // hikari
