#pragma once

#include <windows.h>

namespace classic_ui {
inline constexpr COLORREF Workspace = RGB(172, 169, 162);
inline constexpr COLORREF Panel = RGB(224, 220, 211);
inline constexpr COLORREF Content = RGB(239, 236, 228);
inline constexpr COLORREF Control = RGB(216, 212, 203);
inline constexpr COLORREF Edit = RGB(250, 249, 244);
inline constexpr COLORREF Text = RGB(28, 28, 28);
inline constexpr COLORREF DisabledText = RGB(112, 112, 108);
inline constexpr COLORREF Shadow = RGB(137, 134, 126);
inline constexpr COLORREF Highlight = RGB(255, 255, 252);
inline constexpr COLORREF ActiveTitle = RGB(35, 116, 181);
inline constexpr COLORREF InactiveTitle = RGB(132, 145, 154);
inline constexpr COLORREF Selection = RGB(49, 106, 164);
inline constexpr int FormMargin = 12;
inline constexpr int FormControlHeight = 22;
inline constexpr int FormEditWidth = 44;
inline constexpr int FormButtonWidth = 60;
inline constexpr int FormGap = 8;

void DrawTabs(HDC dc, HFONT font, RECT bounds, const char* const* labels, int count, int active);
int HitTestTabs(RECT bounds, int count, POINT point);
void DrawCenteredText(HDC dc, HFONT font, const char* text, RECT bounds, COLORREF color = Text);
void DrawArrowButton(HDC dc, RECT bounds, bool pointsRight, bool pressed = false);
void DrawCloseButton(HDC dc, RECT bounds, bool pressed = false);
}
