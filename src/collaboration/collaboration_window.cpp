#include "collaboration_window.hpp"

#include <CommCtrl.h>

#include <algorithm>
#include <deque>

namespace collaboration {
namespace {
constexpr wchar_t kWindowClass[] = L"EndlessMapStudioMapTogether";
constexpr UINT kSendFromEntry = WM_APP + 1;
constexpr int kTab = 6000, kTranscript = 6001, kEntry = 6002, kSend = 6003, kStatus = 6010, kAddress = 6011,
              kCount = 6012, kParticipants = 6013, kStart = 6014, kConnect = 6015, kDisconnect = 6016, kRole = 6017,
              kApplyRole = 6018, kKick = 6019, kPasswordStatus = 6020, kPasswordEdit = 6021, kSetPassword = 6022,
              kRemovePassword = 6023;
constexpr std::size_t kMaxTranscriptMessages = 500;

std::wstring Wide(std::string_view text) {
    if (text.empty())
        return {};
    const int count =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0)
        return L"[invalid text]";
    std::wstring result(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(),
                        count);
    return result;
}
std::string Utf8(std::wstring_view text) {
    if (text.empty())
        return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                          nullptr, 0, nullptr, nullptr);
    if (count <= 0)
        return {};
    std::string result(count, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), count,
                        nullptr, nullptr);
    return result;
}
void Font(HWND control, HFONT font) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}
HMENU Id(int value) {
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(value));
}
} // namespace

class Window::Impl {
  public:
    Impl(HWND parent, HFONT uiFont, WindowCallbacks value) : owner(parent), font(uiFont), callbacks(std::move(value)) {
        WNDCLASSEXW cls{sizeof(cls)};
        cls.lpfnWndProc = WindowProc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
        cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        cls.lpszClassName = kWindowClass;
        RegisterClassExW(&cls);
        window = CreateWindowExW(WS_EX_TOOLWINDOW, kWindowClass, L"Map together - not connected",
                                 WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, CW_USEDEFAULT, CW_USEDEFAULT,
                                 430, 430, owner, nullptr, GetModuleHandleW(nullptr), this);
    }
    ~Impl() {
        if (window)
            DestroyWindow(window);
    }

    void Build() {
        tab = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 8, 8, 398, 300, window,
                              Id(kTab), GetModuleHandleW(nullptr), nullptr);
        Font(tab, font);
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t*>(L"Chat");
        TabCtrl_InsertItem(tab, 0, &item);
        item.pszText = const_cast<wchar_t*>(L"Actions");
        TabCtrl_InsertItem(tab, 1, &item);
        transcript = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
                                     20, 38, 374, 220, window, Id(kTranscript), GetModuleHandleW(nullptr), nullptr);
        entry = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 20,
                                268, 302, 23, window, Id(kEntry), GetModuleHandleW(nullptr), nullptr);
        send = CreateWindowExW(0, L"BUTTON", L"send", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 330, 267, 64,
                               24, window, Id(kSend), GetModuleHandleW(nullptr), nullptr);
        status = CreateWindowExW(0, L"STATIC", L"Status: Not connected", WS_CHILD, 22, 43, 360, 20, window, Id(kStatus),
                                 GetModuleHandleW(nullptr), nullptr);
        address = CreateWindowExW(0, L"STATIC", L"", WS_CHILD, 22, 65, 360, 20, window, Id(kAddress),
                                  GetModuleHandleW(nullptr), nullptr);
        count = CreateWindowExW(0, L"STATIC", L"Participants: 0 / 8", WS_CHILD, 22, 87, 360, 20, window, Id(kCount),
                                GetModuleHandleW(nullptr), nullptr);
        participants =
            CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"", WS_CHILD | WS_VSCROLL | LBS_NOINTEGRALHEIGHT, 22, 110,
                            360, 112, window, Id(kParticipants), GetModuleHandleW(nullptr), nullptr);
        start = CreateWindowExW(0, L"BUTTON", L"Start Server...", WS_CHILD | WS_TABSTOP, 22, 237, 105, 25, window,
                                Id(kStart), GetModuleHandleW(nullptr), nullptr);
        connect = CreateWindowExW(0, L"BUTTON", L"Connect...", WS_CHILD | WS_TABSTOP, 134, 237, 88, 25, window,
                                  Id(kConnect), GetModuleHandleW(nullptr), nullptr);
        disconnect = CreateWindowExW(0, L"BUTTON", L"Disconnect", WS_CHILD | WS_TABSTOP, 277, 237, 105, 25, window,
                                     Id(kDisconnect), GetModuleHandleW(nullptr), nullptr);
        role = CreateWindowExW(0, WC_COMBOBOXW, L"", WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST, 22, 230, 120, 120,
                               window, Id(kRole), GetModuleHandleW(nullptr), nullptr);
        SendMessageW(role, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Editor"));
        SendMessageW(role, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Viewer"));
        SendMessageW(role, CB_SETCURSEL, 0, 0);
        applyRole = CreateWindowExW(0, L"BUTTON", L"Change Role", WS_CHILD | WS_TABSTOP, 148, 230, 95, 24, window,
                                    Id(kApplyRole), GetModuleHandleW(nullptr), nullptr);
        kick = CreateWindowExW(0, L"BUTTON", L"Disconnect User", WS_CHILD | WS_TABSTOP, 249, 230, 133, 24, window,
                               Id(kKick), GetModuleHandleW(nullptr), nullptr);
        passwordStatus = CreateWindowExW(0, L"STATIC", L"Password: None", WS_CHILD, 22, 260, 160, 20, window,
                                         Id(kPasswordStatus), GetModuleHandleW(nullptr), nullptr);
        passwordEdit =
            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_TABSTOP | ES_PASSWORD | ES_AUTOHSCROLL, 22,
                            282, 180, 23, window, Id(kPasswordEdit), GetModuleHandleW(nullptr), nullptr);
        setPassword = CreateWindowExW(0, L"BUTTON", L"Set / Change", WS_CHILD | WS_TABSTOP, 208, 281, 92, 24, window,
                                      Id(kSetPassword), GetModuleHandleW(nullptr), nullptr);
        removePassword = CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | WS_TABSTOP, 306, 281, 76, 24, window,
                                         Id(kRemovePassword), GetModuleHandleW(nullptr), nullptr);
        for (HWND control : {transcript, entry, send, status, address, count, participants, start, connect, disconnect,
                             role, applyRole, kick, passwordStatus, passwordEdit, setPassword, removePassword})
            Font(control, font);
        SetWindowSubclass(entry, EntryProc, 1, reinterpret_cast<DWORD_PTR>(this));
        Layout();
        ShowTab(0);
        ApplyState();
    }
    void Layout() {
        if (!window || !tab)
            return;
        RECT client{};
        GetClientRect(window, &client);
        const int w = client.right, h = client.bottom;
        SetWindowPos(tab, nullptr, 8, 8, w - 16, h - 16, SWP_NOZORDER);
        SetWindowPos(transcript, nullptr, 20, 38, w - 40, h - 104, SWP_NOZORDER);
        SetWindowPos(entry, nullptr, 20, h - 57, w - 110, 23, SWP_NOZORDER);
        SetWindowPos(send, nullptr, w - 82, h - 58, 62, 24, SWP_NOZORDER);
        SetWindowPos(participants, nullptr, 22, 110, w - 44, 105, SWP_NOZORDER);
        const int buttonsY = h - 70;
        SetWindowPos(start, nullptr, 22, buttonsY, 105, 25, SWP_NOZORDER);
        SetWindowPos(connect, nullptr, 134, buttonsY, 88, 25, SWP_NOZORDER);
        SetWindowPos(disconnect, nullptr, w - 127, buttonsY, 105, 25, SWP_NOZORDER);
    }
    void ShowTab(int selected) {
        const bool chat = selected == 0;
        for (HWND control : {transcript, entry, send})
            ShowWindow(control, chat ? SW_SHOW : SW_HIDE);
        for (HWND control : {status, address, count, participants, start, connect, disconnect, role, applyRole, kick,
                             passwordStatus, passwordEdit, setPassword, removePassword})
            ShowWindow(control, chat ? SW_HIDE : SW_SHOW);
    }
    void Show() {
        ShowWindow(window, SW_SHOW);
        SetForegroundWindow(window);
        ApplyState();
    }
    void Update(WindowState value) {
        state = std::move(value);
        ApplyState();
    }
    void ApplyState() {
        if (!window)
            return;
        std::wstring title = L"Map together - ";
        if (state.state == SessionState::Hosting)
            title += L"hosting";
        else if (state.state == SessionState::Connected)
            title += L"connected to " + Wide(state.hostAddress);
        else if (state.state == SessionState::Connecting)
            title += L"connecting";
        else
            title += L"not connected";
        SetWindowTextW(window, title.c_str());
        const bool active = state.state == SessionState::Hosting || state.state == SessionState::Connected;
        const bool hosting = state.state == SessionState::Hosting;
        EnableWindow(entry, active);
        EnableWindow(send, active);
        EnableWindow(start, !active && state.state != SessionState::Connecting);
        EnableWindow(connect, !active && state.state != SessionState::Connecting);
        EnableWindow(disconnect, state.state != SessionState::Disconnected);
        for (HWND control : {role, applyRole, kick, passwordEdit, setPassword, removePassword})
            EnableWindow(control, hosting);
        EnableWindow(removePassword, hosting && state.passwordEnabled);
        SetWindowTextW(status, (L"Status: " + Wide(state.status)).c_str());
        std::wstring where;
        if (state.state == SessionState::Hosting)
            where = L"Port: " + std::to_wstring(state.port);
        else if (state.state == SessionState::Connected || state.state == SessionState::Connecting)
            where = L"Host: " + Wide(state.hostAddress) + L":" + std::to_wstring(state.port);
        SetWindowTextW(address, where.c_str());
        SetWindowTextW(count, (L"Participants: " + std::to_wstring(state.participants.size()) + L" / " +
                               std::to_wstring(kMaxParticipants))
                                  .c_str());
        SetWindowTextW(passwordStatus, state.passwordEnabled ? L"Password: Enabled" : L"Password: None");
        SendMessageW(participants, LB_RESETCONTENT, 0, 0);
        for (const auto& p : state.participants) {
            const wchar_t* roleName = p.role == ParticipantRole::Host     ? L"Host"
                                      : p.role == ParticipantRole::Editor ? L"Editor"
                                                                          : L"Viewer";
            std::wstring label = Wide(p.displayName) + L"        " + roleName;
            const LRESULT index = SendMessageW(participants, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            SendMessageW(participants, LB_SETITEMDATA, index, p.userId);
        }
    }
    void DrawParticipant(const DRAWITEMSTRUCT& draw) {
        if (draw.itemID == static_cast<UINT>(-1))
            return;
        const auto id = static_cast<std::uint32_t>(SendMessageW(participants, LB_GETITEMDATA, draw.itemID, 0));
        auto found = std::find_if(state.participants.begin(), state.participants.end(),
                                  [&](const Participant& p) { return p.userId == id; });
        if (found == state.participants.end())
            return;
        const bool selected = (draw.itemState & ODS_SELECTED) != 0;
        FillRect(draw.hDC, &draw.rcItem, GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
        static constexpr COLORREF colors[] = {RGB(62, 112, 180), RGB(190, 133, 36), RGB(181, 70, 65),
                                              RGB(129, 83, 166), RGB(43, 137, 139), RGB(192, 102, 50),
                                              RGB(92, 125, 62),  RGB(112, 112, 112)};
        HBRUSH marker = CreateSolidBrush(colors[found->color % 8]);
        RECT box{draw.rcItem.left + 4, draw.rcItem.top + 4, draw.rcItem.left + 12, draw.rcItem.bottom - 4};
        FillRect(draw.hDC, &box, marker);
        DeleteObject(marker);
        const wchar_t* roleName = found->role == ParticipantRole::Host     ? L"Host"
                                  : found->role == ParticipantRole::Editor ? L"Editor"
                                                                           : L"Viewer";
        const std::wstring label = Wide(found->displayName) + L"        " + roleName;
        RECT text = draw.rcItem;
        text.left += 18;
        SetBkMode(draw.hDC, TRANSPARENT);
        SetTextColor(draw.hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
        DrawTextW(draw.hDC, label.c_str(), -1, &text, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    void Append(std::string line) {
        SCROLLINFO scroll{sizeof(scroll), SIF_ALL};
        GetScrollInfo(transcript, SB_VERT, &scroll);
        const bool atBottom = scroll.nPos >= scroll.nMax - static_cast<int>(scroll.nPage) - 2;
        history.push_back(std::move(line));
        while (history.size() > kMaxTranscriptMessages)
            history.pop_front();
        std::string joined;
        for (const auto& lineItem : history) {
            joined += lineItem;
            joined += "\r\n";
        }
        const std::wstring wide = Wide(joined);
        SetWindowTextW(transcript, wide.c_str());
        if (atBottom) {
            SendMessageW(transcript, EM_SETSEL, wide.size(), wide.size());
            SendMessageW(transcript, EM_SCROLLCARET, 0, 0);
        } else
            SendMessageW(transcript, EM_LINESCROLL, 0, scroll.nPos);
    }
    void SendCurrent() {
        const int length = GetWindowTextLengthW(entry);
        if (length <= 0)
            return;
        std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
        GetWindowTextW(entry, value.data(), length + 1);
        value.resize(length);
        std::string error;
        if (callbacks.sendChat && callbacks.sendChat(Utf8(value), error)) {
            SetWindowTextW(entry, L"");
            SetFocus(entry);
        } else if (!error.empty())
            MessageBoxW(window, Wide(error).c_str(), L"Map together", MB_OK | MB_ICONWARNING);
    }
    static LRESULT CALLBACK EntryProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
        if (message == WM_KEYDOWN && wParam == VK_RETURN) {
            PostMessageW(GetParent(control), kSendFromEntry, 0, 0);
            return 0;
        }
        return DefSubclassProc(control, message, wParam, lParam);
    }
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        auto* self = reinterpret_cast<Impl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = reinterpret_cast<Impl*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            self->window = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self)
            return DefWindowProcW(hwnd, message, wParam, lParam);
        switch (message) {
        case WM_CREATE:
            self->Build();
            return 0;
        case WM_SIZE:
            self->Layout();
            return 0;
        case WM_NOTIFY:
            if (reinterpret_cast<NMHDR*>(lParam)->idFrom == kTab &&
                reinterpret_cast<NMHDR*>(lParam)->code == TCN_SELCHANGE)
                self->ShowTab(TabCtrl_GetCurSel(self->tab));
            return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case kSend:
                self->SendCurrent();
                return 0;
            case kStart:
                if (self->callbacks.startServer)
                    self->callbacks.startServer();
                return 0;
            case kConnect:
                if (self->callbacks.connect)
                    self->callbacks.connect();
                return 0;
            case kDisconnect:
                if (self->callbacks.disconnect)
                    self->callbacks.disconnect();
                return 0;
            case kApplyRole: {
                const int selected = static_cast<int>(SendMessageW(self->participants, LB_GETCURSEL, 0, 0));
                if (selected >= 0 && self->callbacks.changeRole) {
                    const auto id =
                        static_cast<std::uint32_t>(SendMessageW(self->participants, LB_GETITEMDATA, selected, 0));
                    const auto roleValue = SendMessageW(self->role, CB_GETCURSEL, 0, 0) == 1 ? ParticipantRole::Viewer
                                                                                             : ParticipantRole::Editor;
                    std::string error;
                    if (!self->callbacks.changeRole(id, roleValue, error) && !error.empty())
                        MessageBoxW(hwnd, Wide(error).c_str(), L"Map together", MB_OK | MB_ICONWARNING);
                }
                return 0;
            }
            case kKick: {
                const int selected = static_cast<int>(SendMessageW(self->participants, LB_GETCURSEL, 0, 0));
                if (selected >= 0 && self->callbacks.disconnectParticipant) {
                    const auto id =
                        static_cast<std::uint32_t>(SendMessageW(self->participants, LB_GETITEMDATA, selected, 0));
                    std::string error;
                    if (!self->callbacks.disconnectParticipant(id, error) && !error.empty())
                        MessageBoxW(hwnd, Wide(error).c_str(), L"Map together", MB_OK | MB_ICONWARNING);
                }
                return 0;
            }
            case kSetPassword: {
                wchar_t buffer[129]{};
                GetWindowTextW(self->passwordEdit, buffer, 129);
                std::string error;
                if (self->callbacks.setPassword && self->callbacks.setPassword(Utf8(buffer), error)) {
                    SetWindowTextW(self->passwordEdit, L"");
                } else if (!error.empty())
                    MessageBoxW(hwnd, Wide(error).c_str(), L"Map together", MB_OK | MB_ICONWARNING);
                return 0;
            }
            case kRemovePassword: {
                std::string error;
                if (self->callbacks.setPassword)
                    self->callbacks.setPassword("", error);
                SetWindowTextW(self->passwordEdit, L"");
                return 0;
            }
            }
            break;
        case kSendFromEntry:
            self->SendCurrent();
            return 0;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_DESTROY:
            self->window = nullptr;
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    HWND owner = nullptr, window = nullptr, tab = nullptr, transcript = nullptr, entry = nullptr, send = nullptr,
         status = nullptr, address = nullptr, count = nullptr, participants = nullptr, start = nullptr,
         connect = nullptr, disconnect = nullptr, role = nullptr, applyRole = nullptr, kick = nullptr,
         passwordStatus = nullptr, passwordEdit = nullptr, setPassword = nullptr, removePassword = nullptr;
    HFONT font = nullptr;
    WindowCallbacks callbacks;
    WindowState state;
    std::deque<std::string> history;
};

Window::Window(HWND owner, HFONT font, WindowCallbacks callbacks)
    : impl_(std::make_unique<Impl>(owner, font, std::move(callbacks))) {}
Window::~Window() = default;
void Window::Show() {
    impl_->Show();
}
void Window::Update(const WindowState& state) {
    impl_->Update(state);
}
void Window::AppendSystem(std::string text) {
    impl_->Append(std::move(text));
}
void Window::AppendChat(std::string sender, std::string text) {
    impl_->Append(std::move(sender) + ": " + std::move(text));
}
HWND Window::Handle() const {
    return impl_->window;
}

} // namespace collaboration
