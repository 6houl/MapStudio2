#pragma once

struct CameraPoint {
    double x = 0.0;
    double y = 0.0;
};

struct CameraBounds {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;
};

struct CameraViewport {
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;
};

struct CameraTile {
    int x = 0;
    int y = 0;
};

struct CameraNavigationKeys {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool any() const {
        return left || right || up || down;
    }
    void clear() {
        left = right = up = down = false;
    }
};

class EditorCamera {
  public:
    int centerTileX = 8;
    int centerTileY = 14;
    double offsetX = 0.0;
    double offsetY = 0.0;
    double zoom = 1.0;

    CameraPoint MapToScreenCenter(int tileX, int tileY, const CameraViewport& viewport) const;
    CameraPoint MapToScreenTop(int tileX, int tileY, const CameraViewport& viewport) const;
    CameraTile ScreenToMap(CameraPoint screen, const CameraViewport& viewport) const;
    CameraBounds ProjectedScreenBounds(int mapWidth, int mapHeight, const CameraViewport& viewport,
                                       bool includeOffset = true) const;
    CameraPoint ScreenToWorld(CameraPoint screen, const CameraViewport& viewport) const;
    CameraPoint WorldToScreen(CameraPoint world, const CameraViewport& viewport) const;

    void Pan(double deltaX, double deltaY);
    void ZoomAtPoint(double newZoom, CameraPoint anchor, const CameraViewport& viewport);
};

bool ApplyCameraNavigation(EditorCamera& camera, const CameraNavigationKeys& keys, double elapsedSeconds,
                           double pixelsPerSecond);
