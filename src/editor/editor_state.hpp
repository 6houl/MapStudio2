#pragma once

#include <array>

enum class DrawTool { Pencil, Brush, Eraser, Wipe };
enum class EditDomain { Graphics, Flags };

struct EditorState {
    int graphicsCategory = 0;
    std::array<int, 6> selectedGraphics{ 1, 1, 1, 1, 1, 1 };
    EditDomain editDomain = EditDomain::Graphics;
    DrawTool drawTool = DrawTool::Pencil;
    int brushSize = 1;
    int flagType = 0;
    int currentLayer = 0;

    int& selectedGraphic();
    const int& selectedGraphic() const;
    void selectGraphicsCategory(int category, int layer);
    void selectLayer(int layer, int category);
    void selectDrawTool(DrawTool tool);
    void selectBrushSize(int size);
    void selectEditDomain(EditDomain domain);
    void selectFlagType(int type);
};
