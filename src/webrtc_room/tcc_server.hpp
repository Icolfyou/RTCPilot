#ifndef TCC_SERVER_HPP
#define TCC_SERVER_HPP
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "utils/timeex.hpp"
#include "net/rtprtcp/rtcp_tcc_fb.hpp"
#include "net/rtprtcp/rtp_packet.hpp"
#include "udp_transport.hpp"
#include <memory>

namespace cpp_streamer
{
class TccServer 
{
public:
    TccServer(TransportSendCallbackI* cb, Logger* logger);
    ~TccServer();

public:
    void OnTimer(int64_t now_ms);
    int InsertRtpPacket(RtpPacket* rtp_packet);
    void SetSenderSsrc(uint32_t ssrc);
    void SetTccExtensionId(uint8_t id);
    uint8_t GetTccExtensionId() const { return tcc_extension_id_; }

private:
    bool FlushFeedback();
    void ResetFeedbackPacket();
    int64_t ResolvePacketTimeMs(RtpPacket* rtp_packet) const;

private:
    static constexpr int64_t kFeedbackTimeoutMs = 2*1000;

    TransportSendCallbackI* cb_ = nullptr;
    Logger* logger_ = nullptr;
    std::unique_ptr<RtcpTccFbPacket> tcc_fb_packet_;
    uint8_t fb_pkt_count_ = 0;
    uint32_t sender_ssrc_ = 0;
    uint32_t media_ssrc_ = 0;
    uint8_t tcc_extension_id_ = 0;
};

}

#endif