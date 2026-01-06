#ifndef UDP_TRANSPORT_HPP
#define UDP_TRANSPORT_HPP
#include "net/udp/udp_pub.hpp"
#include "net/rtprtcp/rtp_packet.hpp"


namespace cpp_streamer {

class UdpTransportI
{
public:
    virtual void OnWriteUdpData(const uint8_t* data, size_t sent_size, cpp_streamer::UdpTuple address) = 0;
};

class TransportSendCallbackI
{
public:
    virtual bool IsConnected() = 0;
    virtual void OnTransportSendRtp(uint8_t* data, size_t sent_size) = 0;
    virtual void OnTransportSendRtcp(uint8_t* data, size_t sent_size) = 0;
};

class PacketFromRtcPusherCallbackI
{
public:
    virtual void OnRtpPacketFromRtcPusher(const std::string& user_id,
        const std::string& session_id,
        const std::string& pusher_id, 
        RtpPacket* rtp_packet) = 0;
    virtual void OnRtpPacketFromRemoteRtcPusher(const std::string& pusher_user_id,
        const std::string& pusher_id, 
        RtpPacket* rtp_packet) = 0;
};

} // namespace cpp_streamer

#endif // UDP_TRANSPORT_HPP