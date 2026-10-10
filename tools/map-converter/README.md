# Legacy JSON to Tiled migration

`json-to-tiled.py` converts a legacy Hikari content tree to native TMX stages
and TSX tilesets. It is a one-time migration utility, **not** a runtime loader,
Tiled export step, or normal build dependency. It needs Python 3.9 or newer
and only uses the standard library.

```powershell
python .\tools\map-converter\json-to-tiled.py `
  --source C:\legacy-hikari\content `
  --output C:\migrated-hikari\content
```

The source and destination can be the same directory; source JSON is never
removed by the utility. Existing destination TMX/TSX files are overwritten.
The destination must also contain the source images, scripts, and other
non-map assets; this tool does not copy them.

The converter remaps arbitrary legacy tile rectangles to native atlas IDs,
preserves independent overlapping rooms and gameplay collision flags, creates
an original collision-palette image, and exports gameplay objects with typed
properties. It converts effective legacy 90-degree rotation to native Tiled
GID flags without enabling the old renderer's ignored flip flags.
Dangling legacy optional boss-room indexes become explicit unset object
references, preserving their existing absence rather than selecting room zero.
Boolean properties use the legacy reader's truthiness, and ignored direction
strings become `None`, preserving observed gameplay rather than repairing
old content values during conversion.

Animation lengths are rounded to whole milliseconds, with positive half
milliseconds rounded upward and at most 0.5 ms error per frame. Invalid
rectangles, conflicting atlas animations, unsupported nested instance
configuration, and unrepresentable animation lengths fail explicitly.

`--baseline <path>` captures compact test-only legacy expectations before
conversion. The committed regression snapshot is under
`tests/fixtures/tiled`; it is not loaded by the game.

For normal editing, open the stage TMX directly in Tiled and follow
[the authoring reference](../../docs/tiled-maps.md). The old TMX-to-custom-JSON
prototype and its Node.js dependencies have been retired.
