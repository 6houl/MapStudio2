#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "classic_ui.hpp"
#include "collaboration/collaboration_edit.hpp"
#include "collaboration/collaboration_session.hpp"
#include "collaboration/collaboration_window.hpp"
#include "editor_camera.hpp"
#include "editor_state.hpp"
#include "save_workflow.hpp"

#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#endif
extern "C" {
#include <eolib/data.h>
}

namespace {

constexpr char kMainClass[] = "EndlessMapStudioMain";
constexpr char kPanelClass[] = "EndlessMapStudioPanel";
constexpr int kOpenMapCommand = 1001;
constexpr int kSaveMapCommand = 1002;
constexpr int kSaveMapAsCommand = 1003;
constexpr int kNewMapCommand = 1004;
constexpr int kClearLayerCommand = 1005;
constexpr int kClearFlagsCommand = 1006;
constexpr int kClearAllCommand = 1007;
constexpr int kToggleLayerBaseCommand = 1500;
constexpr int kToggleAllLayersCommand = 1110;
constexpr int kHideAllLayersCommand = 1111;
constexpr int kToggleGridCommand = 1112;
constexpr int kZoomInCommand = 1200;
constexpr int kZoomOutCommand = 1201;
constexpr int kZoomResetCommand = 1202;
constexpr int kPanelBaseCommand = 1400;
constexpr int kTitleHeight = 19;
constexpr int kLayerBanks[] = {3, 4, 5, 6, 6, 7, 3, 22, 5};
constexpr int kPaletteBanks[] = {3, 4, 22, 6, 3};
constexpr int kPaletteLayers[] = {0, 1, 7, 3, 4, 6};
constexpr int kGraphicsMenuBaseCommand = 1600;
constexpr int kLayerGroupMenuBaseCommand = 1700;
constexpr int kFlagMenuBaseCommand = 1800;
constexpr int kBoundaryMenuBaseCommand = 1900;
constexpr int kClearGraphicLayerBaseCommand = 2000;
constexpr int kClearFlagCategoryBaseCommand = 2010;
constexpr int kSingleEditCommand = 1324;
constexpr int kClusterEditCommand = 1325;
constexpr int kNoEditCommand = 1326;
constexpr int kGraphicsModeCommand = 1327;
constexpr int kFlagsModeCommand = 1328;
constexpr int kOverviewCommand = 1329;
constexpr int kAboutCommand = 1330;
constexpr int kCloseMapCommand = 1331;
constexpr int kExitCommand = 1332;
constexpr int kMapWidthEdit = 3001;
constexpr int kMapHeightEdit = 3002;
constexpr int kMapDimensionsChange = 3003;
constexpr int kBaseTileChange = 3004;
constexpr int kLayerGraphicsPreset = 3010;
constexpr int kLayerFlagsPreset = 3011;
constexpr int kDoorRulesCombo = 3020;
constexpr int kChestRulesCombo = 3021;
constexpr int kChairDirectionCombo = 3022;
constexpr int kWarpMapEdit = 3023;
constexpr int kWarpXEdit = 3024;
constexpr int kWarpYEdit = 3025;
constexpr int kWarpMapSpin = 3026;
constexpr int kWarpXSpin = 3027;
constexpr int kWarpYSpin = 3028;
constexpr int kToggleAllFlagsCommand = 1810;
constexpr int kHideAllFlagsCommand = 1811;
constexpr int kToggleAllBoundariesCommand = 1910;
constexpr int kHideAllBoundariesCommand = 1911;
constexpr int kEntitiesModeCommand = 1920;
constexpr int kEntityControlBase = 4000;
constexpr int kStartCollaborationCommand = 2100;
constexpr int kConnectCollaborationCommand = 2101;
constexpr int kManageCollaborationCommand = 2102;
constexpr int kDisconnectCollaborationCommand = 2103;
constexpr int kOpenCollaborationWindowCommand = 2104;
constexpr UINT kCollaborationEventMessage = WM_APP + 42;
constexpr UINT_PTR kAcceptanceTimer = 77;
constexpr double kZoomLevels[] = {0.25, 0.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 12.0, 16.0};

using EditTool = DrawTool;

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

enum class PanelKind {
    Viewer,
    Graphics,
    Layers,
    Properties,
    Flags,
    Toolset,
    Entities,
};

struct PanelData {
    PanelKind kind;
    const char* title;
    HDC backBufferDc = nullptr;
    HBITMAP backBufferBitmap = nullptr;
    HBITMAP backBufferPreviousBitmap = nullptr;
    SIZE backBufferSize{};
};

struct BitmapResource {
    HBITMAP bitmap = nullptr;
    int width = 0;
    int height = 0;
};

class GfxAssets {
  public:
    ~GfxAssets() {
        Clear();
    }

    void SetDirectory(std::filesystem::path directory) {
        directory_ = std::move(directory);
    }

    BitmapResource Get(int bank, int resourceId) {
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

    const std::vector<int>& GraphicIds(int bank) {
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

    void Clear() {
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

  private:
    HMODULE GetBank(int bank) {
        const auto existing = banks_.find(bank);
        if (existing != banks_.end()) {
            return existing->second;
        }
        wchar_t filename[32]{};
        swprintf_s(filename, L"gfx%03d.egf", bank);
        const std::filesystem::path path = directory_ / filename;
        HMODULE module = LoadLibraryExW(path.c_str(), nullptr, 0x22);
        banks_.emplace(bank, module);
        return module;
    }

    std::filesystem::path directory_;
    std::unordered_map<int, HMODULE> banks_;
    std::unordered_map<std::uint64_t, BitmapResource> bitmaps_;
    std::unordered_map<int, std::vector<int>> graphicIds_;
};

MapDocument g_map;
HWND g_mainWindow = nullptr;
HMENU g_fileMenu = nullptr;
HMENU g_viewMenu = nullptr;
HMENU g_windowsMenu = nullptr;
HMENU g_graphicsMenu = nullptr;
HMENU g_flagMenu = nullptr;
HMENU g_boundaryMenu = nullptr;
HMENU g_layerMenu = nullptr;
HMENU g_toolMenu = nullptr;
HMENU g_modeMenu = nullptr;
HMENU g_editMenu = nullptr;
HMENU g_mapTogetherMenu = nullptr;
HWND g_activePanel = nullptr;
HWND g_mapWidthEdit = nullptr;
HWND g_mapHeightEdit = nullptr;
HWND g_mapDimensionsButton = nullptr;
HWND g_baseTileButton = nullptr;
HWND g_layerGraphicsCombo = nullptr;
HWND g_layerFlagsCombo = nullptr;
HWND g_doorRulesCombo = nullptr;
HWND g_chestRulesCombo = nullptr;
HWND g_chairDirectionCombo = nullptr;
HWND g_warpMapEdit = nullptr;
HWND g_warpXEdit = nullptr;
HWND g_warpYEdit = nullptr;
HWND g_warpMapSpin = nullptr;
HWND g_warpXSpin = nullptr;
HWND g_warpYSpin = nullptr;
std::vector<HWND> g_panels;
GfxAssets g_gfx;
HFONT g_uiFont = nullptr;
int g_cursorX = 8;
int g_cursorY = 14;
EditorCamera g_camera;
int& g_viewCenterX = g_camera.centerTileX;
int& g_viewCenterY = g_camera.centerTileY;
EditorState g_editorState;
int& g_paletteCategory = g_editorState.graphicsCategory;
int g_palettePage = 0;
int& g_selectedLayer = g_editorState.currentLayer;
EditTool& g_editTool = g_editorState.drawTool;
double& g_zoom = g_camera.zoom;
double& g_panOffsetX = g_camera.offsetX;
double& g_panOffsetY = g_camera.offsetY;
bool g_panning = false;
POINT g_lastPanPoint{};
CameraNavigationKeys g_viewerNavigation{};
constexpr UINT_PTR kViewerNavigationTimer = 1;
constexpr UINT kViewerNavigationIntervalMs = 16;
constexpr double kViewerNavigationPixelsPerSecond = 320.0;
int g_layerScroll = 0;
int g_layersTab = 0;
int g_propertiesTab = 0;
int g_toolsetTab = 0;
int& g_flagsTab = g_editorState.flagType;
int g_chairDirection = 1;
MapWarp g_warpSettings{};
int& g_brushSize = g_editorState.brushSize;
bool g_entityMode = false;
bool IsFlagsDomain() {
    return g_editorState.editDomain == EditDomain::Flags;
}
void SetEditDomain(bool flags) {
    g_entityMode = false;
    g_editorState.selectEditDomain(flags ? EditDomain::Flags : EditDomain::Graphics);
}
bool g_selectingBaseTile = false;
std::array<bool, 16> g_layerVisible = [] {
    std::array<bool, 16> visible{};
    visible.fill(true);
    visible[9] = false;
    visible[11] = true;
    return visible;
}();
std::array<bool, 5> g_flagVisible{true, true, true, true, true};
std::array<bool, 5> g_boundaryVisible{false, false, false, false, false};
SIZE g_previousClientSize{};
std::vector<MapDocument> g_undoMaps;
std::vector<MapDocument> g_redoMaps;
std::optional<MapDocument> g_strokeBefore;
std::vector<HBITMAP> g_menuBitmaps;
struct EntityDraft {
    int x = -1;
    int y = -1;
    std::optional<MapWarp> warp;
    std::optional<MapSign> sign;
    std::vector<MapNpc> npcs;
    std::vector<MapItem> items;
    std::array<std::uint64_t, 4> generations{};
    std::uint8_t dirtyScopes = 0;
    bool stale = false;
};
std::optional<EntityDraft> g_entityDraft;
std::optional<EntityDraft> g_copiedEntities;
int g_entitiesTab = 0;
std::array<HWND, 5> g_entityWarpEdits{};
std::array<HWND, 2> g_entitySignEdits{};
std::array<HWND, 4> g_entityNpcEdits{};
std::array<HWND, 5> g_entityItemEdits{};
HWND g_entityNpcList = nullptr;
HWND g_entityItemList = nullptr;
std::array<HWND, 15> g_entityButtons{};
constexpr int kMaxUndoSteps = 64;
std::unique_ptr<collaboration::Session> g_collaboration;
std::unique_ptr<collaboration::Window> g_collaborationWindow;
std::uint64_t g_nextCollaborationRequestId = 1;
std::unordered_set<std::uint64_t> g_pendingCollaborationEdits;
std::unordered_map<std::uint64_t, std::uint64_t> g_entityGenerations;
std::unordered_map<std::uint32_t, collaboration::Presence> g_remotePresence;
std::optional<collaboration::Presence> g_lastPublishedPresence;
struct PresenceHit {
    RECT bounds{};
    std::uint32_t userId = 0;
    int x = 0, y = 0;
};
std::vector<PresenceHit> g_presenceHits;
std::deque<std::string> g_noticeQueue;
std::string g_activeNotice;
HWND g_noticeWindow = nullptr;
void DisplayClassicNotice() {
    if (!g_mainWindow || g_noticeQueue.empty())
        return;
    g_activeNotice = std::move(g_noticeQueue.front());
    g_noticeQueue.pop_front();
    if (!IsWindow(g_noticeWindow)) {
        g_noticeWindow =
            CreateWindowExA(WS_EX_NOACTIVATE, "STATIC", "", WS_CHILD | WS_BORDER | SS_LEFT | SS_CENTERIMAGE, 0, 0, 300,
                            27, g_mainWindow, nullptr, GetModuleHandleA(nullptr), nullptr);
        if (g_uiFont)
            SendMessageA(g_noticeWindow, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    }
    RECT client{};
    GetClientRect(g_mainWindow, &client);
    SetWindowTextA(g_noticeWindow, g_activeNotice.c_str());
    SetWindowPos(g_noticeWindow, HWND_TOP, 8, std::max(8L, client.bottom - 35), 300, 27,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetTimer(g_mainWindow, 78, 3500, nullptr);
}
void ShowClassicNotice(std::string text) {
    if (text.empty())
        return;
    if (g_noticeQueue.size() < 4)
        g_noticeQueue.push_back(std::move(text));
    if (g_activeNotice.empty())
        DisplayClassicNotice();
}
std::uint64_t EntityGenerationKey(int scope, int x, int y) {
    return (std::uint64_t(scope) << 32) | (std::uint64_t(y) << 16) | std::uint16_t(x);
}
std::uint64_t EntityGeneration(int scope, int x, int y) {
    return g_entityGenerations[EntityGenerationKey(scope, x, y)];
}
enum class AcceptanceRole { None, Host, Client };
AcceptanceRole g_acceptanceRole = AcceptanceRole::None;
bool g_acceptanceB3 = false;
bool g_acceptanceCD = false;
bool g_acceptanceFinal = false;
std::filesystem::path g_acceptanceDirectory;
int g_acceptanceStep = 0;
ULONGLONG g_acceptanceBrushStart = 0;
std::size_t g_acceptanceSubmitted = 0, g_acceptancePendingHigh = 0;
std::uint64_t g_acceptanceLastRevision = 0;
int g_acceptancePresenceIndex = -1;
enum class AcceptanceDialogAction { Manual, Save, Cancel };
AcceptanceDialogAction g_acceptanceDialogAction = AcceptanceDialogAction::Manual;
std::string g_acceptanceDialogComment;
std::string g_acceptanceDialogToken;
bool g_acceptanceModal = false;
std::optional<std::filesystem::path> g_acceptanceSaveAsPath;
save_workflow::FailureInjection g_acceptanceSaveFailure;
std::filesystem::path g_lastSessionBackup;
save_workflow::SaveResult g_lastSaveResult;
std::uint64_t g_receivedSaveEvents = 0;

void InvalidatePanels();
void SyncLayerControls();
void SyncFlagControls();
void SetAllLayersVisible(bool visible);
void SetAllFlagsVisible(bool visible);
void UpdateWarpAtCursorFromSettings();
void ApplyCollaborationEdit(const collaboration::EditOperation& operation);
bool CollaborationGenerationsMatch(const collaboration::EditOperation& operation);
void PublishPanelPresence(PanelKind kind);

int PaletteCategoryForLayer(int layer) {
    if (layer == 0)
        return 0;
    if (layer == 1 || layer == 2 || layer == 8)
        return 1;
    if (layer == 7)
        return 2;
    if (layer == 3 || layer == 5)
        return 3;
    if (layer == 4)
        return 4;
    return 5;
}

int PaletteBankForCurrentLayer() {
    return kLayerBanks[std::clamp(g_selectedLayer, 0, 8)];
}

void SetSelectedLayer(int layer) {
    g_editorState.selectLayer(layer, PaletteCategoryForLayer(layer));
    g_palettePage = 0;
    if (g_graphicsMenu)
        CheckMenuRadioItem(g_graphicsMenu, kGraphicsMenuBaseCommand, kGraphicsMenuBaseCommand + 5,
                           kGraphicsMenuBaseCommand + g_paletteCategory, MF_BYCOMMAND);
}

void SetGraphicsCategory(int category) {
    category = std::clamp(category, 0, 5);
    g_editorState.selectGraphicsCategory(category, kPaletteLayers[category]);
    if (g_graphicsMenu) {
        for (int index = 0; index < 6; ++index) {
            CheckMenuRadioItem(g_graphicsMenu, kGraphicsMenuBaseCommand, kGraphicsMenuBaseCommand + 5,
                               kGraphicsMenuBaseCommand + index, MF_BYCOMMAND);
        }
    }
}

std::vector<int> LayersInGraphicsGroup(int category) {
    switch (category) {
    case 0:
        return {0};
    case 1:
        return {1, 2, 8};
    case 2:
        return {7};
    case 3:
        return {3, 4, 5};
    default:
        return {6};
    }
}

bool GraphicsGroupVisible(int category) {
    const std::vector<int> layers = LayersInGraphicsGroup(category);
    return std::all_of(layers.begin(), layers.end(), [](int layer) { return g_layerVisible[layer]; });
}

void ToggleGraphicsGroup(int category) {
    const bool visible = !GraphicsGroupVisible(category);
    for (int layer : LayersInGraphicsGroup(category))
        g_layerVisible[layer] = visible;
    if (g_viewMenu)
        CheckMenuItem(g_viewMenu, kLayerGroupMenuBaseCommand + category,
                      MF_BYCOMMAND | (visible ? MF_CHECKED : MF_UNCHECKED));
    InvalidatePanels();
}

void UpdateLayerGroupChecks() {
    if (!g_viewMenu)
        return;
    for (int category = 0; category < 5; ++category) {
        CheckMenuItem(g_viewMenu, kLayerGroupMenuBaseCommand + category,
                      MF_BYCOMMAND | (GraphicsGroupVisible(category) ? MF_CHECKED : MF_UNCHECKED));
    }
}

void SetFlagVisibility(int category, bool visible) {
    if (category < 0 || category >= static_cast<int>(g_flagVisible.size()))
        return;
    g_flagVisible[category] = visible;
    if (g_flagMenu)
        CheckMenuItem(g_flagMenu, kFlagMenuBaseCommand + category,
                      MF_BYCOMMAND | (visible ? MF_CHECKED : MF_UNCHECKED));
    InvalidatePanels();
}

void ToggleFlagVisibility(int category) {
    if (category < 0 || category >= static_cast<int>(g_flagVisible.size()))
        return;
    SetFlagVisibility(category, !g_flagVisible[category]);
}

void SetAllFlagsVisible(bool visible) {
    for (int category = 0; category < static_cast<int>(g_flagVisible.size()); ++category) {
        SetFlagVisibility(category, visible);
    }
}

void ToggleAllFlags() {
    const bool show = !std::all_of(g_flagVisible.begin(), g_flagVisible.end(), [](bool visible) { return visible; });
    SetAllFlagsVisible(show);
}

void SetBoundaryVisibility(int category, bool visible) {
    if (category < 0 || category >= static_cast<int>(g_boundaryVisible.size()))
        return;
    g_boundaryVisible[category] = visible;
    if (g_boundaryMenu)
        CheckMenuItem(g_boundaryMenu, kBoundaryMenuBaseCommand + category,
                      MF_BYCOMMAND | (visible ? MF_CHECKED : MF_UNCHECKED));
    InvalidatePanels();
}

void SetAllBoundariesVisible(bool visible) {
    for (int category = 0; category < static_cast<int>(g_boundaryVisible.size()); ++category) {
        SetBoundaryVisibility(category, visible);
    }
}

void ToggleAllBoundaries() {
    const bool show =
        !std::all_of(g_boundaryVisible.begin(), g_boundaryVisible.end(), [](bool visible) { return visible; });
    SetAllBoundariesVisible(show);
}

std::filesystem::path FindGraphicsDirectory() {
    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    const std::filesystem::path executableDirectory =
        std::filesystem::path(executable, executable + length).parent_path();
    const std::filesystem::path candidates[] = {
        executableDirectory / L"gfx",
        executableDirectory.parent_path() / L"gfx",
        executableDirectory.parent_path().parent_path() / L"gfx",
        std::filesystem::current_path() / L"gfx",
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_directory(candidate)) {
            return candidate;
        }
    }
    return std::filesystem::current_path() / L"gfx";
}

std::filesystem::path FindMapsDirectory() {
    wchar_t executable[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    const std::filesystem::path executableDirectory =
        std::filesystem::path(executable, executable + length).parent_path();
    const std::filesystem::path candidates[] = {
        executableDirectory / L"data" / L"maps",
        executableDirectory.parent_path() / L"data" / L"maps",
        executableDirectory.parent_path().parent_path() / L"data" / L"maps",
        std::filesystem::current_path() / L"data" / L"maps",
    };
    for (const auto& candidate : candidates)
        if (std::filesystem::is_directory(candidate))
            return candidate;
    return executableDirectory / L"data" / L"maps";
}

void CheckEoResult(EoResult result, const char* operation) {
    if (result != EO_SUCCESS) {
        throw std::runtime_error(std::string(operation) + ": " + eo_result_string(result));
    }
}

std::vector<std::uint8_t> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open the selected map file.");
    }

    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

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

MapWarp ReadWarp(EoReader& reader) {
    MapWarp warp;
    warp.destinationMap = ReadShort(reader);
    warp.x = ReadChar(reader);
    warp.y = ReadChar(reader);
    warp.level = ReadChar(reader);
    warp.door = ReadShort(reader);
    return warp;
}

MapDocument ReadEmfBytes(const std::vector<std::uint8_t>& bytes, const std::string& path = {}) {
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

MapDocument ReadEmf(const std::string& path) {
    return ReadEmfBytes(ReadFile(path), path);
}

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

using TileRow = std::pair<int, std::vector<int>>;

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

struct EoWriterOwner {
    EoWriter writer = eo_writer_init();
    ~EoWriterOwner() {
        eo_writer_free(&writer);
    }
};

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

bool EquivalentMaps(const MapDocument& left, const MapDocument& right) {
    return left.name == right.name && left.width == right.width && left.height == right.height &&
           left.type == right.type && left.effect == right.effect && left.musicId == right.musicId &&
           left.musicControl == right.musicControl && left.ambientSoundId == right.ambientSoundId &&
           left.fillTile == right.fillTile && left.mapAvailable == right.mapAvailable &&
           left.canScroll == right.canScroll && left.relogX == right.relogX && left.relogY == right.relogY &&
           left.npcs == right.npcs && left.legacyDoorKeys == right.legacyDoorKeys && left.items == right.items &&
           left.signs == right.signs && left.tiles == right.tiles;
}

void WriteFile(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file || !file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
        throw std::runtime_error("Unable to write the selected map file.");
    }
}

int CheckEmfDirectory(const std::filesystem::path& directory) {
    int parsedMaps = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".emf") {
            continue;
        }
        try {
            ReadEmf(entry.path().string());
            ++parsedMaps;
        } catch (const std::exception& error) {
            throw std::runtime_error(entry.path().string() + ": " + error.what());
        }
    }
    return parsedMaps;
}

void DrawText(HDC dc, const char* text, RECT bounds, COLORREF color = RGB(30, 30, 30)) {
    HGDIOBJ oldFont = g_uiFont ? SelectObject(dc, g_uiFont) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextA(dc, text, -1, &bounds, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) {
        SelectObject(dc, oldFont);
    }
}

void DrawButton(HDC dc, RECT bounds, const char* label, bool pressed = false) {
    const COLORREF face = pressed ? classic_ui::Panel : classic_ui::Control;
    HBRUSH brush = CreateSolidBrush(face);
    FillRect(dc, &bounds, brush);
    DeleteObject(brush);
    DrawEdge(dc, &bounds, pressed ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
    InflateRect(&bounds, -5, -2);
    classic_ui::DrawCenteredText(dc, g_uiFont, label, bounds);
}

void DrawToolRow(HDC dc, RECT bounds, const char* label, int icon, bool selected) {
    if (selected) {
        HBRUSH selection = CreateSolidBrush(classic_ui::Panel);
        FillRect(dc, &bounds, selection);
        DeleteObject(selection);
        DrawEdge(dc, &bounds, EDGE_SUNKEN, BF_RECT);
    }
    const int iconHeight = std::min(16, static_cast<int>(bounds.bottom - bounds.top - 6));
    const int iconTop = bounds.top + (bounds.bottom - bounds.top - iconHeight) / 2;
    RECT iconBounds{bounds.left + 5, iconTop, bounds.left + 22, iconTop + iconHeight};
    HPEN pen = CreatePen(PS_SOLID, 1, classic_ui::Text);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    if (icon == 0) {
        MoveToEx(dc, iconBounds.left + 2, iconBounds.bottom - 1, nullptr);
        LineTo(dc, iconBounds.right - 2, iconBounds.top + 1);
    } else if (icon == 1)
        Rectangle(dc, iconBounds.left + 3, iconBounds.top + 2, iconBounds.right - 3, iconBounds.bottom - 2);
    else if (icon == 2)
        Rectangle(dc, iconBounds.left + 2, iconBounds.top + 4, iconBounds.right - 2, iconBounds.bottom - 1);
    else if (icon == 3) {
        MoveToEx(dc, iconBounds.left + 2, iconBounds.top + 2, nullptr);
        LineTo(dc, iconBounds.right - 2, iconBounds.bottom - 2);
        MoveToEx(dc, iconBounds.right - 2, iconBounds.top + 2, nullptr);
        LineTo(dc, iconBounds.left + 2, iconBounds.bottom - 2);
    } else if (icon == 4)
        Polygon(dc,
                std::array<POINT, 4>{
                    POINT{iconBounds.left + 8, iconBounds.top}, POINT{iconBounds.right, iconBounds.top + 6},
                    POINT{iconBounds.left + 8, iconBounds.bottom}, POINT{iconBounds.left, iconBounds.top + 6}}
                    .data(),
                4);
    else if (icon == 5) {
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 3; ++x)
                Rectangle(dc, iconBounds.left + x * 5, iconBounds.top + y * 4, iconBounds.left + x * 5 + 3,
                          iconBounds.top + y * 4 + 3);
    } else
        DrawText(dc, "no", iconBounds);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
    classic_ui::DrawCenteredText(dc, g_uiFont, label,
                                 RECT{bounds.left + 27, bounds.top + 1, bounds.right - 4, bounds.bottom - 1});
}

void DrawTabStrip(HDC dc, RECT bounds, const char* const* labels, int count, int active) {
    classic_ui::DrawTabs(dc, g_uiFont, bounds, labels, count, active);
}

struct PaletteLayout {
    RECT content{};
    RECT tabs{};
    int columns = 1;
    int rows = 1;
    int capacity = 1;
    int cellWidth = 56;
    int cellHeight = 55;
    int gapX = 8;
    int gapY = 5;
    int gridLeft = 0;
    int gridTop = 0;
};

constexpr int kPalettePadding = 10;
constexpr int kPaletteCellGapX = 8;
constexpr int kPaletteCellGapY = 5;
constexpr int kPalettePreviewInset = 3;
constexpr int kPaletteLabelHeight = 13;
constexpr int kPaletteFooterHeight = 25;

PaletteLayout GetPaletteLayout(const RECT& client) {
    PaletteLayout layout;
    layout.content = RECT{3, kTitleHeight + 3, client.right - 3, client.bottom - 3};
    layout.tabs =
        RECT{layout.content.left + 7, layout.content.top + 3, layout.content.right - 7, layout.content.top + 22};
    layout.gridTop = layout.content.top + 26;
    layout.cellWidth = g_paletteCategory == 0 ? 70 : 76;
    layout.cellHeight = g_paletteCategory == 0 ? 50 : 77;
    const int availableWidth =
        std::max(56, static_cast<int>(layout.content.right - layout.content.left - 2 * kPalettePadding));
    const int availableHeight =
        std::max(55, static_cast<int>(layout.content.bottom - layout.gridTop - kPaletteFooterHeight));
    layout.columns = std::max(1, (availableWidth + layout.gapX) / (layout.cellWidth + layout.gapX));
    layout.rows = std::max(1, (availableHeight + layout.gapY) / (layout.cellHeight + layout.gapY));
    const int gridWidth = layout.columns * layout.cellWidth + (layout.columns - 1) * layout.gapX;
    layout.gridLeft = layout.content.left + (layout.content.right - layout.content.left - gridWidth) / 2;
    layout.capacity = layout.columns * layout.rows;
    return layout;
}

int PalettePageCount(int bank, int capacity) {
    const auto& ids = g_gfx.GraphicIds(bank);
    return std::max(1, static_cast<int>((ids.size() + capacity - 1) / capacity));
}

void DrawBitmapTransparent(HDC dc, BitmapResource resource, const RECT& destination, bool allowUpscale = true) {
    if (!resource.bitmap || resource.width <= 0 || resource.height <= 0) {
        return;
    }
    HDC source = CreateCompatibleDC(dc);
    HGDIOBJ oldBitmap = SelectObject(source, resource.bitmap);
    const int availableWidth = destination.right - destination.left;
    const int availableHeight = destination.bottom - destination.top;
    double scale = std::min(static_cast<double>(availableWidth) / resource.width,
                            static_cast<double>(availableHeight) / resource.height);
    if (!allowUpscale)
        scale = std::min(1.0, scale);
    const int width = std::max(1, static_cast<int>(resource.width * scale));
    const int height = std::max(1, static_cast<int>(resource.height * scale));
    const int left = destination.left + (availableWidth - width) / 2;
    const int top = destination.top + (availableHeight - height) / 2;
    const int previousStretchMode = SetStretchBltMode(dc, COLORONCOLOR);
    TransparentBlt(dc, left, top, width, height, source, 0, 0, resource.width, resource.height, RGB(0, 0, 0));
    SetStretchBltMode(dc, previousStretchMode);
    SelectObject(source, oldBitmap);
    DeleteDC(source);
}

void DrawBitmapTransparentAlpha(HDC dc, BitmapResource resource, const RECT& destination, BYTE opacity) {
    const int width = destination.right - destination.left;
    const int height = destination.bottom - destination.top;
    if (!resource.bitmap || width <= 0 || height <= 0 || opacity == 0)
        return;
    HDC composite = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, width, height);
    HGDIOBJ oldBitmap = SelectObject(composite, bitmap);
    BitBlt(composite, 0, 0, width, height, dc, destination.left, destination.top, SRCCOPY);
    DrawBitmapTransparent(composite, resource, RECT{0, 0, width, height});
    BLENDFUNCTION blend{AC_SRC_OVER, 0, opacity, 0};
    AlphaBlend(dc, destination.left, destination.top, width, height, composite, 0, 0, width, height, blend);
    SelectObject(composite, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(composite);
}

RECT ViewerCanvasRect(const RECT& client) {
    return RECT{10, kTitleHeight + 10, client.right - 10, client.bottom - 30};
}

CameraViewport CameraViewportFromRect(const RECT& area) {
    return CameraViewport{static_cast<double>(area.left), static_cast<double>(area.top),
                          static_cast<double>(area.right), static_cast<double>(area.bottom)};
}

void DrawMapLayer(HDC dc, const RECT& area, int layer) {
    if (!g_map.loaded) {
        return;
    }
    const CameraViewport viewport = CameraViewportFromRect(area);
    const int range = static_cast<int>(((area.right - area.left) / 64 + (area.bottom - area.top) / 32) / g_zoom) + 8;
    const int minX = std::max(0, g_viewCenterX - range);
    const int maxX = std::min(g_map.width, g_viewCenterX + range + 1);
    const int minY = std::max(0, g_viewCenterY - range);
    const int maxY = std::min(g_map.height, g_viewCenterY + range + 1);
    const int offsetsX[] = {0, -2, -2, 0, 32, 0, 0, -24, -2};
    const int offsetsY[] = {0, -2, -2, -1, -1, -64, -32, -12, -2};

    for (int y = minY; y < maxY; ++y) {
        for (int x = minX; x < maxX; ++x) {
            if (!g_layerVisible[layer] && g_selectedLayer != layer) {
                continue;
            }
            const int graphic = g_map.tile(x, y).graphics[layer];
            if (graphic <= 0) {
                continue;
            }

            BitmapResource resource = g_gfx.Get(kLayerBanks[layer], graphic + 100);
            if (!resource.bitmap) {
                continue;
            }
            const int halfWidth = std::max(1, static_cast<int>(std::lround(32 * g_zoom)));
            const int halfHeight = std::max(1, static_cast<int>(std::lround(16 * g_zoom)));
            const int spriteWidth = std::max(1, static_cast<int>(std::lround(resource.width * g_zoom)));
            const int spriteHeight = std::max(1, static_cast<int>(std::lround(resource.height * g_zoom)));
            const CameraPoint tileTop = g_camera.MapToScreenTop(x, y, viewport);
            const int baseX = static_cast<int>(std::lround(tileTop.x)) - halfWidth;
            const int baseY = static_cast<int>(std::lround(tileTop.y));
            int left = baseX + static_cast<int>(std::lround(offsetsX[layer] * g_zoom));
            int top = baseY + static_cast<int>(std::lround(offsetsY[layer] * g_zoom));
            if (layer == 1 || layer == 2 || layer == 8) {
                left -= spriteWidth / 2 - halfWidth;
            }
            if (layer == 1 || layer == 2 || layer == 3 || layer == 4 || layer == 5 || layer == 8) {
                top -= spriteHeight - halfHeight * 2;
            }
            RECT target{left, top, left + spriteWidth, top + spriteHeight};
            if (target.right >= area.left && target.left <= area.right && target.bottom >= area.top &&
                target.top <= area.bottom) {
                if (layer == 7)
                    DrawBitmapTransparentAlpha(dc, resource, target, 51);
                else
                    DrawBitmapTransparent(dc, resource, target);
                const int boundaryGroup = PaletteCategoryForLayer(layer);
                if (g_boundaryVisible[boundaryGroup]) {
                    HPEN boundaryPen = CreatePen(PS_SOLID, 1, RGB(235, 74, 59));
                    HGDIOBJ oldPen = SelectObject(dc, boundaryPen);
                    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
                    Rectangle(dc, target.left, target.top, target.right, target.bottom);
                    SelectObject(dc, oldBrush);
                    SelectObject(dc, oldPen);
                    DeleteObject(boundaryPen);
                }
            }
        }
    }
}

COLORREF PresenceColor(std::uint8_t index) {
    static constexpr COLORREF colors[] = {RGB(62, 112, 180), RGB(190, 133, 36), RGB(181, 70, 65), RGB(129, 83, 166),
                                          RGB(43, 137, 139), RGB(192, 102, 50), RGB(92, 125, 62), RGB(112, 112, 112)};
    return colors[index % std::size(colors)];
}
std::optional<collaboration::Participant> CollaborationParticipant(std::uint32_t id) {
    if (!g_collaboration)
        return std::nullopt;
    for (const auto& p : g_collaboration->Participants())
        if (p.userId == id)
            return p;
    return std::nullopt;
}
collaboration::PresenceArea PresenceAreaForPanel(PanelKind kind) {
    switch (kind) {
    case PanelKind::Viewer:
        return collaboration::PresenceArea::Viewer;
    case PanelKind::Graphics:
        return collaboration::PresenceArea::Graphics;
    case PanelKind::Flags:
        return collaboration::PresenceArea::Flags;
    case PanelKind::Toolset:
        return collaboration::PresenceArea::Toolset;
    case PanelKind::Layers:
        return collaboration::PresenceArea::Layers;
    case PanelKind::Properties:
        return collaboration::PresenceArea::MapProperties;
    case PanelKind::Entities:
        return collaboration::PresenceArea::Entities;
    }
    return collaboration::PresenceArea::None;
}
void PublishPanelPresence(PanelKind kind) {
    if (!g_collaboration || g_collaboration->State() == collaboration::SessionState::Disconnected ||
        g_collaboration->State() == collaboration::SessionState::Connecting)
        return;
    collaboration::Presence value;
    value.area = PresenceAreaForPanel(kind);
    value.viewerActive = value.area == collaboration::PresenceArea::Viewer && g_cursorX >= 0 && g_cursorY >= 0;
    if (value.viewerActive) {
        value.x = static_cast<std::uint16_t>(g_cursorX);
        value.y = static_cast<std::uint16_t>(g_cursorY);
    }
    if (g_lastPublishedPresence && g_lastPublishedPresence->area == value.area &&
        g_lastPublishedPresence->viewerActive == value.viewerActive && g_lastPublishedPresence->x == value.x &&
        g_lastPublishedPresence->y == value.y)
        return;
    std::string error;
    if (g_collaboration->SendPresence(value, error))
        g_lastPublishedPresence = value;
}
void DrawRemotePresence(HDC dc, const RECT& area) {
    g_presenceHits.clear();
    if (!g_collaboration)
        return;
    const auto local = g_collaboration->LocalUserId();
    const CameraViewport viewport = CameraViewportFromRect(area);
    int stack = 0;
    for (const auto& [id, presence] : g_remotePresence) {
        if (id == local || !presence.viewerActive || presence.area != collaboration::PresenceArea::Viewer)
            continue;
        const auto participant = CollaborationParticipant(id);
        if (!participant)
            continue;
        const COLORREF color = PresenceColor(participant->color);
        const CameraPoint projected = g_camera.MapToScreenCenter(presence.x, presence.y, viewport);
        const int cx = static_cast<int>(std::lround(projected.x)), cy = static_cast<int>(std::lround(projected.y));
        const int hw = std::max(8, static_cast<int>(std::lround(32 * g_zoom))),
                  hh = std::max(4, static_cast<int>(std::lround(16 * g_zoom)));
        if (cx >= area.left && cx < area.right && cy >= area.top && cy < area.bottom) {
            POINT diamond[] = {{cx, cy - hh}, {cx + hw, cy}, {cx, cy + hh}, {cx - hw, cy}, {cx, cy - hh}};
            HPEN pen = CreatePen(PS_SOLID, 2, color);
            auto old = SelectObject(dc, pen);
            Polyline(dc, diamond, 5);
            SelectObject(dc, old);
            DeleteObject(pen);
            RECT label{cx - hw, cy - hh - 16, cx + hw + 80, cy - hh};
            SetTextColor(dc, color);
            SetBkMode(dc, TRANSPARENT);
            DrawTextA(dc, participant->displayName.c_str(), -1, &label, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        } else {
            const int margin = 8;
            const int ex =
                std::clamp(cx, static_cast<int>(area.left) + margin, static_cast<int>(area.right) - margin - 90);
            const int ey = std::clamp(cy, static_cast<int>(area.top) + margin + stack * 18,
                                      static_cast<int>(area.bottom) - margin - 18);
            RECT hit{ex, ey, ex + 88, ey + 17};
            HBRUSH brush = CreateSolidBrush(color);
            RECT marker{hit.left, hit.top + 4, hit.left + 7, hit.top + 11};
            FillRect(dc, &marker, brush);
            DeleteObject(brush);
            SetTextColor(dc, color);
            SetBkMode(dc, TRANSPARENT);
            RECT textRect{hit.left + 10, hit.top, hit.right, hit.bottom};
            const char* arrow = cx < area.left ? "< " : cx >= area.right ? "> " : cy < area.top ? "^ " : "v ";
            const std::string label = std::string(arrow) + participant->displayName;
            DrawTextA(dc, label.c_str(), -1, &textRect, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
            g_presenceHits.push_back({hit, id, presence.x, presence.y});
            ++stack;
        }
    }
}

void DrawMapOverlays(HDC dc, const RECT& area) {
    if (!g_map.loaded)
        return;
    const CameraViewport viewport = CameraViewportFromRect(area);
    const int halfHeight = std::max(1, static_cast<int>(std::lround(16 * g_zoom)));
    const int range = static_cast<int>(((area.right - area.left) / 64 + (area.bottom - area.top) / 32) / g_zoom) + 8;
    const int minX = std::max(0, g_viewCenterX - range);
    const int maxX = std::min(g_map.width, g_viewCenterX + range + 1);
    const int minY = std::max(0, g_viewCenterY - range);
    const int maxY = std::min(g_map.height, g_viewCenterY + range + 1);
    const auto tileCenter = [&](int x, int y) {
        const CameraPoint point = g_camera.MapToScreenCenter(x, y, viewport);
        return POINT{static_cast<int>(std::lround(point.x)), static_cast<int>(std::lround(point.y))};
    };

    static const COLORREF flagColors[] = {
        RGB(235, 72, 58), RGB(246, 157, 38), RGB(224, 188, 42), RGB(55, 179, 116), RGB(170, 92, 220),
    };
    static const char* flagLabels[] = {"WALL", "DOOR", "CHEST", "CHAIR", "WARP"};
    for (int y = minY; y < maxY; ++y) {
        for (int x = minX; x < maxX; ++x) {
            const MapTile& tile = g_map.tile(x, y);
            int category = -1;
            if (tile.warp)
                category = tile.warp->door > 0 ? 1 : 4;
            else if (tile.spec == 0)
                category = 0;
            else if (tile.spec == 9)
                category = 2;
            else if (tile.spec >= 1 && tile.spec <= 7)
                category = 3;
            if (category < 0 || !g_flagVisible[category])
                continue;

            const POINT center = tileCenter(x, y);
            const int halfWidth = std::max(8, static_cast<int>(std::lround(32 * g_zoom)));
            HPEN flagPen =
                CreatePen(PS_SOLID, std::max(1, static_cast<int>(std::lround(g_zoom))), flagColors[category]);
            HGDIOBJ oldPen = SelectObject(dc, flagPen);
            HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
            const POINT diamond[] = {{center.x, center.y - halfHeight},
                                     {center.x + halfWidth, center.y},
                                     {center.x, center.y + halfHeight},
                                     {center.x - halfWidth, center.y},
                                     {center.x, center.y - halfHeight}};
            Polyline(dc, diamond, static_cast<int>(std::size(diamond)));
            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            RECT label{center.x - halfWidth, center.y - halfHeight, center.x + halfWidth + 1,
                       center.y + halfHeight + 1};
            SetTextColor(dc, flagColors[category]);
            SetBkMode(dc, TRANSPARENT);
            HGDIOBJ oldFont = g_uiFont ? SelectObject(dc, g_uiFont) : nullptr;
            ::DrawTextA(dc, flagLabels[category], -1, &label, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            if (oldFont)
                SelectObject(dc, oldFont);
            DeleteObject(flagPen);
        }
    }

    if (!g_layerVisible[10])
        return;
    const auto drawEntity = [&](int x, int y, COLORREF color) {
        if (x < minX || x >= maxX || y < minY || y >= maxY)
            return;
        const POINT center = tileCenter(x, y);
        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(28, 28, 28));
        HGDIOBJ oldBrush = SelectObject(dc, brush);
        HGDIOBJ oldPen = SelectObject(dc, pen);
        Ellipse(dc, center.x - 5, center.y - 11, center.x + 5, center.y - 1);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    };
    for (const MapNpc& npc : g_map.npcs)
        drawEntity(npc.x, npc.y, RGB(225, 77, 61));
    for (const MapItem& item : g_map.items)
        drawEntity(item.x, item.y, RGB(239, 207, 70));
    for (const MapSign& sign : g_map.signs)
        drawEntity(sign.x, sign.y, RGB(65, 146, 210));
}

void DrawIsometricGrid(HDC dc, const RECT& area) {
    const int halfWidth = std::max(1, static_cast<int>(std::lround(32 * g_zoom)));
    const int halfHeight = std::max(1, static_cast<int>(std::lround(16 * g_zoom)));
    const CameraViewport viewport = CameraViewportFromRect(area);

    if (!g_layerVisible[11]) {
        return;
    }
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(93, 164, 18));
    HGDIOBJ oldPen = SelectObject(dc, gridPen);

    const int range = static_cast<int>(((area.right - area.left) / 64 + (area.bottom - area.top) / 32) / g_zoom) + 8;
    const int minX = g_map.loaded ? g_viewCenterX - range : -32;
    const int maxX = g_map.loaded ? g_viewCenterX + range + 1 : 32;
    const int minY = g_map.loaded ? g_viewCenterY - range : -32;
    const int maxY = g_map.loaded ? g_viewCenterY + range + 1 : 32;
    for (int y = minY; y < maxY; ++y) {
        for (int x = minX; x < maxX; ++x) {
            const bool insideMap = g_map.loaded && x >= 0 && y >= 0 && x < g_map.width && y < g_map.height;
            if (insideMap && g_map.tile(x, y).graphics[0] > 0)
                continue;
            const CameraPoint top = g_camera.MapToScreenTop(x, y, viewport);
            const int topX = static_cast<int>(std::lround(top.x));
            const int topY = static_cast<int>(std::lround(top.y));
            if (topX < area.left - halfWidth || topX > area.right + halfWidth || topY < area.top - halfHeight ||
                topY > area.bottom + halfHeight) {
                continue;
            }

            const POINT diamond[] = {
                {topX, topY},
                {topX + halfWidth, topY + halfHeight},
                {topX, topY + halfHeight * 2},
                {topX - halfWidth, topY + halfHeight},
                {topX, topY},
            };
            HBRUSH baseBrush = CreateSolidBrush(RGB(0, 55, 10));
            HGDIOBJ previousBrush = SelectObject(dc, baseBrush);
            Polygon(dc, diamond, 4);
            SelectObject(dc, previousBrush);
            DeleteObject(baseBrush);
        }
    }

    if (g_cursorX >= 0 && g_cursorY >= 0 && (!g_map.loaded || (g_cursorX < g_map.width && g_cursorY < g_map.height))) {
        const CameraPoint top = g_camera.MapToScreenTop(g_cursorX, g_cursorY, viewport);
        const int topX = static_cast<int>(std::lround(top.x));
        const int topY = static_cast<int>(std::lround(top.y));
        const POINT selected[] = {
            {topX, topY},
            {topX + halfWidth, topY + halfHeight},
            {topX, topY + halfHeight * 2},
            {topX - halfWidth, topY + halfHeight},
            {topX, topY},
        };
        HPEN selectedPen = CreatePen(PS_SOLID, 2, RGB(219, 155, 44));
        HGDIOBJ previousPen = SelectObject(dc, selectedPen);
        Polyline(dc, selected, static_cast<int>(std::size(selected)));
        SelectObject(dc, previousPen);
        DeleteObject(selectedPen);
    }

    SelectObject(dc, oldPen);
    DeleteObject(gridPen);
}

MapTile* CursorTile() {
    if (!g_map.loaded || g_cursorX < 0 || g_cursorY < 0 || g_cursorX >= g_map.width || g_cursorY >= g_map.height)
        return nullptr;
    return &g_map.tile(g_cursorX, g_cursorY);
}

std::string DescribeCursorTile() {
    const MapTile* tile = CursorTile();
    if (!tile)
        return "No tile";
    if (tile->warp) {
        if (tile->warp->door > 0)
            return tile->warp->door > 1 ? "Keyed door" : "Door";
        return "Warp -> " + std::to_string(tile->warp->destinationMap) + ":" + std::to_string(tile->warp->x) + "," +
               std::to_string(tile->warp->y);
    }
    if (tile->spec == 0)
        return "Blocked tile";
    if (tile->spec == 9)
        return "Chest";
    if (tile->spec >= 1 && tile->spec <= 7)
        return "Chair";
    return "Open tile";
}

void InspectFlagAtCursor(bool selectTab) {
    const MapTile* tile = CursorTile();
    if (!tile)
        return;
    if (tile->warp) {
        g_warpSettings = *tile->warp;
        if (selectTab)
            g_flagsTab = tile->warp->door > 0 ? 2 : 5;
    } else if (tile->spec >= 1 && tile->spec <= 7) {
        g_chairDirection = tile->spec;
        if (selectTab)
            g_flagsTab = 4;
    } else if (selectTab) {
        g_flagsTab = tile->spec == 0 ? 1 : tile->spec == 9 ? 3 : 0;
    }
}

void DrawPanelContents(HDC dc, const RECT& client, const PanelData& panel) {
    RECT content{3, kTitleHeight + 3, client.right - 3, client.bottom - 3};
    HBRUSH background = CreateSolidBrush(classic_ui::Content);
    FillRect(dc, &content, background);
    DeleteObject(background);

    switch (panel.kind) {
    case PanelKind::Viewer: {
        RECT canvas = ViewerCanvasRect(client);
        HBRUSH canvasBrush = CreateSolidBrush(RGB(0, 39, 7));
        FillRect(dc, &canvas, canvasBrush);
        DeleteObject(canvasBrush);
        const int savedDC = SaveDC(dc);
        IntersectClipRect(dc, canvas.left + 1, canvas.top + 1, canvas.right - 1, canvas.bottom - 1);
        static constexpr int drawOrder[] = {0, 7, 1, 2, 3, 4, 5, 6, 8};
        for (int layer : drawOrder)
            DrawMapLayer(dc, canvas, layer);
        DrawMapOverlays(dc, canvas);
        DrawIsometricGrid(dc, canvas);
        DrawRemotePresence(dc, canvas);
        RestoreDC(dc, savedDC);
        DrawEdge(dc, &canvas, EDGE_SUNKEN, BF_RECT);

        RECT openTile{content.left + 4, content.bottom - 21, content.left + 150, content.bottom - 3};
        DrawText(dc, DescribeCursorTile().c_str(), openTile);
        const std::string dimensions = std::to_string(g_map.width) + " x " + std::to_string(g_map.height);
        RECT sizeLabel{content.right - 127, content.bottom - 19, content.right - 72, content.bottom - 3};
        DrawText(dc, dimensions.c_str(), sizeLabel);
        RECT positionLabel{content.right - 68, content.bottom - 19, content.right - 4, content.bottom - 3};
        const std::string position = "cur: " + std::to_string(g_cursorX) + "," + std::to_string(g_cursorY);
        DrawText(dc, position.c_str(), positionLabel);
        break;
    }
    case PanelKind::Graphics: {
        static const char* tabs[] = {"Tile", "Obj", "Mask", "Down", "Right", "Top"};
        const PaletteLayout layout = GetPaletteLayout(client);
        DrawTabStrip(dc, layout.tabs, tabs, 6, g_paletteCategory);
        const int bank = PaletteBankForCurrentLayer();
        const auto& graphicIds = g_gfx.GraphicIds(bank);
        const int pageCount =
            std::max(1, static_cast<int>((graphicIds.size() + layout.capacity - 1) / layout.capacity));
        g_palettePage = std::clamp(g_palettePage, 0, pageCount - 1);
        const std::size_t pageStart = static_cast<std::size_t>(g_palettePage) * layout.capacity;
        for (int row = 0; row < layout.rows; ++row) {
            for (int column = 0; column < layout.columns; ++column) {
                RECT swatch{
                    layout.gridLeft + column * (layout.cellWidth + layout.gapX),
                    layout.gridTop + row * (layout.cellHeight + layout.gapY),
                    layout.gridLeft + column * (layout.cellWidth + layout.gapX) + layout.cellWidth,
                    layout.gridTop + row * (layout.cellHeight + layout.gapY) + layout.cellHeight,
                };
                const std::size_t resourceIndex = pageStart + static_cast<std::size_t>(row * layout.columns + column);
                if (resourceIndex >= graphicIds.size()) {
                    break;
                }
                const int graphicId = graphicIds[resourceIndex];
                HBRUSH swatchBrush = CreateSolidBrush(RGB(188, 185, 172));
                FillRect(dc, &swatch, swatchBrush);
                DeleteObject(swatchBrush);
                const BitmapResource resource = g_gfx.Get(bank, graphicId + 100);
                RECT image{swatch.left + kPalettePreviewInset, swatch.top + kPalettePreviewInset,
                           swatch.right - kPalettePreviewInset, swatch.bottom - kPaletteLabelHeight};
                DrawBitmapTransparent(dc, resource, image, g_paletteCategory != 0);
                DrawEdge(dc, &swatch, EDGE_SUNKEN, BF_RECT);
                if (graphicId == g_editorState.selectedGraphic()) {
                    HPEN selectionPen = CreatePen(PS_SOLID, 1, classic_ui::Selection);
                    HGDIOBJ oldPen = SelectObject(dc, selectionPen);
                    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
                    Rectangle(dc, swatch.left, swatch.top, swatch.right, swatch.bottom);
                    SelectObject(dc, oldBrush);
                    SelectObject(dc, oldPen);
                    DeleteObject(selectionPen);
                }
                RECT idLabel{swatch.left + 3, swatch.bottom - kPaletteLabelHeight + 2, swatch.right - 3,
                             swatch.bottom - 1};
                RECT labelBackground{swatch.left + 1, swatch.bottom - kPaletteLabelHeight, swatch.right - 1,
                                     swatch.bottom - 1};
                HBRUSH labelBrush = CreateSolidBrush(RGB(246, 244, 237));
                FillRect(dc, &labelBackground, labelBrush);
                DeleteObject(labelBrush);
                HPEN labelLine = CreatePen(PS_SOLID, 1, RGB(164, 160, 151));
                HGDIOBJ previousPen = SelectObject(dc, labelLine);
                MoveToEx(dc, labelBackground.left, labelBackground.top, nullptr);
                LineTo(dc, labelBackground.right, labelBackground.top);
                SelectObject(dc, previousPen);
                DeleteObject(labelLine);
                const std::string cellLabel =
                    g_paletteCategory == 0 ? std::to_string(graphicId)
                                           : std::to_string(resource.width) + " x " + std::to_string(resource.height);
                HGDIOBJ oldFont = g_uiFont ? SelectObject(dc, g_uiFont) : nullptr;
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, classic_ui::Text);
                DrawTextA(dc, cellLabel.c_str(), -1, &idLabel, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
                if (oldFont)
                    SelectObject(dc, oldFont);
            }
        }
        classic_ui::DrawArrowButton(
            dc, RECT{content.left + 8, content.bottom - 20, content.left + 28, content.bottom - 3}, false);
        classic_ui::DrawCenteredText(
            dc, g_uiFont, ("page " + std::to_string(g_palettePage) + " / " + std::to_string(pageCount - 1)).c_str(),
            RECT{content.left + 34, content.bottom - 20, content.right - 31, content.bottom - 3});
        classic_ui::DrawArrowButton(
            dc, RECT{content.right - 28, content.bottom - 20, content.right - 8, content.bottom - 3}, true);
        break;
    }
    case PanelKind::Layers: {
        static const char* tabs[] = {"Current layer", "View options"};
        const RECT tabsBounds{content.left + 7, content.top + 3, content.right - 7, content.top + 22};
        DrawTabStrip(dc, tabsBounds, tabs, 2, g_layersTab);
        static const char* names[] = {"Ground", "Object", "Overlay", "Down wall", "Right wall",
                                      "Roof",   "Top",    "Shadow",  "Overlay 2"};
        const int firstRow = content.top + 26;
        const int rowHeight = 20;
        const int visibleRows = std::max(1, static_cast<int>(content.bottom - firstRow - 4) / rowHeight);
        g_layerScroll = std::clamp(g_layerScroll, 0, std::max(0, 9 - visibleRows));
        if (g_layersTab == 0) {
            for (int row = 0; row < visibleRows; ++row) {
                const int layer = g_layerScroll + row;
                if (layer >= 9)
                    break;
                const int top = firstRow + row * rowHeight;
                RECT current{content.left + 9, top + 2, content.left + 24, top + 17};
                DrawFrameControl(dc, &current, DFC_BUTTON,
                                 DFCS_BUTTONRADIO | (layer == g_selectedLayer ? DFCS_CHECKED : 0));
                DrawText(dc, names[layer], RECT{content.left + 30, top + 2, content.right - 7, top + 18});
            }
        } else {
            DrawText(dc, "graphics:", RECT{content.left + 9, content.top + 33, content.left + 66, content.top + 50});
            DrawText(dc, "flags:", RECT{content.left + 9, content.top + 65, content.left + 66, content.top + 82});
        }
        break;
    }
    case PanelKind::Properties: {
        static const char* tabs[] = {"Settings", "Environment", "Pass", "Memory"};
        DrawTabStrip(dc, RECT{content.left + 7, content.top + 3, content.right - 7, content.top + 22}, tabs, 4,
                     g_propertiesTab);
        if (g_propertiesTab == 0) {
            DrawText(dc, "map", RECT{content.left + 9, content.top + 29, content.right - 8, content.top + 46});
            DrawText(dc, "width", RECT{content.left + 9, content.top + 51, content.left + 56, content.top + 69});
            DrawText(dc, "height", RECT{content.left + 105, content.top + 51, content.left + 153, content.top + 69});
            DrawText(dc, "base tile", RECT{content.left + 9, content.top + 84, content.left + 70, content.top + 102});
            RECT preview{content.left + 57, content.top + 78, content.left + 129, content.top + 126};
            HBRUSH previewBrush = CreateSolidBrush(classic_ui::Content);
            FillRect(dc, &preview, previewBrush);
            DeleteObject(previewBrush);
            FrameRect(dc, &preview, GetSysColorBrush(COLOR_BTNSHADOW));
            if (g_map.loaded) {
                const BitmapResource resource = g_gfx.Get(3, g_map.fillTile + 100);
                DrawBitmapTransparent(dc, resource,
                                      RECT{preview.left + 2, preview.top + 2, preview.right - 2, preview.bottom - 2});
                DrawText(dc, std::to_string(g_map.fillTile).c_str(),
                         RECT{preview.left + 3, preview.bottom - 14, preview.right - 3, preview.bottom - 2},
                         RGB(255, 255, 255));
            }
        } else if (g_propertiesTab == 1) {
            const std::pair<const char*, std::string> fields[] = {
                {"map name", g_map.loaded ? g_map.name : "Untitled"},
                {"map type", g_map.type == 3 ? "PK" : "Normal"},
                {"special", g_map.effect == 0 ? "Normal" : std::to_string(g_map.effect)},
                {"music", std::to_string(g_map.musicId)},
                {"ambient sound", std::to_string(g_map.ambientSoundId)},
            };
            const int rowHeight = std::max(21, static_cast<int>(content.bottom - content.top - 31) / 5);
            for (int index = 0; index < 5; ++index) {
                const int y = content.top + 26 + index * rowHeight;
                DrawText(dc, fields[index].first, RECT{content.left + 9, y + 4, content.left + 66, y + 20});
                DrawButton(dc, RECT{content.left + 67, y, content.right - 7, y + rowHeight - 2},
                           fields[index].second.c_str());
            }
        } else if (g_propertiesTab == 2) {
            const std::pair<const char*, std::string> fields[] = {
                {"map available", g_map.mapAvailable ? "Yes" : "No"},
                {"can scroll", g_map.canScroll ? "Yes" : "No"},
                {"relog X", std::to_string(g_map.relogX)},
                {"relog Y", std::to_string(g_map.relogY)},
            };
            for (int index = 0; index < 4; ++index) {
                const int y = content.top + 27 + index * 25;
                DrawText(dc, fields[index].first, RECT{content.left + 9, y + 4, content.left + 75, y + 20});
                DrawButton(dc, RECT{content.left + 77, y, content.right - 7, y + 22}, fields[index].second.c_str());
            }
        } else {
            const std::string counts =
                "NPCs " + std::to_string(g_map.npcs.size()) + "   Items " + std::to_string(g_map.items.size());
            const std::string signs = "Signs " + std::to_string(g_map.signs.size()) + "   Door keys " +
                                      std::to_string(g_map.legacyDoorKeys.size());
            DrawText(dc, counts.c_str(), RECT{content.left + 9, content.top + 34, content.right - 8, content.top + 52});
            DrawText(dc, signs.c_str(), RECT{content.left + 9, content.top + 59, content.right - 8, content.top + 77});
        }
        break;
    }
    case PanelKind::Flags: {
        static const char* tabs[] = {"Open", "B", "D", "CT", "CR", "W"};
        DrawTabStrip(dc, RECT{content.left + 7, content.top + 3, content.right - 7, content.top + 22}, tabs, 6,
                     g_flagsTab);
        const int top = content.top + 29;
        if (g_flagsTab == 0) {
            DrawText(dc, "no attributes, open tile for players",
                     RECT{content.left + 9, top + 4, content.right - 8, top + 22});
        } else if (g_flagsTab == 1) {
            DrawText(dc, "B   no access for players and NPCs",
                     RECT{content.left + 9, top + 4, content.right - 8, top + 22});
        } else if (g_flagsTab == 2) {
            DrawText(dc, "door settings", RECT{content.left + 9, top, content.right - 8, top + 18});
            DrawText(dc, "rules", RECT{content.left + 9, top + 23, content.left + 48, top + 41});
        } else if (g_flagsTab == 3) {
            DrawText(dc, "chest settings", RECT{content.left + 9, top, content.right - 8, top + 18});
            DrawText(dc, "rules", RECT{content.left + 9, top + 23, content.left + 48, top + 41});
        } else if (g_flagsTab == 4) {
            DrawText(dc, "chair settings", RECT{content.left + 9, top, content.right - 8, top + 18});
            DrawText(dc, "direction", RECT{content.left + 9, top + 23, content.left + 60, top + 41});
        } else {
            DrawText(dc, "destination map", RECT{content.left + 9, top, content.left + 95, top + 18});
            DrawText(dc, "x / y coordinates", RECT{content.left + 110, top, content.right - 8, top + 18});
        }
        break;
    }
    case PanelKind::Toolset: {
        static const char* tabs[] = {"Cursor", "Edit mode", "Brushes"};
        DrawTabStrip(dc, RECT{content.left + 7, content.top + 3, content.right - 7, content.top + 22}, tabs, 3,
                     g_toolsetTab);
        if (g_toolsetTab == 0) {
            DrawToolRow(dc, RECT{content.left + 7, content.top + 25, content.right - 7, content.top + 47},
                        "pencil, left click to draw", 0, g_editTool == EditTool::Pencil);
            DrawToolRow(dc, RECT{content.left + 7, content.top + 50, content.right - 7, content.top + 72},
                        "brush, left button down draw", 1, g_editTool == EditTool::Brush);
            DrawToolRow(dc, RECT{content.left + 7, content.top + 75, content.right - 7, content.top + 97},
                        "eraser, left click to erase", 2, g_editTool == EditTool::Eraser);
            DrawToolRow(dc, RECT{content.left + 7, content.top + 100, content.right - 7, content.top + 122},
                        "wipe, left button down erase", 3, g_editTool == EditTool::Wipe);
        } else if (g_toolsetTab == 1) {
            DrawButton(dc, RECT{content.left + 7, content.top + 29, content.right - 7, content.top + 53},
                       "graphic / design mode", !IsFlagsDomain() && !g_entityMode);
            DrawButton(dc, RECT{content.left + 7, content.top + 58, content.right - 7, content.top + 82},
                       "special attribute / flag mode", IsFlagsDomain() && !g_entityMode);
            DrawButton(dc, RECT{content.left + 7, content.top + 87, content.right - 7, content.top + 111},
                       "entity mode", g_entityMode);
        } else {
            DrawToolRow(dc, RECT{content.left + 7, content.top + 29, content.right - 7, content.top + 53},
                        "single tile mode", 4, g_brushSize == 1);
            DrawToolRow(dc, RECT{content.left + 7, content.top + 58, content.right - 7, content.top + 82},
                        "cluster mode 3x3", 5, g_brushSize == 3);
            DrawToolRow(dc, RECT{content.left + 7, content.top + 87, content.right - 7, content.top + 111},
                        "no brush mode", 6, g_brushSize == 0);
        }
        break;
    }
    case PanelKind::Entities: {
        static const char* tabs[] = {"Warp", "Sign", "NPCs", "Items"};
        DrawTabStrip(dc, RECT{content.left + 7, content.top + 3, content.right - 7, content.top + 22}, tabs, 4,
                     g_entitiesTab);
        const std::string tile = g_entityDraft && g_entityDraft->x >= 0 ? "tile " + std::to_string(g_entityDraft->x) +
                                                                              ", " + std::to_string(g_entityDraft->y)
                                                                        : "select a map tile";
        DrawText(dc, tile.c_str(), RECT{content.left + 10, content.top + 28, content.right - 10, content.top + 45});
        if (g_entitiesTab == 0) {
            DrawText(dc, "map", RECT{10, content.top + 51, 75, content.top + 68});
            DrawText(dc, "x", RECT{92, content.top + 51, 140, content.top + 68});
            DrawText(dc, "y", RECT{174, content.top + 51, 220, content.top + 68});
            DrawText(dc, "level", RECT{255, content.top + 51, 315, content.top + 68});
            DrawText(dc, "door/key", RECT{335, content.top + 51, content.right - 8, content.top + 68});
        } else if (g_entitiesTab == 1) {
            DrawText(dc, "title", RECT{10, content.top + 51, content.right - 8, content.top + 68});
            DrawText(dc, "message", RECT{10, content.top + 93, content.right - 8, content.top + 110});
        } else if (g_entitiesTab == 2) {
            DrawText(dc, "NPC ID          amount          speed          spawn time",
                     RECT{10, content.top + 141, content.right - 8, content.top + 158});
        } else {
            DrawText(dc, "item ID       amount       spawn       chest       key",
                     RECT{10, content.top + 141, content.right - 8, content.top + 158});
        }
        break;
    }
    }
}

void InvalidatePanels();
void ApplyFlagToCursor();
void ExecuteCommand(HWND window, int command);
void UpdateViewerScrollbars(HWND viewer, bool clampCamera = true);
void SyncMapPropertyControls();
void EnsureEditorWindowZOrder(HWND activePalette = nullptr);

void UpdateMapTitle() {
    if (!g_mainWindow) {
        return;
    }
    const std::string title =
        "Endless Map Studio - " + (g_map.loaded ? g_map.name : "Untitled") + (g_map.dirty ? " *" : "");
    SetWindowTextA(g_mainWindow, title.c_str());
    EnableMenuItem(g_fileMenu, kSaveMapCommand, MF_BYCOMMAND | (g_map.loaded ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(g_fileMenu, kSaveMapAsCommand, MF_BYCOMMAND | (g_map.loaded ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(g_fileMenu, kCloseMapCommand, MF_BYCOMMAND | (g_map.loaded ? MF_ENABLED : MF_GRAYED));
    SyncMapPropertyControls();
    DrawMenuBar(g_mainWindow);
}

void UpdateToolMenuChecks() {
    if (!g_toolMenu)
        return;
    const int toolCommand = g_editTool == EditTool::Pencil ? 1310 : g_editTool == EditTool::Brush ? 1311 : 1312;
    CheckMenuRadioItem(g_toolMenu, 1310, 1312, toolCommand, MF_BYCOMMAND);
    CheckMenuRadioItem(g_toolMenu, kGraphicsModeCommand, kFlagsModeCommand,
                       g_entityMode ? 0 : (IsFlagsDomain() ? kFlagsModeCommand : kGraphicsModeCommand), MF_BYCOMMAND);
    CheckMenuItem(g_toolMenu, kEntitiesModeCommand, MF_BYCOMMAND | (g_entityMode ? MF_CHECKED : MF_UNCHECKED));
    const int editCommand = g_brushSize == 0   ? kNoEditCommand
                            : g_brushSize == 3 ? kClusterEditCommand
                                               : kSingleEditCommand;
    CheckMenuRadioItem(g_toolMenu, kSingleEditCommand, kNoEditCommand, editCommand, MF_BYCOMMAND);
}

void CommitStroke() {
    if (!g_strokeBefore) {
        return;
    }
    g_undoMaps.push_back(std::move(*g_strokeBefore));
    g_strokeBefore.reset();
    if (g_undoMaps.size() > kMaxUndoSteps) {
        g_undoMaps.erase(g_undoMaps.begin());
    }
    g_redoMaps.clear();
}

void BeginEditSnapshot() {
    if (!g_strokeBefore) {
        g_strokeBefore = g_map;
    }
}

void PushUndoSnapshot() {
    CommitStroke();
    g_undoMaps.push_back(g_map);
    if (g_undoMaps.size() > kMaxUndoSteps) {
        g_undoMaps.erase(g_undoMaps.begin());
    }
    g_redoMaps.clear();
}

void UndoMap() {
    CommitStroke();
    if (g_undoMaps.empty())
        return;
    g_redoMaps.push_back(g_map);
    g_map = std::move(g_undoMaps.back());
    g_undoMaps.pop_back();
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void RedoMap() {
    CommitStroke();
    if (g_redoMaps.empty())
        return;
    g_undoMaps.push_back(g_map);
    g_map = std::move(g_redoMaps.back());
    g_redoMaps.pop_back();
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

bool ResizeMapDocument(MapDocument& map, int newWidth, int newHeight) {
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

bool ResizeWouldDiscardData(const MapDocument& map, int newWidth, int newHeight) {
    if (newWidth >= map.width && newHeight >= map.height)
        return false;
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            if (x < newWidth && y < newHeight)
                continue;
            const MapTile& tile = map.tile(x, y);
            if (tile.spec >= 0 || tile.warp || tile.graphics[0] != map.fillTile ||
                std::any_of(tile.graphics.begin() + 1, tile.graphics.end(), [](int graphic) { return graphic >= 0; }))
                return true;
        }
    }
    const auto outside = [newWidth, newHeight](const auto& entity) {
        return entity.x >= newWidth || entity.y >= newHeight;
    };
    return std::any_of(map.npcs.begin(), map.npcs.end(), outside) ||
           std::any_of(map.items.begin(), map.items.end(), outside) ||
           std::any_of(map.legacyDoorKeys.begin(), map.legacyDoorKeys.end(), outside) ||
           std::any_of(map.signs.begin(), map.signs.end(), outside);
}

void ResizeMap(int newWidth, int newHeight) {
    if (!g_map.loaded)
        return;
    newWidth = std::clamp(newWidth, 1, EO_CHAR_MAX + 1);
    newHeight = std::clamp(newHeight, 1, EO_CHAR_MAX + 1);
    if (newWidth == g_map.width && newHeight == g_map.height)
        return;
    if (ResizeWouldDiscardData(g_map, newWidth, newHeight) &&
        MessageBoxA(g_mainWindow,
                    "Shrinking the map will permanently clip graphics, flags, warps, or entities outside the new "
                    "bounds. Continue?",
                    "Resize map", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::ResizeMap;
    operation.width = static_cast<std::uint16_t>(newWidth);
    operation.height = static_cast<std::uint16_t>(newHeight);
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active) {
        std::string error;
        const auto payload = collaboration::EncodeEditOperation(operation);
        if (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error))
            return;
        g_pendingCollaborationEdits.insert(operation.requestId);
        if (g_collaboration->State() == collaboration::SessionState::Connected)
            return;
    }
    PushUndoSnapshot();
    ResizeMapDocument(g_map, newWidth, newHeight);
    g_cursorX = std::clamp(g_cursorX, 0, newWidth - 1);
    g_cursorY = std::clamp(g_cursorY, 0, newHeight - 1);
    g_viewCenterX = std::clamp(g_viewCenterX, 0, newWidth - 1);
    g_viewCenterY = std::clamp(g_viewCenterY, 0, newHeight - 1);
    if (!g_panels.empty())
        UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

int ReadDimensionControl(HWND control) {
    char text[16]{};
    GetWindowTextA(control, text, static_cast<int>(std::size(text)));
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    if (end == text || *end != '\0' || value < 1 || value > EO_CHAR_MAX + 1)
        return -1;
    return static_cast<int>(value);
}

void CommitMapPropertyDimensions(HWND owner) {
    if (!g_map.loaded)
        return;
    const int width = ReadDimensionControl(g_mapWidthEdit);
    const int height = ReadDimensionControl(g_mapHeightEdit);
    if (width < 1 || height < 1) {
        MessageBoxA(owner, "Width and height must be whole numbers from 1 through 254.", "Invalid map size",
                    MB_OK | MB_ICONWARNING);
        SyncMapPropertyControls();
        return;
    }
    ResizeMap(width, height);
    SyncMapPropertyControls();
}

void LayoutMapPropertyControls(HWND panel) {
    if (!IsWindow(panel))
        return;
    RECT client{};
    GetClientRect(panel, &client);
    const int top = kTitleHeight + 3;
    const int actionX = client.right - classic_ui::FormMargin - classic_ui::FormButtonWidth;
    SetWindowPos(g_mapWidthEdit, nullptr, 60, top + 45, classic_ui::FormEditWidth, classic_ui::FormControlHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_mapHeightEdit, nullptr, 158, top + 45, classic_ui::FormEditWidth, classic_ui::FormControlHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_mapDimensionsButton, nullptr, actionX, top + 44, classic_ui::FormButtonWidth, 24,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_baseTileButton, nullptr, actionX, top + 90, classic_ui::FormButtonWidth, 24,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void ResetViewerNavigation(HWND viewer = nullptr) {
    g_viewerNavigation.clear();
    if (viewer && IsWindow(viewer))
        KillTimer(viewer, kViewerNavigationTimer);
}

void SyncMapPropertyControls() {
    if (!IsWindow(g_mapWidthEdit))
        return;
    const bool settingsVisible = g_propertiesTab == 0;
    const bool enabled = settingsVisible && g_map.loaded;
    if (GetFocus() != g_mapWidthEdit)
        SetWindowTextA(g_mapWidthEdit, std::to_string(g_map.width).c_str());
    if (GetFocus() != g_mapHeightEdit)
        SetWindowTextA(g_mapHeightEdit, std::to_string(g_map.height).c_str());
    const HWND controls[] = {g_mapWidthEdit, g_mapHeightEdit, g_mapDimensionsButton, g_baseTileButton};
    for (HWND control : controls) {
        ShowWindow(control, settingsVisible ? SW_SHOW : SW_HIDE);
        EnableWindow(control, enabled);
    }
}

void CreateMapPropertyControls(HWND panel, HINSTANCE instance) {
    g_mapWidthEdit =
        CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "24", WS_CHILD | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL, 0, 0, 42,
                        21, panel, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMapWidthEdit)), instance, nullptr);
    g_mapHeightEdit =
        CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "24", WS_CHILD | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL, 0, 0, 42,
                        21, panel, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMapHeightEdit)), instance, nullptr);
    g_mapDimensionsButton =
        CreateWindowExA(0, "BUTTON", "change", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 52, 23, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMapDimensionsChange)), instance, nullptr);
    g_baseTileButton =
        CreateWindowExA(0, "BUTTON", "change", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 52, 23, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kBaseTileChange)), instance, nullptr);
    const HWND controls[] = {g_mapWidthEdit, g_mapHeightEdit, g_mapDimensionsButton, g_baseTileButton};
    for (HWND control : controls) {
        SetWindowTheme(control, L"", L"");
        SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    }
    SendMessageA(g_mapWidthEdit, EM_SETLIMITTEXT, 3, 0);
    SendMessageA(g_mapHeightEdit, EM_SETLIMITTEXT, 3, 0);
    LayoutMapPropertyControls(panel);
    SyncMapPropertyControls();
}

int EntityEditValue(HWND edit, int maximum) {
    char text[32]{};
    GetWindowTextA(edit, text, static_cast<int>(std::size(text)));
    try {
        return std::clamp(std::stoi(text), 0, maximum);
    } catch (...) {
        return 0;
    }
}

void SetEntityEditValue(HWND edit, int value) {
    SetWindowTextA(edit, std::to_string(value).c_str());
}

EntityDraft ReadEntitiesAt(int x, int y) {
    EntityDraft draft;
    draft.x = x;
    draft.y = y;
    if (!g_map.loaded || x < 0 || y < 0 || x >= g_map.width || y >= g_map.height)
        return draft;
    draft.warp = g_map.tile(x, y).warp;
    for (const MapNpc& npc : g_map.npcs)
        if (npc.x == x && npc.y == y)
            draft.npcs.push_back(npc);
    for (const MapItem& item : g_map.items)
        if (item.x == x && item.y == y)
            draft.items.push_back(item);
    for (const MapSign& sign : g_map.signs)
        if (sign.x == x && sign.y == y) {
            draft.sign = sign;
            break;
        }
    for (int scope = 0; scope < 4; ++scope)
        draft.generations[scope] = EntityGeneration(scope, x, y);
    return draft;
}

void LayoutEntityControls(HWND panel) {
    if (!IsWindow(panel))
        return;
    RECT client{};
    GetClientRect(panel, &client);
    const int top = kTitleHeight + 52;
    auto placeRow = [&](const auto& edits, int count, int y) {
        const int margin = 10, gap = 6;
        const int width = std::max(42, (static_cast<int>(client.right) - margin * 2 - gap * (count - 1)) / count);
        for (int i = 0; i < count; ++i)
            SetWindowPos(edits[i], nullptr, margin + i * (width + gap), y, width, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    };
    placeRow(g_entityWarpEdits, 5, top + 20);
    SetWindowPos(g_entitySignEdits[0], nullptr, 10, top + 20, client.right - 20, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_entitySignEdits[1], nullptr, 10, top + 62, client.right - 20, 72, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_entityNpcList, nullptr, 10, top, client.right - 20, 88, SWP_NOZORDER | SWP_NOACTIVATE);
    placeRow(g_entityNpcEdits, 4, top + 110);
    SetWindowPos(g_entityItemList, nullptr, 10, top, client.right - 20, 88, SWP_NOZORDER | SWP_NOACTIVATE);
    placeRow(g_entityItemEdits, 5, top + 110);
    const int actionY = top + 145;
    const int buttonIds[] = {4005, 4006, 4012, 4013, 4025, 4026, 4027, 4036, 4037, 4038};
    for (int i = 0; i < 10; ++i) {
        HWND button = GetDlgItem(panel, buttonIds[i]);
        const int local = i < 2 ? i : i < 4 ? i - 2 : i < 7 ? i - 4 : i - 7;
        SetWindowPos(button, nullptr, 10 + local * 72, actionY, 66, 24, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    const int bottom = client.bottom - 32;
    const int commonIds[] = {4040, 4041, 4042, 4043, 4044};
    for (int i = 0; i < 5; ++i)
        SetWindowPos(GetDlgItem(panel, commonIds[i]), nullptr, 10 + i * 78, bottom, 72, 24,
                     SWP_NOZORDER | SWP_NOACTIVATE);
}

void SyncEntityControls() {
    if (g_panels.size() <= static_cast<std::size_t>(PanelKind::Entities))
        return;
    HWND panel = g_panels[static_cast<std::size_t>(PanelKind::Entities)];
    if (!IsWindow(panel))
        return;
    if (!g_entityDraft || g_entityDraft->x != g_cursorX || g_entityDraft->y != g_cursorY)
        g_entityDraft = ReadEntitiesAt(g_cursorX, g_cursorY);
    EntityDraft& draft = *g_entityDraft;
    const MapWarp warp = draft.warp.value_or(MapWarp{});
    const int warpValues[] = {warp.destinationMap, warp.x, warp.y, warp.level, warp.door};
    for (int i = 0; i < 5; ++i)
        SetEntityEditValue(g_entityWarpEdits[i], warpValues[i]);
    std::string signTitle, signMessage;
    if (draft.sign) {
        std::vector<std::uint8_t> decoded = draft.sign->encodedText;
        if (!decoded.empty())
            eo_decode_string(decoded.data(), decoded.size());
        const int split = std::clamp(draft.sign->titleLength, 0, static_cast<int>(decoded.size()));
        signTitle.assign(decoded.begin(), decoded.begin() + split);
        signMessage.assign(decoded.begin() + split, decoded.end());
    }
    SetWindowTextA(g_entitySignEdits[0], signTitle.c_str());
    SetWindowTextA(g_entitySignEdits[1], signMessage.c_str());
    SendMessageA(g_entityNpcList, LB_RESETCONTENT, 0, 0);
    for (const MapNpc& npc : draft.npcs) {
        const std::string row = "ID " + std::to_string(npc.id) + "   amount " + std::to_string(npc.amount) +
                                "   speed " + std::to_string(npc.spawnType) + "   spawn " +
                                std::to_string(npc.spawnTime);
        SendMessageA(g_entityNpcList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
    SendMessageA(g_entityItemList, LB_RESETCONTENT, 0, 0);
    for (const MapItem& item : draft.items) {
        const std::string row = "ID " + std::to_string(item.id) + "   amount " + std::to_string(item.amount) +
                                "   spawn " + std::to_string(item.spawnTime) + "   chest " +
                                std::to_string(item.chestSlot) + "   key " + std::to_string(item.key);
        SendMessageA(g_entityItemList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
    const HWND pageControls[][12] = {
        {g_entityWarpEdits[0], g_entityWarpEdits[1], g_entityWarpEdits[2], g_entityWarpEdits[3], g_entityWarpEdits[4],
         GetDlgItem(panel, 4005), GetDlgItem(panel, 4006)},
        {g_entitySignEdits[0], g_entitySignEdits[1], GetDlgItem(panel, 4012), GetDlgItem(panel, 4013)},
        {g_entityNpcList, g_entityNpcEdits[0], g_entityNpcEdits[1], g_entityNpcEdits[2], g_entityNpcEdits[3],
         GetDlgItem(panel, 4025), GetDlgItem(panel, 4026), GetDlgItem(panel, 4027)},
        {g_entityItemList, g_entityItemEdits[0], g_entityItemEdits[1], g_entityItemEdits[2], g_entityItemEdits[3],
         g_entityItemEdits[4], GetDlgItem(panel, 4036), GetDlgItem(panel, 4037), GetDlgItem(panel, 4038)}};
    const int counts[] = {7, 4, 8, 9};
    for (int page = 0; page < 4; ++page)
        for (int i = 0; i < counts[page]; ++i)
            ShowWindow(pageControls[page][i], page == g_entitiesTab ? SW_SHOW : SW_HIDE);
    for (int id = 4040; id <= 4044; ++id)
        ShowWindow(GetDlgItem(panel, id), SW_SHOW);
    EnableWindow(GetDlgItem(panel, 4043), g_copiedEntities.has_value());
    InvalidateRect(panel, nullptr, TRUE);
}

void CommitEntityDraft() {
    if (!g_entityDraft || !g_map.loaded || g_entityDraft->x < 0 || g_entityDraft->y < 0)
        return;
    const int x = g_entityDraft->x, y = g_entityDraft->y;
    const auto npcAtTile = std::count_if(g_map.npcs.begin(), g_map.npcs.end(),
                                         [=](const MapNpc& value) { return value.x == x && value.y == y; });
    const auto itemAtTile = std::count_if(g_map.items.begin(), g_map.items.end(),
                                          [=](const MapItem& value) { return value.x == x && value.y == y; });
    const auto signAtTile = std::count_if(g_map.signs.begin(), g_map.signs.end(),
                                          [=](const MapSign& value) { return value.x == x && value.y == y; });
    const std::size_t npcCount = g_map.npcs.size() - npcAtTile + g_entityDraft->npcs.size();
    const std::size_t itemCount = g_map.items.size() - itemAtTile + g_entityDraft->items.size();
    const std::size_t signCount = g_map.signs.size() - signAtTile + (g_entityDraft->sign ? 1u : 0u);
    if (npcCount >= EO_CHAR_MAX || itemCount >= EO_CHAR_MAX || signCount >= EO_CHAR_MAX) {
        const char* kind = npcCount >= EO_CHAR_MAX ? "NPC" : itemCount >= EO_CHAR_MAX ? "item" : "sign";
        const std::string message =
            "Only " + std::to_string(EO_CHAR_MAX - 1) + " " + kind + " entities are allowed in a map.";
        MessageBoxA(g_mainWindow, message.c_str(), "Entity limit exceeded", MB_OK | MB_ICONERROR);
        return;
    }
    if (!g_entityDraft->dirtyScopes)
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::Entities;
    collaboration::EntityEdit edit;
    edit.x = static_cast<std::uint16_t>(x);
    edit.y = static_cast<std::uint16_t>(y);
    edit.scopes = g_entityDraft->dirtyScopes;
    edit.warpGeneration = g_entityDraft->generations[0];
    edit.signGeneration = g_entityDraft->generations[1];
    edit.npcGeneration = g_entityDraft->generations[2];
    edit.itemGeneration = g_entityDraft->generations[3];
    edit.hasWarp = g_entityDraft->warp.has_value();
    if (g_entityDraft->warp)
        edit.warp = {static_cast<std::uint16_t>(g_entityDraft->warp->destinationMap),
                     static_cast<std::uint8_t>(g_entityDraft->warp->x),
                     static_cast<std::uint8_t>(g_entityDraft->warp->y),
                     static_cast<std::uint8_t>(g_entityDraft->warp->level),
                     static_cast<std::uint16_t>(g_entityDraft->warp->door)};
    edit.hasSign = g_entityDraft->sign.has_value();
    if (g_entityDraft->sign) {
        edit.sign.titleLength = static_cast<std::uint16_t>(g_entityDraft->sign->titleLength);
        edit.sign.encodedText = g_entityDraft->sign->encodedText;
    }
    for (const auto& n : g_entityDraft->npcs)
        edit.npcs.push_back({static_cast<std::uint16_t>(n.id), static_cast<std::uint16_t>(n.spawnTime),
                             static_cast<std::uint8_t>(n.spawnType), static_cast<std::uint8_t>(n.amount)});
    for (const auto& i : g_entityDraft->items)
        edit.items.push_back({static_cast<std::uint16_t>(i.key), static_cast<std::uint16_t>(i.id),
                              static_cast<std::uint16_t>(i.spawnTime), static_cast<std::uint8_t>(i.chestSlot),
                              static_cast<std::uint32_t>(i.amount)});
    operation.entities.push_back(std::move(edit));
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active && !CollaborationGenerationsMatch(operation)) {
        if (g_collaborationWindow)
            g_collaborationWindow->AppendSystem("Entity changed by another user - review before saving");
        g_entityDraft = ReadEntitiesAt(x, y);
        SyncEntityControls();
        return;
    }
    if (active) {
        std::string error;
        const auto payload = collaboration::EncodeEditOperation(operation);
        if (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error))
            return;
        g_pendingCollaborationEdits.insert(operation.requestId);
        if (g_collaboration->State() == collaboration::SessionState::Connected)
            return;
    }
    PushUndoSnapshot();
    ApplyCollaborationEdit(operation);
    g_entityDraft->dirtyScopes = 0;
    g_entityDraft->stale = false;
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void HandleEntityCommand(int id) {
    if (!g_entityDraft)
        return;
    EntityDraft& draft = *g_entityDraft;
    if (id == 4005) {
        draft.warp = MapWarp{EntityEditValue(g_entityWarpEdits[0], EO_SHORT_MAX - 1),
                             EntityEditValue(g_entityWarpEdits[1], EO_CHAR_MAX - 1),
                             EntityEditValue(g_entityWarpEdits[2], EO_CHAR_MAX - 1),
                             EntityEditValue(g_entityWarpEdits[3], EO_CHAR_MAX - 1),
                             EntityEditValue(g_entityWarpEdits[4], EO_SHORT_MAX - 1)};
        draft.dirtyScopes |= collaboration::EntityWarp;
    } else if (id == 4006) {
        draft.warp.reset();
        draft.dirtyScopes |= collaboration::EntityWarp;
    } else if (id == 4012) {
        char title[256]{}, message[2048]{};
        GetWindowTextA(g_entitySignEdits[0], title, 256);
        GetWindowTextA(g_entitySignEdits[1], message, 2048);
        MapSign sign;
        sign.x = draft.x;
        sign.y = draft.y;
        sign.titleLength = static_cast<int>(strlen(title));
        const std::string combined = std::string(title) + message;
        sign.encodedText.assign(combined.begin(), combined.end());
        if (!sign.encodedText.empty())
            eo_encode_string(sign.encodedText.data(), sign.encodedText.size());
        draft.sign = std::move(sign);
        draft.dirtyScopes |= collaboration::EntitySign;
    } else if (id == 4013) {
        draft.sign.reset();
        draft.dirtyScopes |= collaboration::EntitySign;
    } else if (id >= 4025 && id <= 4027) {
        int index = static_cast<int>(SendMessageA(g_entityNpcList, LB_GETCURSEL, 0, 0));
        if (id == 4027) {
            if (index >= 0 && index < static_cast<int>(draft.npcs.size()))
                draft.npcs.erase(draft.npcs.begin() + index);
        } else {
            MapNpc npc{draft.x,
                       draft.y,
                       EntityEditValue(g_entityNpcEdits[0], EO_SHORT_MAX - 1),
                       EntityEditValue(g_entityNpcEdits[2], EO_CHAR_MAX - 1),
                       EntityEditValue(g_entityNpcEdits[3], EO_SHORT_MAX - 1),
                       EntityEditValue(g_entityNpcEdits[1], EO_CHAR_MAX - 1)};
            if (id == 4026 && index >= 0 && index < static_cast<int>(draft.npcs.size()))
                draft.npcs[index] = npc;
            else
                draft.npcs.push_back(npc);
        }
        draft.dirtyScopes |= collaboration::EntityNpcs;
    } else if (id >= 4036 && id <= 4038) {
        int index = static_cast<int>(SendMessageA(g_entityItemList, LB_GETCURSEL, 0, 0));
        if (id == 4038) {
            if (index >= 0 && index < static_cast<int>(draft.items.size()))
                draft.items.erase(draft.items.begin() + index);
        } else {
            MapItem item{draft.x,
                         draft.y,
                         EntityEditValue(g_entityItemEdits[4], EO_SHORT_MAX - 1),
                         EntityEditValue(g_entityItemEdits[3], EO_CHAR_MAX - 1),
                         EntityEditValue(g_entityItemEdits[0], EO_SHORT_MAX - 1),
                         EntityEditValue(g_entityItemEdits[2], EO_SHORT_MAX - 1),
                         EntityEditValue(g_entityItemEdits[1], EO_THREE_MAX - 1)};
            if (id == 4037 && index >= 0 && index < static_cast<int>(draft.items.size()))
                draft.items[index] = item;
            else
                draft.items.push_back(item);
        }
        draft.dirtyScopes |= collaboration::EntityItems;
    } else if (id == 4040)
        CommitEntityDraft();
    else if (id == 4041)
        g_entityDraft = ReadEntitiesAt(draft.x, draft.y);
    else if (id == 4042)
        g_copiedEntities = draft;
    else if (id == 4043 && g_copiedEntities) {
        EntityDraft pasted = *g_copiedEntities;
        pasted.x = draft.x;
        pasted.y = draft.y;
        for (MapNpc& npc : pasted.npcs) {
            npc.x = pasted.x;
            npc.y = pasted.y;
        }
        for (MapItem& item : pasted.items) {
            item.x = pasted.x;
            item.y = pasted.y;
        }
        if (pasted.sign) {
            pasted.sign->x = pasted.x;
            pasted.sign->y = pasted.y;
        }
        pasted.generations = draft.generations;
        pasted.dirtyScopes = 15;
        g_entityDraft = std::move(pasted);
    } else if (id == 4044) {
        draft.warp.reset();
        draft.sign.reset();
        draft.npcs.clear();
        draft.items.clear();
        draft.dirtyScopes = 15;
    }
    SyncEntityControls();
}

void CreateEntityControls(HWND panel, HINSTANCE instance) {
    auto edit = [&](int id, DWORD extra = 0) {
        HWND h = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0", WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | extra, 0, 0,
                                 50, 22, panel, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
        SendMessageA(h, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        return h;
    };
    for (int i = 0; i < 5; ++i)
        g_entityWarpEdits[i] = edit(4000 + i, ES_NUMBER);
    g_entitySignEdits[0] = edit(4010);
    g_entitySignEdits[1] = edit(4011, ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL);
    g_entityNpcList = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL,
                                      0, 0, 100, 80, panel, reinterpret_cast<HMENU>(4020), instance, nullptr);
    for (int i = 0; i < 4; ++i)
        g_entityNpcEdits[i] = edit(4021 + i, ES_NUMBER);
    g_entityItemList = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_TABSTOP | LBS_NOTIFY | WS_VSCROLL,
                                       0, 0, 100, 80, panel, reinterpret_cast<HMENU>(4030), instance, nullptr);
    for (int i = 0; i < 5; ++i)
        g_entityItemEdits[i] = edit(4031 + i, ES_NUMBER);
    SendMessageA(g_entityNpcList, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    SendMessageA(g_entityItemList, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    const std::pair<int, const char*> buttons[] = {
        {4005, "Apply"},  {4006, "Delete"}, {4012, "Apply"}, {4013, "Delete"}, {4025, "Add"},
        {4026, "Update"}, {4027, "Delete"}, {4036, "Add"},   {4037, "Update"}, {4038, "Delete"},
        {4040, "Save"},   {4041, "Cancel"}, {4042, "Copy"},  {4043, "Paste"},  {4044, "Clear"}};
    for (std::size_t i = 0; i < std::size(buttons); ++i) {
        g_entityButtons[i] =
            CreateWindowExA(0, "BUTTON", buttons[i].second, WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 66, 24, panel,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(buttons[i].first)), instance, nullptr);
        SendMessageA(g_entityButtons[i], WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        SetWindowTheme(g_entityButtons[i], L"", L"");
    }
    LayoutEntityControls(panel);
    SyncEntityControls();
}

void LayoutLayerControls(HWND panel) {
    RECT client{};
    GetClientRect(panel, &client);
    const int width = std::max(90, static_cast<int>(client.right - 82));
    SetWindowPos(g_layerGraphicsCombo, nullptr, 70, kTitleHeight + 32, width, 180, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_layerFlagsCombo, nullptr, 70, kTitleHeight + 64, width, 180, SWP_NOZORDER | SWP_NOACTIVATE);
}

int ExactGraphicsPreset() {
    if (std::all_of(g_layerVisible.begin(), g_layerVisible.begin() + 9, [](bool value) { return value; }))
        return 0;
    if (std::none_of(g_layerVisible.begin(), g_layerVisible.begin() + 9, [](bool value) { return value; }))
        return 1;
    for (int category = 0; category < 5; ++category) {
        const auto selected = LayersInGraphicsGroup(category);
        bool exact = true;
        for (int layer = 0; layer < 9; ++layer) {
            const bool expected = std::find(selected.begin(), selected.end(), layer) != selected.end();
            exact &= g_layerVisible[layer] == expected;
        }
        if (exact)
            return category + 2;
    }
    return -1;
}

int ExactFlagsPreset() {
    if (std::all_of(g_flagVisible.begin(), g_flagVisible.end(), [](bool value) { return value; }))
        return 0;
    if (std::none_of(g_flagVisible.begin(), g_flagVisible.end(), [](bool value) { return value; }))
        return 1;
    for (int category = 0; category < 5; ++category) {
        bool exact = true;
        for (int index = 0; index < 5; ++index)
            exact &= g_flagVisible[index] == (index == category);
        if (exact)
            return category + 2;
    }
    return -1;
}

void SyncLayerControls() {
    if (!IsWindow(g_layerGraphicsCombo))
        return;
    const bool visible = g_layersTab == 1;
    ShowWindow(g_layerGraphicsCombo, visible ? SW_SHOW : SW_HIDE);
    ShowWindow(g_layerFlagsCombo, visible ? SW_SHOW : SW_HIDE);
    SendMessageA(g_layerGraphicsCombo, CB_SETCURSEL, ExactGraphicsPreset(), 0);
    SendMessageA(g_layerFlagsCombo, CB_SETCURSEL, ExactFlagsPreset(), 0);
}

void ApplyGraphicsPreset(int preset) {
    if (preset == 0 || preset == 1)
        SetAllLayersVisible(preset == 0);
    else if (preset >= 2 && preset < 7) {
        SetAllLayersVisible(false);
        for (int layer : LayersInGraphicsGroup(preset - 2))
            g_layerVisible[layer] = true;
        UpdateLayerGroupChecks();
        InvalidatePanels();
    }
}

void ApplyFlagsPreset(int preset) {
    if (preset == 0 || preset == 1)
        SetAllFlagsVisible(preset == 0);
    else if (preset >= 2 && preset < 7) {
        SetAllFlagsVisible(false);
        SetFlagVisibility(preset - 2, true);
    }
}

void CreateLayerControls(HWND panel, HINSTANCE instance) {
    g_layerGraphicsCombo =
        CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 120, 180, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLayerGraphicsPreset)), instance, nullptr);
    g_layerFlagsCombo =
        CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 120, 180, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLayerFlagsPreset)), instance, nullptr);
    const char* graphics[] = {"All layers", "No layers",  "Ground layer", "Object layer",
                              "Mask layer", "Wall layer", "Top layer"};
    const char* flags[] = {"All specials", "No specials", "Block only", "Door only",
                           "Chest only",   "Chair only",  "Warp only"};
    for (const char* item : graphics)
        SendMessageA(g_layerGraphicsCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    for (const char* item : flags)
        SendMessageA(g_layerFlagsCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    SendMessageA(g_layerGraphicsCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    SendMessageA(g_layerFlagsCombo, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    SetWindowTheme(g_layerGraphicsCombo, L"", L"");
    SetWindowTheme(g_layerFlagsCombo, L"", L"");
    LayoutLayerControls(panel);
    SyncLayerControls();
}

void LayoutFlagControls(HWND panel) {
    RECT client{};
    GetClientRect(panel, &client);
    const int top = kTitleHeight + 3 + 48;
    const int comboWidth = std::max(105, static_cast<int>(client.right - 64));
    SetWindowPos(g_doorRulesCombo, nullptr, 53, top, comboWidth, 150, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_chestRulesCombo, nullptr, 53, top, comboWidth, 150, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_chairDirectionCombo, nullptr, 65, top, std::max(93, static_cast<int>(client.right - 76)), 150,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_warpMapEdit, nullptr, 11, top + 1, 71, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_warpMapSpin, nullptr, 82, top + 1, 17, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_warpXEdit, nullptr, 125, top + 1, 55, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_warpXSpin, nullptr, 180, top + 1, 17, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_warpYEdit, nullptr, 210, top + 1, 55, 22, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(g_warpYSpin, nullptr, 265, top + 1, 17, 22, SWP_NOZORDER | SWP_NOACTIVATE);
}

bool IsFlagControlVisible(HWND control) {
    if (control == g_doorRulesCombo)
        return g_flagsTab == 2;
    if (control == g_chestRulesCombo)
        return g_flagsTab == 3;
    if (control == g_chairDirectionCombo)
        return g_flagsTab == 4;
    return g_flagsTab == 5;
}

void SyncFlagControls() {
    if (!IsWindow(g_doorRulesCombo))
        return;
    const HWND controls[] = {g_doorRulesCombo, g_chestRulesCombo, g_chairDirectionCombo, g_warpMapEdit, g_warpXEdit,
                             g_warpYEdit,      g_warpMapSpin,     g_warpXSpin,           g_warpYSpin};
    const HWND focus = GetFocus();
    for (HWND control : controls) {
        const bool visible = IsFlagControlVisible(control);
        if (!visible && focus == control)
            SetFocus(g_panels[static_cast<std::size_t>(PanelKind::Flags)]);
        ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
        EnableWindow(control, visible && g_map.loaded);
    }
    SendMessageA(g_doorRulesCombo, CB_SETCURSEL, g_warpSettings.door > 1 ? 1 : 0, 0);
    SendMessageA(g_chestRulesCombo, CB_SETCURSEL, 0, 0);
    SendMessageA(g_chairDirectionCombo, CB_SETCURSEL, std::clamp(g_chairDirection, 1, 7) - 1, 0);
    if (focus != g_warpMapEdit)
        SetWindowTextA(g_warpMapEdit, std::to_string(g_warpSettings.destinationMap).c_str());
    if (focus != g_warpXEdit)
        SetWindowTextA(g_warpXEdit, std::to_string(g_warpSettings.x).c_str());
    if (focus != g_warpYEdit)
        SetWindowTextA(g_warpYEdit, std::to_string(g_warpSettings.y).c_str());
}

int ReadFlagNumber(HWND edit, int fallback, int maximum) {
    char text[16]{};
    GetWindowTextA(edit, text, static_cast<int>(std::size(text)));
    char* end = nullptr;
    const long value = std::strtol(text, &end, 10);
    return end != text && *end == '\0' ? std::clamp(static_cast<int>(value), 0, maximum) : fallback;
}

void CommitWarpControls() {
    g_warpSettings.destinationMap = ReadFlagNumber(g_warpMapEdit, g_warpSettings.destinationMap, EO_SHORT_MAX - 1);
    g_warpSettings.x = ReadFlagNumber(g_warpXEdit, g_warpSettings.x, EO_CHAR_MAX - 1);
    g_warpSettings.y = ReadFlagNumber(g_warpYEdit, g_warpSettings.y, EO_CHAR_MAX - 1);
    UpdateWarpAtCursorFromSettings();
    SyncFlagControls();
    InvalidatePanels();
}

void CommitChairControl() {
    const int direction = static_cast<int>(SendMessageA(g_chairDirectionCombo, CB_GETCURSEL, 0, 0)) + 1;
    if (direction < 1 || direction > 7 || direction == g_chairDirection)
        return;
    g_chairDirection = direction;
    MapTile* tile = CursorTile();
    if (tile && tile->spec >= 1 && tile->spec <= 7 && tile->spec != direction) {
        PushUndoSnapshot();
        tile->spec = direction;
        g_map.dirty = true;
        UpdateMapTitle();
    }
    InvalidatePanels();
}

void CreateFlagControls(HWND panel, HINSTANCE instance) {
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_UPDOWN_CLASS};
    InitCommonControlsEx(&common);
    const DWORD comboStyle = WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL;
    g_doorRulesCombo =
        CreateWindowExA(0, "COMBOBOX", "", comboStyle, 0, 0, 120, 150, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDoorRulesCombo)), instance, nullptr);
    g_chestRulesCombo =
        CreateWindowExA(0, "COMBOBOX", "", comboStyle, 0, 0, 120, 150, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kChestRulesCombo)), instance, nullptr);
    g_chairDirectionCombo =
        CreateWindowExA(0, "COMBOBOX", "", comboStyle, 0, 0, 120, 150, panel,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kChairDirectionCombo)), instance, nullptr);
    SendMessageA(g_doorRulesCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Normal door"));
    SendMessageA(g_doorRulesCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Existing keyed door"));
    SendMessageA(g_chestRulesCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Normal chest"));
    const char* directions[] = {"Front left", "Front right", "Back left",     "Back right",
                                "Down right", "Up left",     "All directions"};
    for (const char* direction : directions)
        SendMessageA(g_chairDirectionCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(direction));
    const DWORD editStyle = WS_CHILD | WS_TABSTOP | ES_NUMBER | ES_AUTOHSCROLL;
    g_warpMapEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0", editStyle, 0, 0, 80, 22, panel,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kWarpMapEdit)), instance, nullptr);
    g_warpXEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0", editStyle, 0, 0, 64, 22, panel,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kWarpXEdit)), instance, nullptr);
    g_warpYEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0", editStyle, 0, 0, 64, 22, panel,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kWarpYEdit)), instance, nullptr);
    const DWORD spinStyle = WS_CHILD | UDS_SETBUDDYINT | UDS_ARROWKEYS | UDS_NOTHOUSANDS;
    g_warpMapSpin = CreateWindowExA(0, UPDOWN_CLASSA, "", spinStyle, 0, 0, 0, 0, panel,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kWarpMapSpin)), instance, nullptr);
    g_warpXSpin = CreateWindowExA(0, UPDOWN_CLASSA, "", spinStyle, 0, 0, 0, 0, panel,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kWarpXSpin)), instance, nullptr);
    g_warpYSpin = CreateWindowExA(0, UPDOWN_CLASSA, "", spinStyle, 0, 0, 0, 0, panel,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(kWarpYSpin)), instance, nullptr);
    SendMessageA(g_warpMapSpin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(g_warpMapEdit), 0);
    SendMessageA(g_warpXSpin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(g_warpXEdit), 0);
    SendMessageA(g_warpYSpin, UDM_SETBUDDY, reinterpret_cast<WPARAM>(g_warpYEdit), 0);
    SendMessageA(g_warpMapSpin, UDM_SETRANGE32, 0, EO_SHORT_MAX - 1);
    SendMessageA(g_warpXSpin, UDM_SETRANGE32, 0, EO_CHAR_MAX - 1);
    SendMessageA(g_warpYSpin, UDM_SETRANGE32, 0, EO_CHAR_MAX - 1);
    const HWND controls[] = {g_doorRulesCombo, g_chestRulesCombo, g_chairDirectionCombo, g_warpMapEdit, g_warpXEdit,
                             g_warpYEdit,      g_warpMapSpin,     g_warpXSpin,           g_warpYSpin};
    for (HWND control : controls) {
        SetWindowTheme(control, L"", L"");
        SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    }
    SendMessageA(g_warpMapEdit, EM_SETLIMITTEXT, 5, 0);
    SendMessageA(g_warpXEdit, EM_SETLIMITTEXT, 3, 0);
    SendMessageA(g_warpYEdit, EM_SETLIMITTEXT, 3, 0);
    LayoutFlagControls(panel);
    SyncFlagControls();
}

void ClearSelectedLayer() {
    if (!g_map.loaded)
        return;
    const int emptyValue = g_selectedLayer == 0 ? g_map.fillTile : -1;
    if (std::all_of(g_map.tiles.begin(), g_map.tiles.end(),
                    [emptyValue](const MapTile& tile) { return tile.graphics[g_selectedLayer] == emptyValue; }))
        return;
    PushUndoSnapshot();
    for (MapTile& tile : g_map.tiles) {
        tile.graphics[g_selectedLayer] = emptyValue;
    }
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

bool ClearGraphicLayerData(MapDocument& map, int layer) {
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

void ClearGraphicLayer(int layer) {
    if (!g_map.loaded || layer < 0 || layer >= 9)
        return;
    MapDocument updated = g_map;
    if (!ClearGraphicLayerData(updated, layer))
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::ClearGraphicLayer;
    operation.target = static_cast<std::uint8_t>(layer);
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active) {
        std::string error;
        const auto payload = collaboration::EncodeEditOperation(operation);
        if (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error))
            return;
        g_pendingCollaborationEdits.insert(operation.requestId);
        if (g_collaboration->State() == collaboration::SessionState::Connected)
            return;
    }
    PushUndoSnapshot();
    g_map = std::move(updated);
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

bool ClearFlagCategoryData(MapDocument& map, int category) {
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

void ClearFlagCategory(int category) {
    if (!g_map.loaded || category < 0 || category >= 5)
        return;
    MapDocument updated = g_map;
    if (!ClearFlagCategoryData(updated, category))
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::ClearFlagCategory;
    operation.target = static_cast<std::uint8_t>(category);
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active) {
        std::string error;
        const auto payload = collaboration::EncodeEditOperation(operation);
        if (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error))
            return;
        g_pendingCollaborationEdits.insert(operation.requestId);
        if (g_collaboration->State() == collaboration::SessionState::Connected)
            return;
    }
    PushUndoSnapshot();
    g_map = std::move(updated);
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void ClearMapFlags() {
    if (!g_map.loaded)
        return;
    const bool hasTileFlags = std::any_of(g_map.tiles.begin(), g_map.tiles.end(),
                                          [](const MapTile& tile) { return tile.spec >= 0 || tile.warp.has_value(); });
    if (!hasTileFlags && g_map.signs.empty() && g_map.legacyDoorKeys.empty())
        return;
    PushUndoSnapshot();
    for (MapTile& tile : g_map.tiles) {
        tile.spec = -1;
        tile.warp.reset();
    }
    g_map.signs.clear();
    g_map.legacyDoorKeys.clear();
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void ClearAllMapData() {
    if (!g_map.loaded)
        return;
    const bool hasGraphics = std::any_of(g_map.tiles.begin(), g_map.tiles.end(), [](const MapTile& tile) {
        return tile.spec >= 0 || tile.warp.has_value() ||
               std::any_of(tile.graphics.begin(), tile.graphics.end(), [](int id) { return id > 0; });
    });
    if (!hasGraphics && g_map.npcs.empty() && g_map.items.empty() && g_map.legacyDoorKeys.empty() &&
        g_map.signs.empty())
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::ClearAll;
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active) {
        std::string error;
        const auto payload = collaboration::EncodeEditOperation(operation);
        if (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error))
            return;
        g_pendingCollaborationEdits.insert(operation.requestId);
        if (g_collaboration->State() == collaboration::SessionState::Connected)
            return;
    }
    PushUndoSnapshot();
    for (MapTile& tile : g_map.tiles) {
        tile = MapTile{};
        tile.graphics[0] = g_map.fillTile;
    }
    g_map.npcs.clear();
    g_map.items.clear();
    g_map.legacyDoorKeys.clear();
    g_map.signs.clear();
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void ToggleLayerVisibility(int layer) {
    if (layer < 0 || layer >= 9)
        return;
    g_layerVisible[layer] = !g_layerVisible[layer];
    UpdateLayerGroupChecks();
    InvalidatePanels();
}

void SetAllLayersVisible(bool visible) {
    for (int layer = 0; layer < 9; ++layer)
        g_layerVisible[layer] = visible;
    UpdateLayerGroupChecks();
    InvalidatePanels();
}

void ToggleAllLayers() {
    const bool allVisible =
        std::all_of(g_layerVisible.begin(), g_layerVisible.begin() + 9, [](bool visible) { return visible; });
    SetAllLayersVisible(!allVisible);
}

void SetBaseTileGraphic(int graphic) {
    if (!g_map.loaded || graphic < 0 || g_map.fillTile == graphic)
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::BaseTile;
    operation.baseTile = graphic;
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active) {
        std::string error;
        const auto payload = collaboration::EncodeEditOperation(operation);
        if (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error))
            return;
        g_pendingCollaborationEdits.insert(operation.requestId);
    }
    PushUndoSnapshot();
    const int oldFill = g_map.fillTile;
    g_map.fillTile = graphic;
    for (MapTile& tile : g_map.tiles) {
        if (tile.graphics[0] == oldFill || tile.graphics[0] < 0)
            tile.graphics[0] = g_map.fillTile;
    }
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void BeginBaseTileSelection() {
    if (!g_map.loaded || g_panels.size() <= static_cast<std::size_t>(PanelKind::Graphics))
        return;
    g_selectingBaseTile = true;
    SetGraphicsCategory(0);
    HWND graphics = g_panels[static_cast<std::size_t>(PanelKind::Graphics)];
    ShowWindow(graphics, SW_SHOW);
    CheckMenuItem(g_windowsMenu, kPanelBaseCommand + static_cast<int>(PanelKind::Graphics), MF_BYCOMMAND | MF_CHECKED);
    g_activePanel = graphics;
    SetFocus(graphics);
    EnsureEditorWindowZOrder(graphics);
    InvalidatePanels();
}

bool UpdateCursorFromPoint(HWND window, POINT point) {
    RECT client{};
    GetClientRect(window, &client);
    const RECT canvas = ViewerCanvasRect(client);
    if (point.x < canvas.left || point.x >= canvas.right || point.y < canvas.top || point.y >= canvas.bottom) {
        return false;
    }

    const CameraTile tile = g_camera.ScreenToMap(
        CameraPoint{static_cast<double>(point.x), static_cast<double>(point.y)}, CameraViewportFromRect(canvas));
    const int x = tile.x;
    const int y = tile.y;
    if (g_map.loaded && (x < 0 || y < 0 || x >= g_map.width || y >= g_map.height)) {
        if (g_cursorX != -1 || g_cursorY != -1) {
            g_cursorX = -1;
            g_cursorY = -1;
            PublishPanelPresence(PanelKind::Viewer);
            return true;
        }
        return false;
    }
    if (x == g_cursorX && y == g_cursorY) {
        return false;
    }
    g_cursorX = x;
    g_cursorY = y;
    InspectFlagAtCursor(false);
    PublishPanelPresence(PanelKind::Viewer);
    return true;
}

bool CollaborationEditMatchesMap(const collaboration::EditOperation& operation) {
    if (operation.kind == collaboration::EditKind::Graphics) {
        for (const auto& edit : operation.graphics)
            if (g_map.tile(edit.x, edit.y).graphics[edit.layer] != edit.graphic)
                return false;
        return true;
    }
    if (operation.kind == collaboration::EditKind::Flags) {
        for (const auto& edit : operation.flags) {
            const MapTile& tile = g_map.tile(edit.x, edit.y);
            if (tile.spec != edit.spec || tile.warp.has_value() != edit.hasWarp)
                return false;
            if (edit.hasWarp && (tile.warp->destinationMap != edit.warp.destinationMap || tile.warp->x != edit.warp.x ||
                                 tile.warp->y != edit.warp.y || tile.warp->level != edit.warp.level ||
                                 tile.warp->door != edit.warp.door))
                return false;
        }
        return true;
    }
    return false;
}
void ApplyCollaborationEdit(const collaboration::EditOperation& operation) {
    using collaboration::EditKind;
    if (operation.kind == EditKind::Graphics) {
        for (const auto& edit : operation.graphics)
            g_map.tile(edit.x, edit.y).graphics[edit.layer] = edit.graphic;
        return;
    }
    if (operation.kind == EditKind::Flags) {
        for (const auto& edit : operation.flags) {
            MapTile& tile = g_map.tile(edit.x, edit.y);
            tile.spec = edit.spec;
            if (edit.hasWarp)
                tile.warp =
                    MapWarp{edit.warp.destinationMap, edit.warp.x, edit.warp.y, edit.warp.level, edit.warp.door};
            else
                tile.warp.reset();
        }
        return;
    }
    if (operation.kind == EditKind::BaseTile) {
        const int old = g_map.fillTile;
        g_map.fillTile = operation.baseTile;
        for (auto& tile : g_map.tiles)
            if (tile.graphics[0] == old || tile.graphics[0] < 0)
                tile.graphics[0] = g_map.fillTile;
        return;
    }
    if (operation.kind == EditKind::ResizeMap) {
        ResizeMapDocument(g_map, operation.width, operation.height);
        g_cursorX = std::clamp(g_cursorX, 0, g_map.width - 1);
        g_cursorY = std::clamp(g_cursorY, 0, g_map.height - 1);
        return;
    }
    if (operation.kind == EditKind::ClearGraphicLayer) {
        ClearGraphicLayerData(g_map, operation.target);
        return;
    }
    if (operation.kind == EditKind::ClearFlagCategory) {
        ClearFlagCategoryData(g_map, operation.target);
        return;
    }
    if (operation.kind == EditKind::ClearAll) {
        for (auto& tile : g_map.tiles) {
            tile = MapTile{};
            tile.graphics[0] = g_map.fillTile;
        }
        g_map.npcs.clear();
        g_map.items.clear();
        g_map.legacyDoorKeys.clear();
        g_map.signs.clear();
        return;
    }
    for (const auto& edit : operation.entities) {
        const int x = edit.x, y = edit.y;
        if (edit.scopes & collaboration::EntityWarp) {
            if (edit.hasWarp)
                g_map.tile(x, y).warp =
                    MapWarp{edit.warp.destinationMap, edit.warp.x, edit.warp.y, edit.warp.level, edit.warp.door};
            else
                g_map.tile(x, y).warp.reset();
        }
        if (edit.scopes & collaboration::EntitySign) {
            std::erase_if(g_map.signs, [=](const MapSign& v) { return v.x == x && v.y == y; });
            if (edit.hasSign)
                g_map.signs.push_back({x, y, edit.sign.titleLength, edit.sign.encodedText});
        }
        if (edit.scopes & collaboration::EntityNpcs) {
            std::erase_if(g_map.npcs, [=](const MapNpc& v) { return v.x == x && v.y == y; });
            for (const auto& n : edit.npcs)
                g_map.npcs.push_back({x, y, n.id, n.spawnType, n.spawnTime, n.amount});
        }
        if (edit.scopes & collaboration::EntityItems) {
            std::erase_if(g_map.items, [=](const MapItem& v) { return v.x == x && v.y == y; });
            for (const auto& i : edit.items)
                g_map.items.push_back({x, y, i.key, i.chestSlot, i.id, i.spawnTime, static_cast<int>(i.amount)});
        }
    }
}
bool CollaborationGenerationsMatch(const collaboration::EditOperation& operation) {
    if (operation.kind != collaboration::EditKind::Entities)
        return true;
    for (const auto& e : operation.entities) {
        if ((e.scopes & collaboration::EntityWarp) && e.warpGeneration != EntityGeneration(0, e.x, e.y))
            return false;
        if ((e.scopes & collaboration::EntitySign) && e.signGeneration != EntityGeneration(1, e.x, e.y))
            return false;
        if ((e.scopes & collaboration::EntityNpcs) && e.npcGeneration != EntityGeneration(2, e.x, e.y))
            return false;
        if ((e.scopes & collaboration::EntityItems) && e.itemGeneration != EntityGeneration(3, e.x, e.y))
            return false;
    }
    return true;
}
void AdvanceCollaborationGenerations(const collaboration::EditOperation& operation) {
    if (operation.kind == collaboration::EditKind::Flags) {
        for (const auto& e : operation.flags)
            ++g_entityGenerations[EntityGenerationKey(0, e.x, e.y)];
        return;
    }
    if (operation.kind == collaboration::EditKind::ClearFlagCategory &&
        (operation.target == 1 || operation.target == 4)) {
        for (int y = 0; y < g_map.height; ++y)
            for (int x = 0; x < g_map.width; ++x)
                ++g_entityGenerations[EntityGenerationKey(0, x, y)];
        return;
    }
    if (operation.kind == collaboration::EditKind::ClearAll) {
        for (int y = 0; y < g_map.height; ++y)
            for (int x = 0; x < g_map.width; ++x)
                for (int scope = 0; scope < 4; ++scope)
                    ++g_entityGenerations[EntityGenerationKey(scope, x, y)];
        return;
    }
    if (operation.kind != collaboration::EditKind::Entities)
        return;
    for (const auto& e : operation.entities)
        for (int scope = 0; scope < 4; ++scope)
            if (e.scopes & (1 << scope))
                ++g_entityGenerations[EntityGenerationKey(scope, e.x, e.y)];
}
void RefreshEntityDraftAfterRemote(const collaboration::EditOperation& operation) {
    if (!g_entityDraft)
        return;
    bool touches = false;
    if (operation.kind == collaboration::EditKind::Flags)
        for (const auto& e : operation.flags)
            touches |= e.x == g_entityDraft->x && e.y == g_entityDraft->y;
    else if (operation.kind == collaboration::EditKind::Entities)
        for (const auto& e : operation.entities)
            touches |= e.x == g_entityDraft->x && e.y == g_entityDraft->y;
    if (!touches)
        return;
    if (g_entityDraft->dirtyScopes)
        g_entityDraft->stale = true;
    else
        g_entityDraft = ReadEntitiesAt(g_entityDraft->x, g_entityDraft->y);
    SyncEntityControls();
}

void ApplySelectedTool() {
    if (g_brushSize == 0 || !g_map.loaded || g_cursorX < 0 || g_cursorY < 0 || g_cursorX >= g_map.width ||
        g_cursorY >= g_map.height) {
        return;
    }

    const bool erasing = g_editTool == EditTool::Eraser || g_editTool == EditTool::Wipe;
    const int replacement = erasing ? (g_selectedLayer == 0 ? g_map.fillTile : -1) : g_editorState.selectedGraphic();
    const int radius = g_brushSize / 2;
    bool changed = false;
    for (int y = std::max(0, g_cursorY - radius); y <= std::min(g_map.height - 1, g_cursorY + radius); ++y) {
        for (int x = std::max(0, g_cursorX - radius); x <= std::min(g_map.width - 1, g_cursorX + radius); ++x) {
            changed |= g_map.tile(x, y).graphics[g_selectedLayer] != replacement;
        }
    }
    if (!changed)
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::Graphics;
    for (int y = std::max(0, g_cursorY - radius); y <= std::min(g_map.height - 1, g_cursorY + radius); ++y) {
        for (int x = std::max(0, g_cursorX - radius); x <= std::min(g_map.width - 1, g_cursorX + radius); ++x) {
            operation.graphics.push_back({static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                          static_cast<std::uint8_t>(g_selectedLayer), replacement});
        }
    }
    const auto payload = collaboration::EncodeEditOperation(operation);
    std::string error;
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active && (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error)))
        return;
    BeginEditSnapshot();
    ApplyCollaborationEdit(operation);
    if (active)
        g_pendingCollaborationEdits.insert(operation.requestId);
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void HandleGraphicsClick(HWND window, POINT point) {
    RECT client{};
    GetClientRect(window, &client);
    const PaletteLayout layout = GetPaletteLayout(client);
    const RECT content = layout.content;
    if (const int category = classic_ui::HitTestTabs(layout.tabs, 6, point); category >= 0) {
        SetGraphicsCategory(category);
        InvalidatePanels();
        return;
    }

    const int bank = PaletteBankForCurrentLayer();
    const auto& graphicIds = g_gfx.GraphicIds(bank);
    const int pageCount = std::max(1, static_cast<int>((graphicIds.size() + layout.capacity - 1) / layout.capacity));
    g_palettePage = std::clamp(g_palettePage, 0, pageCount - 1);
    for (int row = 0; row < layout.rows; ++row) {
        for (int column = 0; column < layout.columns; ++column) {
            const int left = layout.gridLeft + column * (layout.cellWidth + layout.gapX);
            const int top = layout.gridTop + row * (layout.cellHeight + layout.gapY);
            if (point.x >= left && point.x < left + layout.cellWidth && point.y >= top &&
                point.y < top + layout.cellHeight) {
                const std::size_t resourceIndex =
                    static_cast<std::size_t>(g_palettePage) * layout.capacity + row * layout.columns + column;
                if (resourceIndex < graphicIds.size()) {
                    g_editorState.selectedGraphic() = graphicIds[resourceIndex];
                    SetSelectedLayer(g_selectedLayer);
                    if (g_selectingBaseTile) {
                        g_selectingBaseTile = false;
                        SetBaseTileGraphic(g_editorState.selectedGraphic());
                    }
                    InvalidatePanels();
                }
                return;
            }
        }
    }

    if (point.y >= content.bottom - 20 && point.y < content.bottom - 2) {
        if (point.x < content.left + 30 && g_palettePage > 0) {
            --g_palettePage;
            InvalidatePanels();
        } else if (point.x > content.right - 30 && g_palettePage + 1 < pageCount) {
            ++g_palettePage;
            InvalidatePanels();
        }
    }
}

void HandleToolsetClick(HWND window, POINT point) {
    RECT client{};
    GetClientRect(window, &client);
    const int contentTop = kTitleHeight + 3;
    const int relativeY = point.y - contentTop;
    if (point.x < 7 || point.x > client.right - 10) {
        return;
    }
    const RECT tabs{10, contentTop + 3, client.right - 10, contentTop + 22};
    if (const int tab = classic_ui::HitTestTabs(tabs, 3, point); tab >= 0) {
        g_toolsetTab = tab;
    } else if (g_toolsetTab == 0) {
        if (relativeY >= 25 && relativeY < 47)
            g_editTool = EditTool::Pencil;
        else if (relativeY >= 50 && relativeY < 72)
            g_editTool = EditTool::Brush;
        else if (relativeY >= 75 && relativeY < 97)
            g_editTool = EditTool::Eraser;
        else if (relativeY >= 100 && relativeY < 122)
            g_editTool = EditTool::Wipe;
        else
            return;
    } else if (g_toolsetTab == 1) {
        if (relativeY >= 29 && relativeY < 53)
            SetEditDomain(false);
        else if (relativeY >= 58 && relativeY < 82)
            SetEditDomain(true);
        else if (relativeY >= 87 && relativeY < 111)
            ExecuteCommand(g_mainWindow, kEntitiesModeCommand);
        else
            return;
    } else {
        if (relativeY >= 29 && relativeY < 53)
            g_brushSize = 1;
        else if (relativeY >= 58 && relativeY < 82)
            g_brushSize = 3;
        else if (relativeY >= 87 && relativeY < 111)
            g_brushSize = 0;
        else
            return;
    }
    UpdateToolMenuChecks();
    InvalidatePanels();
}

void HandleLayerPanelClick(HWND window, POINT point) {
    RECT client{};
    GetClientRect(window, &client);
    const RECT content{3, kTitleHeight + 3, client.right - 3, client.bottom - 3};
    const RECT tabs{content.left + 7, content.top + 3, content.right - 7, content.top + 22};
    if (const int tab = classic_ui::HitTestTabs(tabs, 2, point); tab >= 0) {
        g_layersTab = tab;
        SyncLayerControls();
        InvalidatePanels();
        return;
    }
    if (g_layersTab == 1)
        return;
    const int rowHeight = 20;
    const int top = content.top + 26;
    const int row = (point.y - top) / rowHeight;
    if (row < 0)
        return;
    const int layer = g_layerScroll + row;
    const int layerCount = g_layersTab == 0 ? 9 : 12;
    if (layer >= layerCount)
        return;
    if (layer < 9) {
        SetSelectedLayer(layer);
    }
    InvalidatePanels();
}

void HandlePropertiesClick(HWND window, POINT point) {
    RECT client{};
    GetClientRect(window, &client);
    const RECT content{3, kTitleHeight + 3, client.right - 3, client.bottom - 3};
    const RECT tabs{content.left + 7, content.top + 3, content.right - 7, content.top + 22};
    if (const int tab = classic_ui::HitTestTabs(tabs, 4, point); tab >= 0) {
        g_propertiesTab = tab;
        SyncMapPropertyControls();
        InvalidatePanels();
        return;
    }
    if (g_propertiesTab == 1 && point.x >= content.left + 67 && point.y >= content.top + 26) {
        const int rowHeight = std::max(21, static_cast<int>(content.bottom - content.top - 31) / 5);
        const int row = static_cast<int>(point.y - (content.top + 26)) / rowHeight;
        if (row >= 1 && row <= 4 && row < 5) {
            PushUndoSnapshot();
            if (row == 1)
                g_map.type = g_map.type == 3 ? 0 : 3;
            else if (row == 2)
                g_map.effect = (g_map.effect + 1) % 7;
            else if (row == 3)
                g_map.musicId = (g_map.musicId + 1) % 254;
            else
                g_map.ambientSoundId = (g_map.ambientSoundId + 1) % (EO_SHORT_MAX + 1);
            g_map.dirty = true;
            UpdateMapTitle();
            InvalidatePanels();
        }
    } else if (g_propertiesTab == 2 && point.x >= content.left + 77 && point.y >= content.top + 27) {
        const int row = static_cast<int>(point.y - (content.top + 27)) / 25;
        if (row == 0 || row == 1) {
            PushUndoSnapshot();
            if (row == 0)
                g_map.mapAvailable = !g_map.mapAvailable;
            else
                g_map.canScroll = !g_map.canScroll;
            g_map.dirty = true;
            UpdateMapTitle();
            InvalidatePanels();
        }
    }
}

void ZoomAtPoint(POINT point, bool zoomIn) {
    double target = zoomIn ? kZoomLevels[std::size(kZoomLevels) - 1] : kZoomLevels[0];
    if (zoomIn) {
        for (double level : kZoomLevels) {
            if (level > g_zoom + 0.0001) {
                target = level;
                break;
            }
        }
    } else {
        for (std::size_t index = std::size(kZoomLevels); index > 0; --index) {
            if (kZoomLevels[index - 1] < g_zoom - 0.0001) {
                target = kZoomLevels[index - 1];
                break;
            }
        }
    }
    RECT client{};
    GetClientRect(g_panels[static_cast<std::size_t>(PanelKind::Viewer)], &client);
    const RECT canvas = ViewerCanvasRect(client);
    g_camera.ZoomAtPoint(target, CameraPoint{static_cast<double>(point.x), static_cast<double>(point.y)},
                         CameraViewportFromRect(canvas));
    HWND viewer = g_panels[static_cast<std::size_t>(PanelKind::Viewer)];
    UpdateViewerScrollbars(viewer);
    InvalidateRect(viewer, nullptr, FALSE);
}

void ApplyFlagToCursor() {
    if (g_brushSize == 0 || !g_map.loaded || g_cursorX < 0 || g_cursorY < 0 || g_cursorX >= g_map.width ||
        g_cursorY >= g_map.height)
        return;
    const auto applyFlag = [](MapTile tile) {
        switch (g_flagsTab) {
        case 0:
            tile.spec = -1;
            tile.warp.reset();
            break;
        case 1:
            tile.spec = 0;
            break;
        case 2:
            tile.warp = g_warpSettings;
            tile.warp->door = std::max(1, g_warpSettings.door);
            break;
        case 3:
            tile.spec = 9;
            break;
        case 4:
            tile.spec = g_chairDirection;
            break;
        case 5:
            tile.warp = g_warpSettings;
            tile.warp->door = 0;
            break;
        }
        return tile;
    };
    const int radius = g_brushSize / 2;
    bool changed = false;
    for (int y = std::max(0, g_cursorY - radius); y <= std::min(g_map.height - 1, g_cursorY + radius); ++y) {
        for (int x = std::max(0, g_cursorX - radius); x <= std::min(g_map.width - 1, g_cursorX + radius); ++x) {
            changed |= applyFlag(g_map.tile(x, y)) != g_map.tile(x, y);
        }
    }
    if (!changed)
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::Flags;
    for (int y = std::max(0, g_cursorY - radius); y <= std::min(g_map.height - 1, g_cursorY + radius); ++y) {
        for (int x = std::max(0, g_cursorX - radius); x <= std::min(g_map.width - 1, g_cursorX + radius); ++x) {
            const MapTile result = applyFlag(g_map.tile(x, y));
            collaboration::FlagEdit edit;
            edit.x = static_cast<std::uint16_t>(x);
            edit.y = static_cast<std::uint16_t>(y);
            edit.spec = static_cast<std::int16_t>(result.spec);
            edit.hasWarp = result.warp.has_value();
            if (result.warp)
                edit.warp = {static_cast<std::uint16_t>(result.warp->destinationMap),
                             static_cast<std::uint8_t>(result.warp->x), static_cast<std::uint8_t>(result.warp->y),
                             static_cast<std::uint8_t>(result.warp->level),
                             static_cast<std::uint16_t>(result.warp->door)};
            operation.flags.push_back(edit);
        }
    }
    const auto payload = collaboration::EncodeEditOperation(operation);
    std::string error;
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active && (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error)))
        return;
    BeginEditSnapshot();
    ApplyCollaborationEdit(operation);
    if (active)
        g_pendingCollaborationEdits.insert(operation.requestId);
    g_map.dirty = true;
    UpdateMapTitle();
    InvalidatePanels();
}

void UpdateWarpAtCursorFromSettings() {
    MapTile* tile = CursorTile();
    if (!tile || !tile->warp)
        return;
    MapWarp updated = g_warpSettings;
    updated.door = g_flagsTab == 2 ? std::max(1, updated.door) : 0;
    if (*tile->warp == updated)
        return;
    collaboration::EditOperation operation;
    operation.requestId = g_nextCollaborationRequestId++;
    operation.kind = collaboration::EditKind::Flags;
    collaboration::FlagEdit edit;
    edit.x = static_cast<std::uint16_t>(g_cursorX);
    edit.y = static_cast<std::uint16_t>(g_cursorY);
    edit.spec = static_cast<std::int16_t>(tile->spec);
    edit.hasWarp = true;
    edit.warp = {static_cast<std::uint16_t>(updated.destinationMap), static_cast<std::uint8_t>(updated.x),
                 static_cast<std::uint8_t>(updated.y), static_cast<std::uint8_t>(updated.level),
                 static_cast<std::uint16_t>(updated.door)};
    operation.flags.push_back(edit);
    const auto payload = collaboration::EncodeEditOperation(operation);
    std::string error;
    const bool active = g_collaboration && (g_collaboration->State() == collaboration::SessionState::Hosting ||
                                            g_collaboration->State() == collaboration::SessionState::Connected);
    if (active && (g_pendingCollaborationEdits.size() >= 1024 || !g_collaboration->SubmitOperation(payload, error)))
        return;
    PushUndoSnapshot();
    ApplyCollaborationEdit(operation);
    if (active)
        g_pendingCollaborationEdits.insert(operation.requestId);
    g_map.dirty = true;
    UpdateMapTitle();
}

void HandleFlagsClick(HWND window, POINT point) {
    RECT client{};
    GetClientRect(window, &client);
    RECT tabs{10, kTitleHeight + 6, client.right - 10, kTitleHeight + 25};
    if (const int tab = classic_ui::HitTestTabs(tabs, 6, point); tab >= 0) {
        g_flagsTab = tab;
        if (g_flagsTab == 2)
            g_warpSettings.door = std::max(1, g_warpSettings.door);
        SyncFlagControls();
        InvalidatePanels();
        return;
    }
    ApplyFlagToCursor();
}

void ReleasePanelBackBuffer(PanelData& panel) {
    if (panel.backBufferDc && panel.backBufferPreviousBitmap) {
        SelectObject(panel.backBufferDc, panel.backBufferPreviousBitmap);
    }
    if (panel.backBufferBitmap)
        DeleteObject(panel.backBufferBitmap);
    if (panel.backBufferDc)
        DeleteDC(panel.backBufferDc);
    panel.backBufferDc = nullptr;
    panel.backBufferBitmap = nullptr;
    panel.backBufferPreviousBitmap = nullptr;
    panel.backBufferSize = SIZE{};
}

bool EnsurePanelBackBuffer(PanelData& panel, HDC target, int width, int height) {
    if (width <= 0 || height <= 0)
        return false;
    if (panel.backBufferDc && panel.backBufferSize.cx == width && panel.backBufferSize.cy == height)
        return true;
    ReleasePanelBackBuffer(panel);
    panel.backBufferDc = CreateCompatibleDC(target);
    panel.backBufferBitmap = CreateCompatibleBitmap(target, width, height);
    if (!panel.backBufferDc || !panel.backBufferBitmap) {
        ReleasePanelBackBuffer(panel);
        return false;
    }
    panel.backBufferPreviousBitmap = static_cast<HBITMAP>(SelectObject(panel.backBufferDc, panel.backBufferBitmap));
    panel.backBufferSize = SIZE{width, height};
    return true;
}

void EnsureEditorWindowZOrder(HWND activePalette) {
    if (g_panels.empty())
        return;
    HWND viewer = g_panels[static_cast<std::size_t>(PanelKind::Viewer)];
    if (IsWindow(viewer)) {
        SetWindowPos(viewer, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (activePalette && activePalette != viewer && IsWindow(activePalette) && IsWindowVisible(activePalette)) {
        SetWindowPos(activePalette, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void UpdateViewerScrollbars(HWND viewer, bool clampCamera) {
    if (!IsWindow(viewer))
        return;
    RECT client{};
    GetClientRect(viewer, &client);
    const RECT canvas = ViewerCanvasRect(client);
    if (!g_map.loaded || canvas.right <= canvas.left || canvas.bottom <= canvas.top) {
        SCROLLINFO empty{sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 0, 1, 0, 0};
        SetScrollInfo(viewer, SB_HORZ, &empty, TRUE);
        SetScrollInfo(viewer, SB_VERT, &empty, TRUE);
        return;
    }

    const CameraBounds bounds =
        g_camera.ProjectedScreenBounds(g_map.width, g_map.height, CameraViewportFromRect(canvas), false);
    const int margin = std::max(32, static_cast<int>(std::lround(128.0 * g_camera.zoom)));
    SCROLLINFO horizontal{sizeof(SCROLLINFO)};
    horizontal.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    horizontal.nMin = static_cast<int>(std::floor(bounds.left - canvas.left)) - margin;
    horizontal.nMax = static_cast<int>(std::ceil(bounds.right - canvas.left)) + margin - 1;
    horizontal.nPage = static_cast<UINT>(std::max(1L, canvas.right - canvas.left));
    horizontal.nPos = static_cast<int>(std::lround(-g_camera.offsetX));
    SetScrollInfo(viewer, SB_HORZ, &horizontal, TRUE);

    SCROLLINFO vertical{sizeof(SCROLLINFO)};
    vertical.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    vertical.nMin = static_cast<int>(std::floor(bounds.top - canvas.top)) - margin;
    vertical.nMax = static_cast<int>(std::ceil(bounds.bottom - canvas.top)) + margin - 1;
    vertical.nPage = static_cast<UINT>(std::max(1L, canvas.bottom - canvas.top));
    vertical.nPos = static_cast<int>(std::lround(-g_camera.offsetY));
    SetScrollInfo(viewer, SB_VERT, &vertical, TRUE);

    if (clampCamera) {
        horizontal.fMask = SIF_POS;
        vertical.fMask = SIF_POS;
        GetScrollInfo(viewer, SB_HORZ, &horizontal);
        GetScrollInfo(viewer, SB_VERT, &vertical);
        g_camera.offsetX = -static_cast<double>(horizontal.nPos);
        g_camera.offsetY = -static_cast<double>(vertical.nPos);
    }
}

void HandleViewerScroll(HWND viewer, int bar, WPARAM wParam) {
    SCROLLINFO info{sizeof(SCROLLINFO)};
    info.fMask = SIF_ALL;
    if (!GetScrollInfo(viewer, bar, &info))
        return;
    int position = info.nPos;
    const int line = std::max(8, static_cast<int>(std::lround(32.0 * g_camera.zoom)));
    const int page = std::max(1, static_cast<int>(info.nPage) * 3 / 4);
    switch (LOWORD(wParam)) {
    case SB_LINELEFT:
        position -= line;
        break;
    case SB_LINERIGHT:
        position += line;
        break;
    case SB_PAGELEFT:
        position -= page;
        break;
    case SB_PAGERIGHT:
        position += page;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        position = info.nTrackPos;
        break;
    default:
        return;
    }
    info.fMask = SIF_POS;
    info.nPos = position;
    SetScrollInfo(viewer, bar, &info, TRUE);
    GetScrollInfo(viewer, bar, &info);
    if (bar == SB_HORZ)
        g_camera.offsetX = -static_cast<double>(info.nPos);
    else
        g_camera.offsetY = -static_cast<double>(info.nPos);
    InvalidateRect(viewer, nullptr, FALSE);
}

LRESULT CALLBACK PanelProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* panel = reinterpret_cast<PanelData*>(GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message) {
    case WM_CTLCOLOREDIT: {
        static HBRUSH editBrush = CreateSolidBrush(classic_ui::Edit);
        HDC controlDc = reinterpret_cast<HDC>(wParam);
        SetTextColor(controlDc, classic_ui::Text);
        SetBkColor(controlDc, classic_ui::Edit);
        return reinterpret_cast<LRESULT>(editBrush);
    }
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        static HBRUSH controlBrush = CreateSolidBrush(classic_ui::Control);
        HDC controlDc = reinterpret_cast<HDC>(wParam);
        SetTextColor(controlDc, classic_ui::Text);
        SetBkColor(controlDc, classic_ui::Control);
        return reinterpret_cast<LRESULT>(controlBrush);
    }
    case WM_NCCREATE: {
        const auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    case WM_NCHITTEST: {
        const LRESULT hit = DefWindowProcA(window, message, wParam, lParam);
        if (hit != HTCLIENT) {
            return hit;
        }
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(window, &point);
        RECT client{};
        GetClientRect(window, &client);
        if (point.y < kTitleHeight && point.x < client.right - 24) {
            return HTCAPTION;
        }
        return HTCLIENT;
    }
    case WM_NCLBUTTONDOWN: {
        g_activePanel = window;
        SetFocus(window);
        InvalidateRect(window, nullptr, TRUE);
        const LRESULT result = DefWindowProcA(window, message, wParam, lParam);
        EnsureEditorWindowZOrder(panel && panel->kind != PanelKind::Viewer ? window : nullptr);
        return result;
    }
    case WM_LBUTTONDOWN: {
        g_activePanel = window;
        SetFocus(window);
        EnsureEditorWindowZOrder(panel && panel->kind != PanelKind::Viewer ? window : nullptr);
        InvalidateRect(window, nullptr, TRUE);
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        RECT client{};
        GetClientRect(window, &client);
        if (point.y < kTitleHeight && point.x > client.right - 24) {
            if (panel && panel->kind == PanelKind::Entities) {
                g_entityMode = false;
                UpdateToolMenuChecks();
            }
            ShowWindow(window, SW_HIDE);
            CheckMenuItem(g_windowsMenu, kPanelBaseCommand + static_cast<int>(panel->kind),
                          MF_BYCOMMAND | MF_UNCHECKED);
            if (g_activePanel == window) {
                g_activePanel = nullptr;
                for (HWND candidate : g_panels) {
                    if (candidate != window && IsWindowVisible(candidate)) {
                        g_activePanel = candidate;
                        SetFocus(candidate);
                        break;
                    }
                }
                InvalidatePanels();
            }
            return 0;
        }
        if (panel && panel->kind == PanelKind::Viewer) {
            const RECT canvas = ViewerCanvasRect(client);
            if (!PtInRect(&canvas, point)) {
                return 0;
            }
            for (const auto& hit : g_presenceHits)
                if (PtInRect(&hit.bounds, point)) {
                    g_viewCenterX = hit.x;
                    g_viewCenterY = hit.y;
                    g_camera.centerTileX = hit.x;
                    g_camera.centerTileY = hit.y;
                    g_camera.offsetX = 0;
                    g_camera.offsetY = 0;
                    UpdateViewerScrollbars(window);
                    InvalidateRect(window, nullptr, FALSE);
                    return 0;
                }
            if (UpdateCursorFromPoint(window, point)) {
                InvalidateRect(window, nullptr, FALSE);
            }
            if (g_entityMode) {
                g_entityDraft = ReadEntitiesAt(g_cursorX, g_cursorY);
                SyncEntityControls();
                return 0;
            }
            SetCapture(window);
            if (IsFlagsDomain())
                ApplyFlagToCursor();
            else
                ApplySelectedTool();
            return 0;
        }
        if (panel && panel->kind == PanelKind::Graphics) {
            HandleGraphicsClick(window, point);
            return 0;
        }
        if (panel && panel->kind == PanelKind::Toolset) {
            HandleToolsetClick(window, point);
            return 0;
        }
        if (panel && panel->kind == PanelKind::Layers) {
            HandleLayerPanelClick(window, point);
            return 0;
        }
        if (panel && panel->kind == PanelKind::Properties) {
            HandlePropertiesClick(window, point);
            return 0;
        }
        if (panel && panel->kind == PanelKind::Flags) {
            HandleFlagsClick(window, point);
            return 0;
        }
        if (panel && panel->kind == PanelKind::Entities) {
            g_entityMode = true;
            const RECT content{3, kTitleHeight + 3, client.right - 3, client.bottom - 3};
            const RECT tabs{content.left + 7, content.top + 3, content.right - 7, content.top + 22};
            if (const int tab = classic_ui::HitTestTabs(tabs, 4, point); tab >= 0) {
                g_entitiesTab = tab;
                SyncEntityControls();
            }
            return 0;
        }
        break;
    }
    case WM_RBUTTONDOWN:
        if (panel && panel->kind == PanelKind::Viewer) {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            RECT client{};
            GetClientRect(window, &client);
            const RECT canvas = ViewerCanvasRect(client);
            if (PtInRect(&canvas, point)) {
                UpdateCursorFromPoint(window, point);
                if (g_entityMode) {
                    g_entityDraft = ReadEntitiesAt(g_cursorX, g_cursorY);
                    HMENU popup = CreatePopupMenu();
                    const bool empty = !g_entityDraft->warp && !g_entityDraft->sign && g_entityDraft->npcs.empty() &&
                                       g_entityDraft->items.empty();
                    AppendMenuA(popup, MF_STRING | (empty ? MF_GRAYED : 0), 1, "Copy Entities");
                    AppendMenuA(popup, MF_STRING | (g_copiedEntities ? 0 : MF_GRAYED), 2, "Paste Entities");
                    AppendMenuA(popup, MF_STRING | (empty ? MF_GRAYED : 0), 3, "Clear Entities");
                    POINT screenPoint = point;
                    ClientToScreen(window, &screenPoint);
                    const int choice = TrackPopupMenu(popup, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenPoint.x,
                                                      screenPoint.y, 0, window, nullptr);
                    DestroyMenu(popup);
                    if (choice == 1)
                        g_copiedEntities = g_entityDraft;
                    else if (choice == 2) {
                        HandleEntityCommand(4043);
                        CommitEntityDraft();
                    } else if (choice == 3) {
                        HandleEntityCommand(4044);
                        CommitEntityDraft();
                    }
                    SyncEntityControls();
                    return 0;
                }
                InspectFlagAtCursor(true);
                SetEditDomain(true);
                UpdateToolMenuChecks();
                InvalidatePanels();
            }
            return 0;
        }
        break;
    case WM_NOTIFY:
        if (panel && panel->kind == PanelKind::Flags) {
            auto* change = reinterpret_cast<NMUPDOWN*>(lParam);
            if (change && change->hdr.code == UDN_DELTAPOS) {
                if (change->hdr.idFrom == kWarpMapSpin)
                    g_warpSettings.destinationMap = std::clamp(change->iPos + change->iDelta, 0, EO_SHORT_MAX - 1);
                else if (change->hdr.idFrom == kWarpXSpin)
                    g_warpSettings.x = std::clamp(change->iPos + change->iDelta, 0, EO_CHAR_MAX - 1);
                else if (change->hdr.idFrom == kWarpYSpin)
                    g_warpSettings.y = std::clamp(change->iPos + change->iDelta, 0, EO_CHAR_MAX - 1);
                else
                    break;
                UpdateWarpAtCursorFromSettings();
                SyncFlagControls();
                InvalidatePanels();
                return 0;
            }
        }
        break;
    case WM_COMMAND:
        if (panel && panel->kind == PanelKind::Entities && HIWORD(wParam) == LBN_SELCHANGE && g_entityDraft) {
            if (LOWORD(wParam) == 4020) {
                const int index = static_cast<int>(SendMessageA(g_entityNpcList, LB_GETCURSEL, 0, 0));
                if (index >= 0 && index < static_cast<int>(g_entityDraft->npcs.size())) {
                    const MapNpc& npc = g_entityDraft->npcs[index];
                    const int values[] = {npc.id, npc.amount, npc.spawnType, npc.spawnTime};
                    for (int i = 0; i < 4; ++i)
                        SetEntityEditValue(g_entityNpcEdits[i], values[i]);
                }
            } else if (LOWORD(wParam) == 4030) {
                const int index = static_cast<int>(SendMessageA(g_entityItemList, LB_GETCURSEL, 0, 0));
                if (index >= 0 && index < static_cast<int>(g_entityDraft->items.size())) {
                    const MapItem& item = g_entityDraft->items[index];
                    const int values[] = {item.id, item.amount, item.spawnTime, item.chestSlot, item.key};
                    for (int i = 0; i < 5; ++i)
                        SetEntityEditValue(g_entityItemEdits[i], values[i]);
                }
            }
            return 0;
        }
        if (panel && panel->kind == PanelKind::Entities && LOWORD(wParam) >= 4000 && LOWORD(wParam) <= 4044) {
            if (HIWORD(wParam) == BN_CLICKED)
                HandleEntityCommand(LOWORD(wParam));
            return 0;
        }
        if (panel && panel->kind == PanelKind::Flags) {
            if (LOWORD(wParam) == kDoorRulesCombo && HIWORD(wParam) == CBN_SELCHANGE) {
                if (SendMessageA(g_doorRulesCombo, CB_GETCURSEL, 0, 0) == 0)
                    g_warpSettings.door = 1;
                UpdateWarpAtCursorFromSettings();
                InvalidatePanels();
                return 0;
            }
            if (LOWORD(wParam) == kChairDirectionCombo && HIWORD(wParam) == CBN_SELCHANGE) {
                CommitChairControl();
                return 0;
            }
            if ((LOWORD(wParam) == kWarpMapEdit || LOWORD(wParam) == kWarpXEdit || LOWORD(wParam) == kWarpYEdit) &&
                HIWORD(wParam) == EN_KILLFOCUS) {
                CommitWarpControls();
                return 0;
            }
        }
        if (panel && panel->kind == PanelKind::Layers && HIWORD(wParam) == CBN_SELCHANGE) {
            const HWND combo = reinterpret_cast<HWND>(lParam);
            const int selection = static_cast<int>(SendMessageA(combo, CB_GETCURSEL, 0, 0));
            if (LOWORD(wParam) == kLayerGraphicsPreset)
                ApplyGraphicsPreset(selection);
            else if (LOWORD(wParam) == kLayerFlagsPreset)
                ApplyFlagsPreset(selection);
            SyncLayerControls();
            return 0;
        }
        if (panel && panel->kind == PanelKind::Properties) {
            if (LOWORD(wParam) == kMapDimensionsChange && HIWORD(wParam) == BN_CLICKED) {
                CommitMapPropertyDimensions(window);
                return 0;
            }
            if (LOWORD(wParam) == kBaseTileChange && HIWORD(wParam) == BN_CLICKED) {
                BeginBaseTileSelection();
                return 0;
            }
        }
        break;
    case WM_SETFOCUS:
        g_activePanel = window;
        if (panel)
            PublishPanelPresence(panel->kind);
        EnsureEditorWindowZOrder(panel && panel->kind != PanelKind::Viewer ? window : nullptr);
        InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_SIZE:
        if (panel && panel->kind == PanelKind::Viewer)
            UpdateViewerScrollbars(window);
        if (panel && panel->kind == PanelKind::Properties)
            LayoutMapPropertyControls(window);
        if (panel && panel->kind == PanelKind::Layers)
            LayoutLayerControls(window);
        if (panel && panel->kind == PanelKind::Flags)
            LayoutFlagControls(window);
        if (panel && panel->kind == PanelKind::Entities)
            LayoutEntityControls(window);
        InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_KILLFOCUS:
        if (panel && panel->kind == PanelKind::Viewer)
            ResetViewerNavigation(window);
        InvalidateRect(window, nullptr, TRUE);
        return 0;
    case WM_TIMER:
        if (panel && panel->kind == PanelKind::Viewer && wParam == kViewerNavigationTimer) {
            if (!g_viewerNavigation.any()) {
                KillTimer(window, kViewerNavigationTimer);
                return 0;
            }
            if (ApplyCameraNavigation(g_camera, g_viewerNavigation, kViewerNavigationIntervalMs / 1000.0,
                                      kViewerNavigationPixelsPerSecond)) {
                UpdateViewerScrollbars(window);
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        }
        break;
    case WM_HSCROLL:
        if (panel && panel->kind == PanelKind::Viewer) {
            HandleViewerScroll(window, SB_HORZ, wParam);
            return 0;
        }
        break;
    case WM_VSCROLL:
        if (panel && panel->kind == PanelKind::Viewer) {
            HandleViewerScroll(window, SB_VERT, wParam);
            return 0;
        }
        break;
    case WM_MOUSEMOVE:
        if (panel && panel->kind == PanelKind::Viewer) {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            if (g_panning && (wParam & MK_MBUTTON)) {
                g_camera.Pan(point.x - g_lastPanPoint.x, point.y - g_lastPanPoint.y);
                g_lastPanPoint = point;
                UpdateViewerScrollbars(window);
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
            if (UpdateCursorFromPoint(window, point)) {
                InvalidateRect(window, nullptr, FALSE);
                if (g_panels.size() > static_cast<std::size_t>(PanelKind::Flags)) {
                    InvalidateRect(g_panels[static_cast<std::size_t>(PanelKind::Flags)], nullptr, FALSE);
                }
            }
            if ((wParam & MK_LBUTTON) && (g_editTool == EditTool::Brush || g_editTool == EditTool::Wipe)) {
                if (IsFlagsDomain())
                    ApplyFlagToCursor();
                else
                    ApplySelectedTool();
            }
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        CommitStroke();
        if (GetCapture() == window) {
            ReleaseCapture();
        }
        return 0;
    case WM_MBUTTONDOWN:
        if (panel && panel->kind == PanelKind::Viewer) {
            g_panning = true;
            g_lastPanPoint = POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            SetCapture(window);
            return 0;
        }
        break;
    case WM_MBUTTONUP:
        if (g_panning && GetCapture() == window) {
            g_panning = false;
            ReleaseCapture();
            return 0;
        }
        break;
    case WM_MOUSEWHEEL:
        if (panel && panel->kind == PanelKind::Viewer) {
            POINT cursor{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(window, &cursor);
            ZoomAtPoint(cursor, GET_WHEEL_DELTA_WPARAM(wParam) > 0);
            return 0;
        }
        if (panel && panel->kind == PanelKind::Graphics) {
            RECT client{};
            GetClientRect(window, &client);
            const int pages = PalettePageCount(g_paletteCategory, GetPaletteLayout(client).capacity);
            g_palettePage = std::clamp(g_palettePage + (GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 1 : -1), 0, pages - 1);
            InvalidatePanels();
            return 0;
        }
        if (panel && panel->kind == PanelKind::Layers) {
            g_layerScroll = std::clamp(g_layerScroll + (GET_WHEEL_DELTA_WPARAM(wParam) < 0 ? 1 : -1), 0, 8);
            InvalidatePanels();
            return 0;
        }
        break;
    case WM_KEYDOWN: {
        const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        if (panel && panel->kind == PanelKind::Viewer && !control && !alt &&
            (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN)) {
            if (wParam == VK_LEFT)
                g_viewerNavigation.left = true;
            else if (wParam == VK_RIGHT)
                g_viewerNavigation.right = true;
            else if (wParam == VK_UP)
                g_viewerNavigation.up = true;
            else
                g_viewerNavigation.down = true;
            SetTimer(window, kViewerNavigationTimer, kViewerNavigationIntervalMs, nullptr);
            return 0;
        }
        if (control && wParam == 'O')
            ExecuteCommand(g_mainWindow, kOpenMapCommand);
        else if (control && wParam == 'S')
            ExecuteCommand(g_mainWindow, kSaveMapCommand);
        else if (control && wParam == 'N')
            ExecuteCommand(g_mainWindow, kNewMapCommand);
        else if (control && wParam == 'Z' && (GetKeyState(VK_SHIFT) & 0x8000))
            ExecuteCommand(g_mainWindow, 1301);
        else if (control && wParam == 'Z')
            ExecuteCommand(g_mainWindow, 1300);
        else if (control && (wParam == 'Y' || (wParam == 'Z' && (GetKeyState(VK_SHIFT) & 0x8000))))
            ExecuteCommand(g_mainWindow, 1301);
        else if (control && wParam == 'P')
            ExecuteCommand(g_mainWindow, 1310);
        else if (control && wParam == 'B')
            ExecuteCommand(g_mainWindow, 1311);
        else if (control && wParam == 'E')
            ExecuteCommand(g_mainWindow, 1312);
        else if (alt && wParam == 'W')
            ExecuteCommand(g_mainWindow, 1120);
        else if (alt && wParam == 'Q')
            ExecuteCommand(g_mainWindow, 1121);
        else if (alt && wParam == 'S')
            ExecuteCommand(g_mainWindow, 1122);
        else if (alt && wParam == 'A')
            ExecuteCommand(g_mainWindow, 1123);
        else if (alt && wParam == 'C')
            ExecuteCommand(g_mainWindow, kClearAllCommand);
        else if (wParam == VK_F1)
            ExecuteCommand(g_mainWindow, kToggleAllLayersCommand);
        else if (wParam == VK_F2)
            ExecuteCommand(g_mainWindow, kToggleAllFlagsCommand);
        else if (wParam == VK_F3)
            ExecuteCommand(g_mainWindow, kToggleAllBoundariesCommand);
        else if (wParam == VK_F5)
            ExecuteCommand(g_mainWindow, kHideAllLayersCommand);
        else if (wParam == VK_F6)
            ExecuteCommand(g_mainWindow, kHideAllFlagsCommand);
        else if (wParam == VK_F7)
            ExecuteCommand(g_mainWindow, kHideAllBoundariesCommand);
        else if (wParam == VK_F11)
            ExecuteCommand(g_mainWindow, kGraphicsModeCommand);
        else if (wParam == VK_F12)
            ExecuteCommand(g_mainWindow, kFlagsModeCommand);
        else if (wParam == VK_F10)
            ExecuteCommand(g_mainWindow, kEntitiesModeCommand);
        else if (wParam == VK_ADD || wParam == VK_OEM_PLUS)
            ExecuteCommand(g_mainWindow, kZoomInCommand);
        else if (wParam == VK_SUBTRACT || wParam == VK_OEM_MINUS)
            ExecuteCommand(g_mainWindow, kZoomOutCommand);
        return 0;
    }
    case WM_KEYUP:
        if (panel && panel->kind == PanelKind::Viewer &&
            (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN)) {
            if (wParam == VK_LEFT)
                g_viewerNavigation.left = false;
            else if (wParam == VK_RIGHT)
                g_viewerNavigation.right = false;
            else if (wParam == VK_UP)
                g_viewerNavigation.up = false;
            else
                g_viewerNavigation.down = false;
            if (!g_viewerNavigation.any())
                KillTimer(window, kViewerNavigationTimer);
            return 0;
        }
        break;
    case WM_GETDLGCODE:
        if (panel && panel->kind == PanelKind::Viewer)
            return DLGC_WANTARROWS;
        break;
    case WM_WINDOWPOSCHANGING: {
        auto* position = reinterpret_cast<WINDOWPOS*>(lParam);
        if (panel && position) {
            int minWidth = 150;
            int minHeight = 100;
            if (panel->kind == PanelKind::Viewer) {
                minWidth = 260;
                minHeight = 180;
            } else if (panel->kind == PanelKind::Graphics) {
                minWidth = 300;
                minHeight = 225;
            } else if (panel->kind == PanelKind::Layers) {
                minWidth = 185;
                minHeight = 250;
            } else if (panel->kind == PanelKind::Properties) {
                minWidth = 300;
                minHeight = 190;
            } else if (panel->kind == PanelKind::Flags) {
                minWidth = 310;
                minHeight = 115;
            } else if (panel->kind == PanelKind::Toolset) {
                minWidth = 195;
                minHeight = 150;
            } else if (panel->kind == PanelKind::Entities) {
                minWidth = 420;
                minHeight = 300;
            }
            position->cx = std::max(position->cx, minWidth);
            position->cy = std::max(position->cy, minHeight);
        }
        break;
    }
    case WM_GETMINMAXINFO:
        if (panel) {
            auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
            if (panel->kind == PanelKind::Properties) {
                limits->ptMinTrackSize.x = 300;
                limits->ptMinTrackSize.y = 190;
                return 0;
            }
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        HDC renderDc = dc;
        if (panel && panel->kind == PanelKind::Viewer &&
            EnsurePanelBackBuffer(*panel, dc, client.right, client.bottom)) {
            renderDc = panel->backBufferDc;
        }
        HBRUSH frame = CreateSolidBrush(classic_ui::Panel);
        FillRect(renderDc, &client, frame);
        DeleteObject(frame);
        FrameRect(renderDc, &client, GetSysColorBrush(COLOR_BTNSHADOW));

        RECT title{2, 2, client.right - 2, kTitleHeight};
        HBRUSH titleBrush =
            CreateSolidBrush(g_activePanel == window ? classic_ui::ActiveTitle : classic_ui::InactiveTitle);
        FillRect(renderDc, &title, titleBrush);
        DeleteObject(titleBrush);
        DrawText(renderDc, panel ? panel->title : "", RECT{20, 4, client.right - 25, kTitleHeight - 1},
                 g_activePanel == window ? RGB(255, 255, 255) : classic_ui::Text);
        if (panel) {
            int markerX = client.right - 34;
            const auto area = PresenceAreaForPanel(panel->kind);
            for (const auto& [id, presence] : g_remotePresence) {
                if (!g_collaboration || id == g_collaboration->LocalUserId() || presence.area != area)
                    continue;
                const auto participant = CollaborationParticipant(id);
                if (!participant)
                    continue;
                HBRUSH marker = CreateSolidBrush(PresenceColor(participant->color));
                RECT box{markerX, 7, markerX + 6, 13};
                FillRect(renderDc, &box, marker);
                DeleteObject(marker);
                markerX -= 9;
            }
        }
        HBRUSH iconBrush = CreateSolidBrush(g_activePanel == window ? classic_ui::Highlight : classic_ui::Panel);
        RECT icon{7, 6, 15, 14};
        FillRect(renderDc, &icon, iconBrush);
        DeleteObject(iconBrush);
        RECT closeButton{client.right - 18, 3, client.right - 4, kTitleHeight - 2};
        classic_ui::DrawCloseButton(renderDc, closeButton);
        if (panel) {
            DrawPanelContents(renderDc, client, *panel);
        }
        if (renderDc != dc) {
            BitBlt(dc, paint.rcPaint.left, paint.rcPaint.top, paint.rcPaint.right - paint.rcPaint.left,
                   paint.rcPaint.bottom - paint.rcPaint.top, renderDc, paint.rcPaint.left, paint.rcPaint.top, SRCCOPY);
        }
        EndPaint(window, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_NCDESTROY:
        if (panel && panel->kind == PanelKind::Viewer)
            ResetViewerNavigation(window);
        if (panel)
            ReleasePanelBackBuffer(*panel);
        break;
    }

    return DefWindowProcA(window, message, wParam, lParam);
}

void InvalidatePanels() {
    SyncLayerControls();
    SyncFlagControls();
    SyncEntityControls();
    for (HWND panel : g_panels) {
        if (IsWindow(panel)) {
            InvalidateRect(panel, nullptr, TRUE);
        }
    }
}

void SaveMap(HWND owner, bool saveAs);

constexpr wchar_t kSaveCommentDialogClass[] = L"EndlessMapStudioSaveCommentDialog";
constexpr wchar_t kSaveResultDialogClass[] = L"EndlessMapStudioSaveResultDialog";
constexpr int kSaveCommentEdit = 6100;

std::wstring Utf8ToWide(std::string_view value) {
    if (value.empty())
        return {};
    const int count =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0)
        throw std::runtime_error("Unable to display UTF-8 text.");
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(),
                        count);
    return result;
}

std::string WideToUtf8(std::wstring_view value) {
    if (value.empty())
        return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (count <= 0)
        throw std::runtime_error("The save comment contains invalid Unicode.");
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(),
                        count, nullptr, nullptr);
    return result;
}

void AddSaveDialogControl(HWND parent, const wchar_t* kind, const wchar_t* text, DWORD style, int x, int y, int width,
                          int height, int id, DWORD extended = 0) {
    HWND control =
        CreateWindowExW(extended, kind, text, WS_CHILD | WS_VISIBLE | style, x, y, width, height, parent,
                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    if (g_uiFont)
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
}

struct SaveCommentDialogData {
    bool accepted = false;
    std::string comment;
};

void WriteDialogAcceptanceArtifact(const std::string& name, const std::string& value) {
    if (!g_acceptanceFinal || g_acceptanceDirectory.empty())
        return;
    std::error_code ignored;
    std::filesystem::create_directories(g_acceptanceDirectory, ignored);
    std::ofstream output(g_acceptanceDirectory / name, std::ios::binary | std::ios::trunc);
    output << value;
}

LRESULT CALLBACK SaveCommentDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* data = reinterpret_cast<SaveCommentDialogData*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_CREATE) {
        data = reinterpret_cast<SaveCommentDialogData*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        AddSaveDialogControl(window, L"STATIC", L"Optional comments on your map:", SS_LEFT, 16, 16, 350, 18, 0);
        AddSaveDialogControl(window, L"EDIT", L"",
                             WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL, 16, 38, 350, 112,
                             kSaveCommentEdit, WS_EX_CLIENTEDGE);
        SendMessageW(GetDlgItem(window, kSaveCommentEdit), EM_SETLIMITTEXT, 4096, 0);
        AddSaveDialogControl(window, L"STATIC",
                             L"The comment is stored in mapper metadata; the EMF remains EO-compatible.", SS_LEFT, 16,
                             158, 350, 30, 0);
        AddSaveDialogControl(window, L"BUTTON", L"Save", WS_TABSTOP | BS_DEFPUSHBUTTON, 206, 194, 76, 25, IDOK);
        AddSaveDialogControl(window, L"BUTTON", L"Cancel", WS_TABSTOP | BS_PUSHBUTTON, 290, 194, 76, 25, IDCANCEL);
        SetFocus(GetDlgItem(window, kSaveCommentEdit));
        if (g_acceptanceFinal && g_acceptanceDialogAction != AcceptanceDialogAction::Manual) {
            if (g_acceptanceDialogAction == AcceptanceDialogAction::Save) {
                const std::wstring comment = Utf8ToWide(g_acceptanceDialogComment);
                SetWindowTextW(GetDlgItem(window, kSaveCommentEdit), comment.c_str());
            }
            WriteDialogAcceptanceArtifact(g_acceptanceDialogToken + "-comment-visible.flag", "visible");
            SetTimer(window, 1, 100, nullptr);
        }
        return 0;
    }
    if (message == WM_TIMER && wParam == 1 && g_acceptanceFinal) {
        const bool needsCapture = g_acceptanceDialogToken == "primary";
        if (needsCapture && !std::filesystem::exists(g_acceptanceDirectory / "primary-comment-captured.flag"))
            return 0;
        KillTimer(window, 1);
        if (g_acceptanceDialogAction == AcceptanceDialogAction::Cancel)
            SendMessageW(window, WM_COMMAND, IDCANCEL, 0);
        else {
            const std::wstring comment = Utf8ToWide(g_acceptanceDialogComment);
            SetWindowTextW(GetDlgItem(window, kSaveCommentEdit), comment.c_str());
            SendMessageW(window, WM_COMMAND, IDOK, 0);
        }
        return 0;
    }
    if (message == WM_COMMAND && data) {
        if (LOWORD(wParam) == IDOK) {
            HWND edit = GetDlgItem(window, kSaveCommentEdit);
            const int length = GetWindowTextLengthW(edit);
            std::wstring wide(static_cast<std::size_t>(length) + 1, L'\0');
            GetWindowTextW(edit, wide.data(), length + 1);
            wide.resize(static_cast<std::size_t>(length));
            try {
                data->comment = WideToUtf8(wide);
                std::string error;
                if (!save_workflow::ValidateComment(data->comment, error)) {
                    MessageBoxA(window, error.c_str(), "Save comment", MB_OK | MB_ICONWARNING);
                    return 0;
                }
            } catch (const std::exception& exception) {
                MessageBoxA(window, exception.what(), "Save comment", MB_OK | MB_ICONWARNING);
                return 0;
            }
            data->accepted = true;
            DestroyWindow(window);
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(window);
            return 0;
        }
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool ShowSaveCommentDialog(HWND owner, std::string& comment) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW cls{sizeof(cls)};
        cls.lpfnWndProc = SaveCommentDialogProc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        cls.lpszClassName = kSaveCommentDialogClass;
        registered = RegisterClassExW(&cls) != 0;
    }
    SaveCommentDialogData data;
    RECT ownerBounds{};
    GetWindowRect(owner, &ownerBounds);
    constexpr int width = 400, height = 264;
    const int x = ownerBounds.left + std::max(0L, (ownerBounds.right - ownerBounds.left - width) / 2);
    const int y = ownerBounds.top + std::max(0L, (ownerBounds.bottom - ownerBounds.top - height) / 2);
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kSaveCommentDialogClass, L"Save map comment",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height, owner, nullptr,
                                  GetModuleHandleW(nullptr), &data);
    if (!dialog)
        return false;
    g_acceptanceModal = true;
    EnableWindow(owner, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    MSG message{};
    while (IsWindow(dialog) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    g_acceptanceModal = false;
    if (data.accepted)
        comment = std::move(data.comment);
    return data.accepted;
}

struct SaveResultDialogData {
    save_workflow::ResultPresentation presentation;
    std::string warning;
};

LRESULT CALLBACK SaveResultDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* data = reinterpret_cast<SaveResultDialogData*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_CREATE) {
        data = reinterpret_cast<SaveResultDialogData*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        const std::wstring filename = Utf8ToWide(data->presentation.filename);
        const std::wstring size = Utf8ToWide(data->presentation.size);
        const std::wstring date = Utf8ToWide(data->presentation.date);
        const std::wstring comment = Utf8ToWide(data->presentation.comment);
        AddSaveDialogControl(window, L"STATIC", L"Map file created", SS_LEFT, 72, 18, 300, 22, 0);
        AddSaveDialogControl(window, L"STATIC", L"File name:", SS_LEFT, 22, 62, 82, 18, 0);
        AddSaveDialogControl(window, L"STATIC", filename.c_str(), SS_LEFT, 108, 62, 275, 18, 0);
        AddSaveDialogControl(window, L"STATIC", L"Map size:", SS_LEFT, 22, 84, 82, 18, 0);
        AddSaveDialogControl(window, L"STATIC", size.c_str(), SS_LEFT, 108, 84, 275, 18, 0);
        AddSaveDialogControl(window, L"STATIC", L"Date:", SS_LEFT, 22, 106, 82, 18, 0);
        AddSaveDialogControl(window, L"STATIC", date.c_str(), SS_LEFT, 108, 106, 275, 18, 0);
        AddSaveDialogControl(window, L"STATIC", L"comments on your map :", SS_LEFT, 22, 137, 360, 18, 0);
        AddSaveDialogControl(window, L"EDIT", comment.c_str(), ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                             22, 158, 361, 85, 0, WS_EX_CLIENTEDGE);
        if (!data->warning.empty()) {
            const std::wstring warning = Utf8ToWide(data->warning);
            AddSaveDialogControl(window, L"STATIC", warning.c_str(), SS_LEFT, 22, 249, 361, 32, 0);
        }
        AddSaveDialogControl(window, L"BUTTON", L"OK", WS_TABSTOP | BS_DEFPUSHBUTTON, 174, 286, 76, 25, IDOK);
        if (g_acceptanceFinal && g_acceptanceDialogAction != AcceptanceDialogAction::Manual) {
            WriteDialogAcceptanceArtifact(g_acceptanceDialogToken + "-result-visible.flag", "visible");
            SetTimer(window, 2, 100, nullptr);
        }
        return 0;
    }
    if (message == WM_TIMER && wParam == 2 && g_acceptanceFinal) {
        const bool needsCapture = g_acceptanceDialogToken == "primary";
        if (needsCapture && !std::filesystem::exists(g_acceptanceDirectory / "primary-result-captured.flag"))
            return 0;
        KillTimer(window, 2);
        SendMessageW(window, WM_COMMAND, IDOK, 0);
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        POINT diamond[]{{24, 28}, {43, 17}, {62, 28}, {43, 39}};
        HBRUSH mapBrush = CreateSolidBrush(RGB(55, 135, 70));
        HPEN mapPen = CreatePen(PS_SOLID, 1, RGB(25, 80, 35));
        HGDIOBJ oldBrush = SelectObject(dc, mapBrush);
        HGDIOBJ oldPen = SelectObject(dc, mapPen);
        Polygon(dc, diamond, 4);
        SelectObject(dc, oldBrush);
        SelectObject(dc, oldPen);
        DeleteObject(mapBrush);
        DeleteObject(mapPen);
        HPEN check = CreatePen(PS_SOLID, 3, RGB(20, 105, 35));
        oldPen = SelectObject(dc, check);
        MoveToEx(dc, 35, 28, nullptr);
        LineTo(dc, 41, 34);
        LineTo(dc, 54, 21);
        SelectObject(dc, oldPen);
        DeleteObject(check);
        EndPaint(window, &paint);
        return 0;
    }
    if (message == WM_COMMAND && LOWORD(wParam) == IDOK) {
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowSaveResultDialog(HWND owner, SaveResultDialogData& data) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW cls{sizeof(cls)};
        cls.lpfnWndProc = SaveResultDialogProc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        cls.lpszClassName = kSaveResultDialogClass;
        registered = RegisterClassExW(&cls) != 0;
    }
    RECT ownerBounds{};
    GetWindowRect(owner, &ownerBounds);
    constexpr int width = 425, height = 360;
    const int x = ownerBounds.left + std::max(0L, (ownerBounds.right - ownerBounds.left - width) / 2);
    const int y = ownerBounds.top + std::max(0L, (ownerBounds.bottom - ownerBounds.top - height) / 2);
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME, kSaveResultDialogClass, L"Map file created",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height, owner, nullptr,
                                  GetModuleHandleW(nullptr), &data);
    if (!dialog)
        return;
    g_acceptanceModal = true;
    EnableWindow(owner, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    MSG message{};
    while (IsWindow(dialog) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    g_acceptanceModal = false;
}

void OpenMap(HWND owner) {
    char path[MAX_PATH] = {};
    OPENFILENAMEA dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = "Endless Map Files (*.emf)\0*.emf\0All Files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&dialog)) {
        return;
    }

    if (g_map.dirty) {
        const int choice = MessageBoxA(owner, "Save changes to the current map before opening another?",
                                       "Unsaved map changes", MB_YESNOCANCEL | MB_ICONWARNING);
        if (choice == IDCANCEL)
            return;
        if (choice == IDYES) {
            SaveMap(owner, false);
            if (g_map.dirty)
                return;
        }
    }

    try {
        MapDocument loadedMap = ReadEmf(path);
        loadedMap.dirty = false;
        g_map = std::move(loadedMap);
        g_cursorX = std::min(8, g_map.width - 1);
        g_cursorY = std::min(14, g_map.height - 1);
        g_viewCenterX = g_cursorX;
        g_viewCenterY = g_cursorY;
        g_camera.offsetX = 0.0;
        g_camera.offsetY = 0.0;
        g_camera.zoom = 1.0;
        g_undoMaps.clear();
        g_redoMaps.clear();
        UpdateMapTitle();
        if (!g_panels.empty())
            UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
    } catch (const std::exception& error) {
        MessageBoxA(owner, error.what(), "Unable to open map", MB_OK | MB_ICONERROR);
    }
    InvalidatePanels();
}

void SaveMap(HWND owner, bool saveAs) {
    if (!g_map.loaded) {
        return;
    }

    const collaboration::SessionState collaborationState =
        g_collaboration ? g_collaboration->State() : collaboration::SessionState::Disconnected;
    const save_workflow::SessionMode saveMode =
        collaborationState == collaboration::SessionState::Hosting     ? save_workflow::SessionMode::Hosting
        : collaborationState == collaboration::SessionState::Connected ? save_workflow::SessionMode::Connected
                                                                       : save_workflow::SessionMode::Disconnected;
    if (!save_workflow::CanSave(saveMode)) {
        ShowClassicNotice(save_workflow::SaveDeniedMessage());
        return;
    }

    const std::string previousPath = g_map.path;
    std::string path = g_map.path;
    if (saveAs || path.empty()) {
        if (g_acceptanceFinal && g_acceptanceSaveAsPath) {
            path = g_acceptanceSaveAsPath->string();
        } else {
            char selectedPath[MAX_PATH] = {};
            OPENFILENAMEA dialog{};
            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = owner;
            dialog.lpstrFilter = "Endless Map Files (*.emf)\0*.emf\0All Files (*.*)\0*.*\0\0";
            dialog.lpstrFile = selectedPath;
            dialog.nMaxFile = MAX_PATH;
            dialog.lpstrDefExt = "emf";
            dialog.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
            if (!GetSaveFileNameA(&dialog)) {
                return;
            }
            path = selectedPath;
        }
    }

    try {
        std::string comment;
        if (!ShowSaveCommentDialog(owner, comment)) {
            g_acceptanceDialogAction = AcceptanceDialogAction::Manual;
            return;
        }
        const std::vector<std::uint8_t> emf = WriteEmf(g_map);
        save_workflow::Metadata metadata;
        metadata.comment = comment;
        metadata.collaborative = saveMode == save_workflow::SessionMode::Hosting;
        metadata.author = "Local mapper";
        if (metadata.collaborative && g_collaboration) {
            for (const auto& participant : g_collaboration->Participants()) {
                if (participant.host) {
                    metadata.author = participant.displayName;
                    break;
                }
            }
        }
        metadata.timestamp = save_workflow::UtcTimestamp();
        const save_workflow::SaveResult result = save_workflow::SaveMapFile(
            std::filesystem::path(path), emf, metadata, g_acceptanceSaveFailure,
            saveAs && !previousPath.empty() ? std::filesystem::path(previousPath) : std::filesystem::path{});
        if (g_acceptanceFinal)
            g_lastSaveResult = result;
        if (!result.mapSaved)
            throw std::runtime_error(result.error.empty() ? "Unable to save the map file." : result.error);

        g_map.path = path;
        g_map.dirty = false;
        UpdateMapTitle();
        metadata.map = std::filesystem::path(path).filename().string();
        metadata.fileSize = result.fileSize;
        metadata.timestamp = result.timestamp;

        if (!result.metadataSaved) {
            ShowClassicNotice("Map saved, but its metadata could not be written.");
        }
        if (save_workflow::ShouldBroadcastSave(saveMode, result.mapSaved) && g_collaboration) {
            std::string broadcastError;
            const auto seconds =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
            if (!g_collaboration->BroadcastSave(static_cast<std::uint64_t>(seconds), broadcastError))
                ShowClassicNotice("Map saved, but collaborators could not be notified.");
        }
        SaveResultDialogData dialog;
        dialog.presentation = save_workflow::BuildResultPresentation(std::filesystem::path(path), metadata);
        dialog.warning = result.metadataSaved ? "" : "The EMF was saved, but mapper metadata could not be written.";
        ShowSaveResultDialog(owner, dialog);
        g_acceptanceDialogAction = AcceptanceDialogAction::Manual;
        g_acceptanceSaveFailure = {};
        g_acceptanceSaveAsPath.reset();
    } catch (const std::exception& error) {
        g_acceptanceDialogAction = AcceptanceDialogAction::Manual;
        g_acceptanceSaveFailure = {};
        g_acceptanceSaveAsPath.reset();
        if (g_acceptanceFinal) {
            ShowClassicNotice(error.what());
            WriteDialogAcceptanceArtifact("save-failure-notice.flag", error.what());
        } else
            MessageBoxA(owner, error.what(), "Unable to save map", MB_OK | MB_ICONERROR);
    }
}

HBITMAP CreateMenuGlyph(COLORREF color, int symbol) {
    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, 16, 16);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    RECT bounds{0, 0, 16, 16};
    FillRect(dc, &bounds, GetSysColorBrush(COLOR_MENU));
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    if (symbol == 0) {
        Rectangle(dc, 3, 2, 13, 14);
        MoveToEx(dc, 5, 5, nullptr);
        LineTo(dc, 11, 5);
    } else if (symbol == 1) {
        MoveToEx(dc, 3, 12, nullptr);
        LineTo(dc, 12, 3);
        MoveToEx(dc, 4, 13, nullptr);
        LineTo(dc, 13, 4);
    } else if (symbol == 2) {
        for (int y = 3; y <= 11; y += 4) {
            Rectangle(dc, 2, y, 5, y + 3);
            MoveToEx(dc, 7, y + 1, nullptr);
            LineTo(dc, 14, y + 1);
        }
    } else if (symbol == 3) {
        HBRUSH brush = CreateSolidBrush(color);
        SelectObject(dc, brush);
        Ellipse(dc, 4, 4, 12, 12);
        SelectObject(dc, oldBrush);
        DeleteObject(brush);
    } else if (symbol == 4) {
        Rectangle(dc, 2, 2, 14, 14);
        MoveToEx(dc, 2, 14, nullptr);
        LineTo(dc, 14, 2);
    } else if (symbol == 5) {
        MoveToEx(dc, 2, 8, nullptr);
        LineTo(dc, 14, 8);
        MoveToEx(dc, 2, 8, nullptr);
        LineTo(dc, 5, 5);
        MoveToEx(dc, 2, 8, nullptr);
        LineTo(dc, 5, 11);
        MoveToEx(dc, 14, 8, nullptr);
        LineTo(dc, 11, 5);
        MoveToEx(dc, 14, 8, nullptr);
        LineTo(dc, 11, 11);
    } else {
        for (int x = 2; x <= 14; x += 4) {
            MoveToEx(dc, x, 2, nullptr);
            LineTo(dc, x, 14);
        }
        for (int y = 2; y <= 14; y += 4) {
            MoveToEx(dc, 2, y, nullptr);
            LineTo(dc, 14, y);
        }
    }
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBitmap);
    DeleteObject(pen);
    DeleteDC(dc);
    ReleaseDC(nullptr, screen);
    g_menuBitmaps.push_back(bitmap);
    return bitmap;
}

void SetMenuGlyph(HMENU menu, UINT command, COLORREF color, int symbol) {
    MENUITEMINFOA item{sizeof(item)};
    item.fMask = MIIM_BITMAP;
    item.hbmpItem = CreateMenuGlyph(color, symbol);
    SetMenuItemInfoA(menu, command, FALSE, &item);
}

void CreateMenuBar(HWND window) {
    HMENU menu = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();
    g_fileMenu = fileMenu;
    AppendMenuA(fileMenu, MF_STRING, kNewMapCommand, "New map\tCtrl+N");
    AppendMenuA(fileMenu, MF_STRING, kOpenMapCommand, "Open...\tCtrl+O");
    AppendMenuA(fileMenu, MF_STRING | MF_GRAYED, kCloseMapCommand, "Close");
    AppendMenuA(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(fileMenu, MF_STRING | MF_GRAYED, kSaveMapCommand, "Save\tCtrl+S");
    AppendMenuA(fileMenu, MF_STRING | MF_GRAYED, kSaveMapAsCommand, "Save As...");
    AppendMenuA(fileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(fileMenu, MF_STRING, kExitCommand, "Exit");
    SetMenuGlyph(fileMenu, kNewMapCommand, RGB(60, 120, 190), 0);
    SetMenuGlyph(fileMenu, kOpenMapCommand, RGB(190, 145, 30), 0);
    SetMenuGlyph(fileMenu, kSaveMapCommand, RGB(52, 112, 175), 0);
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), "File");

    g_toolMenu = CreatePopupMenu();
    AppendMenuA(g_toolMenu, MF_STRING, kOverviewCommand, "Overview");
    HMENU speedMenu = CreatePopupMenu();
    AppendMenuA(speedMenu, MF_STRING | MF_GRAYED, 0, "Render speed is not implemented");
    AppendMenuA(g_toolMenu, MF_POPUP | MF_GRAYED, reinterpret_cast<UINT_PTR>(speedMenu), "Render speed");
    AppendMenuA(g_toolMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(g_toolMenu, MF_STRING, 1310, "Pencil mode\tCtrl+P");
    AppendMenuA(g_toolMenu, MF_STRING, 1311, "Brush mode\tCtrl+B");
    AppendMenuA(g_toolMenu, MF_STRING, 1312, "Erase mode\tCtrl+E");
    AppendMenuA(g_toolMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(g_toolMenu, MF_STRING, kGraphicsModeCommand, "Graphics\tF11");
    AppendMenuA(g_toolMenu, MF_STRING, kFlagsModeCommand, "Flags\tF12");
    AppendMenuA(g_toolMenu, MF_STRING, kEntitiesModeCommand, "Entities\tF10");
    AppendMenuA(g_toolMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(g_toolMenu, MF_STRING, kSingleEditCommand, "Single edit");
    AppendMenuA(g_toolMenu, MF_STRING, kClusterEditCommand, "Cluster edit");
    AppendMenuA(g_toolMenu, MF_STRING, kNoEditCommand, "No edit");
    SetMenuGlyph(g_toolMenu, 1310, RGB(218, 174, 24), 1);
    SetMenuGlyph(g_toolMenu, 1311, RGB(48, 105, 185), 1);
    SetMenuGlyph(g_toolMenu, 1312, RGB(46, 156, 77), 1);
    SetMenuGlyph(g_toolMenu, kGraphicsModeCommand, RGB(35, 150, 58), 6);
    SetMenuGlyph(g_toolMenu, kFlagsModeCommand, RGB(25, 25, 25), 3);
    SetMenuGlyph(g_toolMenu, kEntitiesModeCommand, RGB(65, 146, 210), 3);
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_toolMenu), "Toolset");
    UpdateToolMenuChecks();

    HMENU mapMenu = CreatePopupMenu();
    AppendMenuA(mapMenu, MF_STRING, 1124, "Base tile");
    AppendMenuA(mapMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(mapMenu, MF_STRING, 1120, "Increase width\tAlt+W");
    AppendMenuA(mapMenu, MF_STRING, 1121, "Increase height\tAlt+Q");
    AppendMenuA(mapMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(mapMenu, MF_STRING, 1122, "Subtract width\tAlt+S");
    AppendMenuA(mapMenu, MF_STRING, 1123, "Subtract height\tAlt+A");
    AppendMenuA(mapMenu, MF_SEPARATOR, 0, nullptr);
    HMENU clearLayerMenu = CreatePopupMenu();
    static const char* layerNames[] = {"Ground", "Object", "Overlay", "Down wall", "Right wall",
                                       "Roof",   "Top",    "Shadow",  "Overlay 2"};
    for (int layer = 0; layer < 9; ++layer)
        AppendMenuA(clearLayerMenu, MF_STRING, kClearGraphicLayerBaseCommand + layer, layerNames[layer]);
    AppendMenuA(mapMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(clearLayerMenu), "Clear layer");
    HMENU clearFlagsMenu = CreatePopupMenu();
    static const char* flagNames[] = {"Block", "Door", "Chest", "Chair", "Warp"};
    for (int category = 0; category < 5; ++category)
        AppendMenuA(clearFlagsMenu, MF_STRING, kClearFlagCategoryBaseCommand + category, flagNames[category]);
    AppendMenuA(mapMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(clearFlagsMenu), "Clear flags");
    AppendMenuA(mapMenu, MF_STRING, kClearAllCommand, "Clear all\tAlt+C");
    SetMenuGlyph(mapMenu, 1120, RGB(72, 145, 42), 5);
    SetMenuGlyph(mapMenu, 1121, RGB(72, 145, 42), 5);
    SetMenuGlyph(mapMenu, 1122, RGB(190, 65, 51), 5);
    SetMenuGlyph(mapMenu, 1123, RGB(190, 65, 51), 5);
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(mapMenu), "Map size");

    g_graphicsMenu = CreatePopupMenu();
    static const char* graphicsNames[] = {"Graphics : Tiles",      "Graphics : Objects",     "Graphics : Masks",
                                          "Graphics : Down walls", "Graphics : Right walls", "Graphics : Top"};
    for (int category = 0; category < 6; ++category) {
        AppendMenuA(g_graphicsMenu, MF_STRING, kGraphicsMenuBaseCommand + category, graphicsNames[category]);
    }
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_graphicsMenu), "Graphics");
    CheckMenuRadioItem(g_graphicsMenu, kGraphicsMenuBaseCommand, kGraphicsMenuBaseCommand + 5,
                       kGraphicsMenuBaseCommand + g_paletteCategory, MF_BYCOMMAND);

    g_viewMenu = CreatePopupMenu();
    AppendMenuA(g_viewMenu, MF_STRING, kToggleAllLayersCommand, "Toggle all layers\tF1");
    AppendMenuA(g_viewMenu, MF_STRING, kHideAllLayersCommand, "Show no layers");
    AppendMenuA(g_viewMenu, MF_SEPARATOR, 0, nullptr);
    static const char* layerGroupNames[] = {"Ground layer", "Object layer", "Mask layer", "Wall layer", "Top layer"};
    for (int group = 0; group < 5; ++group) {
        AppendMenuA(g_viewMenu, MF_STRING | (GraphicsGroupVisible(group) ? MF_CHECKED : MF_UNCHECKED),
                    kLayerGroupMenuBaseCommand + group, layerGroupNames[group]);
    }
    SetMenuGlyph(g_viewMenu, kToggleAllLayersCommand, RGB(65, 92, 138), 2);
    SetMenuGlyph(g_viewMenu, kHideAllLayersCommand, RGB(210, 55, 48), 4);
    for (int group = 0; group < 5; ++group)
        SetMenuGlyph(g_viewMenu, kLayerGroupMenuBaseCommand + group, RGB(74, 104, 159), 2);
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_viewMenu), "Layers");

    g_flagMenu = CreatePopupMenu();
    AppendMenuA(g_flagMenu, MF_STRING, kToggleAllFlagsCommand, "Toggle all flags\tF2");
    AppendMenuA(g_flagMenu, MF_STRING, kHideAllFlagsCommand, "Show no flags\tF6");
    AppendMenuA(g_flagMenu, MF_SEPARATOR, 0, nullptr);
    static const char* flagLayerNames[] = {"Block layer", "Door layer", "Chest layer", "Chair layer", "Warp layer"};
    for (int category = 0; category < 5; ++category) {
        AppendMenuA(g_flagMenu, MF_STRING | (g_flagVisible[category] ? MF_CHECKED : MF_UNCHECKED),
                    kFlagMenuBaseCommand + category, flagLayerNames[category]);
    }
    SetMenuGlyph(g_flagMenu, kToggleAllFlagsCommand, RGB(95, 95, 95), 3);
    SetMenuGlyph(g_flagMenu, kHideAllFlagsCommand, RGB(210, 55, 48), 4);
    static const COLORREF flagIconColors[] = {RGB(235, 72, 58), RGB(246, 157, 38), RGB(224, 188, 42), RGB(55, 179, 116),
                                              RGB(170, 92, 220)};
    for (int category = 0; category < 5; ++category)
        SetMenuGlyph(g_flagMenu, kFlagMenuBaseCommand + category, flagIconColors[category], 3);
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_flagMenu), "Flags");

    g_boundaryMenu = CreatePopupMenu();
    AppendMenuA(g_boundaryMenu, MF_STRING, kToggleAllBoundariesCommand, "Toggle all boundaries\tF3");
    AppendMenuA(g_boundaryMenu, MF_STRING, kHideAllBoundariesCommand, "Show no boundaries\tF7");
    AppendMenuA(g_boundaryMenu, MF_SEPARATOR, 0, nullptr);
    static const char* boundaryNames[] = {"Ground boundaries", "Object boundaries", "Mask boundaries",
                                          "Wall boundaries", "Top boundaries"};
    for (int category = 0; category < 5; ++category) {
        AppendMenuA(g_boundaryMenu, MF_STRING | (g_boundaryVisible[category] ? MF_CHECKED : MF_UNCHECKED),
                    kBoundaryMenuBaseCommand + category, boundaryNames[category]);
    }
    SetMenuGlyph(g_boundaryMenu, kToggleAllBoundariesCommand, RGB(62, 91, 145), 4);
    SetMenuGlyph(g_boundaryMenu, kHideAllBoundariesCommand, RGB(210, 55, 48), 4);
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_boundaryMenu), "Boundaries");

    HMENU extraMenu = CreatePopupMenu();
    g_mapTogetherMenu = CreatePopupMenu();
    AppendMenuA(g_mapTogetherMenu, MF_STRING, kOpenCollaborationWindowCommand, "Open Map Together...");
    AppendMenuA(g_mapTogetherMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(g_mapTogetherMenu, MF_STRING, kStartCollaborationCommand, "Start Server...");
    AppendMenuA(g_mapTogetherMenu, MF_STRING, kConnectCollaborationCommand, "Connect...");
    AppendMenuA(g_mapTogetherMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(g_mapTogetherMenu, MF_STRING | MF_GRAYED, kManageCollaborationCommand, "Manage Session...");
    AppendMenuA(g_mapTogetherMenu, MF_STRING | MF_GRAYED, kDisconnectCollaborationCommand, "Disconnect");
    AppendMenuA(extraMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_mapTogetherMenu), "Map together");
    AppendMenuA(extraMenu, MF_STRING | MF_GRAYED, kZoomResetCommand, "Preview Graphics");
    AppendMenuA(extraMenu, MF_STRING | MF_GRAYED, 0, "Publish graphics");
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(extraMenu), "Extra");
    g_windowsMenu = CreatePopupMenu();
    static const char* panelNames[] = {"Viewer", "Graphics", "Layers",  "Map properties",
                                       "Flags",  "Toolset",  "Entities"};
    for (int panel = 0; panel < 7; ++panel) {
        AppendMenuA(g_windowsMenu, MF_STRING | MF_CHECKED, kPanelBaseCommand + panel, panelNames[panel]);
    }
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(g_windowsMenu), "Windows");
    HMENU helpMenu = CreatePopupMenu();
    AppendMenuA(helpMenu, MF_STRING, kAboutCommand, "About...");
    AppendMenuA(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(helpMenu), "Help");

    SetMenu(window, menu);
}

void CreateNewMap(HWND owner) {
    if (g_map.dirty) {
        const int choice = MessageBoxA(owner, "Save changes to the current map before creating a new one?",
                                       "Unsaved map changes", MB_YESNOCANCEL | MB_ICONWARNING);
        if (choice == IDCANCEL)
            return;
        if (choice == IDYES) {
            SaveMap(owner, false);
            if (g_map.dirty)
                return;
        }
    }
    PushUndoSnapshot();
    MapDocument map;
    map.width = 24;
    map.height = 24;
    map.name = "Untitled";
    map.fillTile = 1;
    map.tiles.resize(static_cast<std::size_t>(map.width) * map.height);
    for (MapTile& tile : map.tiles)
        tile.graphics[0] = map.fillTile;
    map.loaded = true;
    map.dirty = true;
    g_map = std::move(map);
    g_cursorX = 8;
    g_cursorY = 14;
    g_viewCenterX = 8;
    g_viewCenterY = 14;
    g_camera.offsetX = 0.0;
    g_camera.offsetY = 0.0;
    g_camera.zoom = 1.0;
    UpdateMapTitle();
    if (!g_panels.empty())
        UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
    InvalidatePanels();
}

void CloseMap(HWND owner) {
    if (!g_map.loaded)
        return;
    if (g_map.dirty) {
        const int choice = MessageBoxA(owner, "Save changes before closing this map?", "Unsaved map changes",
                                       MB_YESNOCANCEL | MB_ICONWARNING);
        if (choice == IDCANCEL)
            return;
        if (choice == IDYES) {
            SaveMap(owner, false);
            if (g_map.dirty)
                return;
        }
    }
    g_map = MapDocument{};
    g_undoMaps.clear();
    g_redoMaps.clear();
    g_strokeBefore.reset();
    g_cursorX = g_cursorY = -1;
    UpdateMapTitle();
    InvalidatePanels();
}

struct CollaborationDialogData {
    bool hosting = false;
    bool accepted = false;
    std::string displayName;
    std::string sessionName;
    std::string address = "127.0.0.1";
    std::string password;
    std::uint16_t port = collaboration::kDefaultPort;
    bool readOnlyGuests = false;
};

constexpr char kCollaborationDialogClass[] = "EndlessMapStudioCollaborationDialog";
constexpr int kCollabDisplayName = 5100;
constexpr int kCollabSessionName = 5101;
constexpr int kCollabAddress = 5102;
constexpr int kCollabPort = 5103;
constexpr int kCollabPassword = 5104;
constexpr int kCollabReadOnly = 5105;

std::filesystem::path SettingsPath() {
    char appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
    std::filesystem::path directory = length > 0 && length < MAX_PATH ? appData : ".";
    directory /= "EndlessMapStudio";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    return directory / "settings.ini";
}

std::string LoadDisplayName() {
    char value[64]{};
    GetPrivateProfileStringA("Mapper", "DisplayName", "", value, static_cast<DWORD>(std::size(value)),
                             SettingsPath().string().c_str());
    std::string result = value;
    if (result.empty())
        result = "Mapper" + std::to_string(GetCurrentProcessId() % 1000 + 1);
    return result;
}

void SaveDisplayName(const std::string& value) {
    WritePrivateProfileStringA("Mapper", "DisplayName", value.c_str(), SettingsPath().string().c_str());
}

void AddDialogControl(HWND parent, const char* kind, const char* text, DWORD style, int x, int y, int width, int height,
                      int id) {
    HWND control = CreateWindowExA(
        kind == std::string("EDIT") ? WS_EX_CLIENTEDGE : 0, kind, text, WS_CHILD | WS_VISIBLE | style, x, y, width,
        height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleA(nullptr), nullptr);
    SendMessageA(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
}

std::string WindowText(HWND window, int id) {
    HWND control = GetDlgItem(window, id);
    const int length = GetWindowTextLengthA(control);
    std::string result(static_cast<std::size_t>(length) + 1, '\0');
    GetWindowTextA(control, result.data(), length + 1);
    result.resize(static_cast<std::size_t>(length));
    return result;
}

LRESULT CALLBACK CollaborationDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* data = reinterpret_cast<CollaborationDialogData*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    if (message == WM_CREATE) {
        data = reinterpret_cast<CollaborationDialogData*>(reinterpret_cast<CREATESTRUCTA*>(lParam)->lpCreateParams);
        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        int y = 18;
        auto row = [&](const char* label, int id, const std::string& value, DWORD style = ES_AUTOHSCROLL) {
            AddDialogControl(window, "STATIC", label, SS_LEFT, 18, y + 3, 92, 20, 0);
            AddDialogControl(window, "EDIT", value.c_str(), WS_TABSTOP | style, 112, y, 220, 23, id);
            y += 34;
        };
        row("Display name:", kCollabDisplayName, data->displayName);
        if (data->hosting)
            row("Session name:", kCollabSessionName, data->sessionName);
        else
            row("Address:", kCollabAddress, data->address);
        row("Port:", kCollabPort, std::to_string(data->port), ES_AUTOHSCROLL | ES_NUMBER);
        row("Password:", kCollabPassword, "", ES_AUTOHSCROLL | ES_PASSWORD);
        if (data->hosting) {
            AddDialogControl(window, "BUTTON", "Read-only guests", WS_TABSTOP | BS_AUTOCHECKBOX, 112, y, 180, 22,
                             kCollabReadOnly);
            y += 34;
        }
        AddDialogControl(window, "BUTTON", data->hosting ? "Start" : "Connect", WS_TABSTOP | BS_DEFPUSHBUTTON, 172, y,
                         76, 25, IDOK);
        AddDialogControl(window, "BUTTON", "Cancel", WS_TABSTOP | BS_PUSHBUTTON, 256, y, 76, 25, IDCANCEL);
        SetFocus(GetDlgItem(window, kCollabDisplayName));
        return 0;
    }
    if (message == WM_COMMAND && data) {
        if (LOWORD(wParam) == IDOK) {
            data->displayName = WindowText(window, kCollabDisplayName);
            data->password = WindowText(window, kCollabPassword);
            if (data->hosting)
                data->sessionName = WindowText(window, kCollabSessionName);
            else
                data->address = WindowText(window, kCollabAddress);
            const std::string portText = WindowText(window, kCollabPort);
            const long port = std::strtol(portText.c_str(), nullptr, 10);
            if (data->displayName.empty() || data->displayName.size() > collaboration::kMaxDisplayNameBytes ||
                port < 1 || port > 65535) {
                MessageBoxA(window, "Enter a display name and a valid port (1-65535).", "Map together",
                            MB_OK | MB_ICONWARNING);
                return 0;
            }
            data->port = static_cast<std::uint16_t>(port);
            data->readOnlyGuests = IsDlgButtonChecked(window, kCollabReadOnly) == BST_CHECKED;
            data->accepted = true;
            DestroyWindow(window);
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(window);
            return 0;
        }
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

bool ShowCollaborationDialog(HWND owner, CollaborationDialogData& data) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXA cls{sizeof(cls)};
        cls.lpfnWndProc = CollaborationDialogProc;
        cls.hInstance = GetModuleHandleA(nullptr);
        cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        cls.lpszClassName = kCollaborationDialogClass;
        registered = RegisterClassExA(&cls) != 0;
    }
    const int height = data.hosting ? 270 : 236;
    RECT ownerBounds{};
    GetWindowRect(owner, &ownerBounds);
    HWND dialog = CreateWindowExA(WS_EX_DLGMODALFRAME, kCollaborationDialogClass,
                                  data.hosting ? "Start Map Together Server" : "Connect to Map Together",
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU, ownerBounds.left + 80, ownerBounds.top + 80, 370,
                                  height, owner, nullptr, GetModuleHandleA(nullptr), &data);
    if (!dialog)
        return false;
    EnableWindow(owner, FALSE);
    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    MSG message{};
    while (IsWindow(dialog) && GetMessageA(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageA(dialog, &message)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    EnableWindow(owner, TRUE);
    SetForegroundWindow(owner);
    return data.accepted;
}

void UpdateCollaborationMenu() {
    if (!g_mapTogetherMenu || !g_collaboration)
        return;
    const bool disconnected = g_collaboration->State() == collaboration::SessionState::Disconnected;
    EnableMenuItem(g_mapTogetherMenu, kStartCollaborationCommand,
                   MF_BYCOMMAND | (disconnected ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(g_mapTogetherMenu, kConnectCollaborationCommand,
                   MF_BYCOMMAND | (disconnected ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(g_mapTogetherMenu, kManageCollaborationCommand,
                   MF_BYCOMMAND | (disconnected ? MF_GRAYED : MF_ENABLED));
    EnableMenuItem(g_mapTogetherMenu, kDisconnectCollaborationCommand,
                   MF_BYCOMMAND | (disconnected ? MF_GRAYED : MF_ENABLED));
    if (g_mainWindow)
        DrawMenuBar(g_mainWindow);
}

void UpdateCollaborationWindow() {
    if (!g_collaborationWindow || !g_collaboration)
        return;
    g_collaborationWindow->Update({g_collaboration->State(), g_collaboration->StatusText(),
                                   g_collaboration->HostAddress(), g_collaboration->Port(),
                                   g_collaboration->Participants(), g_collaboration->LocalRole(),
                                   g_collaboration->PasswordEnabled()});
}

void ShowCollaborationStatus(HWND owner) {
    (void)owner;
    if (g_collaborationWindow) {
        UpdateCollaborationWindow();
        g_collaborationWindow->Show();
        if (g_collaboration && g_collaboration->State() != collaboration::SessionState::Disconnected) {
            collaboration::Presence value;
            value.area = collaboration::PresenceArea::MapTogether;
            std::string error;
            if (g_collaboration->SendPresence(value, error))
                g_lastPublishedPresence = value;
        }
    }
}

bool StartHostedSessionSafely(const collaboration::HostOptions& options, const std::vector<std::uint8_t>& emf,
                              std::string& error) {
    const std::filesystem::path backupIdentity =
        g_map.path.empty()
            ? FindMapsDirectory() /
                  std::filesystem::path("Unsaved-" + save_workflow::SanitizeFilename(g_map.name) + ".emf")
            : std::filesystem::path(g_map.path);
    std::filesystem::path sessionBackup;
    if (!save_workflow::CreateSessionBackup(backupIdentity, emf, sessionBackup, error, {}, !g_map.path.empty()))
        return false;
    if (!g_collaboration->StartHosting(options, error)) {
        std::error_code ignored;
        std::filesystem::remove(sessionBackup, ignored);
        return false;
    }
    g_lastSessionBackup = std::move(sessionBackup);
    return true;
}

void StartCollaboration(HWND owner) {
    if (!g_map.loaded) {
        MessageBoxA(owner, "Open or create a map before starting a session.", "Map together", MB_OK | MB_ICONWARNING);
        return;
    }
    CollaborationDialogData dialog;
    dialog.hosting = true;
    dialog.displayName = LoadDisplayName();
    dialog.sessionName = g_map.name;
    if (!ShowCollaborationDialog(owner, dialog))
        return;
    SaveDisplayName(dialog.displayName);
    try {
        const std::vector<std::uint8_t> emf = WriteEmf(g_map);
        collaboration::HostOptions options{
            dialog.sessionName,
            dialog.displayName,
            dialog.password,
            dialog.port,
            g_map.name,
            {0, static_cast<std::uint16_t>(g_map.width), static_cast<std::uint16_t>(g_map.height), emf}};
        std::string error;
        if (!StartHostedSessionSafely(options, emf, error))
            MessageBoxA(owner, error.c_str(), "Unable to start session", MB_OK | MB_ICONERROR);
        else
            ShowCollaborationStatus(owner);
    } catch (const std::exception& error) {
        MessageBoxA(owner, error.what(), "Unable to start session", MB_OK | MB_ICONERROR);
    }
    UpdateCollaborationMenu();
}

void ConnectCollaboration(HWND owner) {
    CollaborationDialogData dialog;
    dialog.displayName = LoadDisplayName();
    if (!ShowCollaborationDialog(owner, dialog))
        return;
    SaveDisplayName(dialog.displayName);
    std::string error;
    if (!g_collaboration->Connect({dialog.address, dialog.displayName, dialog.password, dialog.port}, error))
        MessageBoxA(owner, error.c_str(), "Unable to connect", MB_OK | MB_ICONERROR);
    else
        ShowCollaborationStatus(owner);
    UpdateCollaborationMenu();
}

void ProcessCollaborationEvents(HWND owner) {
    if (!g_collaboration)
        return;
    for (auto& event : g_collaboration->DrainEvents()) {
        if (event.type == collaboration::EventType::SnapshotReceived) {
            try {
                MapDocument incoming = ReadEmfBytes(event.snapshot.emfBytes);
                if (incoming.width != event.snapshot.width || incoming.height != event.snapshot.height)
                    throw std::runtime_error("The synchronized map dimensions are inconsistent.");
                incoming.path.clear();
                incoming.dirty = false;
                g_map = std::move(incoming);
                g_undoMaps.clear();
                g_redoMaps.clear();
                g_cursorX = std::min(8, g_map.width - 1);
                g_cursorY = std::min(14, g_map.height - 1);
                g_camera.centerTileX = g_cursorX;
                g_camera.centerTileY = g_cursorY;
                g_camera.offsetX = 0;
                g_camera.offsetY = 0;
                UpdateMapTitle();
                UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
                InvalidatePanels();
                if (g_collaborationWindow)
                    g_collaborationWindow->AppendSystem(event.message);
            } catch (const std::exception& error) {
                g_collaboration->Disconnect("Invalid map snapshot");
                MessageBoxA(owner, error.what(), "Unable to synchronize map", MB_OK | MB_ICONERROR);
            }
        } else if (event.type == collaboration::EventType::ChatMessage) {
            if (g_collaborationWindow)
                g_collaborationWindow->AppendChat(event.displayName, event.message);
        } else if (event.type == collaboration::EventType::OperationRequested) {
            collaboration::EditOperation operation;
            std::string error;
            bool editor = false;
            for (const auto& p : g_collaboration->Participants())
                if (p.userId == event.operation.authorId)
                    editor = p.role == collaboration::ParticipantRole::Editor;
            const bool decoded = collaboration::DecodeEditOperation(event.operation.operation, operation, error);
            if (editor && decoded &&
                collaboration::ValidateEditOperation(operation, static_cast<std::uint16_t>(g_map.width),
                                                     static_cast<std::uint16_t>(g_map.height), error) &&
                CollaborationGenerationsMatch(operation)) {
                ApplyCollaborationEdit(operation);
                AdvanceCollaborationGenerations(operation);
                RefreshEntityDraftAfterRemote(operation);
                g_map.dirty = true;
                UpdateMapTitle();
                InvalidatePanels();
                if (operation.kind == collaboration::EditKind::Flags)
                    SyncFlagControls();
                g_collaboration->AcceptRequestedOperation(event.operation.authorId, event.operation.operation, error);
            } else {
                if (operation.requestId)
                    g_collaboration->RejectRequestedOperation(
                        event.operation.authorId, operation.requestId,
                        editor ? "Entity changed by another user - review before saving"
                               : "Session role: Viewer. Map changes are read-only.",
                        error);
                if (g_collaborationWindow)
                    g_collaborationWindow->AppendSystem(
                        editor ? "A stale or invalid entity save was rejected; refresh the tile and try again."
                               : "A Viewer map mutation was rejected.");
            }
        } else if (event.type == collaboration::EventType::OperationAccepted) {
            collaboration::EditOperation operation;
            std::string error;
            if (!collaboration::DecodeEditOperation(event.operation.operation, operation, error) ||
                !collaboration::ValidateEditOperation(operation, static_cast<std::uint16_t>(std::max(g_map.width, 1)),
                                                      static_cast<std::uint16_t>(std::max(g_map.height, 1)), error)) {
                g_collaboration->Disconnect("Invalid authoritative edit");
                continue;
            }
            const bool own = event.operation.authorId == g_collaboration->LocalUserId() &&
                             g_pendingCollaborationEdits.erase(operation.requestId) > 0;
            if (!own || !CollaborationEditMatchesMap(operation)) {
                ApplyCollaborationEdit(operation);
                g_map.dirty = true;
                UpdateMapTitle();
                InvalidatePanels();
                if (operation.kind == collaboration::EditKind::Flags)
                    SyncFlagControls();
            }
            AdvanceCollaborationGenerations(operation);
            if (!own)
                RefreshEntityDraftAfterRemote(operation);
        } else if (event.type == collaboration::EventType::OperationRejected) {
            g_pendingCollaborationEdits.erase(event.sequence);
            if (g_entityDraft && g_entityDraft->stale)
                g_entityDraft = ReadEntitiesAt(g_entityDraft->x, g_entityDraft->y);
            if (g_collaborationWindow)
                g_collaborationWindow->AppendSystem(event.message);
            SyncEntityControls();
        } else if (event.type == collaboration::EventType::PresenceChanged) {
            if (event.presence.userId != g_collaboration->LocalUserId())
                g_remotePresence[event.presence.userId] = event.presence;
            InvalidatePanels();
        } else if (event.type == collaboration::EventType::ParticipantsChanged) {
            std::unordered_set<std::uint32_t> active;
            for (const auto& p : event.participants)
                active.insert(p.userId);
            std::erase_if(g_remotePresence, [&](const auto& value) { return !active.contains(value.first); });
            InvalidatePanels();
        } else if (event.type == collaboration::EventType::ParticipantConnected ||
                   event.type == collaboration::EventType::ParticipantDisconnected) {
            ShowClassicNotice(event.message);
            if (g_collaborationWindow)
                g_collaborationWindow->AppendSystem(event.message);
        } else if (event.type == collaboration::EventType::ConnectionLost) {
            ShowClassicNotice("Connection lost");
            g_remotePresence.clear();
            if (g_collaborationWindow)
                g_collaborationWindow->AppendSystem(event.message);
        } else if (event.type == collaboration::EventType::MapSaved) {
            ++g_receivedSaveEvents;
            std::string author = "Host";
            for (const auto& p : g_collaboration->Participants())
                if (p.userId == event.saveEvent.authorId) {
                    author = p.displayName;
                    break;
                }
            ShowClassicNotice("Map saved by " + author);
            if (g_acceptanceFinal && g_acceptanceRole == AcceptanceRole::Client)
                WriteDialogAcceptanceArtifact("client-save-event.flag", "Map saved by " + author);
            if (g_collaborationWindow)
                g_collaborationWindow->AppendSystem("Map saved by " + author + ".");
        } else if (g_collaborationWindow && !event.message.empty())
            g_collaborationWindow->AppendSystem(event.message);
    }
    if (g_collaboration->State() == collaboration::SessionState::Disconnected) {
        g_pendingCollaborationEdits.clear();
        g_remotePresence.clear();
        g_lastPublishedPresence.reset();
    }
    UpdateCollaborationMenu();
    UpdateCollaborationWindow();
}

std::string AcceptanceFingerprint() {
    const auto bytes = WriteEmf(g_map);
    std::ostringstream value;
    value << std::hex << std::setw(8) << std::setfill('0') << CalculateCrc32(bytes);
    return value.str();
}
void WriteAcceptanceArtifact(const char* name, const std::string& value) {
    std::filesystem::create_directories(g_acceptanceDirectory);
    std::ofstream output(g_acceptanceDirectory / name, std::ios::binary | std::ios::trunc);
    output << value;
}
void AppendAcceptanceLog(const std::string& value) {
    std::filesystem::create_directories(g_acceptanceDirectory);
    std::ofstream output(g_acceptanceDirectory / "B2-acceptance.log", std::ios::app);
    output << value << '\n';
}
void ExportAcceptanceState(const char* name) {
    std::ostringstream json;
    json << "{\n  \"role\": \"" << (g_acceptanceRole == AcceptanceRole::Host ? "host" : "client")
         << "\",\n  \"revision\": " << g_collaboration->Revision() << ",\n  \"fingerprint\": \""
         << AcceptanceFingerprint() << "\",\n  \"width\": " << g_map.width << ", \"height\": " << g_map.height
         << ", \"baseTile\": " << g_map.fillTile << ",\n  \"npcs\": " << g_map.npcs.size()
         << ", \"items\": " << g_map.items.size() << ", \"signs\": " << g_map.signs.size()
         << ",\n  \"pending\": " << g_pendingCollaborationEdits.size()
         << ", \"pendingHighWater\": " << g_acceptancePendingHigh << ",\n  \"submitted\": " << g_acceptanceSubmitted
         << ",\n  \"ui\": {\"graphicsCategory\": " << g_paletteCategory << ", \"layer\": " << g_selectedLayer
         << ", \"tool\": " << static_cast<int>(g_editTool) << ", \"brush\": " << g_brushSize
         << ", \"flagsPage\": " << g_flagsTab << ", \"zoom\": " << g_zoom << ", \"cameraX\": " << g_camera.offsetX
         << ", \"cameraY\": " << g_camera.offsetY << "}\n}\n";
    WriteAcceptanceArtifact(name, json.str());
}
void AcceptanceGraphic(int x, int y, int layer, int graphic, int brush = 1) {
    SetSelectedLayer(layer);
    g_editorState.selectedGraphic() = graphic;
    g_brushSize = brush;
    g_editTool = EditTool::Brush;
    g_cursorX = x;
    g_cursorY = y;
    const auto before = g_collaboration->Revision();
    ApplySelectedTool();
    if (g_collaboration->Revision() != before || g_collaboration->State() == collaboration::SessionState::Connected)
        ++g_acceptanceSubmitted;
    g_acceptancePendingHigh = std::max(g_acceptancePendingHigh, g_pendingCollaborationEdits.size());
}
void AcceptanceFlag(int x, int y, int page) {
    g_brushSize = 1;
    g_flagsTab = page;
    g_cursorX = x;
    g_cursorY = y;
    ApplyFlagToCursor();
    ++g_acceptanceSubmitted;
    g_acceptancePendingHigh = std::max(g_acceptancePendingHigh, g_pendingCollaborationEdits.size());
}
void AcceptanceEntities(int x, int y, bool sign, bool npcs, bool items) {
    g_entityDraft = ReadEntitiesAt(x, y);
    if (sign) {
        MapSign value{x, y, 18, {}};
        const std::string text = "Collaboration TestThis sign was created by the host.\nSecond line preserved.";
        value.encodedText.assign(text.begin(), text.end());
        eo_encode_string(value.encodedText.data(), value.encodedText.size());
        g_entityDraft->sign = value;
        g_entityDraft->dirtyScopes |= collaboration::EntitySign;
    }
    if (npcs) {
        g_entityDraft->npcs = {{x, y, 101, 2, 30, 3}, {x, y, 202, 4, 60, 1}};
        g_entityDraft->dirtyScopes |= collaboration::EntityNpcs;
    }
    if (items) {
        g_entityDraft->items = {{x, y, 7, 2, 301, 45, 500}, {x, y, 8, 3, 302, 90, 999}};
        g_entityDraft->dirtyScopes |= collaboration::EntityItems;
    }
    CommitEntityDraft();
    ++g_acceptanceSubmitted;
    g_acceptancePendingHigh = std::max(g_acceptancePendingHigh, g_pendingCollaborationEdits.size());
}
void RunB3AcceptanceStep() {
    if (g_acceptanceRole == AcceptanceRole::Host) {
        if (g_acceptanceStep == 0) {
            collaboration::HostOptions options{"B3 acceptance",
                                               "HarnessHost",
                                               "",
                                               39121,
                                               g_map.name,
                                               {0, static_cast<std::uint16_t>(g_map.width),
                                                static_cast<std::uint16_t>(g_map.height), WriteEmf(g_map)}};
            std::string error;
            if (g_collaboration->StartHosting(options, error)) {
                AppendAcceptanceLog("B3 host ready");
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->ConnectedGuestCount() == 1) {
            SetBaseTileGraphic(1);
            ResizeMap(g_map.width + 2, g_map.height + 2);
            AcceptanceGraphic(5, 5, 1, 2);
            AcceptanceFlag(6, 6, 1);
            AcceptanceEntities(7, 7, true, true, true);
            g_entityDraft = ReadEntitiesAt(12, 12);
            g_entityDraft->npcs = {{12, 12, 909, 1, 10, 1}};
            g_entityDraft->dirtyScopes = collaboration::EntityNpcs;
            WriteAcceptanceArtifact("b3-staged.flag", "ready");
            AppendAcceptanceLog("B3 host map operations entities and dirty staged NPC prepared");
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && std::filesystem::exists(g_acceptanceDirectory / "b3-client-conflict.flag") &&
            EntityGeneration(2, 12, 12) > 0) {
            const auto revision = g_collaboration->Revision();
            CommitEntityDraft();
            if (g_collaboration->Revision() == revision) {
                WriteAcceptanceArtifact("b3-conflict-pass.flag", "rejected");
                AppendAcceptanceLog("B3 stale NPC save rejected without revision advance");
            }
            g_acceptanceStep = 3;
            return;
        }
        if (g_acceptanceStep == 3 && std::filesystem::exists(g_acceptanceDirectory / "b3-client-finished.flag")) {
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 4;
        }
        if (g_acceptanceStep == 4 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportAcceptanceState("host-state.json");
            WriteAcceptanceArtifact("host-complete.flag", "done");
            AppendAcceptanceLog("B3 host final revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint());
            g_acceptanceStep = 5;
        }
    } else {
        if (g_acceptanceStep == 0) {
            SetGraphicsCategory(0);
            g_editorState.selectedGraphic() = 4;
            g_editTool = EditTool::Pencil;
            std::string error;
            if (g_collaboration->Connect({"127.0.0.1", "HarnessClient", "", 39121}, error)) {
                AppendAcceptanceLog("B3 client connecting");
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->State() == collaboration::SessionState::Connected &&
            std::filesystem::exists(g_acceptanceDirectory / "b3-staged.flag") && g_collaboration->Revision() >= 5) {
            ExportAcceptanceState("client-ui-initial.json");
            AcceptanceEntities(12, 12, false, true, false);
            WriteAcceptanceArtifact("b3-client-conflict.flag", "saved");
            AppendAcceptanceLog("B3 client committed conflicting NPC scope");
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && std::filesystem::exists(g_acceptanceDirectory / "b3-conflict-pass.flag")) {
            ExportAcceptanceState("client-ui-after-remote.json");
            ClearGraphicLayer(1);
            ClearFlagCategory(0);
            ResizeMap(g_map.width - 1, g_map.height - 1);
            g_copiedEntities = ReadEntitiesAt(7, 7);
            g_entityDraft = ReadEntitiesAt(10, 10);
            EntityDraft pasted = *g_copiedEntities;
            pasted.x = 10;
            pasted.y = 10;
            for (auto& n : pasted.npcs) {
                n.x = 10;
                n.y = 10;
            }
            for (auto& i : pasted.items) {
                i.x = 10;
                i.y = 10;
            }
            if (pasted.sign) {
                pasted.sign->x = 10;
                pasted.sign->y = 10;
            }
            pasted.generations = g_entityDraft->generations;
            pasted.dirtyScopes = 15;
            g_entityDraft = std::move(pasted);
            CommitEntityDraft();
            ClearAllMapData();
            g_acceptanceStep = 3;
            return;
        }
        if (g_acceptanceStep == 3 && g_pendingCollaborationEdits.empty()) {
            AcceptanceGraphic(3, 3, 1, 3);
            AcceptanceEntities(4, 4, true, true, true);
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 4;
            return;
        }
        if (g_acceptanceStep == 4 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000) {
            WriteAcceptanceArtifact("b3-client-finished.flag", "done");
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 5;
        }
        if (g_acceptanceStep == 5 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportAcceptanceState("client-state.json");
            AppendAcceptanceLog("B3 client final revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint());
            g_acceptanceStep = 6;
        }
    }
}
void RunCDAcceptanceStep() {
    if (g_acceptanceRole == AcceptanceRole::Host) {
        if (g_acceptanceStep == 0) {
            collaboration::HostOptions options{"C/D acceptance",
                                               "PresenceHost",
                                               "",
                                               39122,
                                               g_map.name,
                                               {0, static_cast<std::uint16_t>(g_map.width),
                                                static_cast<std::uint16_t>(g_map.height), WriteEmf(g_map)}};
            std::string error;
            if (g_collaboration->StartHosting(options, error)) {
                AppendAcceptanceLog("C/D host ready");
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->ConnectedGuestCount() == 1) {
            const auto people = g_collaboration->Participants();
            if (people.size() < 2)
                return;
            std::string error;
            g_collaboration->SetPassword("acceptance-one", error);
            g_collaboration->ChangeParticipantRole(people[1].userId, collaboration::ParticipantRole::Viewer, error);
            g_acceptanceLastRevision = g_collaboration->Revision();
            WriteAcceptanceArtifact("cd-viewer-role.flag", "ready");
            AppendAcceptanceLog("C/D client role Viewer password enabled");
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && std::filesystem::exists(g_acceptanceDirectory / "cd-viewer-rejected.flag")) {
            if (g_collaboration->Revision() != g_acceptanceLastRevision) {
                AppendAcceptanceLog("C/D ERROR viewer mutation advanced revision");
                return;
            }
            const auto people = g_collaboration->Participants();
            if (people.size() < 2)
                return;
            std::string error;
            g_collaboration->ChangeParticipantRole(people[1].userId, collaboration::ParticipantRole::Editor, error);
            g_collaboration->SetPassword("acceptance-two", error);
            g_collaboration->SetPassword("", error);
            WriteAcceptanceArtifact("cd-editor-role.flag", "ready");
            AppendAcceptanceLog("C/D Viewer mutation rejected; Editor restored; password changed and removed");
            g_acceptanceStep = 3;
            return;
        }
        if (g_acceptanceStep == 3 && std::filesystem::exists(g_acceptanceDirectory / "cd-presence-complete.flag")) {
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 4;
        }
        if (g_acceptanceStep == 4 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportAcceptanceState("host-state.json");
            std::ostringstream metrics;
            metrics << "sent=" << g_collaboration->PresencePacketsSent()
                    << " received=" << g_collaboration->PresencePacketsReceived();
            WriteAcceptanceArtifact("host-presence.txt", metrics.str());
            WriteAcceptanceArtifact("host-complete.flag", "done");
            AppendAcceptanceLog("C/D host final revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint() + " " + metrics.str());
            g_acceptanceStep = 5;
        }
    } else {
        if (g_acceptanceStep == 0) {
            std::string error;
            if (g_collaboration->Connect({"127.0.0.1", "PresenceClient", "", 39122}, error)) {
                AppendAcceptanceLog("C/D client connecting");
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->State() == collaboration::SessionState::Connected &&
            std::filesystem::exists(g_acceptanceDirectory / "cd-viewer-role.flag") &&
            g_collaboration->LocalRole() == collaboration::ParticipantRole::Viewer) {
            collaboration::Presence remoteLocation;
            remoteLocation.area = collaboration::PresenceArea::Viewer;
            remoteLocation.viewerActive = true;
            remoteLocation.x = static_cast<std::uint16_t>(g_map.width - 1);
            remoteLocation.y = static_cast<std::uint16_t>(g_map.height - 1);
            std::string error;
            g_collaboration->SendPresence(remoteLocation, error);
            collaboration::Presence panel;
            panel.area = collaboration::PresenceArea::Entities;
            g_collaboration->SendPresence(panel, error);
            const auto revision = g_collaboration->Revision();
            AcceptanceGraphic(2, 2, 1, 2);
            if (g_collaboration->Revision() == revision && g_pendingCollaborationEdits.empty()) {
                WriteAcceptanceArtifact("cd-viewer-rejected.flag", "pass");
                AppendAcceptanceLog("C/D Viewer edit rejected, presence remains active");
            }
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && std::filesystem::exists(g_acceptanceDirectory / "cd-editor-role.flag") &&
            g_collaboration->LocalRole() == collaboration::ParticipantRole::Editor) {
            AcceptanceGraphic(3, 3, 1, 3);
            AcceptanceFlag(4, 4, 1);
            AcceptanceEntities(5, 5, false, true, false);
            g_acceptanceBrushStart = GetTickCount64();
            WriteAcceptanceArtifact("cd-presence-start.flag", "running");
            AppendAcceptanceLog("C/D 30 second presence movement started");
            g_acceptanceStep = 3;
            return;
        }
        if (g_acceptanceStep == 3) {
            const auto elapsed = GetTickCount64() - g_acceptanceBrushStart;
            if (elapsed < 30000) {
                const int index = static_cast<int>(elapsed / 100);
                if (index != g_acceptancePresenceIndex) {
                    g_acceptancePresenceIndex = index;
                    collaboration::Presence value;
                    value.area = collaboration::PresenceArea::Viewer;
                    value.viewerActive = true;
                    value.x = static_cast<std::uint16_t>(index % g_map.width);
                    value.y = static_cast<std::uint16_t>((index * 3) % g_map.height);
                    std::string error;
                    g_collaboration->SendPresence(value, error);
                }
                return;
            }
            WriteAcceptanceArtifact("cd-presence-complete.flag", "done");
            AppendAcceptanceLog("C/D presence movement duration_ms=" + std::to_string(elapsed) +
                                " sent=" + std::to_string(g_collaboration->PresencePacketsSent()));
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 4;
        }
        if (g_acceptanceStep == 4 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportAcceptanceState("client-state.json");
            std::ostringstream metrics;
            metrics << "sent=" << g_collaboration->PresencePacketsSent()
                    << " received=" << g_collaboration->PresencePacketsReceived();
            WriteAcceptanceArtifact("client-presence.txt", metrics.str());
            AppendAcceptanceLog("C/D client final revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint() + " " + metrics.str());
            g_acceptanceStep = 5;
        }
    }
}
std::string SemanticFingerprint(const std::filesystem::path& path) {
    const MapDocument map = ReadEmf(path.string());
    const auto bytes = WriteEmf(map);
    std::ostringstream value;
    value << std::hex << std::setw(8) << std::setfill('0') << CalculateCrc32(bytes);
    return value.str();
}
std::string AcceptanceFileText(const std::filesystem::path& path) {
    const auto bytes = ReadFile(path.string());
    return {bytes.begin(), bytes.end()};
}
std::size_t AcceptanceBackupCount(const std::filesystem::path& mapPath) {
    const auto directory = save_workflow::BackupsDirectory(mapPath);
    if (!std::filesystem::is_directory(directory))
        return 0;
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == L".emf")
            ++count;
    return count;
}
void ExportFinalAcceptanceState(const char* name) {
    std::ostringstream json;
    json << "{\n  \"role\": \"" << (g_acceptanceRole == AcceptanceRole::Host ? "host" : "client")
         << "\",\n  \"revision\": " << g_collaboration->Revision() << ",\n  \"fingerprint\": \""
         << AcceptanceFingerprint() << "\",\n  \"pending\": " << g_pendingCollaborationEdits.size()
         << ",\n  \"dirty\": " << (g_map.dirty ? "true" : "false") << ",\n  \"saveEvents\": " << g_receivedSaveEvents
         << "\n}\n";
    WriteAcceptanceArtifact(name, json.str());
}

void RunFinalAcceptanceStep() {
    if (g_acceptanceModal)
        return;
    static std::string initialFingerprint, preSaveFingerprint, cancelMap, cancelMetadata;
    static std::uint64_t preSaveRevision = 0, cancelSaveEvents = 0;
    static std::size_t cancelBackups = 0;
    static std::filesystem::path activeMap;
    if (activeMap.empty())
        activeMap = g_acceptanceDirectory / L"maps" / L"collaborative.emf";
    if (g_acceptanceRole == AcceptanceRole::Host) {
        if (g_acceptanceStep == 0) {
            initialFingerprint = AcceptanceFingerprint();
            const auto emf = WriteEmf(g_map);
            collaboration::HostOptions options{
                "Map Together V1 final",
                "ForestHost",
                "",
                39123,
                g_map.name,
                {0, static_cast<std::uint16_t>(g_map.width), static_cast<std::uint16_t>(g_map.height), emf}};
            std::string error;
            if (StartHostedSessionSafely(options, emf, error)) {
                const bool semantic = std::filesystem::exists(g_lastSessionBackup) &&
                                      SemanticFingerprint(g_lastSessionBackup) == initialFingerprint;
                WriteAcceptanceArtifact("session-backup.json", std::string("{\"semanticMatch\":") +
                                                                   (semantic ? "true" : "false") +
                                                                   ",\"fingerprint\":\"" + initialFingerprint + "\"}");
                AppendAcceptanceLog("FINAL session backup semantic=" + std::string(semantic ? "PASS" : "FAIL"));
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->ConnectedGuestCount() == 1) {
            AcceptanceGraphic(5, 5, 1, 2);
            AcceptanceEntities(6, 6, false, true, false);
            WriteAcceptanceArtifact("final-host-edits.flag", "object+npc");
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && g_collaboration->Revision() >= 4 && g_pendingCollaborationEdits.empty()) {
            preSaveRevision = g_collaboration->Revision();
            preSaveFingerprint = AcceptanceFingerprint();
            WriteAcceptanceArtifact("final-edits-converged.flag", "ready");
            AppendAcceptanceLog("FINAL representative edits revision=" + std::to_string(preSaveRevision) +
                                " fingerprint=" + preSaveFingerprint);
            g_acceptanceStep = 3;
            return;
        }
        if (g_acceptanceStep == 3 && std::filesystem::exists(g_acceptanceDirectory / L"client-save-restricted.json")) {
            g_acceptanceDialogAction = AcceptanceDialogAction::Save;
            g_acceptanceDialogComment = "Collaborative forest pass complete.";
            g_acceptanceDialogToken = "primary";
            g_acceptanceStep = 4;
            SaveMap(g_mainWindow, false);
            bool backupSemantic = false, mapSemantic = false, metadataValid = false, privateFree = false;
            save_workflow::Metadata metadata;
            std::string error;
            try {
                backupSemantic = !g_lastSaveResult.backupPath.empty() &&
                                 SemanticFingerprint(g_lastSaveResult.backupPath) == initialFingerprint;
                mapSemantic = SemanticFingerprint(activeMap) == AcceptanceFingerprint();
                metadataValid = save_workflow::ReadMetadata(g_lastSaveResult.metadataPath, metadata, error) &&
                                metadata.comment == "Collaborative forest pass complete." &&
                                metadata.map == "collaborative.emf" &&
                                metadata.fileSize == std::filesystem::file_size(activeMap) && metadata.collaborative &&
                                metadata.author == "ForestHost" && !metadata.timestamp.empty();
                const std::string raw = AcceptanceFileText(g_lastSaveResult.metadataPath);
                privateFree = raw.find("password") == std::string::npos && raw.find("127.0.0.1") == std::string::npos &&
                              raw.find(g_acceptanceDirectory.string()) == std::string::npos &&
                              raw.find("participants") == std::string::npos;
            } catch (...) {
            }
            std::ostringstream json;
            json << "{\"backupSemantic\":" << (backupSemantic ? "true" : "false")
                 << ",\"mapSemantic\":" << (mapSemantic ? "true" : "false")
                 << ",\"metadataValid\":" << (metadataValid ? "true" : "false")
                 << ",\"privateFree\":" << (privateFree ? "true" : "false")
                 << ",\"revision\":" << g_collaboration->Revision() << ",\"fingerprint\":\"" << AcceptanceFingerprint()
                 << "\"}";
            WriteAcceptanceArtifact("primary-save.json", json.str());
            AppendAcceptanceLog("FINAL primary save backup=" + std::string(backupSemantic ? "PASS" : "FAIL") + " emf=" +
                                (mapSemantic ? "PASS" : "FAIL") + " metadata=" + (metadataValid ? "PASS" : "FAIL"));
            g_acceptanceStep = 5;
            return;
        }
        if (g_acceptanceStep == 5 &&
            std::filesystem::exists(g_acceptanceDirectory / L"client-save-event-verified.json")) {
            const auto revision = g_collaboration->Revision();
            g_acceptanceDialogAction = AcceptanceDialogAction::Save;
            g_acceptanceDialogComment = "";
            g_acceptanceDialogToken = "blank";
            SaveMap(g_mainWindow, false);
            save_workflow::Metadata metadata;
            std::string error;
            const bool blank = g_lastSaveResult.mapSaved && g_lastSaveResult.metadataSaved &&
                               save_workflow::ReadMetadata(g_lastSaveResult.metadataPath, metadata, error) &&
                               metadata.comment.empty() && g_collaboration->Revision() == revision;
            WriteAcceptanceArtifact("blank-save.json", std::string("{\"pass\":") + (blank ? "true" : "false") + "}");
            ProcessCollaborationEvents(g_mainWindow);
            AcceptanceGraphic(9, 9, 1, 3);
            cancelMap = AcceptanceFileText(activeMap);
            const auto metaPath = save_workflow::MetadataPath(activeMap);
            cancelMetadata = AcceptanceFileText(metaPath);
            cancelBackups = AcceptanceBackupCount(activeMap);
            cancelSaveEvents = g_receivedSaveEvents;
            WriteAcceptanceArtifact("cancel-ready.flag", "dirty");
            g_acceptanceStep = 6;
            return;
        }
        if (g_acceptanceStep == 6 && std::filesystem::exists(g_acceptanceDirectory / L"cancel-edit-seen.flag")) {
            const auto revision = g_collaboration->Revision();
            g_acceptanceDialogAction = AcceptanceDialogAction::Cancel;
            g_acceptanceDialogComment = "discarded";
            g_acceptanceDialogToken = "cancel";
            SaveMap(g_mainWindow, false);
            const auto metaPath = save_workflow::MetadataPath(activeMap);
            const std::string mapAfter = AcceptanceFileText(activeMap);
            const std::string metadataAfter = AcceptanceFileText(metaPath);
            const bool pass = g_map.dirty && mapAfter == cancelMap && metadataAfter == cancelMetadata &&
                              AcceptanceBackupCount(activeMap) == cancelBackups &&
                              g_receivedSaveEvents == cancelSaveEvents && g_collaboration->Revision() == revision;
            WriteAcceptanceArtifact("cancel-save.json", std::string("{\"pass\":") + (pass ? "true" : "false") + "}");
            g_acceptanceStep = 7;
            return;
        }
        if (g_acceptanceStep == 7) {
            const auto revision = g_collaboration->Revision();
            const std::string before = AcceptanceFileText(activeMap);
            g_acceptanceDialogAction = AcceptanceDialogAction::Save;
            g_acceptanceDialogComment = "must fail";
            g_acceptanceDialogToken = "failure";
            g_acceptanceSaveFailure = {save_workflow::FailurePoint::BackupWrite};
            SaveMap(g_mainWindow, false);
            const std::string after = AcceptanceFileText(activeMap);
            const bool pass =
                g_map.dirty && before == after && g_collaboration->Revision() == revision && !g_lastSaveResult.mapSaved;
            WriteAcceptanceArtifact("failed-save.json", std::string("{\"pass\":") + (pass ? "true" : "false") + "}");
            g_acceptanceStep = 8;
            return;
        }
        if (g_acceptanceStep == 8) {
            const auto revision = g_collaboration->Revision();
            const std::string priorDiskFingerprint = SemanticFingerprint(activeMap);
            g_acceptanceSaveAsPath = g_acceptanceDirectory / L"maps" / L"collaborative-save-as.emf";
            g_acceptanceDialogAction = AcceptanceDialogAction::Save;
            g_acceptanceDialogComment = "Save As collaborative checkpoint.";
            g_acceptanceDialogToken = "save-as";
            SaveMap(g_mainWindow, true);
            activeMap = g_acceptanceDirectory / L"maps" / L"collaborative-save-as.emf";
            save_workflow::Metadata metadata;
            std::string error;
            const bool backup = !g_lastSaveResult.backupPath.empty() &&
                                SemanticFingerprint(g_lastSaveResult.backupPath) == priorDiskFingerprint;
            const bool pass = backup && g_map.path == activeMap.string() && g_lastSaveResult.mapSaved &&
                              SemanticFingerprint(activeMap) == AcceptanceFingerprint() &&
                              save_workflow::ReadMetadata(g_lastSaveResult.metadataPath, metadata, error) &&
                              metadata.comment == "Save As collaborative checkpoint." &&
                              metadata.map == "collaborative-save-as.emf" && g_collaboration->Revision() == revision;
            WriteAcceptanceArtifact("save-as.json", std::string("{\"pass\":") + (pass ? "true" : "false") +
                                                        ",\"backupSemantic\":" + (backup ? "true" : "false") + "}");
            g_acceptanceStep = 9;
            return;
        }
        if (g_acceptanceStep == 9 && g_receivedSaveEvents >= 3) {
            const auto people = g_collaboration->Participants();
            if (people.size() < 2)
                return;
            std::string error;
            g_collaboration->ChangeParticipantRole(people[1].userId, collaboration::ParticipantRole::Viewer, error);
            g_acceptanceLastRevision = g_collaboration->Revision();
            WriteAcceptanceArtifact("final-viewer-role.flag", "ready");
            g_acceptanceStep = 10;
            return;
        }
        if (g_acceptanceStep == 10 && std::filesystem::exists(g_acceptanceDirectory / L"final-viewer-rejected.flag")) {
            const auto people = g_collaboration->Participants();
            if (people.size() < 2)
                return;
            const auto presence = g_remotePresence.find(people[1].userId);
            if (presence == g_remotePresence.end() || !presence->second.viewerActive ||
                presence->second.x != g_map.width - 1 || presence->second.y != g_map.height - 1)
                return;
            std::string error;
            const bool stable = g_collaboration->Revision() == g_acceptanceLastRevision;
            const bool editor =
                g_collaboration->ChangeParticipantRole(people[1].userId, collaboration::ParticipantRole::Editor, error);
            const bool added = g_collaboration->SetPassword("one", error);
            const bool changed = g_collaboration->SetPassword("two", error);
            const bool removed = g_collaboration->SetPassword("", error) && !g_collaboration->PasswordEnabled();
            ResizeMap(g_map.width + 1, g_map.height + 1);
            AcceptanceFlag(10, 10, 1);
            AcceptanceEntities(11, 11, true, true, true);
            const bool admin = stable && editor && added && changed && removed;
            WriteAcceptanceArtifact(
                "final-admin.json",
                std::string("{\"viewerRejected\":") + (stable ? "true" : "false") +
                    ",\"offscreenPresence\":true,\"editorRestored\":" + (editor ? "true" : "false") +
                    ",\"passwordLifecycle\":" + ((added && changed && removed) ? "true" : "false") + "}");
            WriteAcceptanceArtifact("final-editor-role.flag", admin ? "ready" : "admin-error");
            g_acceptanceStep = 11;
            return;
        }
        if (g_acceptanceStep == 11 && std::filesystem::exists(g_acceptanceDirectory / L"final-soak-done.flag")) {
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 12;
            return;
        }
        if (g_acceptanceStep == 12 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1200 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportFinalAcceptanceState("final-host-before-save.json");
            WriteAcceptanceArtifact("final-host-ready.flag", "ready");
            g_acceptanceStep = 13;
            return;
        }
        if (g_acceptanceStep == 13 &&
            std::filesystem::exists(g_acceptanceDirectory / L"final-client-before-save.json")) {
            g_acceptanceDialogAction = AcceptanceDialogAction::Save;
            g_acceptanceDialogComment = "Final Map Together V1 checkpoint.";
            g_acceptanceDialogToken = "final";
            const auto revision = g_collaboration->Revision();
            SaveMap(g_mainWindow, false);
            const bool disk =
                SemanticFingerprint(activeMap) == AcceptanceFingerprint() && g_collaboration->Revision() == revision;
            ExportFinalAcceptanceState("final-host-state.json");
            WriteAcceptanceArtifact("final-disk.json",
                                    std::string("{\"semanticMatch\":") + (disk ? "true" : "false") + "}");
            WriteAcceptanceArtifact("final-complete.flag", "done");
            AppendAcceptanceLog("FINAL complete revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint());
            g_acceptanceStep = 14;
            return;
        }
    } else {
        if (g_acceptanceStep == 0) {
            std::string error;
            if (g_collaboration->Connect({"127.0.0.1", "ForestEditor", "", 39123}, error))
                g_acceptanceStep = 1;
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->State() == collaboration::SessionState::Connected &&
            std::filesystem::exists(g_acceptanceDirectory / L"final-host-edits.flag") &&
            g_collaboration->Revision() >= 2) {
            AcceptanceFlag(7, 7, 1);
            AcceptanceGraphic(8, 8, 1, 4);
            collaboration::Presence presence;
            presence.area = collaboration::PresenceArea::Viewer;
            presence.viewerActive = true;
            presence.x = static_cast<std::uint16_t>(g_map.width - 1);
            presence.y = static_cast<std::uint16_t>(g_map.height - 1);
            std::string error;
            g_collaboration->SendPresence(presence, error);
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && std::filesystem::exists(g_acceptanceDirectory / L"final-edits-converged.flag") &&
            g_pendingCollaborationEdits.empty()) {
            const auto revision = g_collaboration->Revision();
            const auto fingerprint = AcceptanceFingerprint();
            const auto before = ReadFile(activeMap.string());
            const bool modalBefore = g_acceptanceModal;
            SaveMap(g_mainWindow, false);
            const auto after = ReadFile(activeMap.string());
            const bool pass = !modalBefore && !g_acceptanceModal && before == after &&
                              g_collaboration->Revision() == revision && AcceptanceFingerprint() == fingerprint &&
                              g_collaboration->State() == collaboration::SessionState::Connected &&
                              g_activeNotice == save_workflow::SaveDeniedMessage();
            WriteAcceptanceArtifact("client-save-restricted.json",
                                    std::string("{\"pass\":") + (pass ? "true" : "false") +
                                        ",\"revision\":" + std::to_string(revision) + "}");
            g_acceptanceLastRevision = revision;
            initialFingerprint = fingerprint;
            g_acceptanceStep = 3;
            return;
        }
        if (g_acceptanceStep == 3 && g_receivedSaveEvents >= 1) {
            const bool pass = g_collaboration->Revision() == g_acceptanceLastRevision &&
                              AcceptanceFingerprint() == initialFingerprint;
            WriteAcceptanceArtifact("client-save-event-verified.json", std::string("{\"pass\":") +
                                                                           (pass ? "true" : "false") +
                                                                           ",\"notice\":\"Map saved by ForestHost\"}");
            g_acceptanceStep = 4;
            return;
        }
        if (g_acceptanceStep == 4 && std::filesystem::exists(g_acceptanceDirectory / L"cancel-ready.flag") &&
            g_collaboration->Revision() > g_acceptanceLastRevision) {
            WriteAcceptanceArtifact("cancel-edit-seen.flag", "seen");
            g_acceptanceStep = 5;
            return;
        }
        if (g_acceptanceStep == 5 && std::filesystem::exists(g_acceptanceDirectory / L"final-viewer-role.flag") &&
            g_collaboration->LocalRole() == collaboration::ParticipantRole::Viewer) {
            const auto revision = g_collaboration->Revision();
            AcceptanceGraphic(3, 3, 1, 2);
            collaboration::Presence presence;
            presence.area = collaboration::PresenceArea::Viewer;
            presence.viewerActive = true;
            presence.x = static_cast<std::uint16_t>(g_map.width - 1);
            presence.y = static_cast<std::uint16_t>(g_map.height - 1);
            std::string error;
            g_collaboration->SendPresence(presence, error);
            if (g_collaboration->Revision() == revision && g_pendingCollaborationEdits.empty())
                WriteAcceptanceArtifact("final-viewer-rejected.flag", "pass");
            g_acceptanceStep = 6;
            return;
        }
        if (g_acceptanceStep == 6 && std::filesystem::exists(g_acceptanceDirectory / L"final-editor-role.flag") &&
            g_collaboration->LocalRole() == collaboration::ParticipantRole::Editor) {
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptancePresenceIndex = -1;
            WriteAcceptanceArtifact("final-soak-start.flag", "running");
            g_acceptanceStep = 7;
            return;
        }
        if (g_acceptanceStep == 7) {
            const auto elapsed = GetTickCount64() - g_acceptanceBrushStart;
            if (elapsed < 30000) {
                const int index = static_cast<int>(elapsed / 90);
                if (index != g_acceptancePresenceIndex) {
                    g_acceptancePresenceIndex = index;
                    AcceptanceGraphic(2 + index % std::max(1, g_map.width - 4),
                                      2 + (index / 7) % std::max(1, g_map.height - 4), 1, 1 + (index % 4));
                }
                return;
            }
            WriteAcceptanceArtifact("final-soak-done.flag", std::to_string(elapsed));
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 8;
            return;
        }
        if (g_acceptanceStep == 8 && std::filesystem::exists(g_acceptanceDirectory / L"final-host-ready.flag") &&
            g_pendingCollaborationEdits.empty() && GetTickCount64() - g_acceptanceBrushStart > 1200) {
            ExportFinalAcceptanceState("final-client-before-save.json");
            g_acceptanceStep = 9;
            return;
        }
        if (g_acceptanceStep == 9 && std::filesystem::exists(g_acceptanceDirectory / L"final-complete.flag") &&
            g_receivedSaveEvents >= 4) {
            ExportFinalAcceptanceState("final-client-state.json");
            g_acceptanceStep = 10;
            return;
        }
    }
}
void RunAcceptanceStep() {
    if (!g_collaboration || !g_map.loaded)
        return;
    if (g_acceptanceFinal) {
        RunFinalAcceptanceStep();
        return;
    }
    if (g_acceptanceCD) {
        RunCDAcceptanceStep();
        return;
    }
    if (g_acceptanceB3) {
        RunB3AcceptanceStep();
        return;
    }
    if (g_acceptanceRole == AcceptanceRole::Host) {
        if (g_acceptanceStep == 0) {
            collaboration::HostOptions options{"B2 acceptance",
                                               "HarnessHost",
                                               "",
                                               39120,
                                               g_map.name,
                                               {0, static_cast<std::uint16_t>(g_map.width),
                                                static_cast<std::uint16_t>(g_map.height), WriteEmf(g_map)}};
            std::string error;
            if (g_collaboration->StartHosting(options, error)) {
                AppendAcceptanceLog("host ready");
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->ConnectedGuestCount() == 1) {
            AcceptanceGraphic(5, 5, 1, 1);
            AcceptanceGraphic(12, 12, 1, 2, 3);
            AcceptanceFlag(6, 6, 1);
            g_warpSettings = {77, 9, 10, 3, 14};
            AcceptanceFlag(8, 8, 5);
            AppendAcceptanceLog("host graphics cluster block warp submitted");
            g_acceptanceStep = 2;
            return;
        }
        if (g_acceptanceStep == 2 && g_collaboration->Revision() >= 7) {
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 3;
            AppendAcceptanceLog("30 second brush started");
        }
        if (g_acceptanceStep == 3) {
            const auto elapsed = GetTickCount64() - g_acceptanceBrushStart;
            if (elapsed < 30000) {
                const int span = std::max(1, std::min(g_map.width - 4, 40));
                const int index = static_cast<int>(elapsed / 45);
                AcceptanceGraphic(2 + index % span, 2 + (index / span) % std::max(1, std::min(g_map.height - 4, 30)), 1,
                                  3);
                return;
            }
            AppendAcceptanceLog("30 second brush completed duration_ms=" + std::to_string(elapsed));
            WriteAcceptanceArtifact("brush-complete.flag", "done");
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 4;
        }
        if (g_acceptanceStep == 4 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportAcceptanceState("host-state.json");
            WriteAcceptanceArtifact("host-complete.flag", "done");
            AppendAcceptanceLog("host final revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint());
            g_acceptanceStep = 5;
        }
    } else if (g_acceptanceRole == AcceptanceRole::Client) {
        if (g_acceptanceStep == 0) {
            SetGraphicsCategory(0);
            g_editorState.selectedGraphic() = 2;
            g_editTool = EditTool::Pencil;
            g_brushSize = 1;
            std::string error;
            if (g_collaboration->Connect({"127.0.0.1", "HarnessClient", "", 39120}, error)) {
                AppendAcceptanceLog("client connecting");
                g_acceptanceStep = 1;
            }
            return;
        }
        if (g_acceptanceStep == 1 && g_collaboration->State() == collaboration::SessionState::Connected) {
            ExportAcceptanceState("client-ui-initial.json");
            g_acceptanceStep = 2;
        }
        if (g_acceptanceStep == 2 && g_collaboration->Revision() >= 4) {
            ExportAcceptanceState("client-ui-after-remote.json");
            g_chairDirection = 3;
            AcceptanceFlag(7, 7, 4);
            AcceptanceFlag(8, 8, 0);
            AcceptanceGraphic(10, 10, 1, 4);
            AppendAcceptanceLog("client chair open graphic submitted");
            g_acceptanceStep = 3;
        }
        if (g_acceptanceStep == 3 && std::filesystem::exists(g_acceptanceDirectory / "brush-complete.flag")) {
            g_acceptanceLastRevision = g_collaboration->Revision();
            g_acceptanceBrushStart = GetTickCount64();
            g_acceptanceStep = 4;
        }
        if (g_acceptanceStep == 4 && g_pendingCollaborationEdits.empty() &&
            GetTickCount64() - g_acceptanceBrushStart > 1000 &&
            g_collaboration->Revision() == g_acceptanceLastRevision) {
            ExportAcceptanceState("client-state.json");
            AppendAcceptanceLog("client final revision=" + std::to_string(g_collaboration->Revision()) +
                                " fingerprint=" + AcceptanceFingerprint());
            g_acceptanceStep = 5;
        }
    }
}

void ExecuteCommand(HWND window, int command) {
    if (command == kOpenCollaborationWindowCommand)
        ShowCollaborationStatus(window);
    else if (command == kStartCollaborationCommand)
        StartCollaboration(window);
    else if (command == kConnectCollaborationCommand)
        ConnectCollaboration(window);
    else if (command == kManageCollaborationCommand)
        ShowCollaborationStatus(window);
    else if (command == kDisconnectCollaborationCommand) {
        g_collaboration->Disconnect("Session ended.");
        UpdateCollaborationMenu();
        UpdateCollaborationWindow();
    } else if (command == kOpenMapCommand)
        OpenMap(window);
    else if (command == kSaveMapCommand)
        SaveMap(window, false);
    else if (command == kSaveMapAsCommand)
        SaveMap(window, true);
    else if (command == kNewMapCommand)
        CreateNewMap(window);
    else if (command == kCloseMapCommand)
        CloseMap(window);
    else if (command == kExitCommand)
        SendMessageA(window, WM_CLOSE, 0, 0);
    else if (command == kAboutCommand)
        MessageBoxA(
            window,
            "Endless Online\nMapper Studio Reconstruction\n\nBuilt as a modern reconstruction of the original editor.",
            "About Endless Map Studio", MB_OK | MB_ICONINFORMATION);
    else if (command == kOverviewCommand) {
        g_zoom = 1.0;
        g_panOffsetX = 0.0;
        g_panOffsetY = 0.0;
        if (g_map.loaded) {
            g_viewCenterX = g_map.width / 2;
            g_viewCenterY = g_map.height / 2;
        }
        UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
        InvalidatePanels();
    } else if (command == kClearLayerCommand)
        ClearSelectedLayer();
    else if (command == kClearFlagsCommand)
        ClearMapFlags();
    else if (command == kClearAllCommand) {
        if (g_map.loaded &&
            MessageBoxA(window,
                        "Clear all graphics, flags, and map entities? This can be undone until the map is closed.",
                        "Clear map", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES) {
            ClearAllMapData();
        }
    } else if (command == kToggleAllLayersCommand)
        ToggleAllLayers();
    else if (command == kHideAllLayersCommand)
        SetAllLayersVisible(false);
    else if (command == kToggleAllFlagsCommand)
        ToggleAllFlags();
    else if (command == kHideAllFlagsCommand)
        SetAllFlagsVisible(false);
    else if (command == kToggleAllBoundariesCommand)
        ToggleAllBoundaries();
    else if (command == kHideAllBoundariesCommand)
        SetAllBoundariesVisible(false);
    else if (command >= kGraphicsMenuBaseCommand && command < kGraphicsMenuBaseCommand + 6) {
        SetGraphicsCategory(command - kGraphicsMenuBaseCommand);
        InvalidatePanels();
    } else if (command >= kLayerGroupMenuBaseCommand && command < kLayerGroupMenuBaseCommand + 5)
        ToggleGraphicsGroup(command - kLayerGroupMenuBaseCommand);
    else if (command >= kFlagMenuBaseCommand && command < kFlagMenuBaseCommand + 5)
        ToggleFlagVisibility(command - kFlagMenuBaseCommand);
    else if (command >= kBoundaryMenuBaseCommand && command < kBoundaryMenuBaseCommand + 5)
        SetBoundaryVisibility(command - kBoundaryMenuBaseCommand,
                              !g_boundaryVisible[command - kBoundaryMenuBaseCommand]);
    else if (command >= kClearGraphicLayerBaseCommand && command < kClearGraphicLayerBaseCommand + 9) {
        if (MessageBoxA(window, "Clear every graphic on this layer?", "Clear layer",
                        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
            ClearGraphicLayer(command - kClearGraphicLayerBaseCommand);
    } else if (command >= kClearFlagCategoryBaseCommand && command < kClearFlagCategoryBaseCommand + 5) {
        if (MessageBoxA(window, "Clear every flag in this category?", "Clear flags",
                        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
            ClearFlagCategory(command - kClearFlagCategoryBaseCommand);
    } else if (command >= kToggleLayerBaseCommand && command < kToggleLayerBaseCommand + 12)
        ToggleLayerVisibility(command - kToggleLayerBaseCommand);
    else if (command == kZoomInCommand || command == kZoomOutCommand) {
        RECT client{};
        GetClientRect(g_panels[static_cast<std::size_t>(PanelKind::Viewer)], &client);
        const RECT canvas = ViewerCanvasRect(client);
        ZoomAtPoint(POINT{(canvas.left + canvas.right) / 2, (canvas.top + canvas.bottom) / 2},
                    command == kZoomInCommand);
    } else if (command == kZoomResetCommand) {
        g_zoom = 1.0;
        g_panOffsetX = 0;
        g_panOffsetY = 0;
        UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
        InvalidateRect(g_panels[static_cast<std::size_t>(PanelKind::Viewer)], nullptr, FALSE);
    } else if (command >= kPanelBaseCommand && command < kPanelBaseCommand + static_cast<int>(g_panels.size())) {
        const std::size_t index = static_cast<std::size_t>(command - kPanelBaseCommand);
        const bool show = !IsWindowVisible(g_panels[index]);
        ShowWindow(g_panels[index], show ? SW_SHOW : SW_HIDE);
        CheckMenuItem(g_windowsMenu, command, MF_BYCOMMAND | (show ? MF_CHECKED : MF_UNCHECKED));
        if (show) {
            g_activePanel = g_panels[index];
            SetFocus(g_activePanel);
            EnsureEditorWindowZOrder(index == static_cast<std::size_t>(PanelKind::Viewer) ? nullptr : g_activePanel);
            InvalidateRect(g_activePanel, nullptr, TRUE);
        }
    } else if (command == 1120)
        ResizeMap(g_map.width + 1, g_map.height);
    else if (command == 1121)
        ResizeMap(g_map.width, g_map.height + 1);
    else if (command == 1122)
        ResizeMap(g_map.width - 1, g_map.height);
    else if (command == 1123)
        ResizeMap(g_map.width, g_map.height - 1);
    else if (command == 1124)
        BeginBaseTileSelection();
    else if (command == 1300) {
        if (!g_collaboration || g_collaboration->State() == collaboration::SessionState::Disconnected)
            UndoMap();
    } else if (command == 1301) {
        if (!g_collaboration || g_collaboration->State() == collaboration::SessionState::Disconnected)
            RedoMap();
    } else if (command == 1310)
        g_editTool = EditTool::Pencil;
    else if (command == 1311)
        g_editTool = EditTool::Brush;
    else if (command == 1312)
        g_editTool = EditTool::Eraser;
    else if (command == 1313)
        g_editTool = EditTool::Wipe;
    else if (command == 1314 || command == kGraphicsModeCommand) {
        SetEditDomain(false);
        ShowWindow(g_panels[static_cast<std::size_t>(PanelKind::Graphics)], SW_SHOW);
        SetFocus(g_panels[static_cast<std::size_t>(PanelKind::Graphics)]);
        EnsureEditorWindowZOrder(g_panels[static_cast<std::size_t>(PanelKind::Graphics)]);
    } else if (command == 1315 || command == kFlagsModeCommand) {
        SetEditDomain(true);
        ShowWindow(g_panels[static_cast<std::size_t>(PanelKind::Flags)], SW_SHOW);
        SetFocus(g_panels[static_cast<std::size_t>(PanelKind::Flags)]);
        EnsureEditorWindowZOrder(g_panels[static_cast<std::size_t>(PanelKind::Flags)]);
    } else if (command == kEntitiesModeCommand) {
        g_entityMode = true;
        g_entityDraft = ReadEntitiesAt(g_cursorX, g_cursorY);
        HWND entities = g_panels[static_cast<std::size_t>(PanelKind::Entities)];
        ShowWindow(entities, SW_SHOW);
        SetFocus(entities);
        EnsureEditorWindowZOrder(entities);
        SyncEntityControls();
    } else if (command == kSingleEditCommand)
        g_brushSize = 1;
    else if (command == kClusterEditCommand)
        g_brushSize = 3;
    else if (command == kNoEditCommand)
        g_brushSize = 0;
    if ((command >= 1310 && command <= 1315) || (command >= kSingleEditCommand && command <= kFlagsModeCommand) ||
        command == kEntitiesModeCommand) {
        UpdateToolMenuChecks();
        InvalidatePanels();
    }
}

void PositionPanels(HWND window) {
    if (g_panels.empty()) {
        return;
    }
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;
    if (g_previousClientSize.cx == 0 || g_previousClientSize.cy == 0) {
        const int layout[][4] = {
            {12, 12, 500, 300},
            {width - 312, 12, 300, 385},
            {15, height - 275, 215, 260},
            {width / 2 - 150, height / 2 - 80, 300, 170},
            {width * 3 / 4 - 120, height / 2 - 120, 240, 115},
            {width - 300, height - 370, 220, 180},
            {width / 2 - 230, height / 2 - 180, 460, 360},
        };
        for (std::size_t index = 0; index < g_panels.size(); ++index) {
            SetWindowPos(g_panels[index], nullptr, layout[index][0], layout[index][1], layout[index][2],
                         layout[index][3], SWP_NOZORDER | SWP_NOACTIVATE);
        }
    } else {
        const int deltaX = width - g_previousClientSize.cx;
        const int deltaY = height - g_previousClientSize.cy;
        for (std::size_t index = 0; index < g_panels.size(); ++index) {
            RECT bounds{};
            GetWindowRect(g_panels[index], &bounds);
            MapWindowPoints(HWND_DESKTOP, window, reinterpret_cast<POINT*>(&bounds), 2);
            int x = bounds.left;
            int y = bounds.top;
            switch (static_cast<PanelKind>(index)) {
            case PanelKind::Viewer:
                break;
            case PanelKind::Graphics:
                x += deltaX;
                break;
            case PanelKind::Layers:
                y += deltaY;
                break;
            case PanelKind::Properties:
                x += deltaX / 2;
                y += deltaY / 2;
                break;
            case PanelKind::Flags:
                x += deltaX * 3 / 4;
                y += deltaY / 2;
                break;
            case PanelKind::Toolset:
                x += deltaX;
                y += deltaY;
                break;
            case PanelKind::Entities:
                x += deltaX / 2;
                y += deltaY / 2;
                break;
            }
            const int panelWidth = bounds.right - bounds.left;
            const int panelHeight = bounds.bottom - bounds.top;
            x = std::clamp(x, std::min(4, width - panelWidth), std::max(4, width - panelWidth));
            y = std::clamp(y, std::min(4, height - panelHeight), std::max(4, height - panelHeight));
            SetWindowPos(g_panels[index], nullptr, x, y, panelWidth, panelHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    g_previousClientSize = SIZE{width, height};
    EnsureEditorWindowZOrder(g_activePanel && g_activePanel != g_panels[static_cast<std::size_t>(PanelKind::Viewer)]
                                 ? g_activePanel
                                 : nullptr);
    InvalidatePanels();
}

LRESULT CALLBACK MainProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        g_mainWindow = window;
        g_collaboration = std::make_unique<collaboration::Session>(
            [window] { PostMessageA(window, kCollaborationEventMessage, 0, 0); });
        g_collaborationWindow = std::make_unique<collaboration::Window>(
            window, g_uiFont,
            collaboration::WindowCallbacks{
                [window] { StartCollaboration(window); }, [window] { ConnectCollaboration(window); },
                [] {
                    if (g_collaboration)
                        g_collaboration->Disconnect("Session ended.");
                },
                [](std::string_view text, std::string& error) {
                    return g_collaboration && g_collaboration->SendChat(text, error);
                },
                [](std::uint32_t id, collaboration::ParticipantRole role, std::string& error) {
                    const bool result = g_collaboration && g_collaboration->ChangeParticipantRole(id, role, error);
                    UpdateCollaborationWindow();
                    return result;
                },
                [](std::uint32_t id, std::string& error) {
                    return g_collaboration && g_collaboration->DisconnectParticipant(id, error);
                },
                [](std::string password, std::string& error) {
                    const bool result = g_collaboration && g_collaboration->SetPassword(std::move(password), error);
                    UpdateCollaborationWindow();
                    return result;
                }});
        g_collaborationWindow->AppendSystem("Map Together is not connected.");
        CreateMenuBar(window);
        UpdateCollaborationMenu();
        return 0;
    case kCollaborationEventMessage:
        ProcessCollaborationEvents(window);
        return 0;
    case WM_TIMER:
        if (wParam == kAcceptanceTimer) {
            RunAcceptanceStep();
            return 0;
        }
        if (wParam == 78) {
            KillTimer(window, 78);
            g_activeNotice.clear();
            if (IsWindow(g_noticeWindow))
                ShowWindow(g_noticeWindow, SW_HIDE);
            if (!g_noticeQueue.empty())
                DisplayClassicNotice();
            return 0;
        }
    case WM_COMMAND:
        ExecuteCommand(window, LOWORD(wParam));
        return 0;
    case WM_SIZE:
        PositionPanels(window);
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            if (!g_panels.empty())
                ResetViewerNavigation(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
        } else {
            EnsureEditorWindowZOrder(g_activePanel && !g_panels.empty() &&
                                             g_activePanel != g_panels[static_cast<std::size_t>(PanelKind::Viewer)]
                                         ? g_activePanel
                                         : nullptr);
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        HBRUSH background = CreateSolidBrush(classic_ui::Workspace);
        FillRect(dc, &client, background);
        DeleteObject(background);
        if (!g_activeNotice.empty()) {
            RECT notice{8, client.bottom - 34, std::min(client.right - 8, 318L), client.bottom - 8};
            HBRUSH fill = CreateSolidBrush(classic_ui::Panel);
            FillRect(dc, &notice, fill);
            DeleteObject(fill);
            DrawEdge(dc, &notice, EDGE_RAISED, BF_RECT);
            RECT textRect{notice.left + 8, notice.top + 5, notice.right - 6, notice.bottom - 3};
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, classic_ui::Text);
            DrawTextA(dc, g_activeNotice.c_str(), -1, &textRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        }
        EndPaint(window, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        g_collaborationWindow.reset();
        if (g_collaboration) {
            g_collaboration->Disconnect("Application closing");
            g_collaboration.reset();
        }
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        if (g_map.dirty) {
            const int choice = MessageBoxA(window, "Save changes before exiting?", "Unsaved map changes",
                                           MB_YESNOCANCEL | MB_ICONWARNING);
            if (choice == IDCANCEL)
                return 0;
            if (choice == IDYES) {
                SaveMap(window, false);
                if (g_map.dirty)
                    return 0;
            }
        }
        DestroyWindow(window);
        return 0;
    }

    return DefWindowProcA(window, message, wParam, lParam);
}

bool RegisterWindowClasses(HINSTANCE instance) {
    WNDCLASSEXA mainClass{};
    mainClass.cbSize = sizeof(mainClass);
    mainClass.lpfnWndProc = MainProc;
    mainClass.hInstance = instance;
    mainClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    mainClass.lpszClassName = kMainClass;
    if (!RegisterClassExA(&mainClass)) {
        return false;
    }

    WNDCLASSEXA panelClass{};
    panelClass.cbSize = sizeof(panelClass);
    panelClass.lpfnWndProc = PanelProc;
    panelClass.hInstance = instance;
    panelClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    panelClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    panelClass.lpszClassName = kPanelClass;
    return RegisterClassExA(&panelClass) != 0;
}

void CreatePanels(HINSTANCE instance) {
    static PanelData panelData[] = {
        {PanelKind::Viewer, "Viewer"},     {PanelKind::Graphics, "Graphics"},
        {PanelKind::Layers, "Layers"},     {PanelKind::Properties, "Map properties"},
        {PanelKind::Flags, "Flags"},       {PanelKind::Toolset, "Toolset"},
        {PanelKind::Entities, "Entities"},
    };

    for (std::size_t panelIndex = 0; panelIndex < std::size(panelData); ++panelIndex) {
        PanelData& panel = panelData[panelIndex];
        DWORD style = WS_CHILD | WS_VISIBLE | WS_THICKFRAME | WS_CLIPSIBLINGS;
        if (panelIndex == static_cast<std::size_t>(PanelKind::Viewer))
            style |= WS_HSCROLL | WS_VSCROLL;
        HWND handle = CreateWindowExA(WS_EX_TOOLWINDOW, kPanelClass, panel.title, style, 0, 0, 200, 120, g_mainWindow,
                                      nullptr, instance, &panel);
        g_panels.push_back(handle);
    }
    CreateMapPropertyControls(g_panels[static_cast<std::size_t>(PanelKind::Properties)], instance);
    CreateLayerControls(g_panels[static_cast<std::size_t>(PanelKind::Layers)], instance);
    CreateFlagControls(g_panels[static_cast<std::size_t>(PanelKind::Flags)], instance);
    CreateEntityControls(g_panels[static_cast<std::size_t>(PanelKind::Entities)], instance);
    ShowWindow(g_panels[static_cast<std::size_t>(PanelKind::Entities)], SW_HIDE);
    if (!g_panels.empty()) {
        g_activePanel = g_panels.front();
        SetFocus(g_activePanel);
    }
    PositionPanels(g_mainWindow);
}

int RunModelTest(const std::string& testName, const std::string& inputPath, const std::string& outputPath) {
    try {
        MapDocument map = ReadEmf(inputPath.c_str());
        if (testName == "map_resize_grow") {
            const int oldWidth = map.width;
            const int oldHeight = map.height;
            const MapTile retained = map.tile(0, 0);
            if (!ResizeMapDocument(map, oldWidth + 1, oldHeight + 1) || map.width != oldWidth + 1 ||
                map.height != oldHeight + 1 || map.tile(0, 0) != retained ||
                map.tile(oldWidth, oldHeight).graphics[0] != map.fillTile)
                return 3;
            return 0;
        }
        if (testName == "map_resize_shrink" || testName == "map_resize_preserves_in_bounds_data") {
            map.tile(0, 0).graphics[1] = 17;
            map.tile(0, 0).spec = 6;
            map.tile(0, 0).warp = MapWarp{12, 3, 4, 0, 0};
            map.tile(map.width - 1, map.height - 1).graphics[1] = 91;
            const int newWidth = std::max(1, map.width - 1);
            const int newHeight = std::max(1, map.height - 1);
            if (!ResizeWouldDiscardData(map, newWidth, newHeight) || !ResizeMapDocument(map, newWidth, newHeight) ||
                map.width != newWidth || map.height != newHeight)
                return 3;
            const MapTile& retained = map.tile(0, 0);
            if (retained.graphics[1] != 17 || retained.spec != 6 || !retained.warp ||
                retained.warp->destinationMap != 12 || retained.warp->x != 3 || retained.warp->y != 4)
                return 3;
            return 0;
        }
        if (testName == "map_properties_resize") {
            const MapTile retained = map.tile(0, 0);
            if (!ResizeMapDocument(map, map.width + 1, map.height + 1) || map.tile(0, 0) != retained)
                return 3;
            if (!ResizeMapDocument(map, map.width - 1, map.height - 1) || map.tile(0, 0) != retained)
                return 3;
            return 0;
        }
        if (testName == "base_tile_does_not_destroy_ground_graphics") {
            const int oldFill = map.fillTile;
            const int explicitGraphic = oldFill == 777 ? 778 : 777;
            map.tile(0, 0).graphics[0] = explicitGraphic;
            const int newFill = oldFill == 1 ? 2 : 1;
            for (MapTile& tile : map.tiles)
                if (tile.graphics[0] == oldFill || tile.graphics[0] < 0)
                    tile.graphics[0] = newFill;
            map.fillTile = newFill;
            return map.tile(0, 0).graphics[0] == explicitGraphic ? 0 : 3;
        }
        if (testName == "clear_specific_graphic_layer") {
            map.tile(0, 0).graphics[1] = 123;
            map.tile(0, 0).graphics[2] = 456;
            return ClearGraphicLayerData(map, 1) && map.tile(0, 0).graphics[1] == -1 &&
                           map.tile(0, 0).graphics[2] == 456
                       ? 0
                       : 3;
        }
        if (testName == "clear_block_flags_only") {
            map.tile(0, 0).spec = 0;
            map.tile(1, 0).spec = 9;
            return ClearFlagCategoryData(map, 0) && map.tile(0, 0).spec == -1 && map.tile(1, 0).spec == 9 ? 0 : 3;
        }
        if (testName == "clear_warps_only") {
            map.tile(0, 0).warp = MapWarp{4, 5, 6, 0, 0};
            map.tile(1, 0).warp = MapWarp{4, 5, 6, 0, 1};
            return ClearFlagCategoryData(map, 4) && !map.tile(0, 0).warp && map.tile(1, 0).warp.has_value() ? 0 : 3;
        }

        if (testName == "flag_block_roundtrip")
            map.tile(0, 0).spec = 0;
        else if (testName == "flag_warp_roundtrip")
            map.tile(0, 0).warp = MapWarp{123, 17, 29, 4, 0};
        else if (testName == "chair_direction_roundtrip")
            map.tile(0, 0).spec = 6;
        else if (testName == "entity_npc_roundtrip")
            map.npcs.push_back(MapNpc{1, 2, 321, 4, 90, 3});
        else if (testName == "entity_item_roundtrip")
            map.items.push_back(MapItem{1, 2, 77, 3, 456, 120, 5000});
        else if (testName == "entity_sign_roundtrip") {
            MapSign sign;
            sign.x = 1;
            sign.y = 2;
            sign.titleLength = 5;
            const std::string text = "TitleMessage";
            sign.encodedText.assign(text.begin(), text.end());
            eo_encode_string(sign.encodedText.data(), sign.encodedText.size());
            map.signs.push_back(std::move(sign));
        } else if (testName == "base_tile_roundtrip") {
            map.fillTile = map.fillTile == 1 ? 2 : 1;
            map.tile(0, 0).graphics[0] = map.fillTile;
        } else
            return 2;

        WriteFile(outputPath, WriteEmf(map));
        const MapDocument restored = ReadEmf(outputPath.c_str());
        return EquivalentMaps(map, restored) ? 0 : 3;
    } catch (const std::exception&) {
        return 2;
    }
}

int RunEmfCheckMode() {
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (!arguments) {
        return -1;
    }

    int result = -1;
    if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--model-test") {
        result = 2;
        if (argumentCount == 5) {
            const auto narrow = [](const wchar_t* value) {
                const int count = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
                std::string text(static_cast<std::size_t>(std::max(1, count)), '\0');
                if (count > 0)
                    WideCharToMultiByte(CP_UTF8, 0, value, -1, text.data(), count, nullptr, nullptr);
                if (!text.empty() && text.back() == '\0')
                    text.pop_back();
                return text;
            };
            result = RunModelTest(narrow(arguments[2]), narrow(arguments[3]), narrow(arguments[4]));
        }
    } else if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--check-emf") {
        result = 2;
        if (argumentCount == 3) {
            const int byteCount = WideCharToMultiByte(CP_ACP, 0, arguments[2], -1, nullptr, 0, nullptr, nullptr);
            if (byteCount > 0) {
                std::string path(static_cast<std::size_t>(byteCount), '\0');
                if (WideCharToMultiByte(CP_ACP, 0, arguments[2], -1, path.data(), byteCount, nullptr, nullptr) > 0) {
                    try {
                        const MapDocument map = ReadEmf(path.c_str());
                        result = map.loaded ? 0 : 2;
                    } catch (const std::exception&) {
                        result = 2;
                    }
                }
            }
        }
    } else if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--roundtrip-emf") {
        result = 2;
        if (argumentCount == 4) {
            const int inputByteCount = WideCharToMultiByte(CP_ACP, 0, arguments[2], -1, nullptr, 0, nullptr, nullptr);
            const int outputByteCount = WideCharToMultiByte(CP_ACP, 0, arguments[3], -1, nullptr, 0, nullptr, nullptr);
            if (inputByteCount > 0 && outputByteCount > 0) {
                std::string inputPath(static_cast<std::size_t>(inputByteCount), '\0');
                std::string outputPath(static_cast<std::size_t>(outputByteCount), '\0');
                if (WideCharToMultiByte(CP_ACP, 0, arguments[2], -1, inputPath.data(), inputByteCount, nullptr,
                                        nullptr) > 0 &&
                    WideCharToMultiByte(CP_ACP, 0, arguments[3], -1, outputPath.data(), outputByteCount, nullptr,
                                        nullptr) > 0) {
                    try {
                        const MapDocument original = ReadEmf(inputPath.c_str());
                        WriteFile(outputPath.c_str(), WriteEmf(original));
                        const MapDocument restored = ReadEmf(outputPath.c_str());
                        result = EquivalentMaps(original, restored) ? 0 : 3;
                    } catch (const std::exception&) {
                        result = 2;
                    }
                }
            }
        }
    } else if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--edit-roundtrip-emf") {
        result = 2;
        if (argumentCount == 4) {
            const int inputByteCount = WideCharToMultiByte(CP_ACP, 0, arguments[2], -1, nullptr, 0, nullptr, nullptr);
            const int outputByteCount = WideCharToMultiByte(CP_ACP, 0, arguments[3], -1, nullptr, 0, nullptr, nullptr);
            if (inputByteCount > 0 && outputByteCount > 0) {
                std::string inputPath(static_cast<std::size_t>(inputByteCount), '\0');
                std::string outputPath(static_cast<std::size_t>(outputByteCount), '\0');
                if (WideCharToMultiByte(CP_ACP, 0, arguments[2], -1, inputPath.data(), inputByteCount, nullptr,
                                        nullptr) > 0 &&
                    WideCharToMultiByte(CP_ACP, 0, arguments[3], -1, outputPath.data(), outputByteCount, nullptr,
                                        nullptr) > 0) {
                    try {
                        MapDocument edited = ReadEmf(inputPath.c_str());
                        edited.tile(0, 0).graphics[0] = edited.fillTile == 1 ? 2 : 1;
                        edited.tile(0, 0).spec = 18;
                        WriteFile(outputPath.c_str(), WriteEmf(edited));
                        const MapDocument restored = ReadEmf(outputPath.c_str());
                        result = EquivalentMaps(edited, restored) ? 0 : 3;
                    } catch (const std::exception&) {
                        result = 2;
                    }
                }
            }
        }
    } else if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--check-emf-directory") {
        result = 2;
        if (argumentCount == 3) {
            try {
                result = CheckEmfDirectory(std::filesystem::path(arguments[2])) > 0 ? 0 : 2;
            } catch (const std::exception&) {
                result = 2;
            }
        }
    } else if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--check-egf") {
        result = 2;
        if (argumentCount == 4) {
            try {
                const int bank = std::stoi(arguments[2]);
                const int resourceId = std::stoi(arguments[3]);
                result = g_gfx.Get(bank, resourceId).bitmap ? 0 : 2;
            } catch (const std::exception&) {
                result = 2;
            }
        }
    } else if (argumentCount >= 2 && std::wstring(arguments[1]) == L"--check-egf-banks") {
        result = 0;
        for (int bank : kPaletteBanks) {
            const auto& ids = g_gfx.GraphicIds(bank);
            if (ids.empty() || !g_gfx.Get(bank, ids.front() + 100).bitmap) {
                result = 2;
                break;
            }
        }
    }
    LocalFree(arguments);
    return result;
}

void OpenStartupMapIfRequested() {
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (!arguments)
        return;
    int openIndex = -1;
    for (int i = 1; i + 1 < argumentCount; ++i)
        if (std::wstring(arguments[i]) == L"--open-ui") {
            openIndex = i;
            break;
        }
    if (openIndex >= 0) {
        const int count = WideCharToMultiByte(CP_ACP, 0, arguments[openIndex + 1], -1, nullptr, 0, nullptr, nullptr);
        if (count > 0) {
            std::string path(static_cast<std::size_t>(count), '\0');
            if (WideCharToMultiByte(CP_ACP, 0, arguments[openIndex + 1], -1, path.data(), count, nullptr, nullptr) >
                0) {
                try {
                    g_map = ReadEmf(path.c_str());
                    g_map.dirty = false;
                    g_cursorX = std::min(8, g_map.width - 1);
                    g_cursorY = std::min(14, g_map.height - 1);
                    g_camera.centerTileX = g_cursorX;
                    g_camera.centerTileY = g_cursorY;
                    g_camera.offsetX = 0.0;
                    g_camera.offsetY = 0.0;
                    g_camera.zoom = 1.0;
                    UpdateMapTitle();
                    UpdateViewerScrollbars(g_panels[static_cast<std::size_t>(PanelKind::Viewer)]);
                    InvalidatePanels();
                } catch (const std::exception& error) {
                    MessageBoxA(g_mainWindow, error.what(), "Unable to open startup map", MB_OK | MB_ICONERROR);
                }
            }
        }
    }
    LocalFree(arguments);
}

void ConfigureAcceptanceMode() {
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!args)
        return;
    for (int i = 1; i + 1 < count; ++i) {
        const std::wstring option = args[i];
        if (option == L"--b2-accept-host" || option == L"--b2-accept-client" || option == L"--b3-accept-host" ||
            option == L"--b3-accept-client" || option == L"--cd-accept-host" || option == L"--cd-accept-client" ||
            option == L"--final-accept-host" || option == L"--final-accept-client") {
            g_acceptanceB3 = option.find(L"--b3-") == 0;
            g_acceptanceCD = option.find(L"--cd-") == 0;
            g_acceptanceFinal = option.find(L"--final-") == 0;
            g_acceptanceRole =
                option.find(L"host") != std::wstring::npos ? AcceptanceRole::Host : AcceptanceRole::Client;
            g_acceptanceDirectory = args[i + 1];
            break;
        }
    }
    LocalFree(args);
}

} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand) {
    ConfigureAcceptanceMode();
    g_gfx.SetDirectory(FindGraphicsDirectory());
    const int checkResult = RunEmfCheckMode();
    if (checkResult >= 0) {
        return checkResult;
    }
    g_uiFont = CreateFontA(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Tahoma");

    if (!RegisterWindowClasses(instance)) {
        MessageBoxA(nullptr, "Unable to initialize the application windows.", "Endless Map Studio",
                    MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND window = CreateWindowExA(0, kMainClass, "Endless Map Studio - Untitled", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 1440, 820, nullptr, nullptr, instance, nullptr);
    if (!window) {
        return 1;
    }

    CreatePanels(instance);
    OpenStartupMapIfRequested();
    ShowWindow(window, showCommand);
    UpdateWindow(window);
    if (g_acceptanceRole != AcceptanceRole::None) {
        const int x = g_acceptanceRole == AcceptanceRole::Host ? 0 : 720;
        SetWindowPos(window, nullptr, x, 20, 700, 760, SWP_NOZORDER | SWP_SHOWWINDOW);
        SetTimer(window, kAcceptanceTimer, 50, nullptr);
    }

    MSG message{};
    while (GetMessageA(&message, nullptr, 0, 0) > 0) {
        const HWND focus = GetFocus();
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN &&
            (focus == g_mapWidthEdit || focus == g_mapHeightEdit)) {
            CommitMapPropertyDimensions(g_panels[static_cast<std::size_t>(PanelKind::Properties)]);
            continue;
        }
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN &&
            (focus == g_warpMapEdit || focus == g_warpXEdit || focus == g_warpYEdit)) {
            CommitWarpControls();
            continue;
        }
        const HWND viewer = g_panels.empty() ? nullptr : g_panels[static_cast<std::size_t>(PanelKind::Viewer)];
        if (focus != viewer && IsDialogMessageA(g_mainWindow, &message))
            continue;
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    if (g_uiFont) {
        DeleteObject(g_uiFont);
    }
    return static_cast<int>(message.wParam);
}
