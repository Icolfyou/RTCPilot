#include "ws_stream_server.hpp"
#include "ws_publish_session.hpp"
#include "ws_play_session.hpp"
#include "net/http/websocket/websocket_session.hpp"
#include "utils/uuid.hpp"
#include "utils/av/media_stream_manager.hpp"
#include <map>

namespace cpp_streamer {
std::map<std::string, std::shared_ptr<WsPublishSession>> WsStreamServer::ws_publish_sessions;
std::map<std::string, std::shared_ptr<WsPlaySession>> WsStreamServer::ws_play_sessions;

void OnWSStreamPublish(const std::string& uri, WebSocketSession* session) {
	std::shared_ptr<WsPublishSession> session_ptr = std::make_shared<WsPublishSession>(session, session->GetLogger());

	WsStreamServer::ws_publish_sessions[session->GetRemoteAddress()] = session_ptr;
}

void OnWSStreamPlay(const std::string& uri, WebSocketSession* session) {
    Logger* logger = session->GetLogger();

    std::string app = session->GetQueryMap().at("app");
    std::string stream = session->GetQueryMap().at("stream");
    std::string key = app + "/" + stream;

    std::string uuid = UUID::MakeUUID2();

    LogInfof(logger, "http flv request key:%s, uuid:%s", key.c_str(), uuid.c_str());

    std::shared_ptr<WsPlaySession> writer_ptr = std::make_shared<WsPlaySession>(session, logger);
    WsStreamServer::ws_play_sessions[session->GetRemoteAddress()] = writer_ptr;

    MediaStreamManager::AddPlayer(writer_ptr.get());
}

WsStreamServer::WsStreamServer(const std::string& ip, 
    uint16_t port, uv_loop_t* loop, Logger* logger) : TimerInterface(500)
{
    logger_ = logger;
    ws_server_ptr_.reset(new WebSocketServer(ip, port, loop, logger));
    ws_server_ptr_->AddHandle("/publish", OnWSStreamPublish);
    ws_server_ptr_->AddHandle("/play", OnWSStreamPlay);

    StartTimer();
}

WsStreamServer::WsStreamServer(const std::string& ip, uint16_t port, uv_loop_t* loop, 
    const std::string& key_file, const std::string& cert_file, Logger* logger) : TimerInterface(500)
{
    logger_ = logger;
    ws_server_ptr_.reset(new WebSocketServer(ip, port, loop, key_file, cert_file, logger));
    ws_server_ptr_->AddHandle("/publish", OnWSStreamPublish);
    ws_server_ptr_->AddHandle("/play", OnWSStreamPlay);

    StartTimer();
}

WsStreamServer::~WsStreamServer()
{
    ws_server_ptr_.reset();
}

bool WsStreamServer::OnTimer() {
    //clean up closed publish sessions
    for (auto iter = ws_publish_sessions.begin(); iter != ws_publish_sessions.end(); ) {
        std::shared_ptr<WsPublishSession> session_ptr = iter->second;
        if (!session_ptr->IsAlive()) {
            LogInfof(logger_, "remove closed ws publish session, addr:%s", iter->first.c_str());
            ws_publish_sessions.erase(iter++);
            continue;
        }
        ++iter;
    }

    //clean up closed play sessions
    for (auto iter = ws_play_sessions.begin(); iter != ws_play_sessions.end(); ) {
        if (!iter->second->IsAlive()) {
            LogInfof(logger_, "remove closed ws play session, addr:%s", iter->first.c_str());
            MediaStreamManager::RemovePlayer(iter->second.get());
            // log the player session shared_ptr use count
            LogInfof(logger_, "ws play session use count:%d", (int)iter->second.use_count());
			iter->second->Clear();
            iter = ws_play_sessions.erase(iter);
            continue;
        }
        ++iter;
    }
    return timer_running_;
}
} // namespace cpp_streamer