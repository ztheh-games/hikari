# Tiled stage authoring

Hikari loads native Tiled **TMX stages and external TSX tilesets directly**
through PhysicsFS. Map edits do not require recompiling the game. Restart
the game to reload edited stages; live reload is not provided.

Open any `content/assets/stages/*.tmx` in Tiled. The existing stages are examples
of the complete supported contract. Save tile layers as **CSV** in the map's
properties. Normal builds copy these assets unchanged; neither Tiled nor
Python is a build dependency.

## Supported subset

Stages are finite orthogonal maps with square tiles, right-down rendering,
one visual atlas TSX, and one collision-palette TSX. Native horizontal,
vertical, and diagonal GID transforms, atlas spacing/margin, and looping TSX
tile animations are supported. Images must have declared dimensions in TSX.

Compressed/base64 layers, infinite maps, additional visual tilesets, inline
tilesets, image-collection tilesets, image layers, tinted/blended/parallax
rendering, object rotation, tile objects, object templates, and non-rectangle
collision shapes are not supported. Unsupported gameplay data produces an
error naming the file and the relevant XML line/layer/object. Unknown
editor-only XML elements do not require a particular Tiled version.

Asset references are relative to the referring TMX/TSX file, using Tiled's
forward-slash virtual paths, including `..` within the mounted asset root.
Absolute host paths and paths escaping that root are invalid. This is also
the layout used inside PhysicsFS archives.

## Independent room groups

Each top-level group has string property `hikari.role = room` and an integer
`roomId`. IDs must be unique and contiguous from zero. Reordering or renaming
groups does not change room identity. Use native group offsets to move a room,
keeping offsets aligned to the tile grid.

Each room group contains:

| Element | `hikari.role` | Meaning |
| --- | --- | --- |
| Tile layer | `tiles` | Visual GIDs from the visual TSX |
| Tile layer | `attributes` | Independent per-cell gameplay collision masks |
| Rectangle object | `roomBounds` | The room's actual extent |
| Rectangle object | `cameraBounds` | Camera extent |
| Point object, optional | `heroSpawn` | Hero spawn relative to room bounds; defaults to `(0,0)` |

The two tile layers use the **map's full dimensions**, as finite Tiled layers
require. Only the region inside `roomBounds` belongs to that room. Keep cells
outside those bounds empty. This padding is intentional: smaller per-room
finite layers would be resized by Tiled on save. Overlapping rooms must keep
their own layers; flattening them would destroy differing seam/collision data.

Room groups and child object/tile layers can have native pixel offsets.
Ordinary nested groups can organize a room's layers, but cannot introduce a
second room role. Geometry is translated consistently through these offsets.
Hidden collision and object layers still participate in gameplay: visibility
is useful for editor organization, not for deleting gameplay definitions.

Group properties `backgroundColor` (opaque Tiled color) and `bossEntity`
(string) default to black and `None`. A map `backgroundcolor` can provide the
default room color. Native tile transforms are independent of collision flags.

## Tiles and collision attributes

The visual TSX has string property `hikari.role = visual`. Atlas IDs are native
row-major Tiled IDs, not legacy JSON array indexes. GID zero is an empty cell;
the atlas's first actual tile is not empty.

`collision-palette.tsx` has `hikari.role = attributes`. Each usable palette tile
defines integer property `attributes`, combining:

| Flag | Value |
| --- | --- |
| Solid | 1 |
| Ladder | 2 |
| Platform | 4 |
| Spike | 8 |
| Abyss | 16 |
| Water | 256 |

Paint this palette on the room's `attributes` layer independently of visual
art. An empty cell has no collision attributes. Do not flip palette tiles.
The supplied original swatches show these six flags as vertical stripes;
combined masks retain multiple flags. To add a combination, assign the
corresponding integer mask to a palette tile in Tiled.

The palette image is for the editor only; the collision reader does not need
to load it. Visual atlas images must exist at runtime. Invalid GIDs, undefined
palette masks, truncated CSV, and missing atlases fail rather than substituting
tile zero or dropping collision data.

## Stage metadata and room references

Map properties `musicName` and `bossEntity` are strings defaulting to `None`.
Legacy `musicId` remains informational; it does not override `musicName`.

Map properties `starting`, `midpoint`, `bossCorridor`, and `bossChamber` have
Tiled **object** type and reference the desired `roomBounds` object. Omitted
special-room references default to room zero. Explicitly unset (zero)
`bossCorridor` or `bossChamber` references disable those optional rooms;
this preserves stages whose legacy boss-room indexes had no target.
Starting/midpoint references default to room zero when unset. A transition's `to`
property uses the same object-reference mechanism and must resolve to a room.
Tiled can renumber object IDs when exporting; native references preserve the
relationships. Do not copy numeric IDs as ordinary integer properties.

## Gameplay object properties

Identify objects with string `hikari.role`; names are only editor labels.
All positions and sizes are native pixel geometry. Bounds, cameras, doors,
transitions, block-sequence bounds, and block points must align with the tile
grid; enemies, items, forces, and hero spawns can use integer pixel positions.

| Role | Shape | Additional properties |
| --- | --- | --- |
| `enemy` | Point | `entity` string, `direction` string, `spawnLimit` int, `spawnRate` float seconds, `continuous` bool, optional scalar `config.<member>` properties |
| `item` | Point | `entity` string; optional `direction` string |
| `transition` | Rectangle | `to` object reference, `direction` string, optional `door` and `ladderOnly` bools |
| `entranceDoor`, `exitDoor` | Rectangle | At most one of each per room |
| `force` | Rectangle | `velocityX`, `velocityY` float values, default zero |
| `blockSequence` | Rectangle | `sequence` string; optional `interval`, `maximumBlockAge` floats, `entity`, `spawnSound` strings |
| `block` | Point | `sequenceObject` object reference and `blockIndex` int |

Spawner directions are `None`, `Up`, `Right`, `Down`, or `Left`. Enemy defaults
are limit 1, rate 1 second, and non-continuous spawning. Limits must be positive;
rates may be zero but not negative. `config.*` properties preserve their
string/int/float/bool types when passed to Squirrel. Nested configuration and
enemy-only settings on items are not supported.

Transition directions are `forward`, `backward`, `up`, `down`, `teleport`, or
`transport`. Door/ladder flags default to false.

Disappearing blocks reference a sequence within their own room. Block indexes
are contiguous from zero. A timing string such as `0;1;2;3;4,5` means one step
per semicolon, with blocks 4 and 5 appearing together on the last step. An
empty step waits without spawning a block. References must use valid indexes.
Defaults are 1 second per step, `106/60` seconds maximum block age, entity
`Appearing Block (Red)`, and sound `Disappearing Block`.

## Animations and migration

TSX animation frames use native tile IDs and positive whole-millisecond
durations, looping through the existing `Animation`/`TileAnimator` runtime.
The migrated content intentionally rounds old fractional-millisecond lengths
to the nearest millisecond, introducing no more than 0.5 ms error per frame.
There are no custom precision properties.

The migration preserves effective legacy rendering: active 90-degree rotation
became native GID transforms, while formerly ignored horizontal/vertical
attribute bits were not suddenly enabled. Gameplay masks remain unchanged.
Legacy tile IDs were remapped by their resolved image rectangles, including
duplicate equivalent rectangles.

See [the migration utility](../tools/map-converter/README.md) for old content.
The game does not load custom JSON maps or tilesets. JSON is still used for
unrelated game configuration, entity templates, and sprite animations.
