#include "panel.hpp"
#include <algorithm>
#include <vector>

// External dependencies from main.cpp
extern std::vector<HWND> g_panels;

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
