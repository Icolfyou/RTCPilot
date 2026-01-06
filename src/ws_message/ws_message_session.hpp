#ifndef WS_MESSAGE_SESSION_HPP
#define WS_MESSAGE_SESSION_HPP
#include "net/http/websocket/websocket_session.hpp"
#include "ws_protoo_info.hpp"
#include "utils/logger.hpp"
#include <string>
#include <memory>

namespace cpp_streamer {

class WsMessageSession : public WebSocketSessionCallBackI, public ProtooResponseI
{
public:
    WsMessageSession(WebSocketSession* session, ProtooCallBackI* cb, Logger* logger);
    virtual ~WsMessageSession();

public:
    bool IsAlive();
    void Clear();

public:
    virtual void OnReadData(int code, const uint8_t* data, size_t len) override;
    virtual void OnReadText(int code, const std::string& text) override;
    virtual void OnClose(int code, const std::string& desc) override;

public:
    virtual void OnProtooResponse(ProtooResponse& resp) override;
    virtual void Request(const std::string& method, nlohmann::json& j) override;
    virtual void Notification(const std::string& method, nlohmann::json& j) override;
    virtual void SetUserInfo(const std::string& room_id, const std::string& user_id) override;

private:
    WebSocketSession* session_ = nullptr;
    ProtooCallBackI* protoo_cb_ = nullptr;
    Logger* logger_ = nullptr;
    int64_t alive_ms_ = -1;
    int id_ = 0;

private:
    bool closed_ = false;
    std::string room_id_;
    std::string user_id_;
};

} // namespace cpp_streamer
#endif // WS_MESSAGE_SESSION_HPP