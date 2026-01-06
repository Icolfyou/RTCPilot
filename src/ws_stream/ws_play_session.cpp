#include "ws_play_session.hpp"
#include "utils/uuid.hpp"
#include "flv_pub.hpp"
#include "byte_stream.hpp"

namespace cpp_streamer {

WsPlaySession::WsPlaySession(WebSocketSession* session, Logger* logger)
    :session_(session)
    ,logger_(logger)
{
    session_->SetSessionCallback(this);

    app_ = session_->GetQueryMap().at("app");
    stream_ = session_->GetQueryMap().at("stream");
    key_ = app_ + "/" + stream_;

    auto r = UUID::GetRandomUint(10000, 99999);
    stream_name_ = "WsPlaySession_" + app_ + "_" + stream_ + "_" + std::to_string(r);
    LogInfof(logger_, "WsPlaySession construct, app:%s, stream:%s, key:%s",
        app_.c_str(), stream_.c_str(), key_.c_str());
}

WsPlaySession::~WsPlaySession() {
    Clear();
    LogInfof(logger_, "WsPlaySession destruct, app:%s, stream:%s, key:%s",
        app_.c_str(), stream_.c_str(), key_.c_str());
}

void WsPlaySession::Clear() {
    if (session_) {
        session_->SetSessionCallback(nullptr);
        session_->CloseSession();
        session_ = nullptr;//don't own the session
    }
}
void WsPlaySession::OnReadData(int code, const uint8_t* data, size_t len) {
    if (code < 0) {
        LogErrorf(logger_, "WsPlaySession::OnReadData error code:%d", code);
        closed_ = true;
        return;
    }
    if (app_.empty() || stream_.empty()) {
        LogErrorf(logger_, "WsPlaySession::OnReadData app or stream is empty, app:%s, stream:%s",
                  app_.c_str(), stream_.c_str());
        return;
    }
    LogDebugf(logger_, "WsPlaySession::OnReadData, app:%s, stream:%s, data len:%zu",
        app_.c_str(), stream_.c_str(), len);
}

void WsPlaySession::OnReadText(int code, const std::string& text) {
    if (code < 0) {
        LogErrorf(logger_, "WsPlaySession::OnReadText error code:%d", code);
        closed_ = true;
        return;
    }
    LogDebugf(logger_, "WsPlaySession::OnReadText, text len:%zu, text:%s", text.length(), text.c_str());
}

void WsPlaySession::OnClose(int code, const std::string& desc) {
    LogInfof(logger_, "WsPlaySession::OnClose, code:%d, desc:%s", code, desc.c_str());
    closed_ = true;
}

int WsPlaySession::WritePacket(Media_Packet_Ptr pkt_ptr) {
    int ret = 0;
    uint8_t flv_header[11];
    uint8_t pre_size_data[4];
    uint32_t pre_size = 0;

    if (!session_) {
        LogErrorf(logger_, "WsPlaySession::WritePacket session is null");
        return -1;
    }
    if (closed_) {
        LogErrorf(logger_, "WsPlaySession::WritePacket session is closed");
        return -1;
    }

    ret = SendFlvHeader();
    if (ret != 0) {
        return 0;
    }

    /*|Tagtype(8)|DataSize(24)|Timestamp(24)|TimestampExtended(8)|StreamID(24)|Data(...)|PreviousTagSize(32)|*/
    if (pkt_ptr->av_type_ == MEDIA_VIDEO_TYPE) {
        flv_header[0] = FLV_TAG_VIDEO;
    }
    else if (pkt_ptr->av_type_ == MEDIA_AUDIO_TYPE) {
        flv_header[0] = FLV_TAG_AUDIO;
    }
    else {
        LogErrorf(logger_, "httpflv writer does not suport av type:%d", pkt_ptr->av_type_);
        return 0;
    }
    uint32_t payload_size = (uint32_t)pkt_ptr->buffer_ptr_->DataLen();
    uint32_t timestamp_base = (uint32_t)(pkt_ptr->dts_ & 0xffffff);
    uint8_t timestamp_ext = (uint8_t)((pkt_ptr->dts_ >> 24) & 0xff);

    ByteStream::Write3Bytes(flv_header + 1, payload_size);
    if (timestamp_base >= 0xffffff) {
        ByteStream::Write3Bytes(flv_header + 4, 0xffffff);
    }
    else {
        ByteStream::Write3Bytes(flv_header + 4, timestamp_base);
    }
    flv_header[7] = timestamp_ext;
    //Set StreamID(24) as 0
    flv_header[8] = 0;
    flv_header[9] = 0;
    flv_header[10] = 0;

	WsSendData(flv_header, sizeof(flv_header));
    WsSendData((uint8_t*)pkt_ptr->buffer_ptr_->Data(), pkt_ptr->buffer_ptr_->DataLen());

    pre_size = (uint32_t)(sizeof(flv_header) + pkt_ptr->buffer_ptr_->DataLen());
    ByteStream::Write4Bytes(pre_size_data, pre_size);
    WsSendData(pre_size_data, sizeof(pre_size_data));

	alive_ms_ = GetNowMilliSec();
    return 0;
}

std::string WsPlaySession::GetKey() {
    return key_;
}

std::string WsPlaySession::GetWriterId() {
    return stream_name_;
}

void WsPlaySession::CloseWriter() {
}

bool WsPlaySession::IsInited() {
    return init_flag_;
}

void WsPlaySession::SetInitFlag(bool flag) {
    init_flag_ = flag;
}

int WsPlaySession::SendFlvHeader() {
    /*|'F'(8)|'L'(8)|'V'(8)|version(8)|TypeFlagsReserved(5)|TypeFlagsAudio(1)|TypeFlagsReserved(1)|TypeFlagsVideo(1)|DataOffset(32)|PreviousTagSize(32)|*/
    uint8_t flag = 0;

    if (flv_header_ready_) {
        return 0;
    }

    if (!has_audio_ && !has_video_) {
        return -1;
    }
    if (has_video_) {
        flag |= 0x01;
    }
    if (has_audio_) {
        flag |= 0x04;
    }

    uint8_t flv_header[] = { 0x46, 0x4c, 0x56, 0x01, flag, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00 };

    if (!session_) {
        LogErrorf(logger_, "WsPlaySession::SendFlvHeader session is null");
        return -1;
    }
    WsSendData(flv_header, sizeof(flv_header));

    flv_header_ready_ = true;
    return 0;
}

void WsPlaySession::WsSendData(const uint8_t* data, size_t len) {
    if (!session_) {
        LogErrorf(logger_, "WsPlaySession::WsSendData session is null");
        return;
    }
    if (closed_) {
        return;
    }
    
    try {
        session_->AsyncWriteData(data, len);
    }
    catch (std::exception& e) {
        LogErrorf(logger_, "WsPlaySession::WsSendData exception when AsyncWriteData: %s", e.what());
        closed_ = true;
    }
}

bool WsPlaySession::IsAlive() {
    if (closed_) {
        return false;
    }
    if (session_ == nullptr) {
        LogErrorf(logger_, "WsPlaySession::IsAlive session is null");
        return false;
    }
    if (session_->IsClose()) {
        LogInfof(logger_, "WsPlaySession::IsAlive session is closed, app:%s, stream:%s",
            app_.c_str(), stream_.c_str());
        return false;
    }
    int64_t now_ms = GetNowMilliSec();
    if (alive_ms_ <= 0) {
        alive_ms_ = now_ms;
        return true;
    }
    if (now_ms - alive_ms_ > 15000) {//15s no data
        LogInfof(logger_, "WsPlaySession::IsAlive false for no data timeout, app:%s, stream:%s",
            app_.c_str(), stream_.c_str());
        return false;
    }
    return true;
}

} // namespace cpp_streamer