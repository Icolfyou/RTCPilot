#include "port_generator.hpp"

namespace cpp_streamer {
PortGenerator* PortGenerator::instance_ = nullptr;

PortGenerator::PortGenerator() {

}

PortGenerator::~PortGenerator() {

}

PortGenerator* PortGenerator::Instance() {
    if (instance_ == nullptr) {
        instance_ = new PortGenerator();
    }
    return instance_;
}

void PortGenerator::Initialize(uint16_t start_port, uint16_t end_port, Logger* logger) {
    PortGenerator* pg = PortGenerator::Instance();
    pg->start_port_ = start_port;
    pg->end_port_ = end_port;
    pg->current_port_ = start_port;
    pg->logger_ = logger;
}

uint16_t PortGenerator::GeneratePort() {
    if (current_port_ > end_port_) {
        current_port_ = start_port_;
    }
    uint16_t port = current_port_;
    current_port_++;
    LogDebugf(logger_, "GeneratePort: %u", port);
    return port;
}

} // namespace cpp_streamer
