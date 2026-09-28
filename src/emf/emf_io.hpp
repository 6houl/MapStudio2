#pragma once

#include "model/map_document.hpp"
#include <cstdint>
#include <string>
#include <vector>

// EMF map file I/O functions
// Preserves exact binary compatibility with Endless Online EMF format

namespace emf {

// Read an EMF map from bytes
MapDocument ReadEmfBytes(const std::vector<std::uint8_t>& bytes, const std::string& path = {});

// Write an EMF map to bytes
std::vector<std::uint8_t> WriteEmf(const MapDocument& map);

// Calculate CRC32 checksum (used for EMF hash and fingerprinting)
std::uint32_t CalculateCrc32(const std::vector<std::uint8_t>& bytes);

} // namespace emf
