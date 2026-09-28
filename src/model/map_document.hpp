#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct MapWarp {
    int destinationMap = 0;
    int x = 0;
    int y = 0;
    int level = 0;
    int door = 0;
    bool operator==(const MapWarp&) const = default;
};

struct MapTile {
    std::array<int, 9> graphics;
    int spec = -1;
    std::optional<MapWarp> warp;
    MapTile() {
        graphics.fill(-1);
    }
    bool operator==(const MapTile&) const = default;
};

struct MapLegacyDoorKey {
    int x = 0;
    int y = 0;
    int key = 0;
    bool operator==(const MapLegacyDoorKey&) const = default;
};

struct MapNpc {
    int x = 0;
    int y = 0;
    int id = 0;
    int spawnType = 0;
    int spawnTime = 0;
    int amount = 0;
    bool operator==(const MapNpc&) const = default;
};

struct MapItem {
    int x = 0;
    int y = 0;
    int key = 0;
    int chestSlot = 0;
    int id = 0;
    int spawnTime = 0;
    int amount = 0;
    bool operator==(const MapItem&) const = default;
};

struct MapSign {
    int x = 0;
    int y = 0;
    int titleLength = 0;
    std::vector<std::uint8_t> encodedText;
    bool operator==(const MapSign&) const = default;
};

struct MapDocument {
    int width = 24;
    int height = 24;
    std::string name = "Untitled";
    std::string path;
    int type = 0;
    int effect = 0;
    int musicId = 0;
    int musicControl = 0;
    int ambientSoundId = 0;
    int fillTile = 0;
    bool mapAvailable = true;
    bool canScroll = true;
    int relogX = 0;
    int relogY = 0;
    std::vector<MapNpc> npcs;
    std::vector<MapLegacyDoorKey> legacyDoorKeys;
    std::vector<MapItem> items;
    std::vector<MapSign> signs;
    std::vector<MapTile> tiles;
    bool loaded = false;
    bool dirty = false;

    MapTile& tile(int x, int y) {
        return tiles[static_cast<std::size_t>(y) * width + x];
    }
    const MapTile& tile(int x, int y) const {
        return tiles[static_cast<std::size_t>(y) * width + x];
    }
};
