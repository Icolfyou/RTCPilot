#include "ws_message_session.hpp"
#include "ws_protoo_info.hpp"
#include "webrtc_room/room_mgr.hpp"
#include "utils/timeex.hpp"
#include "utils/json.hpp"

namespace cpp_streamer {

using json = nlohmann::json;

static ProtooMessageType GetProtooMessageType(const std::string& msg) {
	try {
		nlohmann::json j = nlohmann::json::parse(msg);
		auto reqIt = j.find("request");
		if (reqIt != j.end() && reqIt->is_boolean() && reqIt->get<bool>()) {
			return PROTOO_MESSAGE_REQUEST;
		}
		auto respIt = j.find("response");
		if (respIt != j.end() && respIt->is_boolean() && respIt->get<bool>()) {
			return PROTOO_MESSAGE_RESPONSE;
		}
		auto notiIt = j.find("notification");
		if (notiIt != j.end() && notiIt->is_boolean() && notiIt->get<bool>()) {
			return PROTOO_MESSAGE_NOTIFICATION;
		}
	}
	catch (std::exception& e) {
		std::cout << "GetProtooMessageType parse exception:" << e.what() << std::endl;
		return PROTOO_MESSAGE_UNKNOWN;
	}

	return PROTOO_MESSAGE_UNKNOWN;
}

WsMessageSession::WsMessageSession(WebSocketSession* session, ProtooCallBackI* cb, Logger* logger)
    :session_(session)
    ,protoo_cb_(cb)
    ,logger_(logger)
{
    session_->SetSessionCallback(this);
    alive_ms_ = now_millisec();
    LogInfof(logger_, "WsMessageSession construct, remote addr:%s", session_->GetRemoteAddress().c_str());
}

WsMessageSession::~WsMessageSession()
{
    Clear();
}

void WsMessageSession::Clear() {
    if (session_) {
        // LogInfof(logger_, "WsMessageSession::Clear, remote addr:%s", session_->GetRemoteAddress().c_str());
        session_->SetSessionCallback(nullptr);
        session_->CloseSession();
        session_ = nullptr;//don't own the session
    }
    if (protoo_cb_) {
        protoo_cb_->OnWsSessionClose(room_id_, user_id_);
        protoo_cb_ = nullptr;
    }
}

bool WsMessageSession::IsAlive() {
    if (closed_) {
        return false;
    }
    if (!session_ || session_->IsClose()) {
        return false;
    }
    int64_t now_ms = now_millisec();
    if (alive_ms_ <= 0) {
        alive_ms_ = now_ms;
        return true;
    }
    if (alive_ms_ < session_->GetLastPongMs()) {
        alive_ms_ = session_->GetLastPongMs();
    }

    if (now_ms - alive_ms_ > 30000) {//30s no data
        LogInfof(logger_, "WsMessageSession::IsAlive false for no data timeout, addr:%s",
            session_->GetRemoteAddress().c_str());
        return false;
    }
    return true;
}

void WsMessageSession::OnReadData(int code, const uint8_t* data, size_t len)
{
    if (code < 0) {
        closed_ = true;
        LogErrorf(logger_, "WsMessageSession::OnReadData error code:%d", code);
        return;
    }
    alive_ms_ = now_millisec();
    LogInfof(logger_, "WsMessageSession::OnReadData, addr:%s, data len:%zu",
        session_->GetRemoteAddress().c_str(), len);
}

void WsMessageSession::OnReadText(int code, const std::string& text) {
    if (code < 0) {
        closed_ = true;
        LogErrorf(logger_, "WsMessageSession::OnReadText error code:%d", code);
        return;
    }
    alive_ms_ = now_millisec();
    LogDebugf(logger_, "WsMessageSession::OnReadText, addr:%s, text len:%zu, text:%s",
        session_->GetRemoteAddress().c_str(), text.length(), text.c_str());
    auto protoo_msg_type = GetProtooMessageType(text);
    
    switch(protoo_msg_type) {
        case PROTOO_MESSAGE_REQUEST: {
            try {
                json j = json::parse(text);
                auto idIt = j.find("id");
                auto methodIt = j.find("method");
                if (idIt == j.end() || !idIt->is_number_integer() ||
                    methodIt == j.end() || !methodIt->is_string()) {
                    LogErrorf(logger_, "WsMessageSession::OnReadText invalid protoo request message");
                    return;
                }
                int id = idIt->get<int>();
                std::string method = methodIt->get<std::string>();
                if (protoo_cb_) {
                    protoo_cb_->OnProtooRequest(id, method, j, this);
                }
            }
            catch (std::exception& e) {
                LogErrorf(logger_, "WsMessageSession::OnReadText parse protoo request exception:%s", e.what());
            }
            break;
        }
        case PROTOO_MESSAGE_NOTIFICATION: {
            try {
                json j = json::parse(text);
                auto methodIt = j.find("method");
                if (methodIt == j.end() || !methodIt->is_string()) {
                    LogErrorf(logger_, "WsMessageSession::OnReadText invalid protoo notification message");
                    return;
                }
                std::string method = methodIt->get<std::string>();
                if (protoo_cb_) {
                    protoo_cb_->OnProtooNotification(method, j);
                }
            }
            catch (std::exception& e) {
                LogErrorf(logger_, "WsMessageSession::OnReadText parse protoo notification exception:%s", e.what());
            }
            break;
        }
        case PROTOO_MESSAGE_RESPONSE: {
            try {
                json j = json::parse(text);
                auto idIt = j.find("id");
                if (idIt == j.end() || !idIt->is_number_integer()) {
                    LogErrorf(logger_, "WsMessageSession::OnReadText invalid protoo response message");
                    return;
                }
                int id = idIt->get<int>();
                int code = 0;
                std::string err_msg;
                auto okIt = j.find("ok");

                if (okIt == j.end() || !okIt->is_boolean()) {
                    LogErrorf(logger_, "WsMessageSession::OnReadText invalid protoo response message no ok field");
                    return;
                }
                bool ok = okIt->get<bool>();
                if (!ok) {
                    auto errorCodeIt = j.find("errorCode");
                    if (errorCodeIt != j.end() && errorCodeIt->is_number_integer()) {
                        code = errorCodeIt->get<int>();
                    }
                    auto errorReasonIt = j.find("errorReason");
                    if (errorReasonIt != j.end() && errorReasonIt->is_string()) {
                        err_msg = errorReasonIt->get<std::string>();
                    }
                    if (protoo_cb_) {
                        protoo_cb_->OnProtooResponse(id, code,  err_msg, j);
                    }
                    return;
                }
                if (protoo_cb_) {
                    protoo_cb_->OnProtooResponse(id, 0, "", j);
                }
            }
            catch (std::exception& e) {
                LogErrorf(logger_, "WsMessageSession::OnReadText parse protoo response exception:%s", e.what());
            }
            break;
        }
        default:
        {
            LogErrorf(logger_, "not handle protoo message type:%d", protoo_msg_type);
        }
    }
}

void WsMessageSession::OnClose(int code, const std::string& desc) {
    if (closed_) {
        return;
    }
    closed_ = true;
    LogInfof(logger_, "WsMessageSession::OnClose, addr:%s, code:%d, desc:%s",
        session_->GetRemoteAddress().c_str(), code, desc.c_str());
    if (protoo_cb_) {
        protoo_cb_->OnWsSessionClose(room_id_, user_id_);
        protoo_cb_ = nullptr;
    }
}

void WsMessageSession::OnProtooResponse(ProtooResponse& resp) {
    // Handle Protoo response if needed
    try {
        json j = resp.ToJson();
        LogDebugf(logger_, "send response:%s", j.dump().c_str());
        session_->AsyncWriteText(j.dump());
    } catch(const std::exception& e) {
        LogErrorf(logger_, "WsMessageSession::OnProtooResponse exception:%s", e.what());
    }
}

void WsMessageSession::Request(const std::string& method, json& j) {
    try {
        json req_json = json::object();
        req_json["request"] = true;
        req_json["method"] = method;
        req_json["id"] = ++id_;
        req_json["data"] = j;

        LogDebugf(logger_, "ws send protoo request:%s", req_json.dump().c_str());
        session_->AsyncWriteText(req_json.dump());
    } catch(const std::exception& e) {
        LogErrorf(logger_, "WsMessageSession::Request exception:%s", e.what());
    }
}

void WsMessageSession::Notification(const std::string& method, json& j) {
    try {
        json req_json = json::object();
        req_json["notification"] = true;
        req_json["method"] = method;
        req_json["data"] = j;
        LogDebugf(logger_, "ws send protoo notification:%s", req_json.dump().c_str());
        session_->AsyncWriteText(req_json.dump());
    } catch(const std::exception& e) {
        LogErrorf(logger_, "WsMessageSession::Notification exception:%s", e.what());
    }
}

void WsMessageSession::SetUserInfo(const std::string& room_id, const std::string& user_id) {
    room_id_ = room_id;
    user_id_ = user_id;
    LogInfof(logger_, "websocket setup session for user_id:%s, room_id:%s",
        user_id.c_str(), room_id.c_str());
}

} // namespace cpp_streamer
