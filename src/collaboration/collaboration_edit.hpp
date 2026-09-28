#pragma once

#include "collaboration_protocol.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace collaboration {

enum class EditKind : std::uint8_t {
    Graphics = 1,
    Flags = 2,
    BaseTile = 3,
    ResizeMap = 4,
    ClearGraphicLayer = 5,
    ClearFlagCategory = 6,
    ClearAll = 7,
    Entities = 8
};

struct GraphicEdit {
    std::uint16_t x = 0, y = 0;
    std::uint8_t layer = 0;
    std::int32_t graphic = -1;
    bool operator==(const GraphicEdit&) const = default;
};
struct WarpValue {
    std::uint16_t destinationMap = 0;
    std::uint8_t x = 0, y = 0, level = 0;
    std::uint16_t door = 0;
    bool operator==(const WarpValue&) const = default;
};
struct FlagEdit {
    std::uint16_t x = 0, y = 0;
    std::int16_t spec = -1;
    bool hasWarp = false;
    WarpValue warp;
    bool operator==(const FlagEdit&) const = default;
};
enum EntityScope : std::uint8_t { EntityWarp = 1, EntitySign = 2, EntityNpcs = 4, EntityItems = 8 };
struct SignValue {
    std::uint16_t titleLength = 0;
    std::vector<std::uint8_t> encodedText;
    bool operator==(const SignValue&) const = default;
};
struct NpcValue {
    std::uint16_t id = 0, spawnTime = 0;
    std::uint8_t spawnType = 0, amount = 0;
    bool operator==(const NpcValue&) const = default;
};
struct ItemValue {
    std::uint16_t key = 0, id = 0, spawnTime = 0;
    std::uint8_t chestSlot = 0;
    std::uint32_t amount = 0;
    bool operator==(const ItemValue&) const = default;
};
struct EntityEdit {
    std::uint16_t x = 0, y = 0;
    std::uint8_t scopes = 0;
    std::uint64_t warpGeneration = 0, signGeneration = 0, npcGeneration = 0, itemGeneration = 0;
    bool hasWarp = false, hasSign = false;
    WarpValue warp;
    SignValue sign;
    std::vector<NpcValue> npcs;
    std::vector<ItemValue> items;
    bool operator==(const EntityEdit&) const = default;
};
struct EditOperation {
    std::uint64_t requestId = 0;
    EditKind kind = EditKind::Graphics;
    std::vector<GraphicEdit> graphics;
    std::vector<FlagEdit> flags;
    std::int32_t baseTile = -1;
    std::uint16_t width = 0, height = 0;
    std::uint8_t target = 0;
    std::vector<EntityEdit> entities;
    bool operator==(const EditOperation&) const = default;
};

std::vector<std::uint8_t> EncodeEditOperation(const EditOperation& operation);
bool DecodeEditOperation(std::span<const std::uint8_t> bytes, EditOperation& operation, std::string& error);
bool ValidateEditOperation(const EditOperation& operation, std::uint16_t width, std::uint16_t height,
                           std::string& error);

} // namespace collaboration
