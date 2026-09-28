#include "editor_state.hpp"

#include <algorithm>

int& EditorState::selectedGraphic() {
    return selectedGraphics[static_cast<std::size_t>(std::clamp(graphicsCategory, 0, 5))];
}

const int& EditorState::selectedGraphic() const {
    return selectedGraphics[static_cast<std::size_t>(std::clamp(graphicsCategory, 0, 5))];
}

void EditorState::selectGraphicsCategory(int category, int layer) {
    graphicsCategory = std::clamp(category, 0, 5);
    currentLayer = std::clamp(layer, 0, 8);
}

void EditorState::selectLayer(int layer, int category) {
    currentLayer = std::clamp(layer, 0, 8);
    graphicsCategory = std::clamp(category, 0, 5);
}

void EditorState::selectDrawTool(DrawTool tool) { drawTool = tool; }
void EditorState::selectBrushSize(int size) { brushSize = size; }
void EditorState::selectEditDomain(EditDomain domain) { editDomain = domain; }
void EditorState::selectFlagType(int type) { flagType = std::clamp(type, 0, 5); }
