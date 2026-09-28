#pragma once

#include "collaboration_session.hpp"

#include <Windows.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace collaboration {

struct WindowCallbacks {
    std::function<void()> startServer;
    std::function<void()> connect;
    std::function<void()> disconnect;
    std::function<bool(std::string_view, std::string&)> sendChat;
    std::function<bool(std::uint32_t, ParticipantRole, std::string&)> changeRole;
    std::function<bool(std::uint32_t, std::string&)> disconnectParticipant;
    std::function<bool(std::string, std::string&)> setPassword;
};

struct WindowState {
    SessionState state = SessionState::Disconnected;
    std::string status;
    std::string hostAddress;
    std::uint16_t port = 0;
    std::vector<Participant> participants;
    ParticipantRole localRole = ParticipantRole::Viewer;
    bool passwordEnabled = false;
};

class Window {
  public:
    Window(HWND owner, HFONT font, WindowCallbacks callbacks);
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void Show();
    void Update(const WindowState& state);
    void AppendSystem(std::string text);
    void AppendChat(std::string sender, std::string text);
    HWND Handle() const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace collaboration
