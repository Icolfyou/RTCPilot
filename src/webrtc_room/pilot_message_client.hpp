#ifndef PILOT_MESSAGE_CLIENT_HPP
#define PILOT_MESSAGE_CLIENT_HPP

#include "config/config.hpp"
#include "ws_message/ws_protoo_client.hpp"
#include "rtc_info.hpp"
#include "utils/logger.hpp"
#include "utils/json.hpp"
#include "utils/timeex.hpp"
#include <string>
#include <memory>
#include <map>

namespace cpp_streamer 
{
using json = nlohmann::json;

class PilotMessageClient : public WsProtooClientCallbackI, public PilotClientI
{
public:
    class PilotCallbackInfo
    {
        public:
            PilotCallbackInfo(int req_id, const std::string& method, AsyncRequestCallbackI* cb)
            {
                request_id_ = req_id;
                method_ = method;
                callback = cb;
                created_ms_ = now_millisec();
            }
        public:
            int request_id_ = 0;
            std::string method_;
            AsyncRequestCallbackI* callback;
            int64_t created_ms_ = 0;
    };

public:
    PilotMessageClient(const PilotCenterConfig& cfg, uv_loop_t* loop, Logger* logger);
    ~PilotMessageClient();

public:
    void SetAsyncNotificationCallbackI(AsyncNotificationCallbackI* cb) {
        async_notification_cb_ = cb;
    }
    
public:
    virtual void AsyncConnect() override;
    virtual int AsyncRequest(const std::string& method, json& data_json, AsyncRequestCallbackI* cb) override;
    virtual void AsyncNotification(const std::string& method, json& data_json) override;

protected: // WsProtooClientCallbackI
    virtual void OnConnected() override;
    virtual void OnResponse(const std::string& text) override;
    virtual void OnNotification(const std::string& text) override;
    virtual void OnClosed(int code, const std::string& reason) override;

private:
    PilotCenterConfig cfg_;
    uv_loop_t* loop_ = nullptr;
    Logger* logger_ = nullptr;

private:
    std::shared_ptr<WsProtooClient> ws_protoo_client_ptr_;
    bool is_connected_ = false;
    int64_t last_connecting_ms_ = -1;
    int request_id_{1};

private:
    std::map<int, PilotCallbackInfo> async_request_cbs_;
    AsyncNotificationCallbackI* async_notification_cb_ = nullptr;

private:
    std::map<int64_t, std::shared_ptr<WsProtooClient>> old_clients_;
};
}
#endif // PILOT_MESSAGE_CLIENT_HPP