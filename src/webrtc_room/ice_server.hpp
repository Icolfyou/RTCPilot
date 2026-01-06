#ifndef ICE_SERVER_HPP
#define ICE_SERVER_HPP
#include "utils/logger.hpp"
#include "net/stun/stun.hpp"
#include "net/udp/udp_pub.hpp"

namespace cpp_streamer {

class IceOnDataWriteCallbackI
{
public:
    virtual void OnIceWrite(const uint8_t* data, size_t sent_size, UdpTuple address) = 0;
};

class IceServer
{
public:
    IceServer(IceOnDataWriteCallbackI* cb, Logger* logger);
    ~IceServer();

public:
    int HandleStunPacket(StunPacket* stun_pkt, UdpTuple addr);

public:
    std::string GetIceUfrag() { return ice_ufrag_;}
    std::string GetIcePwd() { return ice_pwd_;}
    
private:
    IceOnDataWriteCallbackI* cb_ = nullptr;
    Logger* logger_;
    std::string ice_ufrag_;
    std::string ice_pwd_;
};

} // namespace cpp_streamer

#endif // ICE_SERVER_HPP