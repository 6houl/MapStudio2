#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace save_workflow {

constexpr std::size_t kMaxCommentBytes = 4096;

enum class SessionMode { Disconnected, Hosting, Connected };
enum class FailurePoint { None, Directory, BackupWrite, MapWrite, Replace, MetadataWrite };

struct FailureInjection {
    FailurePoint point = FailurePoint::None;
};

struct Metadata {
    int version = 1;
    std::string map;
    std::string timestamp;
    std::string comment;
    std::uintmax_t fileSize = 0;
    bool collaborative = false;
    std::string author;
};

struct SaveResult {
    bool mapSaved = false;
    bool metadataSaved = false;
    std::filesystem::path backupPath;
    std::filesystem::path metadataPath;
    std::string timestamp;
    std::uintmax_t fileSize = 0;
    std::string error;
    std::string metadataError;
};

struct ResultPresentation {
    std::string filename;
    std::string size;
    std::string date;
    std::string comment;
};

bool CanSave(SessionMode mode);
std::string SaveDeniedMessage();
bool ShouldBroadcastSave(SessionMode mode, bool mapSaved);
bool IsValidUtf8(std::string_view value);
bool ValidateComment(std::string_view value, std::string& error);
std::string SanitizeFilename(std::string_view value);
std::string UtcTimestamp(std::chrono::system_clock::time_point when = std::chrono::system_clock::now());
std::filesystem::path BackupsDirectory(const std::filesystem::path& mapPath);
std::filesystem::path MetadataDirectory(const std::filesystem::path& mapPath);
std::filesystem::path MetadataPath(const std::filesystem::path& mapPath);

bool CreateSessionBackup(
    const std::filesystem::path& mapPath,
    std::span<const std::uint8_t> inMemoryEmf,
    std::filesystem::path& backupPath,
    std::string& error,
    FailureInjection injection = {},
    bool useExistingSource = true);

SaveResult SaveMapFile(
    const std::filesystem::path& mapPath,
    std::span<const std::uint8_t> emf,
    Metadata metadata,
    FailureInjection injection = {},
    const std::filesystem::path& recoverySource = {});

bool ReadMetadata(const std::filesystem::path& path, Metadata& metadata, std::string& error);
ResultPresentation BuildResultPresentation(const std::filesystem::path& path, const Metadata& metadata);

} // namespace save_workflow
