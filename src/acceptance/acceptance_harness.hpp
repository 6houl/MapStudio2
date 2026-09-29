#pragma once

#include "../collaboration/collaboration_session.hpp"
#include "../model/map_document.hpp"
#include "../persistence/save_workflow.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

// In-application acceptance test harness
//
// This module provides state management and infrastructure for automated acceptance tests.
// The step logic remains in main.cpp for simplicity, but configuration, artifact I/O,
// and state consolidation are handled here.
//
// The harness is activated by command-line arguments.

namespace acceptance_harness {

enum class Role { None, Host, Client };

enum class DialogAction { Manual, Save, Cancel };

// Consolidated acceptance test state
struct AcceptanceState {
    // Configuration (from command line)
    Role role = Role::None;
    bool b3 = false;
    bool cd = false;
    bool final = false;
    std::filesystem::path directory;

    // Step execution state
    int step = 0;
    std::uint64_t brushStart = 0;
    std::size_t submitted = 0;
    std::size_t pendingHigh = 0;
    std::uint64_t lastRevision = 0;
    int presenceIndex = -1;

    // Dialog automation state
    DialogAction dialogAction = DialogAction::Manual;
    std::string dialogComment;
    std::string dialogToken;
    bool modal = false;
    std::optional<std::filesystem::path> saveAsPath;
    save_workflow::FailureInjection saveFailure;
};

// Configure acceptance mode from command line
// Returns true if acceptance mode was activated
bool ConfigureFromCommandLine(AcceptanceState& state);

// Check if acceptance mode is active
inline bool IsActive(const AcceptanceState& state) { return state.role != Role::None; }

// Artifact I/O
void WriteArtifact(const AcceptanceState& state, const char* name, const std::string& value);
void AppendLog(const AcceptanceState& state, const std::string& value);

// Generate EMF fingerprint for verification
std::string GenerateFingerprint(const MapDocument& map);

// Export state snapshots for verification
void ExportState(const AcceptanceState& state, const collaboration::Session& collaboration,
                 const MapDocument& map, const char* name);
void ExportFinalState(const AcceptanceState& state, const collaboration::Session& collaboration,
                      const MapDocument& map, const char* name);

// Test script helpers
std::string SemanticFingerprint(const std::filesystem::path& path);
std::string FileText(const std::filesystem::path& path);
std::size_t BackupCount(const std::filesystem::path& mapPath);

} // namespace acceptance_harness
