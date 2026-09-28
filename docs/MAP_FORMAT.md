# EMF compatibility

Endless Map Studio reads and writes EO EMF maps through the project's verified parser/serializer and EOLib C data primitives. Compatibility is protected by sample round trips, edit round trips, focused model tests, and a full parse of the local map corpus.

## In-memory model

The editor preserves the EMF information it currently understands:

- map name, type, effect, music, ambient sound, availability, scrolling, and relog coordinates
- width, height, and base/fill tile
- nine graphic slots per tile (ground through top-layer variants)
- tile specs and optional warp destinations
- legacy door keys
- NPC spawns, including ID, spawn type/time, and amount
- map items, including key/chest slot, item ID, spawn time, and amount
- signs, including title length and encoded text

Dimensions and EO numeric ranges are validated before collaborative operations are accepted. Save/reload tests verify the graphic, flag, warp, entity, resize, clear, and base-tile paths exercised by the editor.

## Saving

The serializer emits a normal compatible EMF. Existing files are backed up before overwrite and replacement uses a sibling temporary file followed by an atomic replace. Backup files are ordinary EMFs under a `backups/` directory beside the map directory.

The EO EMF schema verified by this project does not contain a mapper save-comment field. Save comments are **not** stored by inventing an EMF extension or appending private bytes. Instead, the save workflow writes UTF-8 JSON metadata under:

```text
<map directory>/metadata/<map filename>.json
```

The sidecar records its format version, map filename, timestamp, exact optional comment, saved file size, collaborative status, and author display name. It contains no password, participant list, network address, or absolute map path. EMF remains authoritative and can be opened if the sidecar is absent or damaged.

## Runtime data

`data/maps/` is the repository's local EMF corpus and default map directory. `gfx/` contains the EGF resource banks used for rendering. These files are runtime/project data rather than compiled source, and they are not embedded into the executable.
