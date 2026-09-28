#include "editor_camera.hpp"

#include <cmath>

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr double kTileHalfWidth = 32.0;
constexpr double kTileHalfHeight = 16.0;

CameraPoint ViewportCenter(const CameraViewport& viewport) {
    return { (viewport.left + viewport.right) / 2.0, (viewport.top + viewport.bottom) / 2.0 };
}

}

bool ApplyCameraNavigation(EditorCamera& camera, const CameraNavigationKeys& keys,
    double elapsedSeconds, double pixelsPerSecond) {
    double x = static_cast<double>(keys.left) - static_cast<double>(keys.right);
    double y = static_cast<double>(keys.up) - static_cast<double>(keys.down);
    const double length = std::sqrt(x * x + y * y);
    if (length == 0.0 || elapsedSeconds <= 0.0 || pixelsPerSecond <= 0.0) return false;
    const double distance = elapsedSeconds * pixelsPerSecond / length;
    camera.Pan(x * distance, y * distance);
    return true;
}

CameraPoint EditorCamera::MapToScreenCenter(int tileX, int tileY, const CameraViewport& viewport) const {
    const CameraPoint center = ViewportCenter(viewport);
    const double dx = static_cast<double>(tileX - centerTileX);
    const double dy = static_cast<double>(tileY - centerTileY);
    return {
        center.x + offsetX + ((dx - dy) * kTileHalfWidth + kTileHalfWidth) * zoom,
        center.y + offsetY + ((dx + dy) * kTileHalfHeight + kTileHalfHeight) * zoom,
    };
}

CameraPoint EditorCamera::MapToScreenTop(int tileX, int tileY, const CameraViewport& viewport) const {
    CameraPoint point = MapToScreenCenter(tileX, tileY, viewport);
    point.y -= kTileHalfHeight * zoom;
    return point;
}

CameraTile EditorCamera::ScreenToMap(CameraPoint screen, const CameraViewport& viewport) const {
    const CameraPoint center = ViewportCenter(viewport);
    const double normalizedX = (screen.x - center.x - offsetX) / (kTileHalfWidth * zoom);
    const double normalizedY = (screen.y - center.y - offsetY) / (kTileHalfHeight * zoom);
    return {
        centerTileX + static_cast<int>(std::lround((normalizedX + normalizedY - 2.0) / 2.0)),
        centerTileY + static_cast<int>(std::lround((normalizedY - normalizedX) / 2.0)),
    };
}

CameraPoint EditorCamera::ScreenToWorld(CameraPoint screen, const CameraViewport& viewport) const {
    const CameraPoint center = ViewportCenter(viewport);
    return { (screen.x - center.x - offsetX) / zoom, (screen.y - center.y - offsetY) / zoom };
}

CameraPoint EditorCamera::WorldToScreen(CameraPoint world, const CameraViewport& viewport) const {
    const CameraPoint center = ViewportCenter(viewport);
    return { center.x + offsetX + world.x * zoom, center.y + offsetY + world.y * zoom };
}

CameraBounds EditorCamera::ProjectedScreenBounds(int mapWidth, int mapHeight,
    const CameraViewport& viewport, bool includeOffset) const {
    if (mapWidth <= 0 || mapHeight <= 0) return {};
    const std::array<CameraTile, 4> corners{{
        { 0, 0 }, { mapWidth - 1, 0 }, { 0, mapHeight - 1 }, { mapWidth - 1, mapHeight - 1 },
    }};
    CameraBounds bounds{ 1e30, 1e30, -1e30, -1e30 };
    for (const CameraTile& tile : corners) {
        CameraPoint point = MapToScreenCenter(tile.x, tile.y, viewport);
        if (!includeOffset) { point.x -= offsetX; point.y -= offsetY; }
        bounds.left = std::min(bounds.left, point.x - kTileHalfWidth * zoom);
        bounds.right = std::max(bounds.right, point.x + kTileHalfWidth * zoom);
        bounds.top = std::min(bounds.top, point.y - kTileHalfHeight * zoom);
        bounds.bottom = std::max(bounds.bottom, point.y + kTileHalfHeight * zoom);
    }
    return bounds;
}

void EditorCamera::Pan(double deltaX, double deltaY) {
    offsetX += deltaX;
    offsetY += deltaY;
}

void EditorCamera::ZoomAtPoint(double newZoom, CameraPoint anchor, const CameraViewport& viewport) {
    newZoom = std::max(0.01, newZoom);
    const CameraPoint world = ScreenToWorld(anchor, viewport);
    zoom = newZoom;
    const CameraPoint projected = WorldToScreen(world, viewport);
    offsetX += anchor.x - projected.x;
    offsetY += anchor.y - projected.y;
}
