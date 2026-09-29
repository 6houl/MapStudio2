#include "map_operations.hpp"
#include <algorithm>
#include <cmath>

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#endif
extern "C" {
#include <eolib/data.h>
}

namespace map_operations {

bool ResizeMap(MapDocument& map, int newWidth, int newHeight) {
    newWidth = std::clamp(newWidth, 1, EO_CHAR_MAX + 1);
    newHeight = std::clamp(newHeight, 1, EO_CHAR_MAX + 1);
    if (newWidth == map.width && newHeight == map.height)
        return false;

    std::vector<MapTile> tiles(static_cast<std::size_t>(newWidth) * newHeight);
    for (MapTile& tile : tiles)
        tile.graphics[0] = map.fillTile;
    for (int y = 0; y < std::min(newHeight, map.height); ++y) {
        for (int x = 0; x < std::min(newWidth, map.width); ++x) {
            tiles[static_cast<std::size_t>(y) * newWidth + x] = map.tile(x, y);
        }
    }
    map.width = newWidth;
    map.height = newHeight;
    map.tiles = std::move(tiles);
    std::erase_if(map.npcs, [newWidth, newHeight](const MapNpc& entity) {
        return entity.x >= newWidth || entity.y >= newHeight;
    });
    std::erase_if(map.items, [newWidth, newHeight](const MapItem& entity) {
        return entity.x >= newWidth || entity.y >= newHeight;
    });
    std::erase_if(map.legacyDoorKeys, [newWidth, newHeight](const MapLegacyDoorKey& entity) {
        return entity.x >= newWidth || entity.y >= newHeight;
    });
    std::erase_if(map.signs, [newWidth, newHeight](const MapSign& entity) {
        return entity.x >= newWidth || entity.y >= newHeight;
    });
    return true;
}

void SetTileGraphic(MapDocument& map, int x, int y, int layer, int graphic) {
    map.tile(x, y).graphics[layer] = graphic;
}

void SetTileGraphics(MapDocument& map, const std::vector<GraphicEdit>& edits) {
    for (const auto& edit : edits)
        map.tile(edit.x, edit.y).graphics[edit.layer] = edit.graphic;
}

void SetTileFlag(MapDocument& map, int x, int y, int spec, const std::optional<MapWarp>& warp) {
    MapTile& tile = map.tile(x, y);
    tile.spec = spec;
    tile.warp = warp;
}

void SetTileFlags(MapDocument& map, const std::vector<FlagEdit>& edits) {
    for (const auto& edit : edits) {
        MapTile& tile = map.tile(edit.x, edit.y);
        tile.spec = edit.spec;
        if (edit.hasWarp)
            tile.warp = edit.warp;
        else
            tile.warp.reset();
    }
}

void SetBaseTile(MapDocument& map, int graphic) {
    const int oldFill = map.fillTile;
    map.fillTile = graphic;
    for (MapTile& tile : map.tiles) {
        if (tile.graphics[0] == oldFill || tile.graphics[0] < 0)
            tile.graphics[0] = map.fillTile;
    }
}

bool ClearGraphicLayer(MapDocument& map, int layer) {
    if (layer < 0 || layer >= 9)
        return false;
    const int emptyValue = layer == 0 ? map.fillTile : -1;
    bool changed = false;
    for (MapTile& tile : map.tiles) {
        if (tile.graphics[layer] != emptyValue) {
            tile.graphics[layer] = emptyValue;
            changed = true;
        }
    }
    return changed;
}

bool ClearFlagCategory(MapDocument& map, int category) {
    if (category < 0 || category >= 5)
        return false;
    bool changed = false;
    for (MapTile& tile : map.tiles) {
        switch (category) {
        case 0:
            if (tile.spec == 0) {
                tile.spec = -1;
                changed = true;
            }
            break;
        case 1:
            if (tile.warp && tile.warp->door > 0) {
                tile.warp.reset();
                changed = true;
            }
            break;
        case 2:
            if (tile.spec == 9) {
                tile.spec = -1;
                changed = true;
            }
            break;
        case 3:
            if (tile.spec >= 1 && tile.spec <= 7) {
                tile.spec = -1;
                changed = true;
            }
            break;
        case 4:
            if (tile.warp && tile.warp->door == 0) {
                tile.warp.reset();
                changed = true;
            }
            break;
        }
    }
    return changed;
}

void ClearAll(MapDocument& map) {
    for (MapTile& tile : map.tiles) {
        tile = MapTile{};
        tile.graphics[0] = map.fillTile;
    }
    map.npcs.clear();
    map.items.clear();
    map.legacyDoorKeys.clear();
    map.signs.clear();
}

void ReplaceEntitiesAt(MapDocument& map, const EntityReplacement& replacement) {
    const int x = replacement.x;
    const int y = replacement.y;

    if (replacement.scopes & 0x01) {
        if (replacement.warp)
            map.tile(x, y).warp = replacement.warp;
        else
            map.tile(x, y).warp.reset();
    }

    if (replacement.scopes & 0x02) {
        std::erase_if(map.signs, [=](const MapSign& v) { return v.x == x && v.y == y; });
        if (replacement.sign)
            map.signs.push_back(*replacement.sign);
    }

    if (replacement.scopes & 0x04) {
        std::erase_if(map.npcs, [=](const MapNpc& v) { return v.x == x && v.y == y; });
        for (const auto& npc : replacement.npcs)
            map.npcs.push_back(npc);
    }

    if (replacement.scopes & 0x08) {
        std::erase_if(map.items, [=](const MapItem& v) { return v.x == x && v.y == y; });
        for (const auto& item : replacement.items)
            map.items.push_back(item);
    }
}

} // namespace map_operations
