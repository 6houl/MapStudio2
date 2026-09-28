#include "save_workflow.hpp"

#include <windows.h>

#include <atomic>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace save_workflow {
namespace {

std::atomic<std::uint64_t> g_temporaryCounter{0};

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Unable to read the existing map for backup.";
        return {};
    }
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) {
        error = "Unable to finish reading the existing map for backup.";
        return {};
    }
    return bytes;
}

std::filesystem::path TemporarySibling(const std::filesystem::path& destination) {
    std::wostringstream suffix;
    suffix << L".tmp." << GetCurrentProcessId() << L'.' << ++g_temporaryCounter;
    return destination.parent_path() / (destination.filename().wstring() + suffix.str());
}

bool EnsureDirectory(const std::filesystem::path& directory, std::string& error, FailureInjection injection) {
    if (injection.point == FailurePoint::Directory) {
        error = "Unable to create the save directory.";
        return false;
    }
    std::error_code code;
    std::filesystem::create_directories(directory, code);
    if (code || !std::filesystem::is_directory(directory)) {
        error = "Unable to create the save directory.";
        return false;
    }
    return true;
}

bool AtomicWrite(
    const std::filesystem::path& destination,
    std::span<const std::uint8_t> bytes,
    FailurePoint writePoint,
    std::string& error,
    FailureInjection injection) {
    if (!EnsureDirectory(destination.parent_path(), error, injection)) return false;
    const std::filesystem::path temporary = TemporarySibling(destination);
    const auto cleanTemporary = [&] {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
    };
    if (injection.point == writePoint) {
        error = writePoint == FailurePoint::BackupWrite ? "Unable to write the required backup." :
            writePoint == FailurePoint::MetadataWrite ? "Unable to write save metadata." : "Unable to write the map file.";
        cleanTemporary();
        return false;
    }
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output || (!bytes.empty() && !output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))) {
            error = "Unable to write the temporary save file.";
            cleanTemporary();
            return false;
        }
        output.flush();
        if (!output) {
            error = "Unable to flush the temporary save file.";
            cleanTemporary();
            return false;
        }
        output.close();
        if (!output) {
            error = "Unable to close the temporary save file.";
            cleanTemporary();
            return false;
        }
    }
    if (injection.point == FailurePoint::Replace) {
        error = "Unable to replace the destination map file.";
        cleanTemporary();
        return false;
    }
    const std::wstring source = temporary.wstring();
    const std::wstring target = destination.wstring();
    const bool existed = std::filesystem::exists(destination);
    BOOL replaced = FALSE;
    if (existed) {
        replaced = ReplaceFileW(target.c_str(), source.c_str(), nullptr, REPLACEFILE_WRITE_THROUGH, nullptr, nullptr);
        if (!replaced) replaced = MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    } else {
        replaced = MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH);
    }
    if (!replaced) {
        error = "Unable to atomically replace the destination file.";
        cleanTemporary();
        return false;
    }
    return true;
}

std::string JsonEscape(std::string_view value) {
    std::ostringstream result;
    for (const unsigned char character : value) {
        switch (character) {
        case '\"': result << "\\\""; break;
        case '\\': result << "\\\\"; break;
        case '\b': result << "\\b"; break;
        case '\f': result << "\\f"; break;
        case '\n': result << "\\n"; break;
        case '\r': result << "\\r"; break;
        case '\t': result << "\\t"; break;
        default:
            if (character < 0x20) result << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(character) << std::dec;
            else result << static_cast<char>(character);
        }
    }
    return result.str();
}

std::string MetadataJson(const Metadata& value) {
    std::ostringstream json;
    json << "{\n"
         << "  \"version\": " << value.version << ",\n"
         << "  \"map\": \"" << JsonEscape(value.map) << "\",\n"
         << "  \"lastSave\": {\n"
         << "    \"timestamp\": \"" << JsonEscape(value.timestamp) << "\",\n"
         << "    \"comment\": \"" << JsonEscape(value.comment) << "\",\n"
         << "    \"fileSize\": " << value.fileSize << ",\n"
         << "    \"collaborative\": " << (value.collaborative ? "true" : "false") << ",\n"
         << "    \"author\": \"" << JsonEscape(value.author) << "\"\n"
         << "  }\n}\n";
    return json.str();
}

std::filesystem::path UniqueBackupPath(const std::filesystem::path& mapPath, std::string_view purpose) {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    std::ostringstream stamp;
    stamp << std::setfill('0') << now.wYear << std::setw(2) << now.wMonth << std::setw(2) << now.wDay
          << '-' << std::setw(2) << now.wHour << std::setw(2) << now.wMinute << std::setw(2) << now.wSecond
          << '-' << std::setw(3) << now.wMilliseconds;
    const auto stemUtf8 = mapPath.stem().u8string();
    const std::string rawStem = stemUtf8.empty() ? "Untitled" :
        std::string(reinterpret_cast<const char*>(stemUtf8.data()), stemUtf8.size());
    const std::string stem = SanitizeFilename(rawStem);
    const std::string base = stem + "-" + SanitizeFilename(purpose) + "-" + stamp.str();
    const auto pathFromUtf8 = [](const std::string& value) {
        return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()),
            reinterpret_cast<const char8_t*>(value.data() + value.size())));
    };
    for (unsigned suffix = 0; suffix < 10000; ++suffix) {
        const std::string name = base + (suffix ? "-" + std::to_string(suffix) : "") + ".emf";
        const auto candidate = BackupsDirectory(mapPath) / pathFromUtf8(name);
        if (!std::filesystem::exists(candidate)) return candidate;
    }
    return BackupsDirectory(mapPath) / pathFromUtf8(base + "-overflow.emf");
}

bool CreateBackup(
    const std::filesystem::path& mapPath,
    std::span<const std::uint8_t> bytes,
    std::string_view purpose,
    std::filesystem::path& backupPath,
    std::string& error,
    FailureInjection injection) {
    backupPath = UniqueBackupPath(mapPath, purpose);
    if (!AtomicWrite(backupPath, bytes, FailurePoint::BackupWrite, error, injection)) {
        backupPath.clear();
        return false;
    }
    return true;
}

bool ExtractString(const std::string& json, std::string_view key, std::string& output) {
    const std::string needle = "\"" + std::string(key) + "\"";
    std::size_t position = json.find(needle);
    if (position == std::string::npos || (position = json.find(':', position + needle.size())) == std::string::npos ||
        (position = json.find('\"', position + 1)) == std::string::npos) return false;
    ++position;
    output.clear();
    while (position < json.size()) {
        const char character = json[position++];
        if (character == '\"') return true;
        if (character != '\\') { output.push_back(character); continue; }
        if (position >= json.size()) return false;
        const char escaped = json[position++];
        switch (escaped) {
        case '\"': output.push_back('\"'); break;
        case '\\': output.push_back('\\'); break;
        case 'b': output.push_back('\b'); break;
        case 'f': output.push_back('\f'); break;
        case 'n': output.push_back('\n'); break;
        case 'r': output.push_back('\r'); break;
        case 't': output.push_back('\t'); break;
        default: return false;
        }
    }
    return false;
}

bool ExtractUnsigned(const std::string& json, std::string_view key, std::uintmax_t& output) {
    const std::string needle = "\"" + std::string(key) + "\"";
    std::size_t position = json.find(needle);
    if (position == std::string::npos || (position = json.find(':', position + needle.size())) == std::string::npos) return false;
    while (++position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) {}
    const std::size_t begin = position;
    while (position < json.size() && std::isdigit(static_cast<unsigned char>(json[position]))) ++position;
    if (position == begin) return false;
    try { output = std::stoull(json.substr(begin, position - begin)); return true; }
    catch (...) { return false; }
}

bool ExtractBool(const std::string& json, std::string_view key, bool& output) {
    const std::string needle = "\"" + std::string(key) + "\"";
    std::size_t position = json.find(needle);
    if (position == std::string::npos || (position = json.find(':', position + needle.size())) == std::string::npos) return false;
    while (++position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) {}
    if (json.compare(position, 4, "true") == 0) { output = true; return true; }
    if (json.compare(position, 5, "false") == 0) { output = false; return true; }
    return false;
}

} // namespace

bool CanSave(SessionMode mode) { return mode != SessionMode::Connected; }
std::string SaveDeniedMessage() { return "Only the host can save during a Map Together session."; }
bool ShouldBroadcastSave(SessionMode mode, bool mapSaved) { return mode == SessionMode::Hosting && mapSaved; }

bool IsValidUtf8(std::string_view value) {
    std::size_t index = 0;
    while (index < value.size()) {
        const unsigned char first = static_cast<unsigned char>(value[index++]);
        if (first < 0x80) continue;
        int remaining = 0;
        std::uint32_t codepoint = 0;
        if ((first & 0xe0) == 0xc0) { remaining = 1; codepoint = first & 0x1f; if (codepoint < 2) return false; }
        else if ((first & 0xf0) == 0xe0) { remaining = 2; codepoint = first & 0x0f; }
        else if ((first & 0xf8) == 0xf0) { remaining = 3; codepoint = first & 0x07; }
        else return false;
        if (index + static_cast<std::size_t>(remaining) > value.size()) return false;
        for (int i = 0; i < remaining; ++i) {
            const unsigned char next = static_cast<unsigned char>(value[index++]);
            if ((next & 0xc0) != 0x80) return false;
            codepoint = (codepoint << 6) | (next & 0x3f);
        }
        if ((remaining == 2 && codepoint < 0x800) || (remaining == 3 && codepoint < 0x10000) ||
            codepoint > 0x10ffff || (codepoint >= 0xd800 && codepoint <= 0xdfff)) return false;
    }
    return true;
}

bool ValidateComment(std::string_view value, std::string& error) {
    if (value.size() > kMaxCommentBytes) { error = "The save comment is too long (maximum 4096 UTF-8 bytes)."; return false; }
    if (!IsValidUtf8(value)) { error = "The save comment contains invalid Unicode."; return false; }
    error.clear();
    return true;
}

std::string SanitizeFilename(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        if (character < 0x20 || character == '<' || character == '>' || character == ':' || character == '\"' ||
            character == '/' || character == '\\' || character == '|' || character == '?' || character == '*') result.push_back('_');
        else result.push_back(static_cast<char>(character));
    }
    while (!result.empty() && (result.back() == ' ' || result.back() == '.')) result.pop_back();
    return result.empty() ? "Untitled" : result;
}

std::string UtcTimestamp(std::chrono::system_clock::time_point when) {
    const std::time_t raw = std::chrono::system_clock::to_time_t(when);
    std::tm utc{};
    gmtime_s(&utc, &raw);
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

std::filesystem::path BackupsDirectory(const std::filesystem::path& mapPath) { return mapPath.parent_path() / L"backups"; }
std::filesystem::path MetadataDirectory(const std::filesystem::path& mapPath) { return mapPath.parent_path() / L"metadata"; }
std::filesystem::path MetadataPath(const std::filesystem::path& mapPath) { return MetadataDirectory(mapPath) / (mapPath.filename().wstring() + L".json"); }

bool CreateSessionBackup(const std::filesystem::path& mapPath, std::span<const std::uint8_t> inMemoryEmf,
    std::filesystem::path& backupPath, std::string& error, FailureInjection injection, bool useExistingSource) {
    std::vector<std::uint8_t> diskBytes;
    std::span<const std::uint8_t> source = inMemoryEmf;
    if (useExistingSource && std::filesystem::exists(mapPath)) {
        diskBytes = ReadBytes(mapPath, error);
        if (!error.empty()) return false;
        source = diskBytes;
    }
    return CreateBackup(mapPath, source, "session-start", backupPath, error, injection);
}

SaveResult SaveMapFile(const std::filesystem::path& mapPath, std::span<const std::uint8_t> emf,
    Metadata metadata, FailureInjection injection, const std::filesystem::path& recoverySource) {
    SaveResult result;
    result.timestamp = metadata.timestamp.empty() ? UtcTimestamp() : metadata.timestamp;
    metadata.timestamp = result.timestamp;
    if (std::filesystem::exists(mapPath)) {
        std::string readError;
        const std::vector<std::uint8_t> previous = ReadBytes(mapPath, readError);
        if (!readError.empty()) { result.error = readError; return result; }
        if (!CreateBackup(mapPath, previous, "pre-save", result.backupPath, result.error, injection)) return result;
    } else if (!recoverySource.empty() && std::filesystem::exists(recoverySource)) {
        std::string readError;
        const std::vector<std::uint8_t> previous = ReadBytes(recoverySource, readError);
        if (!readError.empty()) { result.error = readError; return result; }
        if (!CreateBackup(recoverySource, previous, "pre-save", result.backupPath, result.error, injection)) return result;
    }
    if (!AtomicWrite(mapPath, emf, FailurePoint::MapWrite, result.error, injection)) return result;
    result.mapSaved = true;
    std::error_code sizeError;
    result.fileSize = std::filesystem::file_size(mapPath, sizeError);
    if (sizeError) result.fileSize = emf.size();
    const auto filenameUtf8 = mapPath.filename().u8string();
    metadata.map.assign(reinterpret_cast<const char*>(filenameUtf8.data()), filenameUtf8.size());
    metadata.fileSize = result.fileSize;
    result.metadataPath = MetadataPath(mapPath);
    const std::string json = MetadataJson(metadata);
    const std::span<const std::uint8_t> jsonBytes(reinterpret_cast<const std::uint8_t*>(json.data()), json.size());
    if (!AtomicWrite(result.metadataPath, jsonBytes, FailurePoint::MetadataWrite, result.metadataError, injection)) return result;
    result.metadataSaved = true;
    return result;
}

bool ReadMetadata(const std::filesystem::path& path, Metadata& metadata, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) { error = "Unable to read save metadata."; return false; }
    const std::string json{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::uintmax_t version = 0;
    if (!ExtractUnsigned(json, "version", version) || version > static_cast<std::uintmax_t>(std::numeric_limits<int>::max()) ||
        !ExtractString(json, "map", metadata.map) || !ExtractString(json, "timestamp", metadata.timestamp) ||
        !ExtractString(json, "comment", metadata.comment) || !ExtractUnsigned(json, "fileSize", metadata.fileSize) ||
        !ExtractBool(json, "collaborative", metadata.collaborative) || !ExtractString(json, "author", metadata.author)) {
        error = "Save metadata is malformed.";
        return false;
    }
    metadata.version = static_cast<int>(version);
    if (!ValidateComment(metadata.comment, error)) return false;
    error.clear();
    return true;
}

ResultPresentation BuildResultPresentation(const std::filesystem::path& path, const Metadata& metadata) {
    ResultPresentation result;
    const auto utf8 = path.filename().u8string();
    result.filename.assign(reinterpret_cast<const char*>(utf8.data()), utf8.size());
    result.size = std::to_string(metadata.fileSize) + " bytes";
    result.date = metadata.timestamp;
    result.comment = metadata.comment;
    return result;
}

} // namespace save_workflow
