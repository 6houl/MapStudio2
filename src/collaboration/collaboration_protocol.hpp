#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace collaboration {

inline constexpr std::uint32_t kProtocolMagic = 0x31544d45; // EMT1
inline constexpr std::uint16_t kProtocolVersion = 5;
inline constexpr std::size_t kMaxFrameBytes = 16 * 1024 * 1024;
inline constexpr std::size_t kMaxSnapshotBytes = 15 * 1024 * 1024;
inline constexpr std::size_t kMaxDisplayNameBytes = 48;
inline constexpr std::size_t kMaxSessionNameBytes = 80;
inline constexpr std::size_t kMaxMapNameBytes = 96;
inline constexpr std::size_t kMaxChatMessageBytes = 512;
inline constexpr std::size_t kMaxOperationBytes = 2 * 1024 * 1024;
inline constexpr std::size_t kMaxOperationBatchEntries = 65536;
inline constexpr std::size_t kPasswordProofBytes = 32;
inline constexpr std::size_t kChallengeBytes = 32;
inline constexpr std::size_t kMaxParticipants = 8;
inline constexpr std::uint16_t kDefaultPort = 8079;

enum class MessageType : std::uint16_t {
    Hello = 1,
    Challenge = 2,
    Authenticate = 3,
    Accepted = 4,
    Rejected = 5,
    Snapshot = 6,
    Disconnect = 7,
    ChatRequest = 8,
    ChatBroadcast = 9,
    ParticipantList = 10,
    OperationRequest = 11,
    OperationBroadcast = 12,
    Presence = 13,
    SaveEvent = 14,
    OperationRejected = 15,
    ParticipantDisconnect = 16,
};

enum class RejectReason : std::uint8_t {
    InvalidMessage = 1,
    IncompatibleProtocol = 2,
    WrongPassword = 3,
    SessionFull = 4,
    ServerStopping = 5,
};

struct Frame {
    MessageType type{};
    std::vector<std::uint8_t> payload;
};

struct Hello {
    std::uint16_t protocolVersion = kProtocolVersion;
    std::string applicationVersion;
    std::string displayName;
};

struct Challenge {
    std::array<std::uint8_t, kChallengeBytes> nonce{};
    bool passwordRequired = false;
    std::string sessionName;
    std::string mapName;
    std::uint64_t revision = 0;
};

struct Authentication {
    std::array<std::uint8_t, kPasswordProofBytes> proof{};
};

struct Accepted {
    std::uint32_t userId = 0;
    std::string hostDisplayName;
};

struct Rejected {
    RejectReason reason = RejectReason::InvalidMessage;
    std::string message;
};

struct Snapshot {
    std::uint64_t revision = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::vector<std::uint8_t> emfBytes;
};

struct ChatBroadcast {
    std::uint64_t sequence = 0;
    std::uint32_t senderId = 0;
    std::string text;
};

enum class ParticipantRole : std::uint8_t { Host=1, Editor=2, Viewer=3 };
struct Participant {
    std::uint32_t userId = 0;
    std::string displayName;
    bool host = false;
    ParticipantRole role = ParticipantRole::Editor;
    std::uint8_t color = 0;
    bool operator==(const Participant&) const = default;
};

struct OperationBroadcast {
    std::uint64_t revision = 0;
    std::uint64_t operationId = 0;
    std::uint32_t authorId = 0;
    std::vector<std::uint8_t> operation;
};
struct OperationRejection { std::uint64_t requestId=0; std::string message; };

enum class PresenceArea : std::uint8_t { None, Viewer, Graphics, Flags, Toolset, Layers, MapProperties, Entities, MapTogether };
struct Presence {
    std::uint32_t userId = 0;
    PresenceArea area = PresenceArea::Viewer;
    bool viewerActive = false;
    std::uint16_t x = 0;
    std::uint16_t y = 0;
};

struct SaveEvent {
    std::uint32_t authorId = 0;
    std::uint64_t revision = 0;
    std::uint64_t unixTimestamp = 0;
};

std::vector<std::uint8_t> EncodeFrame(const Frame& frame);
bool DecodeFrame(std::span<const std::uint8_t> bytes, Frame& frame, std::string& error);

Frame EncodeHello(const Hello& value);
Frame EncodeChallenge(const Challenge& value);
Frame EncodeAuthentication(const Authentication& value);
Frame EncodeAccepted(const Accepted& value);
Frame EncodeRejected(const Rejected& value);
Frame EncodeSnapshot(const Snapshot& value);
Frame EncodeDisconnect(std::string_view reason);
Frame EncodeChatRequest(std::string_view text);
Frame EncodeChatBroadcast(const ChatBroadcast& value);
Frame EncodeParticipantList(std::span<const Participant> participants);
Frame EncodeOperationRequest(std::span<const std::uint8_t> operation);
Frame EncodeOperationBroadcast(const OperationBroadcast& value);
Frame EncodeOperationRejection(const OperationRejection& value);
Frame EncodePresence(const Presence& value);
Frame EncodeSaveEvent(const SaveEvent& value);

bool DecodeHello(const Frame& frame, Hello& value, std::string& error);
bool DecodeChallenge(const Frame& frame, Challenge& value, std::string& error);
bool DecodeAuthentication(const Frame& frame, Authentication& value, std::string& error);
bool DecodeAccepted(const Frame& frame, Accepted& value, std::string& error);
bool DecodeRejected(const Frame& frame, Rejected& value, std::string& error);
bool DecodeSnapshot(const Frame& frame, Snapshot& value, std::string& error);
bool DecodeDisconnect(const Frame& frame, std::string& reason, std::string& error);
bool DecodeChatRequest(const Frame& frame, std::string& text, std::string& error);
bool DecodeChatBroadcast(const Frame& frame, ChatBroadcast& value, std::string& error);
bool DecodeParticipantList(const Frame& frame, std::vector<Participant>& participants, std::string& error);
bool DecodeOperationRequest(const Frame& frame, std::vector<std::uint8_t>& operation, std::string& error);
bool DecodeOperationBroadcast(const Frame& frame, OperationBroadcast& value, std::string& error);
bool DecodeOperationRejection(const Frame& frame, OperationRejection& value, std::string& error);
bool DecodePresence(const Frame& frame, Presence& value, std::string& error);
bool DecodeSaveEvent(const Frame& frame, SaveEvent& value, std::string& error);

bool IsValidUtf8(std::string_view text);
bool NormalizeChatText(std::string_view input, std::string& normalized, std::string& error);
bool NormalizeDisplayName(std::string_view input, std::string& normalized, std::string& error);

std::array<std::uint8_t, kChallengeBytes> CreateChallengeNonce();
std::array<std::uint8_t, kPasswordProofBytes> MakePasswordProof(
    std::string_view password, std::span<const std::uint8_t, kChallengeBytes> nonce);
bool ConstantTimeEqual(std::span<const std::uint8_t> left, std::span<const std::uint8_t> right);

} // namespace collaboration
