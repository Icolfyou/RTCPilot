#include "co_rtmp_session.hpp"
#include "co_chunk_stream.hpp"
#include "co_rtmp_writer.hpp"
#include "net/tcp/co_tcp/co_tcp_server/co_tcp_accept_conn.hpp"
#include "utils/av/media_stream_manager.hpp"
#include "utils/stringex.hpp"

namespace cpp_streamer
{

CoRtmpSession::CoRtmpSession(const std::string& id, std::shared_ptr<TcpCoAcceptConn> conn_ptr, Logger* logger)
    : CoRtmpSessionBase(logger)
	, id_(id)
	, conn_ptr_(conn_ptr)
    , logger_(logger)
    , hs_(logger)
    , control_handler_(this, logger)
{
    MediaStreamManager::SetLogger(logger);
    running_ = true;
}

CoRtmpSession::~CoRtmpSession()
{
}

bool CoRtmpSession::IsAlive() {
	if (!running_) {
		LogInfof(logger_, "RTMP session %s is closed.", id_.c_str());
		return false;
	}

	if (alive_count_++ > 5) {
		LogInfof(logger_, "RTMP session %s is not alive, count: %d", id_.c_str(), alive_count_);
        return false;
	}
    return true;
}

void CoRtmpSession::Close() {
    if (closed_flag_) {
        return;
    }
    closed_flag_ = true;
    running_ = false;
    LogInfof(logger_, "rtmp session close, request isReady:%s, action:%s", BOOL2STRING(req_.is_ready_), BOOL2STRING(req_.publish_flag_));

    if (req_.is_ready_ && !req_.publish_flag_) {
        if (play_writer_) {
            MediaStreamManager::RemovePlayer(play_writer_);
            delete play_writer_;
        }
    }
    else {
        if (!req_.key_.empty()) {
            MediaStreamManager::RemovePublisher(req_.key_);
        }
    }

    conn_ptr_->Close();
}

CoVoidTask CoRtmpSession::Run() {
    const size_t recv_data_size = 10 * 1024;
    std::vector<uint8_t> recv_data(recv_data_size, 0);

	LogInfof(logger_, "RTMP session %s is starting to run.", id_.c_str());
    while (running_) {
        RecvResult result = co_await conn_ptr_->CoReceive(recv_data.data(), recv_data_size);
        if (result.status != RECV_OK || result.recv_len <= 0) {
            LogErrorf(logger_, "Failed to receive data, status: %d, length: %d", result.status, result.recv_len);
            running_ = false;
            break;
        }
        alive_count_ = 0;
        recv_buffer_.AppendData((char*)recv_data.data(), result.recv_len);

        HandleRequest();
    }
	LogInfof(logger_, "RTMP session %s has stopped running.", id_.c_str());

    co_return;
}

CoTask<int> CoRtmpSession::HandleRequest() {
    int ret = RTMP_OK;

    if (phase_ == initial_phase) {
        LogInfof(logger_, "RTMP session is in initial phase, handling C0C1 handshake.");
        ret = hs_.HandleC0C1(recv_buffer_);
        if (ret < 0) {
            LogErrorf(logger_, "handle C0C1 error, ret: %d", ret);
            running_ = false;
            co_return ret;
        }
        if (ret == RTMP_NEED_READ_MORE) {
            LogInfof(logger_, "need read more data for C0C1");
            co_return ret;
        }
        if (ret == RTMP_SIMPLE_HANDSHAKE) {
            LogInfof(logger_, "try to rtmp handshake in simple mode");
        }
        recv_buffer_.Reset();

        std::vector<uint8_t> s0s1s2;
        ret = hs_.SendS0S1S2(s0s1s2);
        if (ret < 0) {
            LogErrorf(logger_, "Failed to send S0S1S2, ret: %d", ret);
            running_ = false;
            co_return ret;
        }
		LogInfof(logger_, "RTMP session S0S1S2 data size: %zu", s0s1s2.size());

        SendResult result = co_await conn_ptr_->CoSend(s0s1s2.data(), s0s1s2.size());
        if (result.status != SEND_OK) {
            LogErrorf(logger_, "Failed to send S0S1S2, status: %d", result.status);
            co_return -1;
        }
        phase_ = handshake_c2_phase;
    } else if (phase_ == handshake_c2_phase) {
        LogInfof(logger_, "RTMP session is in handshake C2 phase, handling C2 handshake.");
        ret = hs_.HandleC2(recv_buffer_);
        if (ret < 0) {
            LogErrorf(logger_, "handle C2 error, ret: %d", ret);
            running_ = false;
            co_return ret;
        }
        if (ret == RTMP_NEED_READ_MORE) {
            LogInfof(logger_, "need read more data for C2");
            co_return ret;
        }
        phase_ = connect_phase;
        LogInfof(logger_, "RTMP session phase changed to connect_phase, recv buffer len:%zu",
                recv_buffer_.DataLen());
        if (recv_buffer_.DataLen() == 0) {
            LogInfof(logger_, "RTMP session has no data in buffer");
            co_return RTMP_NEED_READ_MORE;
        }
    } else if (phase_ >= connect_phase) {
        ret = ReceiveChunkStream();
        if (ret < 0) {
            running_ = false;
            co_return ret;
        }

        if (ret == RTMP_OK) {
            ret = RTMP_NEED_READ_MORE;
        }
    }
    co_return ret;
}

int CoRtmpSession::ReceiveChunkStream() {
    CHUNK_STREAM_PTR cs_ptr;
    int ret = -1;

    while (true) {
        ret = ReadChunkStream(cs_ptr);
        if ((ret < RTMP_OK) || (ret == RTMP_NEED_READ_MORE)) {
            return ret;
        }
        //check whether chunk stream is ready(data is full)
        if (!cs_ptr || !cs_ptr->IsReady()) {
            if (recv_buffer_.DataLen() > 0) {
                continue;
            }
            return RTMP_NEED_READ_MORE;
        }
        //check whether we need to send rtmp control ack
        //(void)send_rtmp_ack(cs_ptr->chunk_data_ptr_->data_len());
        //Todo: implement RTMP control ack

        if ((cs_ptr->type_id_ >= RTMP_CONTROL_SET_CHUNK_SIZE) && (cs_ptr->type_id_ <= RTMP_CONTROL_SET_PEER_BANDWIDTH)) {
            ret = control_handler_.HandleRtmpControlMessage(cs_ptr, true);
            if (ret < RTMP_OK) {
                cs_ptr->Reset();
                LogErrorf(logger_, "handle rtmp control message error, ret: %d", ret);
                return ret;
            }
            cs_ptr->Reset();
            if (recv_buffer_.DataLen() > 0) {
                continue;
            }
            break;
        } else if (cs_ptr->type_id_ == RTMP_COMMAND_MESSAGES_AMF0) {
            std::vector<AMF_ITERM*> amf_vec;
            ret = control_handler_.HandleClientCommandMessage(cs_ptr, amf_vec);
            if (ret < RTMP_OK) {
                for (auto iter : amf_vec) {
                    AMF_ITERM* temp = iter;
                    delete temp;
                }
                cs_ptr->Reset();
                LogErrorf(logger_, "handle client command message error, ret: %d", ret);
                return ret;
            }
            for (auto iter : amf_vec) {
                AMF_ITERM* temp = iter;
                delete temp;
            }
            cs_ptr->Reset();
            if (req_.is_ready_ && !req_.publish_flag_) {
                // rtmp play is ready
                LogInfof(logger_, "RTMP play is ready, stream name: %s", req_.stream_name_.c_str());
                play_writer_ = new RtmpWriter(this, logger_);
                MediaStreamManager::AddPlayer(play_writer_);
            }
            if (recv_buffer_.DataLen() > 0) {
                continue;
            }
            break;
        } else if (cs_ptr->type_id_ == RTMP_COMMAND_MESSAGES_AMF3) {
            LogErrorf(logger_, "RTMP AMF3 command messages are not supported yet, type_id: %d", cs_ptr->type_id_);
            cs_ptr->Reset();
            return -1;
        } else if ((cs_ptr->type_id_ == RTMP_MEDIA_PACKET_VIDEO) || (cs_ptr->type_id_ == RTMP_MEDIA_PACKET_AUDIO)
                || (cs_ptr->type_id_ == RTMP_COMMAND_MESSAGES_META_DATA0) || (cs_ptr->type_id_ == RTMP_COMMAND_MESSAGES_META_DATA3)) {
            Media_Packet_Ptr pkt_ptr = GetMediaPacket(cs_ptr);
            if (!pkt_ptr) {
                LogErrorf(logger_, "Failed to get media packet from chunk stream, type_id: %d", cs_ptr->type_id_);
                cs_ptr->Reset();
                return -1;
            }
			MediaStreamManager::WriterMediaPacket(pkt_ptr);
            
            cs_ptr->Reset();
            if (recv_buffer_.DataLen() > 0) {
                continue;
            }
        } else {
            LogErrorf(logger_, "Unsupported chunk stream type, type_id: %d", cs_ptr->type_id_);
            cs_ptr->Reset();
            if (recv_buffer_.DataLen() > 0) {
                continue;
            }
        }
    }
    return 0;
}

DataBuffer* CoRtmpSession::GetRecvBuffer() {
    return &recv_buffer_;
}

int CoRtmpSession::RtmpSend(char* data, int len) {
    if (!running_) {
        LogInfof(logger_, "RtmpSend error for running is false");
        return -1;
    }
    alive_count_ = 0;
    auto ret = CoRtmpSend(data, len);
    (void)ret; // Suppress unused variable warning
    return len;
}

int CoRtmpSession::RtmpSend(std::shared_ptr<DataBuffer> data_ptr) {
    if (!running_) {
        LogInfof(logger_, "RtmpSend error for running is false");
        return -1;
    }
    alive_count_ = 0;
    return RtmpSend(data_ptr->Data(), static_cast<int>(data_ptr->DataLen()));
}

CoTask<int> CoRtmpSession::CoRtmpSend(char* data, int len) {
    if (len <= 0) {
        co_return -1; // Invalid length
    }
    int total = len;
    char* p = data;
    while (total > 0) {
        SendResult result = co_await conn_ptr_->CoSend((uint8_t*)p, static_cast<size_t>(total));
        if (result.status != SEND_OK || result.sent_len <= 0) {
            LogErrorf(logger_, "Failed to send data, status: %d, length: %d", result.status, result.sent_len);
            running_ = false;
            co_return -1; // Sending failed
        }
		p += result.sent_len;
		total -= result.sent_len;
    }

    co_return len;
}

std::string CoRtmpSession::GetSessonKey() {
    if (!conn_ptr_) {
        return "";
    }
    return conn_ptr_->GetRemoteEndpoint();
}

}