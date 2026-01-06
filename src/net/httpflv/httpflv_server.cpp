#include "httpflv_server.hpp"
#include "httpflv_writer.hpp"
#include "media_stream_manager.hpp"
#include "utils/uuid.hpp"

#include "logger.hpp"
#include "uuid.hpp"
#include <string>
#include <unordered_map>

namespace cpp_streamer
{
    std::unordered_map<std::string, HttpFlvWriter*> s_httpflv_handle_map;
    
    void HttpFlvHandle(const HttpRequest* request, std::shared_ptr<HttpResponse> response_ptr) {
        Logger* logger = response_ptr->GetLogger();

        auto pos = request->uri_.find(".flv");
        if (pos == std::string::npos) {
            LogErrorf(logger, "http flv request uri error:%s", request->uri_.c_str());
            //TODO: write response 404
            return;
        }
        std::string key = request->uri_.substr(0, pos);
        if (key[0] == '/') {
            key = key.substr(1);
        }


        std::string uuid = UUID::MakeUUID2();

        LogInfof(logger, "http flv request key:%s, uuid:%s", key.c_str(), uuid.c_str());

        HttpFlvWriter* writer_p = new HttpFlvWriter(key, uuid, response_ptr, logger);

        s_httpflv_handle_map.insert(std::make_pair(uuid, writer_p));

        MediaStreamManager::AddPlayer(writer_p);

        return;
    }
    
    HttpFlvServer::HttpFlvServer(uv_loop_t* loop, const std::string& ip, uint16_t port, Logger* logger):TimerInterface(100)
        , server_(loop, ip, port, logger)
		, logger_(logger)
    {
        Run();
        StartTimer();
        LogInfof(logger_, "http flv server is listen on %s:%d", ip.c_str(), port);
    }

    HttpFlvServer::~HttpFlvServer()
    {
        StopTimer();
    }

    void HttpFlvServer::Run() {
		server_.AddGetHandle("/", HttpFlvHandle);
        return;
    }

    bool HttpFlvServer::OnTimer() {
        OnCheckAlive();
        return timer_running_;
    }

    void HttpFlvServer::OnCheckAlive() {
        auto iter = s_httpflv_handle_map.begin();

        while (iter != s_httpflv_handle_map.end()) {
            HttpFlvWriter* writer_p = iter->second;
            bool is_alive = writer_p->IsAlive();
            if (!is_alive) {
                MediaStreamManager::RemovePlayer(writer_p);
                s_httpflv_handle_map.erase(iter++);
                delete writer_p;
                continue;
            }
            iter++;
        }
        return;
    }

}

