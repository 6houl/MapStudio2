#pragma once

#include "../model/map_document.hpp"
#include <cstdint>
#include <vector>

// Core map editing operations
//
// This module provides pure map mutation functions that operate on MapDocument.
// These operations are reusable by both local editing and collaborative editing.
//
// Application concerns (history, dirty-state, UI refresh, collaboration networking)
// are handled by callers, not by these functions.

namespace map_operations {

// Resize the map, preserving existing tile data where possible.
// Returns true if the resize was performed, false if dimensions are already correct.
// Tiles outside the new bounds are discarded.
// New tiles are initialized with fillTile on layer 0, -1 on other layers.
bool ResizeMap(MapDocument& map, int newWidth, int newHeight);

// Set a graphic ID on a specific tile layer.
// No bounds checking - caller must ensure coordinates are valid.
void SetTileGraphic(MapDocument& map, int x, int y, int layer, int graphic);

// Set multiple tile graphics from a batch of edits.
// Each edit specifies x, y, layer, and graphic ID.
struct GraphicEdit {
    std::uint16_t x;
    std::uint16_t y;
    std::uint8_t layer;
    int graphic;
};
void SetTileGraphics(MapDocument& map, const std::vector<GraphicEdit>& edits);

// Set tile spec and warp on a specific tile.
// No bounds checking - caller must ensure coordinates are valid.
void SetTileFlag(MapDocument& map, int x, int y, int spec, const std::optional<MapWarp>& warp);

// Set multiple tile flags from a batch of edits.
struct FlagEdit {
    std::uint16_t x;
    std::uint16_t y;
    std::int16_t spec;
    bool hasWarp;
    MapWarp warp;
};
void SetTileFlags(MapDocument& map, const std::vector<FlagEdit>& edits);

// Change the base/fill tile graphic.
// Replaces all occurrences of the old fillTile (or negative values) on layer 0 with the new fillTile.
void SetBaseTile(MapDocument& map, int graphic);

// Clear a graphic layer, setting all tiles to the appropriate empty value.
// Layer 0 is set to map.fillTile, other layers are set to -1.
// Returns true if any tile was changed, false if layer was already empty.
bool ClearGraphicLayer(MapDocument& map, int layer);

// Clear a flag category, removing flags of a specific type.
// Categories: 0=banners, 1=board/chest/door, 2=jukebox, 3=chairs/edges, 4=warps
// Returns true if any tile was changed, false if category was already empty.
bool ClearFlagCategory(MapDocument& map, int category);

// Clear all map data: graphics, flags, entities.
// Layer 0 graphics are reset to fillTile, all other data is removed.
void ClearAll(MapDocument& map);

// Replace entities at a specific tile coordinate.
// Removes all existing entities at (x, y) and adds the provided ones.
// This is an atomic operation for a single tile's entities.
struct EntityReplacement {
    int x;
    int y;
    std::optional<MapWarp> warp;
    std::optional<MapSign> sign;
    std::vector<MapNpc> npcs;
    std::vector<MapItem> items;
    // Scopes bitmask: which entity types should be replaced
    // Bit 0 = warp, 1 = sign, 2 = npcs, 3 = items
    int scopes;
};
void ReplaceEntitiesAt(MapDocument& map, const EntityReplacement& replacement);

} // namespace map_operations
