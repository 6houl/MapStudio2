#include "classic_ui.hpp"

#include <algorithm>

namespace classic_ui {
void DrawCenteredText(HDC dc, HFONT font, const char* text, RECT bounds, COLORREF color) {
    const HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextA(dc, text, -1, &bounds, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    if (oldFont) SelectObject(dc, oldFont);
}

void DrawTabs(HDC dc, HFONT font, RECT bounds, const char* const* labels, int count, int active) {
    if (count <= 0) return;
    const int width = std::max(18, static_cast<int>(bounds.right - bounds.left) / count);
    const HGDIOBJ oldFont = font ? SelectObject(dc, font) : nullptr;
    SetBkMode(dc, TRANSPARENT);
    for (int index = 0; index < count; ++index) {
        const int left = bounds.left + index * width;
        const int right = index == count - 1 ? bounds.right : bounds.left + (index + 1) * width;
        POINT shape[] = {
            { left, bounds.bottom - 1 }, { left + 4, bounds.top + 1 },
            { right - 4, bounds.top + 1 }, { right, bounds.bottom - 1 }
        };
        HBRUSH fill = CreateSolidBrush(index == active ? Content : Control);
        HPEN border = CreatePen(PS_SOLID, 1, Shadow);
        HGDIOBJ oldBrush = SelectObject(dc, fill);
        HGDIOBJ oldPen = SelectObject(dc, border);
        Polygon(dc, shape, 4);
        SelectObject(dc, oldPen);
        SelectObject(dc, oldBrush);
        DeleteObject(border);
        DeleteObject(fill);
        RECT text{ left + 5, bounds.top + 1, right - 4, bounds.bottom - 1 };
        DrawCenteredText(dc, font, labels[index], text);
        if (index == active) {
            HPEN seam = CreatePen(PS_SOLID, 1, Content);
            oldPen = SelectObject(dc, seam);
            MoveToEx(dc, left + 1, bounds.bottom - 1, nullptr);
            LineTo(dc, right, bounds.bottom - 1);
            SelectObject(dc, oldPen);
            DeleteObject(seam);
        }
    }
    if (oldFont) SelectObject(dc, oldFont);
}

int HitTestTabs(RECT bounds, int count, POINT point) {
    if (count <= 0 || point.y < bounds.top || point.y >= bounds.bottom) return -1;
    const int width = std::max(18, static_cast<int>(bounds.right - bounds.left) / count);
    const int height = std::max(1, static_cast<int>(bounds.bottom - bounds.top));
    const int inset = 4 * (bounds.bottom - point.y) / height;
    for (int index = 0; index < count; ++index) {
        const int left = bounds.left + index * width;
        const int right = index == count - 1 ? bounds.right : bounds.left + (index + 1) * width;
        if (point.x >= left + inset && point.x < right - inset) return index;
    }
    return -1;
}

void DrawArrowButton(HDC dc, RECT bounds, bool pointsRight, bool pressed) {
    HBRUSH face = CreateSolidBrush(pressed ? Panel : Control);
    FillRect(dc, &bounds, face);
    DeleteObject(face);
    DrawEdge(dc, &bounds, pressed ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
    const int shift = pressed ? 1 : 0;
    const int cx = (bounds.left + bounds.right) / 2 + shift;
    const int cy = (bounds.top + bounds.bottom) / 2 + shift;
    POINT triangle[3] = { pointsRight ? POINT{cx - 2, cy - 4} : POINT{cx + 2, cy - 4},
        pointsRight ? POINT{cx - 2, cy + 4} : POINT{cx + 2, cy + 4},
        pointsRight ? POINT{cx + 3, cy} : POINT{cx - 3, cy} };
    HBRUSH ink = CreateSolidBrush(Text);
    HPEN pen = CreatePen(PS_SOLID, 1, Text);
    HGDIOBJ oldBrush = SelectObject(dc, ink);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    Polygon(dc, triangle, 3);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(pen);
    DeleteObject(ink);
}

void DrawCloseButton(HDC dc, RECT bounds, bool pressed) {
    HBRUSH face = CreateSolidBrush(pressed ? Panel : Control);
    FillRect(dc, &bounds, face);
    DeleteObject(face);
    DrawEdge(dc, &bounds, pressed ? EDGE_SUNKEN : EDGE_RAISED, BF_RECT);
    const int shift = pressed ? 1 : 0;
    HPEN pen = CreatePen(PS_SOLID, 1, Text);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    MoveToEx(dc, bounds.left + 4 + shift, bounds.top + 4 + shift, nullptr);
    LineTo(dc, bounds.right - 4 + shift, bounds.bottom - 4 + shift);
    MoveToEx(dc, bounds.right - 5 + shift, bounds.top + 4 + shift, nullptr);
    LineTo(dc, bounds.left + 3 + shift, bounds.bottom - 4 + shift);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}
}
