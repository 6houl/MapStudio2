#include "emf_io.hpp"

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#endif

extern "C" {
#include <eolib/data.h>
}

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace emf {

namespace {

// EO library error checking helper
void CheckEoResult(EoResult result, const char* operation) {
    if (result != EO_SUCCESS) {
        throw std::runtime_error(std::string(operation) + ": " + eo_result_string(result));
    }
}

// Reader helpers
void SkipBytes(EoReader& reader, std::size_t length) {
    std::uint8_t* bytes = nullptr;
    CheckEoResult(eo_reader_get_bytes(&reader, length, &bytes), "Reading EMF header");
    free(bytes);
}

int ReadChar(EoReader& reader) {
    int32_t value = 0;
    CheckEoResult(eo_reader_get_char(&reader, &value), "Reading EMF header");
    return value;
}

int ReadShort(EoReader& reader) {
    int32_t value = 0;
    CheckEoResult(eo_reader_get_short(&reader, &value), "Reading EMF header");
    return value;
}

int ReadThree(EoReader& reader) {
    int32_t value = 0;
    CheckEoResult(eo_reader_get_three(&reader, &value), "Reading EMF data");
    return value;
}

// Legacy 0.4.x format detection
bool LooksLike04xFormat(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() <= 0x2d) {
        return false;
    }

    double score = 0.0;
    const int type = static_cast<int>(bytes[0x1f]) - 1;
    const int effect = static_cast<int>(bytes[0x20]) - 1;
    const int musicControl = static_cast<int>(bytes[0x22]) - 1;
    const int ambientSoundSecondByte = bytes[0x24];
    const int height = bytes[0x26];
    const int zeroField = bytes[0x2d];

    if (type > 3)
        score += 0.2;
    if (effect > 6)
        score += 0.4;
    if (musicControl > 6)
        score += 0.4;
    if (ambientSoundSecondByte != 0xfe)
        score += ambientSoundSecondByte == 0x01 ? 0.5 : 0.9;
    if (height > 252)
        score += 1.0;
    if (zeroField != 1)
        score += zeroField > 252 ? 1.0 : 0.5;
    return score >= 1.0;
}

// Read a warp from the EMF data
MapWarp ReadWarp(EoReader& reader) {
    MapWarp warp;
    warp.destinationMap = ReadShort(reader);
    warp.x = ReadChar(reader);
    warp.y = ReadChar(reader);
    warp.level = ReadChar(reader);
    warp.door = ReadShort(reader);
    return warp;
}

// Writer helpers
void AddByte(EoWriter& writer, std::uint8_t value) {
    CheckEoResult(eo_writer_add_byte(&writer, value), "Writing EMF data");
}

void AddChar(EoWriter& writer, int value) {
    CheckEoResult(eo_writer_add_char(&writer, value), "Writing EMF data");
}

void AddShort(EoWriter& writer, int value) {
    CheckEoResult(eo_writer_add_short(&writer, value), "Writing EMF data");
}

void AddThree(EoWriter& writer, int value) {
    CheckEoResult(eo_writer_add_three(&writer, value), "Writing EMF data");
}

void AddWarp(EoWriter& writer, const MapWarp& warp) {
    AddShort(writer, warp.destinationMap);
    AddChar(writer, warp.x);
    AddChar(writer, warp.y);
    AddChar(writer, warp.level);
    AddShort(writer, warp.door);
}

// Row encoding helper type
using TileRow = std::pair<int, std::vector<int>>;

// Build sparse tile rows based on a predicate
template <typename Predicate> std::vector<TileRow> BuildTileRows(const MapDocument& map, Predicate predicate) {
    std::vector<TileRow> rows;
    for (int y = 0; y < map.height; ++y) {
        std::vector<int> columns;
        for (int x = 0; x < map.width; ++x) {
            if (predicate(map.tile(x, y), x, y)) {
                columns.push_back(x);
            }
        }
        if (!columns.empty()) {
            rows.emplace_back(y, std::move(columns));
        }
    }
    return rows;
}

// RAII wrapper for EoWriter
struct EoWriterOwner {
    EoWriter writer = eo_writer_init();
    ~EoWriterOwner() {
        eo_writer_free(&writer);
    }
};

} // anonymous namespace

// Public API implementation

std::uint32_t CalculateCrc32(const std::vector<std::uint8_t>& bytes) {
    std::uint32_t crc = 0xffffffff;
    for (std::uint8_t byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
        }
    }
    return ~crc;
}

MapDocument ReadEmfBytes(const std::vector<std::uint8_t>& bytes, const std::string& path) {
    if (bytes.size() < 48) {
        throw std::runtime_error("The file is too small to be a valid EMF map.");
    }

    EoReader reader = eo_reader_init(bytes.data(), bytes.size());
    char* signature = nullptr;
    CheckEoResult(eo_reader_get_fixed_string(&reader, 3, &signature), "Reading EMF signature");
    const bool validSignature = signature != nullptr && std::string(signature, 3) == "EMF";
    free(signature);
    if (!validSignature) {
        throw std::runtime_error("Invalid EMF file signature.");
    }
    if (LooksLike04xFormat(bytes)) {
        throw std::runtime_error("Legacy 0.4.x EMF files are not supported.");
    }

    MapDocument map;
    SkipBytes(reader, 4);
    char* encodedName = nullptr;
    CheckEoResult(eo_reader_get_fixed_encoded_string(&reader, 24, &encodedName), "Reading EMF name");
    map.name = encodedName && encodedName[0] ? encodedName : "Untitled";
    while (!map.name.empty() && (static_cast<unsigned char>(map.name.back()) == 0xff || map.name.back() == '\0')) {
        map.name.pop_back();
    }
    if (map.name.empty())
        map.name = "Untitled";
    free(encodedName);

    map.type = ReadChar(reader);
    map.effect = ReadChar(reader);
    map.musicId = ReadChar(reader);
    map.musicControl = ReadChar(reader);
    map.ambientSoundId = ReadShort(reader);
    map.width = ReadChar(reader) + 1;
    map.height = ReadChar(reader) + 1;
    map.fillTile = ReadShort(reader);
    map.mapAvailable = ReadChar(reader) != 0;
    map.canScroll = ReadChar(reader) != 0;
    map.relogX = ReadChar(reader);
    map.relogY = ReadChar(reader);
    SkipBytes(reader, 1);

    if (map.width < 1 || map.width > EO_CHAR_MAX + 1 || map.height < 1 || map.height > EO_CHAR_MAX + 1) {
        throw std::runtime_error("The EMF map dimensions are outside the supported range.");
    }

    const int npcCount = ReadChar(reader);
    map.npcs.reserve(npcCount);
    for (int index = 0; index < npcCount; ++index) {
        MapNpc npc;
        npc.x = ReadChar(reader);
        npc.y = ReadChar(reader);
        npc.id = ReadShort(reader);
        npc.spawnType = ReadChar(reader);
        npc.spawnTime = ReadShort(reader);
        npc.amount = ReadChar(reader);
        map.npcs.push_back(npc);
    }

    const int legacyDoorKeyCount = ReadChar(reader);
    map.legacyDoorKeys.reserve(legacyDoorKeyCount);
    for (int index = 0; index < legacyDoorKeyCount; ++index) {
        MapLegacyDoorKey key;
        key.x = ReadChar(reader);
        key.y = ReadChar(reader);
        key.key = ReadShort(reader);
        map.legacyDoorKeys.push_back(key);
    }

    const int itemCount = ReadChar(reader);
    map.items.reserve(itemCount);
    for (int index = 0; index < itemCount; ++index) {
        MapItem item;
        item.x = ReadChar(reader);
        item.y = ReadChar(reader);
        item.key = ReadShort(reader);
        item.chestSlot = ReadChar(reader);
        item.id = ReadShort(reader);
        item.spawnTime = ReadShort(reader);
        item.amount = ReadThree(reader);
        map.items.push_back(item);
    }

    map.tiles.resize(static_cast<std::size_t>(map.width) * map.height);
    for (MapTile& tile : map.tiles) {
        tile.graphics[0] = map.fillTile;
    }

    const int specRowCount = ReadChar(reader);
    for (int row = 0; row < specRowCount; ++row) {
        const int y = ReadChar(reader);
        const int tileCount = ReadChar(reader);
        for (int tileIndex = 0; tileIndex < tileCount; ++tileIndex) {
            const int x = ReadChar(reader);
            const int spec = ReadChar(reader);
            if (x < map.width && y < map.height) {
                map.tile(x, y).spec = spec;
            }
        }
    }

    const int warpRowCount = ReadChar(reader);
    for (int row = 0; row < warpRowCount; ++row) {
        const int y = ReadChar(reader);
        const int tileCount = ReadChar(reader);
        for (int tileIndex = 0; tileIndex < tileCount; ++tileIndex) {
            const int x = ReadChar(reader);
            MapWarp warp = ReadWarp(reader);
            if (x < map.width && y < map.height) {
                map.tile(x, y).warp = warp;
            }
        }
    }

    for (int layer = 0; layer < 9; ++layer) {
        const int rowCount = ReadChar(reader);
        for (int row = 0; row < rowCount; ++row) {
            const int y = ReadChar(reader);
            const int tileCount = ReadChar(reader);
            for (int tileIndex = 0; tileIndex < tileCount; ++tileIndex) {
                const int x = ReadChar(reader);
                const int graphic = ReadShort(reader);
                if (x < map.width && y < map.height && (layer == 0 || graphic != 0)) {
                    map.tile(x, y).graphics[layer] = graphic;
                }
            }
        }
    }

    if (eo_reader_remaining(&reader) > 0) {
        const int signCount = ReadChar(reader);
        map.signs.reserve(signCount);
        for (int index = 0; index < signCount; ++index) {
            MapSign sign;
            sign.x = ReadChar(reader);
            sign.y = ReadChar(reader);
            const int stringLength = ReadShort(reader) - 1;
            if (stringLength < 0 || static_cast<std::size_t>(stringLength) > eo_reader_remaining(&reader)) {
                throw std::runtime_error("Invalid sign text length in EMF file.");
            }
            std::uint8_t* text = nullptr;
            CheckEoResult(eo_reader_get_bytes(&reader, static_cast<std::size_t>(stringLength), &text),
                          "Reading EMF sign");
            if (stringLength > 0) {
                sign.encodedText.assign(text, text + stringLength);
            }
            free(text);
            sign.titleLength = ReadChar(reader);
            map.signs.push_back(std::move(sign));
        }
    }

    map.path = path;
    if (map.name == "Untitled" && !path.empty()) {
        const std::size_t separator = path.find_last_of("\\/");
        map.name = path.substr(separator == std::string::npos ? 0 : separator + 1);
    }
    map.loaded = true;
    return map;
}

std::vector<std::uint8_t> WriteEmf(const MapDocument& map) {
    EoWriterOwner owner;
    EoWriter& writer = owner.writer;
    const std::uint8_t signature[] = {'E', 'M', 'F'};
    CheckEoResult(eo_writer_add_bytes(&writer, signature, sizeof(signature)), "Writing EMF signature");
    const std::size_t hashPosition = writer.length;
    AddShort(writer, 0);
    AddShort(writer, 0);
    CheckEoResult(eo_writer_add_fixed_encoded_string(&writer, map.name.c_str(), 24, true), "Writing EMF name");
    AddChar(writer, map.type);
    AddChar(writer, map.effect);
    AddChar(writer, map.musicId);
    AddChar(writer, map.musicControl);
    AddShort(writer, map.ambientSoundId);
    AddChar(writer, map.width - 1);
    AddChar(writer, map.height - 1);
    AddShort(writer, map.fillTile);
    AddChar(writer, map.mapAvailable ? 1 : 0);
    AddChar(writer, map.canScroll ? 1 : 0);
    AddChar(writer, map.relogX);
    AddChar(writer, map.relogY);
    AddChar(writer, 0);

    AddChar(writer, static_cast<int>(map.npcs.size()));
    for (const MapNpc& npc : map.npcs) {
        AddChar(writer, npc.x);
        AddChar(writer, npc.y);
        AddShort(writer, npc.id);
        AddChar(writer, npc.spawnType);
        AddShort(writer, npc.spawnTime);
        AddChar(writer, npc.amount);
    }

    AddChar(writer, static_cast<int>(map.legacyDoorKeys.size()));
    for (const MapLegacyDoorKey& key : map.legacyDoorKeys) {
        AddChar(writer, key.x);
        AddChar(writer, key.y);
        AddShort(writer, key.key);
    }

    AddChar(writer, static_cast<int>(map.items.size()));
    for (const MapItem& item : map.items) {
        AddChar(writer, item.x);
        AddChar(writer, item.y);
        AddShort(writer, item.key);
        AddChar(writer, item.chestSlot);
        AddShort(writer, item.id);
        AddShort(writer, item.spawnTime);
        AddThree(writer, item.amount);
    }

    const auto specRows = BuildTileRows(map, [](const MapTile& tile, int, int) { return tile.spec >= 0; });
    AddChar(writer, static_cast<int>(specRows.size()));
    for (const auto& [y, columns] : specRows) {
        AddChar(writer, y);
        AddChar(writer, static_cast<int>(columns.size()));
        for (int x : columns) {
            AddChar(writer, x);
            AddChar(writer, map.tile(x, y).spec);
        }
    }

    const auto warpRows = BuildTileRows(map, [](const MapTile& tile, int, int) { return tile.warp.has_value(); });
    AddChar(writer, static_cast<int>(warpRows.size()));
    for (const auto& [y, columns] : warpRows) {
        AddChar(writer, y);
        AddChar(writer, static_cast<int>(columns.size()));
        for (int x : columns) {
            AddChar(writer, x);
            AddWarp(writer, *map.tile(x, y).warp);
        }
    }

    for (int layer = 0; layer < 9; ++layer) {
        const auto graphicRows = BuildTileRows(map, [layer, &map](const MapTile& tile, int, int) {
            const int graphic = tile.graphics[layer];
            return graphic >= 0 && (layer == 0 ? graphic != map.fillTile : graphic != 0);
        });
        AddChar(writer, static_cast<int>(graphicRows.size()));
        for (const auto& [y, columns] : graphicRows) {
            AddChar(writer, y);
            AddChar(writer, static_cast<int>(columns.size()));
            for (int x : columns) {
                AddChar(writer, x);
                AddShort(writer, map.tile(x, y).graphics[layer]);
            }
        }
    }

    if (!map.signs.empty()) {
        AddChar(writer, static_cast<int>(map.signs.size()));
        for (const MapSign& sign : map.signs) {
            AddChar(writer, sign.x);
            AddChar(writer, sign.y);
            AddShort(writer, static_cast<int>(sign.encodedText.size()) + 1);
            if (!sign.encodedText.empty()) {
                CheckEoResult(eo_writer_add_bytes(&writer, sign.encodedText.data(), sign.encodedText.size()),
                              "Writing EMF sign");
            }
            AddChar(writer, sign.titleLength);
        }
    }

    std::vector<std::uint8_t> result(writer.data, writer.data + writer.length);
    const std::uint32_t hash = CalculateCrc32(result);
    const std::uint16_t hashParts[] = {
        static_cast<std::uint16_t>(hash & 0xffff),
        static_cast<std::uint16_t>(hash >> 16),
    };
    for (int index = 0; index < 2; ++index) {
        std::uint8_t encoded[4]{};
        CheckEoResult(eo_encode_number(hashParts[index], encoded), "Encoding EMF hash");
        result[hashPosition + index * 2] = encoded[0];
        result[hashPosition + index * 2 + 1] = encoded[1];
    }
    return result;
}

} // namespace emf
