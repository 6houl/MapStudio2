#pragma once

#include "collaboration_protocol.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace collaboration {

enum class SessionState { Disconnected, Hosting, Connecting, Connected };
enum class EventType {
    Status,
    ParticipantConnected,
    ParticipantDisconnected,
    SnapshotReceived,
    Rejected,
    ConnectionLost,
    ChatMessage,
    ParticipantsChanged,
    OperationRequested,
    OperationAccepted,
    OperationRejected,
    PresenceChanged,
    MapSaved
};

struct Event {
    EventType type = EventType::Status;
    std::string message;
    std::string displayName;
    std::uint64_t sequence = 0;
    std::uint32_t senderId = 0;
    std::vector<Participant> participants;
    OperationBroadcast operation;
    Presence presence;
    SaveEvent saveEvent;
    Snapshot snapshot;
};

struct HostOptions {
    std::string sessionName;
    std::string displayName;
    std::string password;
    std::uint16_t port = kDefaultPort;
    std::string mapName;
    Snapshot snapshot;
};

struct ConnectOptions {
    std::string address = "127.0.0.1";
    std::string displayName;
    std::string password;
    std::uint16_t port = kDefaultPort;
    std::uint16_t protocolVersion = kProtocolVersion; // exposed for compatibility tests
};

class Session {
  public:
    explicit Session(std::function<void()> wakeUi = {});
    ~Session();
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    bool StartHosting(const HostOptions& options, std::string& error);
    bool Connect(const ConnectOptions& options, std::string& error);
    void Disconnect(std::string reason = "Disconnected");
    bool SendChat(std::string_view text, std::string& error);
    bool SubmitOperation(std::span<const std::uint8_t> operation, std::string& error);
    bool AcceptRequestedOperation(std::uint32_t authorId, std::span<const std::uint8_t> operation, std::string& error);
    bool RejectRequestedOperation(std::uint32_t authorId, std::uint64_t requestId, std::string_view reason,
                                  std::string& error);
    bool SendPresence(Presence presence, std::string& error);
    bool BroadcastSave(std::uint64_t unixTimestamp, std::string& error);
    bool ChangeParticipantRole(std::uint32_t userId, ParticipantRole role, std::string& error);
    bool DisconnectParticipant(std::uint32_t userId, std::string& error);
    bool SetPassword(std::string password, std::string& error);

    SessionState State() const;
    std::size_t ConnectedGuestCount() const;
    std::string StatusText() const;
    std::string HostAddress() const;
    std::uint16_t Port() const;
    std::vector<Participant> Participants() const;
    std::uint64_t Revision() const;
    std::uint32_t LocalUserId() const;
    ParticipantRole LocalRole() const;
    bool PasswordEnabled() const;
    std::uint64_t PresencePacketsSent() const;
    std::uint64_t PresencePacketsReceived() const;
    std::vector<Event> DrainEvents();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace collaboration
