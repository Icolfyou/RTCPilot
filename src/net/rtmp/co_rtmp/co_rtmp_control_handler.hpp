#ifndef RTMP_CONTROL_HANDLER_HPP
#define RTMP_CONTROL_HANDLER_HPP
#include "co_chunk_stream.hpp"
#include "amf/amf0.hpp"
#include "co_rtmp_pub.hpp"
#include "logger.hpp"
#include <vector>

namespace cpp_streamer
{

class CoRtmpSessionBase;

class CoRtmpControlHandler
{
public:
    CoRtmpControlHandler(CoRtmpSessionBase* session, Logger* logger = nullptr);
    ~CoRtmpControlHandler();

public:
    int HandleServerCommandMessage(CHUNK_STREAM_PTR cs_ptr, std::vector<AMF_ITERM*>& amf_vec);
    int HandleClientCommandMessage(CHUNK_STREAM_PTR cs_ptr, std::vector<AMF_ITERM*>& amf_vec);
    int HandleRtmpPublishCommand(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec);
    int HandleRtmpPlayCommand(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec);
    int HandleRtmpCreatestreamCommand(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec);
    int HandleRtmpConnectCommand(uint32_t stream_id, std::vector<AMF_ITERM*>& amf_vec);
    int SendRtmpPublishResp();
    int SendRtmpPlayResp();
    int SendRtmpPlayResetResp();
    int SendRtmpPlayStartResp();
    int SendRtmpPlayDataResp();
    int SendRtmpPlayNotifyResp();
    int SendRtmpCreateStreamResp(double transaction_id);
    int SendRtmpConnectResp(uint32_t stream_id);
    int SendRtmpAck(uint32_t size);
    int SendSetChunksize(uint32_t chunk_size);

public:
    int HandleRtmpControlMessage(CHUNK_STREAM_PTR cs_ptr, bool is_server = true);

private:
    CoRtmpSessionBase* session_;

private:
    Logger* logger_ = nullptr;
};

}

#endif //RTMP_CONTROL_HANDLER_HPP
