#pragma once

#include "../editor/editor_state.hpp"
#include <Windows.h>

// Panel kind enumeration
enum class PanelKind {
    Viewer = 0,
    Graphics = 1,
    Layers = 2,
    Properties = 3,
    Flags = 4,
    Toolset = 5,
    Entities = 6,
};

// Panel data structure storing panel state
struct PanelData {
    PanelKind kind;
    const char* title;
    HDC backBufferDc = nullptr;
    HBITMAP backBufferBitmap = nullptr;
    HBITMAP backBufferPreviousBitmap = nullptr;
    SIZE backBufferSize{};
};

// Panel back-buffer management
bool EnsurePanelBackBuffer(PanelData& panel, HDC target, int width, int height);
void ReleasePanelBackBuffer(PanelData& panel);

// Panel z-order management
void EnsureEditorWindowZOrder(HWND activePalette);
