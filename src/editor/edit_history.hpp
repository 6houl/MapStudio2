#pragma once

#include "model/map_document.hpp"
#include <optional>
#include <vector>

// Editor undo/redo history management
// Stores map snapshots for undo/redo operations

class EditHistory {
  public:
    // History state queries
    bool CanUndo() const;
    bool CanRedo() const;

    // Edit tracking for continuous operations (e.g., brush strokes)
    void BeginStroke(const MapDocument& map);
    void CommitStroke();
    bool HasPendingStroke() const;

    // History operations
    void Push(const MapDocument& map);
    MapDocument Undo(const MapDocument& current);
    MapDocument Redo(const MapDocument& current);

    // History management
    void Clear();

  private:
    static constexpr std::size_t kMaxUndoSteps = 64;

    std::vector<MapDocument> undoStack_;
    std::vector<MapDocument> redoStack_;
    std::optional<MapDocument> strokeBefore_;
};
