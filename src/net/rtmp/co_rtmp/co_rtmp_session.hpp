#ifndef CO_RTMP_SESSION_HPP
#define CO_RTMP_SESSION_HPP
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN  // ÆÁ±Î Windows ¾É°æÈßÓàÍ·ÎÄ¼þ£¨°üÀ¨ winsock.h£©
#endif
#include "utils/logger.hpp"
#include "utils/data_buffer.hpp"
#include "utils/co_pub.hpp"
#include "net/tcp/co_tcp/co_tcp_server/co_tcp_accept_conn.hpp"
#include "co_rtmp_pub.hpp"
#include "co_rtmp_handshake.hpp"
#include "co_rtmp_session_base.hpp"
#include "co_rtmp_control_handler.hpp"
#include "co_rtmp_request.hpp"
#include "media_packet.hpp"
#include <memory>
#include <string>

namespace cpp_streamer
{
class RtmpWriter;

class CoRtmpSession : public CoRtmpSessionBase
{
public:
    CoRtmpSession(const std::string& id, std::shared_ptr<TcpCoAcceptConn> conn_ptr, Logger* logger);
    virtual ~CoRtmpSession();

public:
    CoVoidTask Run();
    void Close();

public:
    bool IsAlive();
    std::string GetSessonKey();

public:
    CoTask<int> HandleRequest();

public:
    virtual DataBuffer* GetRecvBuffer() override;
    virtual int RtmpSend(char* data, int len) override;
    virtual int RtmpSend(std::shared_ptr<DataBuffer> data_ptr) override;

private:
    int ReceiveChunkStream();
    CoTask<int> CoRtmpSend(char* data, int len);
	
private:
    std::string id_;
    std::shared_ptr<TcpCoAcceptConn> conn_ptr_;
    Logger* logger_ = nullptr;

private:
    RTMP_SERVER_SESSION_PHASE phase_ = initial_phase;
    bool running_ = false;
    bool closed_flag_ = false;

private:
    RtmpServerHandshake hs_;
    CoRtmpControlHandler control_handler_;

private:
    int alive_count_ = 0;

private:
    RtmpWriter* play_writer_ = nullptr;
};

}
#endif // CO_RTMP_SESSION_HPP