#pragma once

#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <vector>

// Graphics resource management for Endless Online EGF files
// Handles loading, caching, and lifetime of Win32 bitmap resources

struct BitmapResource {
    HBITMAP bitmap = nullptr;
    int width = 0;
    int height = 0;
};

class GfxAssets {
  public:
    ~GfxAssets();

    void SetDirectory(std::filesystem::path directory);
    BitmapResource Get(int bank, int resourceId);
    const std::vector<int>& GraphicIds(int bank);
    void Clear();

  private:
    HMODULE GetBank(int bank);

    std::filesystem::path directory_;
    std::unordered_map<int, HMODULE> banks_;
    std::unordered_map<std::uint64_t, BitmapResource> bitmaps_;
    std::unordered_map<int, std::vector<int>> graphicIds_;
};
