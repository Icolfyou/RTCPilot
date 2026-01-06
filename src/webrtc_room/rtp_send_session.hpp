#ifndef RTP_SEND_SESSION_HPP
#define RTP_SEND_SESSION_HPP
#include "utils/logger.hpp"
#include "utils/av/av.hpp"
#include "utils/stream_statics.hpp"
#include "rtp_session.hpp"
#include "udp_transport.hpp"
#include "net/rtprtcp/rtp_packet.hpp"
#include "net/rtprtcp/rtcpfb_nack.hpp"
#include "net/rtprtcp/rtcp_rr.hpp"
#include <vector>

namespace cpp_streamer {

class RtpSendSession : public RtpSession
{
public:
    RtpSendSession(const RtpSessionParam& param,
        const std::string& room_id,
        const std::string& puller_user_id,
        const std::string& pusher_user_id,
        TransportSendCallbackI* send_cb,
        uv_loop_t* loop, Logger* logger);
    virtual ~RtpSendSession();

public:
    bool SendRtpPacket(RtpPacket* rtp_pkt);
    int RecvRtcpFbNack(RtcpFbNack* nack_pkt);
    int RecvRtcpRrBlock(RtcpRrBlockInfo& rr_block);
    void OnTimer(int64_t now_ms);
    StreamStatics& GetSendStatics() { return send_statics_; }
    
private:
    void RetransmitRtxPackets(RtpPacket* rtp_pkt);
    void OnSendRtcpSr(int64_t now_ms);
    void StoreRtxPacket(RtpPacket* rtx_pkt);

private:
    std::string pusher_user_id_;
    std::string puller_user_id_;
    TransportSendCallbackI* send_cb_ = nullptr;

private:
    std::vector<RtpPacket*> rtx_packet_cache_;
    StreamStatics send_statics_;
    int64_t last_rtcp_sr_ms_ = -1;
    int64_t last_rtcp_sr_rtp_ts_ = 0;
    int64_t avg_rtt_ms_ = 30;//default 30ms
    int64_t lost_total_ = 0;
    int64_t lost_fract_ = 0;
    float lost_rate_ = 0.0f;
    int64_t rtcp_rr_dbg_ms_ = -1;

private:
    uint16_t rtx_seq_ = 0;
    uint16_t last_seq_ = 0;
};

} // namespace cpp_streamer

#endif