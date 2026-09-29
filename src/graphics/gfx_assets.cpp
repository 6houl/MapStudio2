#include "gfx_assets.hpp"
#include <algorithm>

GfxAssets::~GfxAssets() {
    Clear();
}

void GfxAssets::SetDirectory(std::filesystem::path directory) {
    directory_ = std::move(directory);
}

BitmapResource GfxAssets::Get(int bank, int resourceId) {
    const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(bank)) << 32) |
                              static_cast<std::uint32_t>(resourceId);
    const auto existing = bitmaps_.find(key);
    if (existing != bitmaps_.end()) {
        return existing->second;
    }

    HMODULE module = GetBank(bank);
    BitmapResource resource;
    if (module && resourceId > 0 && resourceId <= 0xffff) {
        resource.bitmap = static_cast<HBITMAP>(
            LoadImageW(module, MAKEINTRESOURCEW(resourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (resource.bitmap) {
            BITMAP bitmap{};
            if (GetObjectW(resource.bitmap, sizeof(bitmap), &bitmap) == sizeof(bitmap)) {
                resource.width = bitmap.bmWidth;
                resource.height = bitmap.bmHeight;
            } else {
                DeleteObject(resource.bitmap);
                resource.bitmap = nullptr;
            }
        }
    }
    bitmaps_.emplace(key, resource);
    return resource;
}

const std::vector<int>& GfxAssets::GraphicIds(int bank) {
    const auto existing = graphicIds_.find(bank);
    if (existing != graphicIds_.end()) {
        return existing->second;
    }

    std::vector<int> ids;
    HMODULE module = GetBank(bank);
    if (module) {
        EnumResourceNamesW(
            module, MAKEINTRESOURCEW(2),
            [](HMODULE, LPCWSTR, LPWSTR name, LONG_PTR parameter) -> BOOL {
                if (IS_INTRESOURCE(name)) {
                    const int resourceId = static_cast<int>(reinterpret_cast<ULONG_PTR>(name));
                    if (resourceId >= 100) {
                        reinterpret_cast<std::vector<int>*>(parameter)->push_back(resourceId - 100);
                    }
                }
                return TRUE;
            },
            reinterpret_cast<LONG_PTR>(&ids));
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return graphicIds_.emplace(bank, std::move(ids)).first->second;
}

void GfxAssets::Clear() {
    for (const auto& [key, resource] : bitmaps_) {
        if (resource.bitmap) {
            DeleteObject(resource.bitmap);
        }
    }
    bitmaps_.clear();
    for (const auto& [bank, module] : banks_) {
        if (module) {
            FreeLibrary(module);
        }
    }
    banks_.clear();
    graphicIds_.clear();
}

HMODULE GfxAssets::GetBank(int bank) {
    const auto existing = banks_.find(bank);
    if (existing != banks_.end()) {
        return existing->second;
    }
    wchar_t filename[32]{};
    swprintf_s(filename, L"gfx%03d.egf", bank);
    const std::filesystem::path path = directory_ / filename;
    HMODULE module = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    banks_.emplace(bank, module);
    return module;
}
