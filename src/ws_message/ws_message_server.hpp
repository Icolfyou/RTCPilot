#ifndef WS_MESSAGE_SERVER_HPP
#define WS_MESSAGE_SERVER_HPP

#include "net/http/websocket/websocket_server.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include <memory>
#include <string>
#include <stdint.h>
#include <map>

namespace cpp_streamer {
class WebSocketSession;
class WsMessageSession;
class WsMessageServer : public TimerInterface
{
    friend void OnWSMessageSessionHandle(const std::string& uri, WebSocketSession* session);
public:
    WsMessageServer(const std::string& ip, uint16_t port, uv_loop_t* loop, Logger* logger);
    WsMessageServer(const std::string& ip, 
        uint16_t port, 
        uv_loop_t* loop, 
        const std::string& key_file, 
        const std::string& cert_file, 
        Logger* logger);
    virtual ~WsMessageServer();
protected:
    virtual bool OnTimer() override;
private:
    Logger* logger_ = nullptr;
    std::unique_ptr<WebSocketServer> ws_server_ptr_ = nullptr;
private:
    static std::map<std::string, std::shared_ptr<WsMessageSession>> ws_message_sessions;
};
}
#endif // WS_MESSAGE_SERVER_HPP