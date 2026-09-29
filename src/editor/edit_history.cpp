#include "edit_history.hpp"
#include <algorithm>
#include <utility>

bool EditHistory::CanUndo() const {
    return !undoStack_.empty();
}

bool EditHistory::CanRedo() const {
    return !redoStack_.empty();
}

void EditHistory::BeginStroke(const MapDocument& map) {
    if (!strokeBefore_) {
        strokeBefore_ = map;
    }
}

void EditHistory::CommitStroke() {
    if (!strokeBefore_) {
        return;
    }
    undoStack_.push_back(std::move(*strokeBefore_));
    strokeBefore_.reset();
    if (undoStack_.size() > kMaxUndoSteps) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

bool EditHistory::HasPendingStroke() const {
    return strokeBefore_.has_value();
}

void EditHistory::Push(const MapDocument& map) {
    CommitStroke();
    undoStack_.push_back(map);
    if (undoStack_.size() > kMaxUndoSteps) {
        undoStack_.erase(undoStack_.begin());
    }
    redoStack_.clear();
}

MapDocument EditHistory::Undo(const MapDocument& current) {
    CommitStroke();
    if (undoStack_.empty()) {
        return current;
    }
    redoStack_.push_back(current);
    MapDocument result = std::move(undoStack_.back());
    undoStack_.pop_back();
    return result;
}

MapDocument EditHistory::Redo(const MapDocument& current) {
    CommitStroke();
    if (redoStack_.empty()) {
        return current;
    }
    undoStack_.push_back(current);
    MapDocument result = std::move(redoStack_.back());
    redoStack_.pop_back();
    return result;
}

void EditHistory::Clear() {
    undoStack_.clear();
    redoStack_.clear();
    strokeBefore_.reset();
}
