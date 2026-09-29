#include "acceptance_harness.hpp"
#include "../emf/emf_io.hpp"
#include "../persistence/save_workflow.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <windows.h>
#include <shellapi.h>

namespace acceptance_harness {

bool ConfigureFromCommandLine(AcceptanceState& state) {
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!args)
        return false;

    bool configured = false;
    for (int i = 1; i + 1 < count; ++i) {
        const std::wstring option = args[i];
        if (option == L"--b2-accept-host" || option == L"--b2-accept-client" || option == L"--b3-accept-host" ||
            option == L"--b3-accept-client" || option == L"--cd-accept-host" || option == L"--cd-accept-client" ||
            option == L"--final-accept-host" || option == L"--final-accept-client") {
            state.b3 = option.find(L"--b3-") == 0;
            state.cd = option.find(L"--cd-") == 0;
            state.final = option.find(L"--final-") == 0;
            state.role = option.find(L"host") != std::wstring::npos ? Role::Host : Role::Client;
            state.directory = args[i + 1];
            configured = true;
            break;
        }
    }

    LocalFree(args);
    return configured;
}

void WriteArtifact(const AcceptanceState& state, const char* name, const std::string& value) {
    std::filesystem::create_directories(state.directory);
    std::ofstream output(state.directory / name, std::ios::binary | std::ios::trunc);
    output << value;
}

void AppendLog(const AcceptanceState& state, const std::string& value) {
    std::filesystem::create_directories(state.directory);
    std::ofstream output(state.directory / "B2-acceptance.log", std::ios::app);
    output << value << '\n';
}

std::string GenerateFingerprint(const MapDocument& map) {
    const auto bytes = emf::WriteEmf(map);
    std::ostringstream value;
    for (std::size_t i = 0; i < bytes.size(); ++i)
        value << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    return value.str();
}

void ExportState(const AcceptanceState& state, const collaboration::Session& collaboration,
                 const MapDocument& map, const char* name) {
    std::ostringstream json;
    json << "{\n  \"role\": \"" << (state.role == Role::Host ? "host" : "client")
         << "\",\n  \"step\": " << state.step << ",\n  \"revision\": " << collaboration.Revision()
         << ",\n  \"participants\": " << collaboration.Participants().size()
         << ",\n  \"guests\": " << collaboration.ConnectedGuestCount()
         << ",\n  \"state\": " << static_cast<int>(collaboration.State())
         << ",\n  \"fingerprint\": \"" << GenerateFingerprint(map) << "\",\n  \"pending_high\": " << state.pendingHigh
         << "\n}\n";
    WriteArtifact(state, name, json.str());
}

void ExportFinalState(const AcceptanceState& state, const collaboration::Session& collaboration,
                      const MapDocument& map, const char* name) {
    std::ostringstream json;
    json << "{\n  \"role\": \"" << (state.role == Role::Host ? "host" : "client")
         << "\",\n  \"step\": " << state.step << ",\n  \"revision\": " << collaboration.Revision()
         << ",\n  \"fingerprint\": \"" << GenerateFingerprint(map) << "\"\n}\n";
    WriteArtifact(state, name, json.str());
}

std::string SemanticFingerprint(const std::filesystem::path& path) {
    // Read file directly without ReadFile helper to avoid circular dependency
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return "";
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    const MapDocument map = emf::ReadEmfBytes(bytes, path.string());
    const auto emfBytes = emf::WriteEmf(map);
    std::ostringstream value;
    for (std::size_t i = 0; i < emfBytes.size(); ++i)
        value << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(emfBytes[i]);
    return value.str();
}

std::string FileText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return "";
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return {bytes.begin(), bytes.end()};
}

std::size_t BackupCount(const std::filesystem::path& mapPath) {
    const auto directory = save_workflow::BackupsDirectory(mapPath);
    if (!std::filesystem::is_directory(directory))
        return 0;
    std::size_t count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file())
            ++count;
    }
    return count;
}

} // namespace acceptance_harness
