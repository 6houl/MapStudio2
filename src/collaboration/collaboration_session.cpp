#include "collaboration_session.hpp"
#include "collaboration_edit.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

namespace collaboration {
namespace {

constexpr const char* kApplicationVersion = "EndlessMapStudio/1";
constexpr int kIoTimeoutMs = 6000;

void CloseSocket(SOCKET& socket) {
    if (socket != INVALID_SOCKET) { shutdown(socket, SD_BOTH); closesocket(socket); socket = INVALID_SOCKET; }
}

bool SendAll(SOCKET socket, const std::uint8_t* data, std::size_t size) {
    while (size > 0) {
        const int amount = send(socket, reinterpret_cast<const char*>(data), static_cast<int>(std::min<std::size_t>(size, 64 * 1024)), 0);
        if (amount <= 0) return false; data += amount; size -= static_cast<std::size_t>(amount);
    }
    return true;
}

bool ReceiveAll(SOCKET socket, std::uint8_t* data, std::size_t size) {
    while (size > 0) {
        const int amount = recv(socket, reinterpret_cast<char*>(data), static_cast<int>(std::min<std::size_t>(size, 64 * 1024)), 0);
        if (amount <= 0) return false; data += amount; size -= static_cast<std::size_t>(amount);
    }
    return true;
}

bool SendFrame(SOCKET socket, const Frame& frame) {
    const auto bytes = EncodeFrame(frame); return SendAll(socket, bytes.data(), bytes.size());
}

bool ReceiveFrame(SOCKET socket, Frame& frame, std::string& error) {
    std::array<std::uint8_t, 12> header{};
    if (!ReceiveAll(socket, header.data(), header.size())) { error = "Connection closed."; return false; }
    const std::uint32_t length = static_cast<std::uint32_t>(header[8]) |
        (static_cast<std::uint32_t>(header[9]) << 8) | (static_cast<std::uint32_t>(header[10]) << 16) |
        (static_cast<std::uint32_t>(header[11]) << 24);
    if (length > kMaxFrameBytes) { error = "Incoming collaboration message is too large."; return false; }
    std::vector<std::uint8_t> bytes(header.begin(), header.end()); bytes.resize(12u + length);
    if (length && !ReceiveAll(socket, bytes.data() + 12, length)) { error = "Connection closed during a message."; return false; }
    return DecodeFrame(bytes, frame, error);
}

void SetTimeouts(SOCKET socket) {
    DWORD timeout = kIoTimeoutMs;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
}
void ClearReceiveTimeout(SOCKET socket){DWORD timeout=0;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof(timeout));}

SOCKET ConnectSocket(const std::string& address, std::uint16_t port, std::string& error) {
    addrinfo hints{}; hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP;
    addrinfo* results = nullptr; const std::string service = std::to_string(port);
    if (getaddrinfo(address.c_str(), service.c_str(), &hints, &results) != 0) { error = "Address could not be resolved."; return INVALID_SOCKET; }
    SOCKET connected = INVALID_SOCKET;
    for (addrinfo* current = results; current; current = current->ai_next) {
        SOCKET candidate = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (candidate == INVALID_SOCKET) continue;
        u_long nonBlocking = 1; ioctlsocket(candidate, FIONBIO, &nonBlocking);
        int result = connect(candidate, current->ai_addr, static_cast<int>(current->ai_addrlen));
        if (result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set writes; FD_ZERO(&writes); FD_SET(candidate, &writes); timeval wait{5, 0};
            result = select(0, nullptr, &writes, nullptr, &wait);
            if (result > 0) { int socketError = 0; int size = sizeof(socketError); getsockopt(candidate, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &size); result = socketError == 0 ? 0 : SOCKET_ERROR; }
        }
        nonBlocking = 0; ioctlsocket(candidate, FIONBIO, &nonBlocking);
        if (result == 0) { connected = candidate; break; }
        closesocket(candidate);
    }
    freeaddrinfo(results);
    if (connected == INVALID_SOCKET) error = "Unable to connect to the collaboration server."; else SetTimeouts(connected);
    return connected;
}

} // namespace

class Session::Impl {
public:
    explicit Impl(std::function<void()> wake) : wakeUi(std::move(wake)) { WSADATA data{}; winsockReady = WSAStartup(MAKEWORD(2,2), &data) == 0; }
    ~Impl() { Stop("Application closing", false); if (winsockReady) WSACleanup(); }

    void Queue(Event event) {
        { std::lock_guard lock(eventMutex); events.push_back(std::move(event)); }
        if (wakeUi) wakeUi();
    }
    void SetStatus(std::string value) { std::lock_guard lock(dataMutex); status = std::move(value); }
    void QueueText(EventType type,std::string message,std::string name={}){Event event;event.type=type;event.message=std::move(message);event.displayName=std::move(name);Queue(std::move(event));}

    bool StartHost(const HostOptions& value, std::string& error) {
        if (!winsockReady) { error="Windows networking is unavailable."; return false; }
        if (state.load()!=SessionState::Disconnected) { error="A collaboration session is already active."; return false; }
        std::string normalized;if(!NormalizeDisplayName(value.displayName,normalized,error)||value.sessionName.size()>kMaxSessionNameBytes || value.mapName.size()>kMaxMapNameBytes || value.snapshot.emfBytes.size()>kMaxSnapshotBytes || value.snapshot.width<1 || value.snapshot.width>254 || value.snapshot.height<1 || value.snapshot.height>254) { if(error.empty())error="Invalid collaboration server settings."; return false; }
        SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); if(listener==INVALID_SOCKET){error="Unable to create the collaboration server.";return false;}
        sockaddr_in endpoint{}; endpoint.sin_family=AF_INET; endpoint.sin_addr.s_addr=htonl(INADDR_ANY); endpoint.sin_port=htons(value.port);
        BOOL reuse=TRUE; setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));
        if(bind(listener,reinterpret_cast<sockaddr*>(&endpoint),sizeof(endpoint))==SOCKET_ERROR||listen(listener,SOMAXCONN)==SOCKET_ERROR){closesocket(listener);error="Unable to listen on that port.";return false;}
        { std::lock_guard lock(dataMutex); host=value;host.displayName=normalized; listenSocket=listener; status="Hosting on port "+std::to_string(value.port);hostAddress="127.0.0.1";sessionPort=value.port;participants={{1,normalized,true,ParticipantRole::Host,0}}; }
        sessionRevision=value.snapshot.revision;nextOperationId=value.snapshot.revision+1;stopping=false; state=SessionState::Hosting;
        StartSender();acceptThread=std::thread([this]{ AcceptLoop(); });QueueText(EventType::Status,"Session started. Hosting on port "+std::to_string(value.port)+".");QueueParticipants();return true;
    }

    bool StartClient(const ConnectOptions& value, std::string& error) {
        if (!winsockReady) { error="Windows networking is unavailable."; return false; }
        if(state.load()!=SessionState::Disconnected){error="A collaboration session is already active.";return false;}
        std::string normalized;if(value.address.empty()||!NormalizeDisplayName(value.displayName,normalized,error)){if(error.empty())error="Enter a valid address and display name.";return false;}
        stopping=false; state=SessionState::Connecting;{std::lock_guard lock(dataMutex);hostAddress=value.address;sessionPort=value.port;participants.clear();}SetStatus("Connecting to "+value.address+":"+std::to_string(value.port));QueueText(EventType::Status,"Connecting to "+value.address+":"+std::to_string(value.port)+"...");
        if (clientThread.joinable()) clientThread.join();
        ConnectOptions normalizedValue=value;normalizedValue.displayName=normalized;clientThread=std::thread([this,normalizedValue]{ ClientLoop(normalizedValue); }); return true;
    }

    void Stop(std::string reason, bool notify) {
        stopping=true;
        outgoingCv.notify_all();
        SOCKET listener=INVALID_SOCKET, client=INVALID_SOCKET;
        { std::lock_guard lock(dataMutex); listener=listenSocket; listenSocket=INVALID_SOCKET; client=clientSocket; clientSocket=INVALID_SOCKET; }
        if(client!=INVALID_SOCKET){if(notify) SendFrame(client,EncodeDisconnect(reason)); CloseSocket(client);}
        if(listener!=INVALID_SOCKET) CloseSocket(listener);
        std::vector<SOCKET> guestsCopy;
        { std::lock_guard lock(guestMutex); guestsCopy=guestSockets; guestSockets.clear(); authenticatedSockets.clear(); authenticatedUsers.clear(); }
        for(SOCKET guest:guestsCopy){if(notify) SendFrame(guest,EncodeDisconnect(reason)); CloseSocket(guest);}
        if(acceptThread.joinable()&&acceptThread.get_id()!=std::this_thread::get_id())acceptThread.join();
        if(clientThread.joinable()&&clientThread.get_id()!=std::this_thread::get_id())clientThread.join();
        if(senderThread.joinable()&&senderThread.get_id()!=std::this_thread::get_id())senderThread.join();
        { std::lock_guard lock(workerMutex); for(auto& thread:workers)if(thread.joinable()&&thread.get_id()!=std::this_thread::get_id())thread.join(); workers.clear(); }
        guestCount=0;state=SessionState::Disconnected;{std::lock_guard lock(dataMutex);status="Disconnected";participants.clear();}
        QueueParticipants();if(notify)QueueText(EventType::Status,reason);
    }

    void StartSender(){if(senderThread.joinable())senderThread.join();senderThread=std::thread([this]{SenderLoop();});}
    void SenderLoop(){while(!stopping){std::string text;{std::unique_lock lock(outgoingMutex);outgoingCv.wait(lock,[&]{return stopping||state.load()==SessionState::Disconnected||!outgoing.empty();});if(stopping||state.load()==SessionState::Disconnected)break;text=std::move(outgoing.front());outgoing.pop_front();}if(state.load()==SessionState::Hosting)BroadcastChat(1,text);else if(state.load()==SessionState::Connected){SOCKET socket;{std::lock_guard lock(dataMutex);socket=clientSocket;}if(socket!=INVALID_SOCKET){std::lock_guard sendLock(sendMutex);if(!SendFrame(socket,EncodeChatRequest(text)))QueueText(EventType::ConnectionLost,"Unable to send chat message.");}}}}

    bool EnqueueChat(std::string_view input,std::string& error){std::string normalized;if(!NormalizeChatText(input,normalized,error))return false;const auto current=state.load();if(current!=SessionState::Hosting&&current!=SessionState::Connected){error="Chat is unavailable while disconnected.";return false;}{std::lock_guard lock(outgoingMutex);outgoing.push_back(std::move(normalized));}outgoingCv.notify_one();return true;}
    ParticipantRole RoleFor(std::uint32_t id)const{std::lock_guard lock(dataMutex);auto found=std::find_if(participants.begin(),participants.end(),[&](const Participant&p){return p.userId==id;});return found==participants.end()?ParticipantRole::Viewer:found->role;}
    bool Submit(std::span<const std::uint8_t> operation,std::string& error){if(operation.empty()||operation.size()>kMaxOperationBytes){error="Invalid collaboration operation size.";return false;}EditOperation edit;if(!DecodeEditOperation(operation,edit,error))return false;if(state.load()==SessionState::Hosting){HostOptions options;{std::lock_guard lock(dataMutex);options=host;}if(!ValidateEditOperation(edit,options.snapshot.width,options.snapshot.height,error))return false;if(edit.kind==EditKind::ResizeMap){std::lock_guard lock(dataMutex);host.snapshot.width=edit.width;host.snapshot.height=edit.height;}BroadcastOperation(1,operation);return true;}if(state.load()!=SessionState::Connected){error="Map editing is unavailable while disconnected.";return false;}if(RoleFor(localUserId)==ParticipantRole::Viewer){error="Session role: Viewer. Map changes are read-only.";return false;}SOCKET socket;{std::lock_guard lock(dataMutex);socket=clientSocket;}std::lock_guard sendLock(sendMutex);if(socket==INVALID_SOCKET||!SendFrame(socket,EncodeOperationRequest(operation))){error="Unable to send map operation.";return false;}return true;}
    void BroadcastOperation(std::uint32_t authorId,std::span<const std::uint8_t> operation,bool queueHost=true){std::lock_guard orderLock(operationMutex);OperationBroadcast accepted{++sessionRevision,nextOperationId++,authorId,{operation.begin(),operation.end()}};const Frame frame=EncodeOperationBroadcast(accepted);std::vector<SOCKET>sockets;{std::lock_guard lock(guestMutex);sockets=authenticatedSockets;}{std::lock_guard sendLock(sendMutex);for(SOCKET socket:sockets)SendFrame(socket,frame);}if(queueHost){Event event;event.type=EventType::OperationAccepted;event.operation=std::move(accepted);Queue(std::move(event));}}
    bool AcceptRequested(std::uint32_t authorId,std::span<const std::uint8_t> operation,std::string& error){if(state.load()!=SessionState::Hosting||authorId<2){error="Invalid requested operation author.";return false;}EditOperation edit;HostOptions options;{std::lock_guard lock(dataMutex);options=host;}if(!DecodeEditOperation(operation,edit,error)||!ValidateEditOperation(edit,options.snapshot.width,options.snapshot.height,error))return false;if(edit.kind==EditKind::ResizeMap){std::lock_guard lock(dataMutex);host.snapshot.width=edit.width;host.snapshot.height=edit.height;}BroadcastOperation(authorId,operation,false);return true;}
    bool RejectRequested(std::uint32_t authorId,std::uint64_t requestId,std::string_view reason,std::string& error){SOCKET socket=INVALID_SOCKET;{std::lock_guard lock(guestMutex);auto found=std::find_if(authenticatedUsers.begin(),authenticatedUsers.end(),[&](const auto& value){return value.first==authorId;});if(found!=authenticatedUsers.end())socket=found->second;}if(socket==INVALID_SOCKET||!requestId){error="Requested operation author is no longer connected.";return false;}std::lock_guard sendLock(sendMutex);if(!SendFrame(socket,EncodeOperationRejection({requestId,std::string(reason)}))){error="Unable to reject requested operation.";return false;}return true;}
    bool PublishPresence(Presence value,std::string& error){const auto current=state.load();if(current!=SessionState::Hosting&&current!=SessionState::Connected){error="Presence is unavailable while disconnected.";return false;}HostOptions options;{std::lock_guard lock(dataMutex);options=host;}if(static_cast<std::uint8_t>(value.area)>static_cast<std::uint8_t>(PresenceArea::MapTogether)||(value.viewerActive&&(value.area!=PresenceArea::Viewer||value.x>=options.snapshot.width||value.y>=options.snapshot.height))){error="Presence state is outside supported ranges.";return false;}value.userId=current==SessionState::Hosting?1:localUserId;if(current==SessionState::Hosting){BroadcastPresence(value);return true;}SOCKET socket;{std::lock_guard lock(dataMutex);socket=clientSocket;}std::lock_guard sendLock(sendMutex);const bool sent=socket!=INVALID_SOCKET&&SendFrame(socket,EncodePresence(value));if(sent)++presenceSent;return sent;}
    void BroadcastPresence(const Presence& value){const Frame frame=EncodePresence(value);std::vector<SOCKET>sockets;{std::lock_guard lock(guestMutex);sockets=authenticatedSockets;}{std::lock_guard sendLock(sendMutex);for(SOCKET socket:sockets)SendFrame(socket,frame);}presenceSent+=sockets.size();Event event;event.type=EventType::PresenceChanged;event.presence=value;Queue(std::move(event));}
    bool ChangeRole(std::uint32_t id,ParticipantRole role,std::string& error){if(state.load()!=SessionState::Hosting||id==1||role==ParticipantRole::Host){error="The host role cannot be changed.";return false;}{std::lock_guard lock(dataMutex);auto found=std::find_if(participants.begin(),participants.end(),[&](const Participant&p){return p.userId==id;});if(found==participants.end()){error="Participant is no longer connected.";return false;}found->role=role;}BroadcastParticipants();return true;}
    bool Kick(std::uint32_t id,std::string& error){if(state.load()!=SessionState::Hosting||id==1){error="The host cannot disconnect itself here.";return false;}SOCKET socket=INVALID_SOCKET;{std::lock_guard lock(guestMutex);auto found=std::find_if(authenticatedUsers.begin(),authenticatedUsers.end(),[&](const auto&v){return v.first==id;});if(found!=authenticatedUsers.end())socket=found->second;}if(socket==INVALID_SOCKET){error="Participant is no longer connected.";return false;}{std::lock_guard lock(sendMutex);SendFrame(socket,EncodeDisconnect("Disconnected by host."));shutdown(socket,SD_BOTH);}return true;}
    bool ChangePassword(std::string value,std::string& error){if(state.load()!=SessionState::Hosting){error="Only the host can change the session password.";return false;}if(value.size()>128){error="Password is too long.";return false;}{std::lock_guard lock(dataMutex);host.password=std::move(value);}return true;}
    bool PublishSave(std::uint64_t timestamp,std::string& error){if(state.load()!=SessionState::Hosting){error="Only the host can publish a save.";return false;}SaveEvent saved{1,sessionRevision.load(),timestamp};const Frame frame=EncodeSaveEvent(saved);std::vector<SOCKET>sockets;{std::lock_guard lock(guestMutex);sockets=authenticatedSockets;}{std::lock_guard sendLock(sendMutex);for(SOCKET socket:sockets)SendFrame(socket,frame);}Event event;event.type=EventType::MapSaved;event.saveEvent=saved;Queue(std::move(event));return true;}

    std::vector<Participant> ParticipantCopy()const{std::lock_guard lock(dataMutex);return participants;}
    void QueueParticipants(){Event event;event.type=EventType::ParticipantsChanged;event.participants=ParticipantCopy();Queue(std::move(event));}
    void BroadcastParticipants(){const auto list=ParticipantCopy();const Frame frame=EncodeParticipantList(list);std::vector<SOCKET> sockets;{std::lock_guard lock(guestMutex);sockets=authenticatedSockets;}std::lock_guard sendLock(sendMutex);for(SOCKET socket:sockets)SendFrame(socket,frame);QueueParticipants();}
    std::string ParticipantName(std::uint32_t id){std::lock_guard lock(dataMutex);auto found=std::find_if(participants.begin(),participants.end(),[&](const Participant& p){return p.userId==id;});return found==participants.end()?"Mapper":found->displayName;}
    void BroadcastChat(std::uint32_t senderId,const std::string& text){std::lock_guard orderLock(broadcastMutex);ChatBroadcast chat{nextChatSequence++,senderId,text};const Frame frame=EncodeChatBroadcast(chat);std::vector<SOCKET>sockets;{std::lock_guard lock(guestMutex);sockets=authenticatedSockets;}{std::lock_guard sendLock(sendMutex);for(SOCKET socket:sockets)SendFrame(socket,frame);}Event event;event.type=EventType::ChatMessage;event.message=text;event.displayName=ParticipantName(senderId);event.sequence=chat.sequence;event.senderId=senderId;Queue(std::move(event));}

    void AcceptLoop() {
        while(!stopping){
            SOCKET listener; {std::lock_guard lock(dataMutex);listener=listenSocket;} if(listener==INVALID_SOCKET)break;
            fd_set reads; FD_ZERO(&reads); FD_SET(listener,&reads); timeval wait{0,200000}; const int ready=select(0,&reads,nullptr,nullptr,&wait);
            if(ready<=0)continue; SOCKET guest=accept(listener,nullptr,nullptr); if(guest==INVALID_SOCKET)continue; SetTimeouts(guest);
            {std::lock_guard lock(workerMutex);workers.emplace_back([this,guest]{HostGuestLoop(guest);});}
        }
    }

    void Reject(SOCKET socket, RejectReason reason, const std::string& message) { SendFrame(socket,EncodeRejected({reason,message})); }

    void HostGuestLoop(SOCKET socket) {
        const std::size_t previousConnections=activeConnections.fetch_add(1);
        if(previousConnections>=kMaxParticipants-1){--activeConnections;Reject(socket,RejectReason::SessionFull,"The session is full.");CloseSocket(socket);return;}
        {std::lock_guard lock(guestMutex);guestSockets.push_back(socket);}
        auto close=[&]{
            bool owned=false;{std::lock_guard lock(guestMutex);auto it=std::find(guestSockets.begin(),guestSockets.end(),socket);if(it!=guestSockets.end()){guestSockets.erase(it);owned=true;}authenticatedSockets.erase(std::remove(authenticatedSockets.begin(),authenticatedSockets.end(),socket),authenticatedSockets.end());std::erase_if(authenticatedUsers,[&](const auto& value){return value.second==socket;});}
            if(owned)CloseSocket(socket);--activeConnections;
        };
        Frame frame; std::string error; Hello hello;
        if(!ReceiveFrame(socket,frame,error)||!DecodeHello(frame,hello,error)){Reject(socket,RejectReason::InvalidMessage,error);close();return;}
        if(hello.protocolVersion!=kProtocolVersion){Reject(socket,RejectReason::IncompatibleProtocol,"This collaboration protocol version is not supported.");close();return;}
        if(hello.applicationVersion!=kApplicationVersion){Reject(socket,RejectReason::IncompatibleProtocol,"This Endless Map Studio version is not compatible with the session.");close();return;}
        std::string normalizedName;if(!NormalizeDisplayName(hello.displayName,normalizedName,error)){Reject(socket,RejectReason::InvalidMessage,error);close();return;}
        HostOptions options; {std::lock_guard lock(dataMutex);options=host;}
        Challenge challenge{CreateChallengeNonce(),!options.password.empty(),options.sessionName,options.mapName,options.snapshot.revision};
        if(!SendFrame(socket,EncodeChallenge(challenge))||!ReceiveFrame(socket,frame,error)){close();return;}
        Authentication authentication; if(!DecodeAuthentication(frame,authentication,error)){Reject(socket,RejectReason::InvalidMessage,error);close();return;}
        const auto expected=MakePasswordProof(options.password,challenge.nonce);
        if(!ConstantTimeEqual(authentication.proof,expected)){Reject(socket,RejectReason::WrongPassword,"The session password is incorrect.");close();return;}
        const std::uint32_t userId=nextUserId.fetch_add(1);std::string presentedName=normalizedName;{std::lock_guard lock(dataMutex);int duplicate=1;for(const auto&p:participants)if(p.displayName==normalizedName||p.displayName.rfind(normalizedName+" (",0)==0)++duplicate;if(duplicate>1)presentedName=normalizedName+" ("+std::to_string(duplicate)+")";}
        {std::lock_guard sendLock(sendMutex);if(!SendFrame(socket,EncodeAccepted({userId,options.displayName}))||!SendFrame(socket,EncodeSnapshot(options.snapshot))){close();return;}}
        {std::lock_guard lock(guestMutex);authenticatedSockets.push_back(socket);authenticatedUsers.push_back({userId,socket});}{std::lock_guard lock(dataMutex);participants.push_back({userId,presentedName,false,ParticipantRole::Editor,static_cast<std::uint8_t>(1+(userId-2)%7)});}
        ClearReceiveTimeout(socket);++guestCount;QueueText(EventType::ParticipantConnected,presentedName+" has connected",presentedName);BroadcastParticipants();
        while(!stopping){if(!ReceiveFrame(socket,frame,error))break;if(frame.type==MessageType::Disconnect)break;if(frame.type==MessageType::ChatRequest){std::string text;if(!DecodeChatRequest(frame,text,error)){Reject(socket,RejectReason::InvalidMessage,error);break;}BroadcastChat(userId,text);continue;}if(frame.type==MessageType::OperationRequest){std::vector<std::uint8_t> operation;EditOperation edit;HostOptions current;{std::lock_guard lock(dataMutex);current=host;}if(!DecodeOperationRequest(frame,operation,error)||!DecodeEditOperation(operation,edit,error)||!ValidateEditOperation(edit,current.snapshot.width,current.snapshot.height,error)){Reject(socket,RejectReason::InvalidMessage,error);break;}if(RoleFor(userId)==ParticipantRole::Viewer){RejectRequested(userId,edit.requestId,"Session role: Viewer. Map changes are read-only.",error);continue;}Event requested;requested.type=EventType::OperationRequested;requested.operation.authorId=userId;requested.operation.operation=std::move(operation);Queue(std::move(requested));continue;}if(frame.type==MessageType::Presence){Presence presence;HostOptions current;{std::lock_guard lock(dataMutex);current=host;}if(!DecodePresence(frame,presence,error)||(presence.viewerActive&&(presence.area!=PresenceArea::Viewer||presence.x>=current.snapshot.width||presence.y>=current.snapshot.height))){Reject(socket,RejectReason::InvalidMessage,error);break;}presence.userId=userId;++presenceReceived;BroadcastPresence(presence);continue;}Reject(socket,RejectReason::InvalidMessage,"Unsupported collaboration message.");break;}
        --guestCount;{std::lock_guard lock(dataMutex);participants.erase(std::remove_if(participants.begin(),participants.end(),[&](const Participant& p){return p.userId==userId;}),participants.end());}close();if(!stopping){QueueText(EventType::ParticipantDisconnected,presentedName+" has disconnected",presentedName);BroadcastParticipants();}
    }

    void ClientLoop(ConnectOptions options) {
        std::string error; SOCKET socket=ConnectSocket(options.address,options.port,error);
        if(socket==INVALID_SOCKET){state=SessionState::Disconnected;SetStatus("Connection failed");QueueText(EventType::Rejected,error);return;}
        {std::lock_guard lock(dataMutex);clientSocket=socket;}
        Frame frame; Hello hello{options.protocolVersion,kApplicationVersion,options.displayName};
        if(!SendFrame(socket,EncodeHello(hello))||!ReceiveFrame(socket,frame,error)){ClientFailed(error.empty()?"Connection failed.":error);return;}
        if(frame.type==MessageType::Rejected){Rejected rejected;if(DecodeRejected(frame,rejected,error))ClientFailed(rejected.message);else ClientFailed(error);return;}
        Challenge challenge; if(!DecodeChallenge(frame,challenge,error)){ClientFailed(error);return;}
        Authentication authentication{MakePasswordProof(options.password,challenge.nonce)};
        if(!SendFrame(socket,EncodeAuthentication(authentication))||!ReceiveFrame(socket,frame,error)){ClientFailed(error.empty()?"Authentication failed.":error);return;}
        if(frame.type==MessageType::Rejected){Rejected rejected;if(DecodeRejected(frame,rejected,error))ClientFailed(rejected.message);else ClientFailed(error);return;}
        Accepted accepted;if(!DecodeAccepted(frame,accepted,error)){ClientFailed(error);return;}QueueText(EventType::Status,"Authentication successful.");QueueText(EventType::Status,"Synchronizing initial map...");if(!ReceiveFrame(socket,frame,error)){ClientFailed(error);return;}
        Snapshot snapshot; if(!DecodeSnapshot(frame,snapshot,error)){ClientFailed(error);return;}ClearReceiveTimeout(socket);
        {std::lock_guard lock(dataMutex);localUserId=accepted.userId;host.snapshot.width=snapshot.width;host.snapshot.height=snapshot.height;}sessionRevision=snapshot.revision;state=SessionState::Connected;SetStatus("Connected to "+accepted.hostDisplayName);StartSender();{Event event;event.type=EventType::SnapshotReceived;event.message="Initial map synchronized.";event.displayName=accepted.hostDisplayName;event.snapshot=std::move(snapshot);Queue(std::move(event));}
        while(!stopping){if(!ReceiveFrame(socket,frame,error))break;if(frame.type==MessageType::Disconnect){std::string reason;DecodeDisconnect(frame,reason,error);error=reason;break;}if(frame.type==MessageType::ParticipantList){std::vector<Participant> updated;if(!DecodeParticipantList(frame,updated,error))break;{std::lock_guard lock(dataMutex);participants=updated;}QueueParticipants();continue;}if(frame.type==MessageType::ChatBroadcast){ChatBroadcast chat;if(!DecodeChatBroadcast(frame,chat,error))break;Event event;event.type=EventType::ChatMessage;event.message=chat.text;event.displayName=ParticipantName(chat.senderId);event.sequence=chat.sequence;event.senderId=chat.senderId;Queue(std::move(event));continue;}if(frame.type==MessageType::OperationBroadcast){OperationBroadcast operation;if(!DecodeOperationBroadcast(frame,operation,error))break;if(operation.revision!=sessionRevision.load()+1){error="Authoritative revision gap detected; reconnect to resynchronize.";break;}sessionRevision=operation.revision;Event event;event.type=EventType::OperationAccepted;event.operation=std::move(operation);Queue(std::move(event));continue;}if(frame.type==MessageType::OperationRejected){OperationRejection rejected;if(!DecodeOperationRejection(frame,rejected,error))break;Event event;event.type=EventType::OperationRejected;event.sequence=rejected.requestId;event.message=std::move(rejected.message);Queue(std::move(event));continue;}if(frame.type==MessageType::Presence){Presence presence;if(!DecodePresence(frame,presence,error))break;++presenceReceived;Event event;event.type=EventType::PresenceChanged;event.presence=presence;Queue(std::move(event));continue;}if(frame.type==MessageType::SaveEvent){SaveEvent saved;if(!DecodeSaveEvent(frame,saved,error))break;Event event;event.type=EventType::MapSaved;event.saveEvent=saved;Queue(std::move(event));continue;}error="Unexpected collaboration message.";break;}
        if(!stopping){state=SessionState::Disconnected;outgoingCv.notify_all();SetStatus("Connection lost");QueueText(EventType::ConnectionLost,error.empty()?"Connection lost.":error);}
        SOCKET closing;{std::lock_guard lock(dataMutex);closing=clientSocket;clientSocket=INVALID_SOCKET;}CloseSocket(closing);
    }

    void ClientFailed(const std::string& error) {
        SOCKET closing;{std::lock_guard lock(dataMutex);closing=clientSocket;clientSocket=INVALID_SOCKET;}CloseSocket(closing);
        state=SessionState::Disconnected;SetStatus("Connection failed");QueueText(EventType::Rejected,error.empty()?"Connection rejected.":error);
    }

    std::function<void()> wakeUi; bool winsockReady=false; std::atomic<bool> stopping{false}; std::atomic<SessionState> state{SessionState::Disconnected};
    std::atomic<std::size_t> guestCount{0},activeConnections{0};std::atomic<std::uint32_t> nextUserId{2};std::atomic<std::uint64_t> sessionRevision{0},presenceSent{0},presenceReceived{0};std::uint64_t nextOperationId=1;
    mutable std::mutex dataMutex,eventMutex,guestMutex,workerMutex,sendMutex,broadcastMutex,operationMutex,outgoingMutex;HostOptions host;std::string status="Disconnected",hostAddress;std::uint16_t sessionPort=0;std::uint32_t localUserId=0;std::vector<Participant> participants;
    std::condition_variable outgoingCv;std::deque<std::string> outgoing;std::uint64_t nextChatSequence=1;
    SOCKET listenSocket=INVALID_SOCKET,clientSocket=INVALID_SOCKET;std::vector<SOCKET> guestSockets,authenticatedSockets;std::vector<std::pair<std::uint32_t,SOCKET>> authenticatedUsers;std::vector<Event> events;
    std::thread acceptThread,clientThread,senderThread;std::vector<std::thread> workers;
};

Session::Session(std::function<void()> wakeUi):impl_(std::make_unique<Impl>(std::move(wakeUi))){}
Session::~Session()=default;
bool Session::StartHosting(const HostOptions& options,std::string& error){return impl_->StartHost(options,error);}
bool Session::Connect(const ConnectOptions& options,std::string& error){return impl_->StartClient(options,error);}
void Session::Disconnect(std::string reason){impl_->Stop(std::move(reason),true);}
bool Session::SendChat(std::string_view text,std::string& error){return impl_->EnqueueChat(text,error);}
bool Session::SubmitOperation(std::span<const std::uint8_t> operation,std::string& error){return impl_->Submit(operation,error);}
bool Session::AcceptRequestedOperation(std::uint32_t authorId,std::span<const std::uint8_t> operation,std::string& error){return impl_->AcceptRequested(authorId,operation,error);}
bool Session::RejectRequestedOperation(std::uint32_t authorId,std::uint64_t requestId,std::string_view reason,std::string& error){return impl_->RejectRequested(authorId,requestId,reason,error);}
bool Session::SendPresence(Presence presence,std::string& error){return impl_->PublishPresence(presence,error);}
bool Session::BroadcastSave(std::uint64_t timestamp,std::string& error){return impl_->PublishSave(timestamp,error);}
bool Session::ChangeParticipantRole(std::uint32_t id,ParticipantRole role,std::string& error){return impl_->ChangeRole(id,role,error);}
bool Session::DisconnectParticipant(std::uint32_t id,std::string& error){return impl_->Kick(id,error);}
bool Session::SetPassword(std::string password,std::string& error){return impl_->ChangePassword(std::move(password),error);}
SessionState Session::State()const{return impl_->state.load();}
std::size_t Session::ConnectedGuestCount()const{return impl_->guestCount.load();}
std::string Session::StatusText()const{std::lock_guard lock(impl_->dataMutex);return impl_->status;}
std::string Session::HostAddress()const{std::lock_guard lock(impl_->dataMutex);return impl_->hostAddress;}
std::uint16_t Session::Port()const{std::lock_guard lock(impl_->dataMutex);return impl_->sessionPort;}
std::vector<Participant> Session::Participants()const{return impl_->ParticipantCopy();}
std::uint64_t Session::Revision()const{return impl_->sessionRevision.load();}
std::uint32_t Session::LocalUserId()const{return State()==SessionState::Hosting?1:impl_->localUserId;}
ParticipantRole Session::LocalRole()const{return State()==SessionState::Hosting?ParticipantRole::Host:impl_->RoleFor(impl_->localUserId);}
bool Session::PasswordEnabled()const{std::lock_guard lock(impl_->dataMutex);return !impl_->host.password.empty();}
std::uint64_t Session::PresencePacketsSent()const{return impl_->presenceSent.load();}
std::uint64_t Session::PresencePacketsReceived()const{return impl_->presenceReceived.load();}
std::vector<Event> Session::DrainEvents(){std::lock_guard lock(impl_->eventMutex);std::vector<Event> result;result.swap(impl_->events);return result;}

} // namespace collaboration
