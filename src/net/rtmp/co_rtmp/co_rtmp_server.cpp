#include "co_rtmp_server.hpp"
#include "co_rtmp_session.hpp"
#include "net/tcp/co_tcp/co_tcp_server/co_tcp_accept_conn.hpp"
#include "utils/uuid.hpp"
#include "utils/timeex.hpp"

namespace cpp_streamer
{
RtmpServer::RtmpServer(uv_loop_t* loop,
            const std::string& host,
            int port,
            Logger* logger) : TimerInterface(200)
	                        , logger_(logger)
{
    tcp_server_ = std::make_unique<CoTcpServer>(loop, host, port, logger);
    StartTimer();
}

RtmpServer::~RtmpServer()
{
    StopTimer();
}

bool RtmpServer::OnTimer() {
    int64_t now_ms = now_millisec();
    OnCheckAlive(now_ms);
    return timer_running_;
}

void RtmpServer::OnCheckAlive(int64_t now_ms) {
    if (last_check_ms_ < 0) {
        last_check_ms_ = now_ms;
        return;
    }

    if (now_ms - last_check_ms_ < 2000) {
        return;
    }
    last_check_ms_ = now_ms;

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (!it->second->IsAlive()) {
            LogInfof(logger_, "RTMP session %s is no longer alive, closing it.", it->first.c_str());
            it->second->Close();
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

CoVoidTask RtmpServer::Run() {
    LogInfof(logger_, "RTMP server is running...");
    while(true) {
        std::shared_ptr<TcpCoAcceptConn> conn_ptr = co_await tcp_server_->CoAccept();
        std::string uuid = UUID::MakeUUID2();

        LogInfof(logger_, "Accepted new RTMP connection from %s, session UUID: %s",
            conn_ptr->GetRemoteEndpoint().c_str(), uuid.c_str());
        auto session = std::make_shared<CoRtmpSession>(uuid, conn_ptr, logger_);
        sessions_[uuid] = session;
        try {
            session->Run();
        }
        catch (const std::exception& e) {
			LogInfof(logger_, "Exception in RTMP session %s: %s", uuid.c_str(), e.what());
        }
		
    }
}
}