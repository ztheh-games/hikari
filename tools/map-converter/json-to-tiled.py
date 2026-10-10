"""One-time migration of legacy Hikari content to native TMX/TSX."""

import argparse
import json
import math
import pathlib
import posixpath
import struct
import xml.etree.ElementTree as ET
import zlib


COLLISION_BITS = 0x11F
ROTATE_CLOCKWISE = 0xA0000000


def property_value(element, name, value, kind=None):
    properties = element.find("properties")
    if properties is None:
        properties = ET.SubElement(element, "properties")
    if kind is None:
        kind = ("bool" if isinstance(value, bool) else
                "int" if isinstance(value, int) else
                "float" if isinstance(value, float) else "string")
    text = str(value).lower() if isinstance(value, bool) else str(value)
    ET.SubElement(properties, "property", name=name, type=kind, value=text)


def write_xml(path, root):
    path.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(root, space=" ")
    ET.ElementTree(root).write(path, encoding="UTF-8", xml_declaration=True)


def relative(path, parent):
    return posixpath.relpath(path, parent)


def image_size(path):
    data = path.read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"Expected PNG: {path}")
    return struct.unpack(">II", data[16:24])


def milliseconds(seconds):
    if not math.isfinite(seconds) or seconds <= 0:
        raise ValueError("Animation durations must be positive")
    result = math.floor(seconds * 1000 + 0.5)
    if result < 1 or abs(result / 1000 - seconds) > 0.000500001:
        raise ValueError("Animation duration cannot be represented in Tiled")
    return result


def convert_tileset(name, data, source, output):
    size = data["size"]
    width, height = image_size(source / data["surface"])
    if size <= 0 or width < size or height < size:
        raise ValueError(f"Invalid tile size or atlas dimensions in {name}")
    columns = width // size
    count = columns * (height // size)
    result = ET.Element("tileset", version="1.10", name=pathlib.Path(name).stem,
                        tilewidth=str(size), tileheight=str(size),
                        tilecount=str(count), columns=str(columns))
    property_value(result, "hikari.role", "visual")
    ET.SubElement(result, "image",
                  source=relative(data["surface"], posixpath.dirname(name)),
                  width=str(width), height=str(height))

    def tile_id(tile):
        x, y = tile["x"], tile["y"]
        if x % size or y % size or not (0 <= x <= width - size and 0 <= y <= height - size):
            raise ValueError(f"Invalid tile rectangle in {name}: {tile}")
        return x // size + y // size * columns

    animations = {}
    definitions = {}
    for tile in data["tiles"]:
        index = tile_id(tile)
        definition = tile.get("animation")
        if index in definitions and definitions[index] != definition:
            raise ValueError(f"Conflicting static/animated definitions for atlas cell in {name}")
        definitions[index] = definition
        if "animation" in tile:
            animation = tile["animation"]
            if not animation.get("repeat") or animation.get("keyframe", 0) != 0:
                raise ValueError(f"Unsupported legacy animation in {name}")
            if any(frame["width"] != size or frame["height"] != size
                   for frame in animation["frames"]):
                raise ValueError(f"Non-tile-sized animation frame in {name}")
            frames = [(tile_id(frame), milliseconds(frame["length"])) for frame in animation["frames"]]
            if index in animations and animations[index] != frames:
                raise ValueError(f"Conflicting animations for atlas cell in {name}")
            animations[index] = frames
    for index, frames in sorted(animations.items()):
        animation = ET.SubElement(ET.SubElement(result, "tile", id=str(index)),
                                  "animation")
        for index, duration in frames:
            ET.SubElement(animation, "frame", tileid=str(index), duration=str(duration))
    write_xml(output / pathlib.Path(name).with_suffix(".tsx"), result)
    return [tile_id(tile) for tile in data["tiles"]], count


def write_palette(output, masks):
    size, columns = 16, 8
    rows = (len(masks) + columns - 1) // columns
    width, height = columns * size, rows * size
    pixels = bytearray()
    for y in range(height):
        pixels.append(0)
        for x in range(width):
            index = x // size + y // size * columns
            mask = masks[index] if index < len(masks) else 0
            # Original swatches show each of the six gameplay flags as a stripe.
            bit = (1, 2, 4, 8, 16, 256)[min(5, (x % size) * 6 // size)]
            border = x % size in (0, 15) or y % size in (0, 15)
            color = ((240, 190, 70, 255) if mask & bit else (36, 46, 64, 255))
            pixels.extend((12, 16, 24, 255) if border else color)

    def chunk(kind, value):
        return (struct.pack(">I", len(value)) + kind + value +
                struct.pack(">I", zlib.crc32(kind + value)))

    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
           chunk(b"IDAT", zlib.compress(pixels)) + chunk(b"IEND", b""))
    image = output / "assets" / "images" / "collision-palette.png"
    image.parent.mkdir(parents=True, exist_ok=True)
    image.write_bytes(png)
    tileset = ET.Element("tileset", version="1.10", name="collision-palette",
                         tilewidth=str(size), tileheight=str(size),
                         tilecount=str(len(masks)), columns=str(columns))
    property_value(tileset, "hikari.role", "attributes")
    ET.SubElement(tileset, "image", source="../images/collision-palette.png",
                  width=str(width), height=str(height))
    for index, mask in enumerate(masks):
        tile = ET.SubElement(tileset, "tile", id=str(index))
        property_value(tile, "attributes", mask)
    write_xml(output / "assets" / "tilesets" / "collision-palette.tsx", tileset)


def convert_map(name, data, tile_ids, tile_count, masks, output):
    grid = data["gridsize"]
    rooms = data["rooms"]
    if [room["id"] for room in rooms] != list(range(len(rooms))):
        raise ValueError(f"Noncontiguous room IDs in {name}")
    width = max(room["x"] + room["width"] for room in rooms)
    height = max(room["y"] + room["height"] for room in rooms)
    root = ET.Element("map", version="1.10", orientation="orthogonal",
                      renderorder="right-down", width=str(width), height=str(height),
                      tilewidth=str(grid), tileheight=str(grid), infinite="0")
    for key in ("musicName", "bossEntity", "musicId"):
        if key in data:
            property_value(root, key, data[key])
    for key in ("starting", "midpoint", "bossCorridor", "bossChamber"):
        target = data.get("specialRooms", {}).get(key, 0)
        if target < 0 or target >= len(rooms):
            if key not in ("bossCorridor", "bossChamber"):
                raise ValueError(f"Invalid required special room {key} in {name}")
            print(f"{name}: preserving absent {key} (legacy index {target}) as an unset object reference")
            reference = 0
        else:
            reference = target + 1
        property_value(root, key, reference, "object")
    visual_path = str(pathlib.PurePosixPath(data["tileset"]).with_suffix(".tsx"))
    ET.SubElement(root, "tileset", firstgid="1",
                  source=relative(visual_path, posixpath.dirname(name)))
    ET.SubElement(root, "tileset", firstgid=str(tile_count + 1),
                  source="../tilesets/collision-palette.tsx")
    object_id = len(rooms) + 1
    layer_id = 0

    def layer(parent, tag, **attrs):
        nonlocal layer_id
        layer_id += 1
        return ET.SubElement(parent, tag, id=str(layer_id),
                             **{key: str(value) for key, value in attrs.items()})

    def object_value(parent, role, x, y, w=None, h=None, fixed_id=None):
        nonlocal object_id
        index = object_id if fixed_id is None else fixed_id
        if fixed_id is None:
            object_id += 1
        obj = ET.SubElement(parent, "object", id=str(index), name=role, x=str(x), y=str(y))
        if w is None:
            ET.SubElement(obj, "point")
        else:
            obj.set("width", str(w))
            obj.set("height", str(h))
        property_value(obj, "hikari.role", role)
        return obj

    for room in rooms:
        ox, oy = room["x"] * grid, room["y"] * grid
        group = layer(root, "group", name=f"Room {room['id']}", offsetx=ox, offsety=oy)
        property_value(group, "hikari.role", "room")
        property_value(group, "roomId", room["id"])
        property_value(group, "backgroundColor", f"#{room.get('backgroundColor', 0):06x}", "color")
        if "bossEntity" in room:
            property_value(group, "bossEntity", room["bossEntity"])
        for kind in ("tiles", "attributes"):
            tile_layer = layer(group, "layer", name=kind.title(), width=width,
                               height=height, **({"visible": 0} if kind == "attributes" else {}))
            property_value(tile_layer, "hikari.role", kind)
            values = [0] * (width * height)
            count = room["width"] * room["height"]
            if len(room["tile"]) != count or len(room["attr"]) != count:
                raise ValueError(f"Bad room cell count in {name}")
            for y in range(room["height"]):
                for x in range(room["width"]):
                    i = x + y * room["width"]
                    old_tile, flags = room["tile"][i], room["attr"][i]
                    if old_tile < -1 or old_tile >= len(tile_ids) or flags < 0 or flags & ~0x1FF:
                        raise ValueError(f"Invalid legacy cell in {name}, room {room['id']}")
                    if kind == "tiles":
                        value = 0 if old_tile == -1 else tile_ids[old_tile] + 1
                        if value and flags & 128:
                            value |= ROTATE_CLOCKWISE
                    else:
                        mask = flags & COLLISION_BITS
                        value = masks.index(mask) + tile_count + 1 if mask else 0
                    values[x + y * width] = value
            csv = ET.SubElement(tile_layer, "data", encoding="csv")
            csv.text = "\n" + ",\n".join(
                ",".join(map(str, values[y * width:(y + 1) * width]))
                for y in range(height)) + "\n"
        objects = layer(group, "objectgroup", name="Gameplay")
        object_value(objects, "roomBounds", 0, 0, room["width"] * grid,
                     room["height"] * grid, room["id"] + 1)
        object_value(objects, "heroSpawn", room.get("heroSpawnX", 0), room.get("heroSpawnY", 0))
        camera = room["cameraBounds"]
        object_value(objects, "cameraBounds", camera["x"] * grid, camera["y"] * grid,
                     camera["width"] * grid, camera["height"] * grid)
        for kind in ("enemies", "items"):
            for spawn in room.get(kind, []):
                obj = object_value(objects, "enemy" if kind == "enemies" else "item",
                                   spawn["x"], spawn["y"])
                property_value(obj, "entity", spawn["type"])
                for key in ("direction", "spawnLimit", "spawnRate", "continuous"):
                    if key in spawn:
                        value = bool(spawn[key]) if key == "continuous" else spawn[key]
                        if key == "direction" and value not in ("None", "Up", "Right", "Down", "Left"):
                            print(f"{name}: preserving ignored legacy spawner direction {value!r} as None")
                            value = "None"
                        property_value(obj, key, value,
                                       "float" if key == "spawnRate" else None)
                for key, value in spawn.get("config", {}).items():
                    if isinstance(value, (list, dict)) or value is None:
                        raise ValueError(f"Unsupported nested enemy config in {name}")
                    property_value(obj, "config." + key, value)
        for transition in room.get("transitions", []):
            obj = object_value(objects, "transition",
                               transition["x"] * grid - ox, transition["y"] * grid - oy,
                               transition["width"] * grid, transition["height"] * grid)
            property_value(obj, "to", transition["to"] + 1, "object")
            for key in ("direction", "door", "ladderOnly"):
                if key in transition:
                    value = bool(transition[key]) if key != "direction" else transition[key]
                    if key == "direction" and value not in ("forward", "backward", "up", "down"):
                        value = "teleport"
                    property_value(obj, key, value)
        doors = room.get("doors") or {}
        if not isinstance(doors, dict):
            raise ValueError(f"Invalid legacy doors in {name}")
        for role, door in doors.items():
            object_value(objects, role + "Door", door["x"] * grid, door["y"] * grid,
                         door["width"] * grid, door["height"] * grid)
        for force in room.get("forces", []):
            # The legacy loader treats force rectangles as world pixels.
            obj = object_value(objects, "force", force["x"] - ox, force["y"] - oy,
                               force["width"], force["height"])
            for key in ("velocityX", "velocityY"):
                property_value(obj, key, force.get(key, 0), "float")
        for sequence in room.get("blockSequences", []):
            x, y = sequence["x"] * grid, sequence["y"] * grid
            obj = object_value(objects, "blockSequence", x, y,
                               sequence["width"] * grid, sequence["height"] * grid)
            property_value(obj, "sequence", ";".join(",".join(map(str, step))
                                                   for step in sequence["sequence"]))
            for key in ("interval", "maximumBlockAge", "entity", "spawnSound"):
                if key in sequence:
                    property_value(obj, key, sequence[key])
            for index, block in enumerate(sequence["blocks"]):
                child = object_value(objects, "block", x + block["x"] * grid,
                                     y + block["y"] * grid)
                property_value(child, "sequenceObject", int(obj.get("id")), "object")
                property_value(child, "blockIndex", index)
    root.set("nextlayerid", str(layer_id + 1))
    root.set("nextobjectid", str(object_id))
    write_xml(output / pathlib.Path(name).with_suffix(".tmx"), root)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=pathlib.Path, required=True,
                        help="Legacy content root (unchanged by this tool)")
    parser.add_argument("--output", type=pathlib.Path, required=True,
                        help="Destination content root")
    parser.add_argument("--baseline", type=pathlib.Path,
                        help="Optional test-only legacy regression snapshot")
    args = parser.parse_args()
    maps = {p.relative_to(args.source).as_posix(): json.loads(p.read_text())
            for p in sorted((args.source / "assets" / "stages").glob("*.json"))}
    if not maps:
        raise ValueError("No legacy stage JSON files found")
    tilesets = {name: json.loads((args.source / name).read_text())
                for name in sorted({data["tileset"] for data in maps.values()})}
    if args.baseline:
        args.baseline.parent.mkdir(parents=True, exist_ok=True)
        args.baseline.write_text(json.dumps({"maps": maps, "tilesets": tilesets},
                                          separators=(",", ":")) + "\n")
    masks = sorted({flags & COLLISION_BITS
                    for data in maps.values() for room in data["rooms"]
                    for flags in room["attr"]})
    write_palette(args.output, masks)
    converted = {name: convert_tileset(name, data, args.source, args.output)
                 for name, data in tilesets.items()}
    for name, data in maps.items():
        tile_ids, count = converted[data["tileset"]]
        convert_map(name, data, tile_ids, count, masks, args.output)
    print(f"Converted {len(maps)} stages, "
          f"{sum(len(m['rooms']) for m in maps.values())} rooms, "
          f"and {len(tilesets)} tilesets")


if __name__ == "__main__":
    main()
