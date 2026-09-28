#include "collaboration_protocol.hpp"

#include <Windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace collaboration {
namespace {

class Writer {
public:
    void U8(std::uint8_t value) { bytes.push_back(value); }
    void U16(std::uint16_t value) {
        bytes.push_back(static_cast<std::uint8_t>(value));
        bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    }
    void U32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
    void U64(std::uint64_t value) {
        for (int shift = 0; shift < 64; shift += 8) bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
    void Raw(std::span<const std::uint8_t> value) { bytes.insert(bytes.end(), value.begin(), value.end()); }
    void String(std::string_view value, std::size_t maximum) {
        if (value.size() > maximum || value.size() > std::numeric_limits<std::uint16_t>::max()) {
            throw std::invalid_argument("Collaboration string is too long.");
        }
        U16(static_cast<std::uint16_t>(value.size()));
        Raw({reinterpret_cast<const std::uint8_t*>(value.data()), value.size()});
    }
    std::vector<std::uint8_t> bytes;
};

class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> source) : source_(source) {}
    bool U8(std::uint8_t& value) { if (!Need(1)) return false; value = source_[position_++]; return true; }
    bool U16(std::uint16_t& value) {
        if (!Need(2)) return false;
        value = static_cast<std::uint16_t>(source_[position_] | (source_[position_ + 1] << 8)); position_ += 2; return true;
    }
    bool U32(std::uint32_t& value) {
        if (!Need(4)) return false; value = 0;
        for (int shift = 0; shift < 32; shift += 8) value |= static_cast<std::uint32_t>(source_[position_++]) << shift;
        return true;
    }
    bool U64(std::uint64_t& value) {
        if (!Need(8)) return false; value = 0;
        for (int shift = 0; shift < 64; shift += 8) value |= static_cast<std::uint64_t>(source_[position_++]) << shift;
        return true;
    }
    bool Raw(std::span<std::uint8_t> target) {
        if (!Need(target.size())) return false;
        std::copy_n(source_.begin() + static_cast<std::ptrdiff_t>(position_), target.size(), target.begin());
        position_ += target.size(); return true;
    }
    bool String(std::string& value, std::size_t maximum) {
        std::uint16_t length = 0;
        if (!U16(length) || length > maximum || !Need(length)) return false;
        value.assign(reinterpret_cast<const char*>(source_.data() + position_), length); position_ += length;
        return value.find('\0') == std::string::npos;
    }
    bool Done() const { return position_ == source_.size(); }
private:
    bool Need(std::size_t count) const { return count <= source_.size() - position_; }
    std::span<const std::uint8_t> source_;
    std::size_t position_ = 0;
};

bool Type(const Frame& frame, MessageType expected, std::string& error) {
    if (frame.type == expected) return true;
    error = "Unexpected collaboration message type."; return false;
}
bool Finish(const Reader& reader, std::string& error) {
    if (reader.Done()) return true;
    error = "Collaboration message contains trailing data."; return false;
}

} // namespace

std::vector<std::uint8_t> EncodeFrame(const Frame& frame) {
    if (frame.payload.size() > kMaxFrameBytes) throw std::invalid_argument("Collaboration frame is too large.");
    Writer writer; writer.U32(kProtocolMagic); writer.U16(kProtocolVersion); writer.U16(static_cast<std::uint16_t>(frame.type));
    writer.U32(static_cast<std::uint32_t>(frame.payload.size())); writer.Raw(frame.payload); return std::move(writer.bytes);
}

bool DecodeFrame(std::span<const std::uint8_t> bytes, Frame& frame, std::string& error) {
    Reader reader(bytes); std::uint32_t magic = 0, length = 0; std::uint16_t version = 0, type = 0;
    if (!reader.U32(magic) || !reader.U16(version) || !reader.U16(type) || !reader.U32(length)) { error = "Incomplete collaboration frame header."; return false; }
    if (magic != kProtocolMagic) { error = "Invalid collaboration frame signature."; return false; }
    if (version != kProtocolVersion) { error = "Incompatible collaboration frame version."; return false; }
    if (type < static_cast<std::uint16_t>(MessageType::Hello) || type > static_cast<std::uint16_t>(MessageType::SaveEvent)) { error = "Unknown collaboration message type."; return false; }
    if (length > kMaxFrameBytes || bytes.size() != 12u + length) { error = "Invalid collaboration frame length."; return false; }
    frame.type = static_cast<MessageType>(type); frame.payload.assign(bytes.begin() + 12, bytes.end()); return true;
}

Frame EncodeHello(const Hello& value) { Writer w; w.U16(value.protocolVersion); w.String(value.applicationVersion, 32); w.String(value.displayName, kMaxDisplayNameBytes); return {MessageType::Hello, std::move(w.bytes)}; }
Frame EncodeChallenge(const Challenge& value) { Writer w; w.Raw(value.nonce); w.U8(value.passwordRequired ? 1 : 0); w.String(value.sessionName, kMaxSessionNameBytes); w.String(value.mapName, kMaxMapNameBytes); w.U64(value.revision); return {MessageType::Challenge, std::move(w.bytes)}; }
Frame EncodeAuthentication(const Authentication& value) { Writer w; w.Raw(value.proof); return {MessageType::Authenticate, std::move(w.bytes)}; }
Frame EncodeAccepted(const Accepted& value) { Writer w; w.U32(value.userId); w.String(value.hostDisplayName, kMaxDisplayNameBytes); return {MessageType::Accepted, std::move(w.bytes)}; }
Frame EncodeRejected(const Rejected& value) { Writer w; w.U8(static_cast<std::uint8_t>(value.reason)); w.String(value.message, 160); return {MessageType::Rejected, std::move(w.bytes)}; }
Frame EncodeSnapshot(const Snapshot& value) { if (value.emfBytes.size() > kMaxSnapshotBytes) throw std::invalid_argument("Map snapshot is too large."); Writer w; w.U64(value.revision); w.U16(value.width); w.U16(value.height); w.U32(static_cast<std::uint32_t>(value.emfBytes.size())); w.Raw(value.emfBytes); return {MessageType::Snapshot, std::move(w.bytes)}; }
Frame EncodeDisconnect(std::string_view reason) { Writer w; w.String(reason, 160); return {MessageType::Disconnect, std::move(w.bytes)}; }
Frame EncodeChatRequest(std::string_view text) { Writer w; w.String(text,kMaxChatMessageBytes); return {MessageType::ChatRequest,std::move(w.bytes)}; }
Frame EncodeChatBroadcast(const ChatBroadcast& value) { Writer w;w.U64(value.sequence);w.U32(value.senderId);w.String(value.text,kMaxChatMessageBytes);return {MessageType::ChatBroadcast,std::move(w.bytes)}; }
Frame EncodeParticipantList(std::span<const Participant> participants) { if(participants.empty()||participants.size()>kMaxParticipants)throw std::invalid_argument("Invalid participant count.");Writer w;w.U8(static_cast<std::uint8_t>(participants.size()));for(const auto& participant:participants){w.U32(participant.userId);w.String(participant.displayName,kMaxDisplayNameBytes);w.U8(participant.host?1:0);w.U8(static_cast<std::uint8_t>(participant.role));w.U8(participant.color);}return {MessageType::ParticipantList,std::move(w.bytes)}; }
Frame EncodeOperationRequest(std::span<const std::uint8_t> operation){if(operation.empty()||operation.size()>kMaxOperationBytes)throw std::invalid_argument("Invalid operation size.");Writer w;w.U32(static_cast<std::uint32_t>(operation.size()));w.Raw(operation);return {MessageType::OperationRequest,std::move(w.bytes)};}
Frame EncodeOperationBroadcast(const OperationBroadcast& value){if(value.revision==0||value.operationId==0||value.authorId==0||value.operation.empty()||value.operation.size()>kMaxOperationBytes)throw std::invalid_argument("Invalid authoritative operation.");Writer w;w.U64(value.revision);w.U64(value.operationId);w.U32(value.authorId);w.U32(static_cast<std::uint32_t>(value.operation.size()));w.Raw(value.operation);return {MessageType::OperationBroadcast,std::move(w.bytes)};}
Frame EncodeOperationRejection(const OperationRejection& value){if(!value.requestId)throw std::invalid_argument("Invalid rejected operation.");Writer w;w.U64(value.requestId);w.String(value.message,160);return {MessageType::OperationRejected,std::move(w.bytes)};}
Frame EncodePresence(const Presence& value){Writer w;w.U32(value.userId);w.U8(static_cast<std::uint8_t>(value.area));w.U8(value.viewerActive?1:0);w.U16(value.x);w.U16(value.y);return {MessageType::Presence,std::move(w.bytes)};}
Frame EncodeSaveEvent(const SaveEvent& value){Writer w;w.U32(value.authorId);w.U64(value.revision);w.U64(value.unixTimestamp);return {MessageType::SaveEvent,std::move(w.bytes)};}

bool DecodeHello(const Frame& f, Hello& v, std::string& e) { if (!Type(f, MessageType::Hello, e)) return false; Reader r(f.payload); if (!r.U16(v.protocolVersion) || !r.String(v.applicationVersion, 32) || !r.String(v.displayName, kMaxDisplayNameBytes)) { e="Malformed hello message."; return false; } return Finish(r,e); }
bool DecodeChallenge(const Frame& f, Challenge& v, std::string& e) { if (!Type(f, MessageType::Challenge,e)) return false; Reader r(f.payload); std::uint8_t p=0; if(!r.Raw(v.nonce)||!r.U8(p)||p>1||!r.String(v.sessionName,kMaxSessionNameBytes)||!r.String(v.mapName,kMaxMapNameBytes)||!r.U64(v.revision)){e="Malformed challenge message.";return false;} v.passwordRequired=p!=0; return Finish(r,e); }
bool DecodeAuthentication(const Frame& f, Authentication& v, std::string& e) { if(!Type(f,MessageType::Authenticate,e))return false; Reader r(f.payload); if(!r.Raw(v.proof)){e="Malformed authentication message.";return false;} return Finish(r,e); }
bool DecodeAccepted(const Frame& f, Accepted& v, std::string& e) { if(!Type(f,MessageType::Accepted,e))return false; Reader r(f.payload); if(!r.U32(v.userId)||v.userId==0||!r.String(v.hostDisplayName,kMaxDisplayNameBytes)){e="Malformed acceptance message.";return false;} return Finish(r,e); }
bool DecodeRejected(const Frame& f, Rejected& v, std::string& e) { if(!Type(f,MessageType::Rejected,e))return false; Reader r(f.payload); std::uint8_t reason=0; if(!r.U8(reason)||reason<1||reason>5||!r.String(v.message,160)){e="Malformed rejection message.";return false;} v.reason=static_cast<RejectReason>(reason); return Finish(r,e); }
bool DecodeSnapshot(const Frame& f, Snapshot& v, std::string& e) { if(!Type(f,MessageType::Snapshot,e))return false; Reader r(f.payload); std::uint32_t size=0; if(!r.U64(v.revision)||!r.U16(v.width)||!r.U16(v.height)||!r.U32(size)||v.width<1||v.width>253||v.height<1||v.height>253||size>kMaxSnapshotBytes){e="Invalid map snapshot metadata.";return false;} v.emfBytes.resize(size); if(!r.Raw(v.emfBytes)){e="Truncated map snapshot.";return false;} return Finish(r,e); }
bool DecodeDisconnect(const Frame& f, std::string& v, std::string& e) { if(!Type(f,MessageType::Disconnect,e))return false; Reader r(f.payload); if(!r.String(v,160)){e="Malformed disconnect message.";return false;} return Finish(r,e); }
bool DecodeChatRequest(const Frame& f,std::string& v,std::string& e){if(!Type(f,MessageType::ChatRequest,e))return false;Reader r(f.payload);if(!r.String(v,kMaxChatMessageBytes)||!Finish(r,e)){if(e.empty())e="Malformed chat request.";return false;}std::string normalized;if(!NormalizeChatText(v,normalized,e))return false;v=std::move(normalized);return true;}
bool DecodeChatBroadcast(const Frame& f,ChatBroadcast& v,std::string& e){if(!Type(f,MessageType::ChatBroadcast,e))return false;Reader r(f.payload);if(!r.U64(v.sequence)||v.sequence==0||!r.U32(v.senderId)||v.senderId==0||!r.String(v.text,kMaxChatMessageBytes)||!Finish(r,e)){if(e.empty())e="Malformed chat broadcast.";return false;}std::string normalized;if(!NormalizeChatText(v.text,normalized,e))return false;v.text=std::move(normalized);return true;}
bool DecodeParticipantList(const Frame& f,std::vector<Participant>& v,std::string& e){if(!Type(f,MessageType::ParticipantList,e))return false;Reader r(f.payload);std::uint8_t count=0;if(!r.U8(count)||count<1||count>kMaxParticipants){e="Invalid participant count.";return false;}v.clear();v.reserve(count);bool foundHost=false;for(std::uint8_t i=0;i<count;++i){Participant p;std::uint8_t host=0,role=0;if(!r.U32(p.userId)||p.userId==0||!r.String(p.displayName,kMaxDisplayNameBytes)||p.displayName.empty()||!r.U8(host)||host>1||!r.U8(role)||role<1||role>3||!r.U8(p.color)||p.color>7){e="Malformed participant list.";return false;}p.host=host!=0;p.role=static_cast<ParticipantRole>(role);if(p.host){if(foundHost||p.role!=ParticipantRole::Host){e="Participant list has invalid host.";return false;}foundHost=true;}if(std::any_of(v.begin(),v.end(),[&](const Participant& current){return current.userId==p.userId;})){e="Participant list contains duplicate IDs.";return false;}v.push_back(std::move(p));}if(!foundHost||!Finish(r,e)){if(e.empty())e="Participant list has no host.";return false;}return true;}
bool DecodeOperationRequest(const Frame& f,std::vector<std::uint8_t>& operation,std::string& e){if(!Type(f,MessageType::OperationRequest,e))return false;Reader r(f.payload);std::uint32_t size=0;if(!r.U32(size)||size==0||size>kMaxOperationBytes){e="Invalid operation size.";return false;}operation.resize(size);if(!r.Raw(operation)||!Finish(r,e)){if(e.empty())e="Malformed operation request.";return false;}return true;}
bool DecodeOperationBroadcast(const Frame& f,OperationBroadcast& v,std::string& e){if(!Type(f,MessageType::OperationBroadcast,e))return false;Reader r(f.payload);std::uint32_t size=0;if(!r.U64(v.revision)||v.revision==0||!r.U64(v.operationId)||v.operationId==0||!r.U32(v.authorId)||v.authorId==0||!r.U32(size)||size==0||size>kMaxOperationBytes){e="Invalid authoritative operation.";return false;}v.operation.resize(size);if(!r.Raw(v.operation)||!Finish(r,e)){if(e.empty())e="Malformed authoritative operation.";return false;}return true;}
bool DecodeOperationRejection(const Frame& f,OperationRejection& v,std::string& e){if(!Type(f,MessageType::OperationRejected,e))return false;Reader r(f.payload);if(!r.U64(v.requestId)||!v.requestId||!r.String(v.message,160)||!Finish(r,e)){if(e.empty())e="Malformed rejected operation.";return false;}return true;}
bool DecodePresence(const Frame& f,Presence& v,std::string& e){if(!Type(f,MessageType::Presence,e))return false;Reader r(f.payload);std::uint8_t area=0,active=0;if(!r.U32(v.userId)||v.userId==0||!r.U8(area)||area>static_cast<std::uint8_t>(PresenceArea::MapTogether)||!r.U8(active)||active>1||!r.U16(v.x)||!r.U16(v.y)||!Finish(r,e)){if(e.empty())e="Malformed presence message.";return false;}v.area=static_cast<PresenceArea>(area);v.viewerActive=active!=0;return true;}
bool DecodeSaveEvent(const Frame& f,SaveEvent& v,std::string& e){if(!Type(f,MessageType::SaveEvent,e))return false;Reader r(f.payload);if(!r.U32(v.authorId)||v.authorId==0||!r.U64(v.revision)||!r.U64(v.unixTimestamp)||!Finish(r,e)){if(e.empty())e="Malformed save event.";return false;}return true;}

bool IsValidUtf8(std::string_view text){std::size_t i=0;while(i<text.size()){const auto first=static_cast<unsigned char>(text[i]);std::size_t count=0;std::uint32_t value=0;if(first<=0x7f){count=1;value=first;}else if((first&0xe0)==0xc0){count=2;value=first&0x1f;}else if((first&0xf0)==0xe0){count=3;value=first&0x0f;}else if((first&0xf8)==0xf0){count=4;value=first&0x07;}else return false;if(i+count>text.size())return false;for(std::size_t j=1;j<count;++j){const auto next=static_cast<unsigned char>(text[i+j]);if((next&0xc0)!=0x80)return false;value=(value<<6)|(next&0x3f);}if((count==2&&value<0x80)||(count==3&&value<0x800)||(count==4&&value<0x10000)||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return false;i+=count;}return true;}
bool NormalizeChatText(std::string_view input,std::string& normalized,std::string& error){std::size_t first=0,last=input.size();while(first<last&&(input[first]==' '||input[first]=='\t'||input[first]=='\r'||input[first]=='\n'))++first;while(last>first&&(input[last-1]==' '||input[last-1]=='\t'||input[last-1]=='\r'||input[last-1]=='\n'))--last;normalized.assign(input.substr(first,last-first));if(normalized.empty()){error="Chat message is empty.";return false;}if(normalized.size()>kMaxChatMessageBytes){error="Chat message is too long.";return false;}if(normalized.find('\0')!=std::string::npos||!IsValidUtf8(normalized)){error="Chat message is not valid UTF-8.";return false;}return true;}
bool NormalizeDisplayName(std::string_view input,std::string& normalized,std::string& error){std::size_t first=0,last=input.size();while(first<last&&(input[first]==' '||input[first]=='\t'||input[first]=='\r'||input[first]=='\n'))++first;while(last>first&&(input[last-1]==' '||input[last-1]=='\t'||input[last-1]=='\r'||input[last-1]=='\n'))--last;normalized.assign(input.substr(first,last-first));if(normalized.empty()||normalized.size()>kMaxDisplayNameBytes||!IsValidUtf8(normalized)){error="Collaboration name must be valid UTF-8 and no more than 48 bytes.";return false;}for(unsigned char c:normalized)if(c<32||c==127){error="Collaboration name cannot contain control characters.";return false;}return true;}

std::array<std::uint8_t, kChallengeBytes> CreateChallengeNonce() {
    std::array<std::uint8_t, kChallengeBytes> result{};
    if (BCryptGenRandom(nullptr, result.data(), static_cast<ULONG>(result.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) throw std::runtime_error("Unable to create a secure session challenge.");
    return result;
}

std::array<std::uint8_t, kPasswordProofBytes> MakePasswordProof(std::string_view password, std::span<const std::uint8_t, kChallengeBytes> nonce) {
    BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr; DWORD objectSize = 0, ignored = 0;
    std::array<std::uint8_t, kPasswordProofBytes> result{}; std::vector<std::uint8_t> object;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0 ||
        BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &ignored, 0) != 0) goto fail;
    object.resize(objectSize);
    if (BCryptCreateHash(algorithm, &hash, object.data(), objectSize,
            reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())), static_cast<ULONG>(password.size()), 0) != 0 ||
        BCryptHashData(hash, const_cast<PUCHAR>(nonce.data()), static_cast<ULONG>(nonce.size()), 0) != 0 ||
        BCryptFinishHash(hash, result.data(), static_cast<ULONG>(result.size()), 0) != 0) goto fail;
    BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0); return result;
fail:
    if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    throw std::runtime_error("Unable to verify the session password.");
}

bool ConstantTimeEqual(std::span<const std::uint8_t> left, std::span<const std::uint8_t> right) {
    if (left.size() != right.size()) return false; std::uint8_t difference = 0;
    for (std::size_t i=0;i<left.size();++i) difference |= left[i] ^ right[i]; return difference == 0;
}

} // namespace collaboration
