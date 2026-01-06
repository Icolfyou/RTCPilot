#include "tcc_server.hpp"

namespace cpp_streamer {

TccServer::TccServer(TransportSendCallbackI* cb, Logger* logger) : cb_(cb)
    , logger_(logger)
{
    tcc_fb_packet_ = std::make_unique<RtcpTccFbPacket>();
    ResetFeedbackPacket();
}

TccServer::~TccServer() {
}

void TccServer::OnTimer(int64_t now_ms) {
    if (!tcc_fb_packet_ || tcc_fb_packet_->PacketCount() == 0) {
        return;
    }

    int64_t oldest_ms = tcc_fb_packet_->OldestPacketTimeMs();
    if (oldest_ms <= 0) {
        return;
    }

    if (now_ms - oldest_ms >= kFeedbackTimeoutMs) {
        LogWarnf(logger_, "TCC feedback timeout(%ld), flush feedback, now_ms:%ld, oldest_ms:%ld",
            now_ms - oldest_ms, now_ms, oldest_ms);
        FlushFeedback();
    }
}

int TccServer::InsertRtpPacket(RtpPacket* rtp_packet) {
    if (!rtp_packet || !tcc_fb_packet_) {
        return -1;
    }
    if (tcc_extension_id_ == 0) {
        return -1;
    }

    uint32_t pkt_ssrc = rtp_packet->GetSsrc();

    media_ssrc_ = pkt_ssrc;
    sender_ssrc_ = 0;

    if (!tcc_fb_packet_) {
        tcc_fb_packet_.reset(new RtcpTccFbPacket());
    }
    tcc_fb_packet_->SetSsrc(sender_ssrc_, media_ssrc_);

    uint16_t wide_seq = 0;
    if (!rtp_packet->ReadWideSeq(wide_seq)) {
        return -1;
    }

    int64_t now_ms = ResolvePacketTimeMs(rtp_packet);

    if (tcc_fb_packet_->InsertPacket(wide_seq, now_ms) != 0) {
        FlushFeedback();
        ResetFeedbackPacket();
        tcc_fb_packet_->InsertPacket(wide_seq, now_ms);
        return 0;
    }

    if (tcc_fb_packet_->IsFullRtcp()) {
        FlushFeedback();
    }

    return 0;
}

void TccServer::SetSenderSsrc(uint32_t ssrc) {
    sender_ssrc_ = ssrc;
    if (tcc_fb_packet_) {
        tcc_fb_packet_->SetSsrc(sender_ssrc_, media_ssrc_);
    }
}

void TccServer::SetTccExtensionId(uint8_t id) {
    tcc_extension_id_ = id;
}

bool TccServer::FlushFeedback() {
    if (!tcc_fb_packet_ || tcc_fb_packet_->PacketCount() == 0) {
        ResetFeedbackPacket();
        return false;
    }

    if (!cb_) {
        ResetFeedbackPacket();
        return false;
    }

    uint8_t buffer[1500] = {0};
    size_t len = sizeof(buffer);

    tcc_fb_packet_->SetSsrc(sender_ssrc_, media_ssrc_);
    tcc_fb_packet_->SetFbPktCount(fb_pkt_count_++);

    bool r = tcc_fb_packet_->Serial(buffer, len);
    if (!r) {
        LogWarnf(logger_, "serialize tcc feedback failed, len:%zu", len);
        ResetFeedbackPacket();
        return false;
    }

    // LogInfoData(logger_, buffer, len, "TCC Feedback Packet Data:");
    cb_->OnTransportSendRtcp(buffer, len);
    ResetFeedbackPacket();
    return true;
}

void TccServer::ResetFeedbackPacket() {
    if (!tcc_fb_packet_) {
        return;
    }

    tcc_fb_packet_->Reset();
    tcc_fb_packet_->SetSsrc(sender_ssrc_, media_ssrc_);
    tcc_fb_packet_->SetFbPktCount(fb_pkt_count_);
}

int64_t TccServer::ResolvePacketTimeMs(RtpPacket* rtp_packet) const {
    if (!rtp_packet) {
        return now_millisec();
    }

    int64_t pkt_ms = rtp_packet->GetLocalMs();
    if (pkt_ms <= 0) {
        pkt_ms = now_millisec();
    }
    return pkt_ms;
}

} // namespace cpp_streamer