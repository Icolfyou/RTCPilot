#ifndef RTMP_SESSION_BASE_HPP
#define RTMP_SESSION_BASE_HPP
#include "data_buffer.hpp"
#include "co_chunk_stream.hpp"
#include "co_rtmp_pub.hpp"
#include "co_rtmp_request.hpp"
#include "co_rtmp_control_handler.hpp"
#include "logger.hpp"
#include "media_packet.hpp"

#include <memory>
#include <map>
#include <stdint.h>

namespace cpp_streamer
{

const char* GetServerPhaseDesc(RTMP_SERVER_SESSION_PHASE phase);

const char* GetClientPhaseDesc(RTMP_CLIENT_SESSION_PHASE phase);

class CoRtmpSessionBase
{
public:
    CoRtmpSessionBase(Logger* logger);
    virtual ~CoRtmpSessionBase();
    
public:
    virtual DataBuffer* GetRecvBuffer() = 0;
    virtual int RtmpSend(char* data, int len) = 0;
    virtual int RtmpSend(std::shared_ptr<DataBuffer> data_ptr) = 0;

public:
    void SetChunkSize(uint32_t chunk_size);
    uint32_t GetChunkSize();
    bool IsPublish();
    const char* IsPublishDesc();

protected:
    int ReadFmtCsid();
    int ReadChunkStream(CHUNK_STREAM_PTR& cs_ptr);
    Media_Packet_Ptr GetMediaPacket(CHUNK_STREAM_PTR cs_ptr);

public:
    DataBuffer recv_buffer_;
    bool fmt_ready_ = false;
    uint8_t fmt_    = 0;
    uint16_t csid_  = 0;
    std::map<uint8_t, CHUNK_STREAM_PTR> cs_map_;
    uint32_t remote_window_acksize_ = 2500000;
    uint32_t ack_received_          = 0;
    CoRtmpRequest req_;
    uint32_t stream_id_ = 1;
    RTMP_SERVER_SESSION_PHASE server_phase_ = initial_phase;
    RTMP_CLIENT_SESSION_PHASE client_phase_ = client_initial_phase;

protected:
    uint32_t chunk_size_ = CHUNK_DEF_SIZE;

protected:
    Logger* logger_ = nullptr;
};

}

#endif //RTMP_SESSION_BASE_HPP
