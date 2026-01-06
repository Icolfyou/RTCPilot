#ifndef PORT_GENERATER_HPP
#define PORT_GENERATER_HPP
#include <utils/logger.hpp>

namespace cpp_streamer {

class PortGenerator
{
public:
    ~PortGenerator();

public:
    uint16_t GeneratePort();
    
public:
    static PortGenerator* Instance();
    static void Initialize(uint16_t start_port, uint16_t end_port, Logger* logger);

private:
    PortGenerator();

private:
    static PortGenerator* instance_;

private:
    Logger* logger_ = nullptr;
    uint16_t start_port_ = 0;
    uint16_t end_port_ = 0;
    uint16_t current_port_ = 0;
};


}

#endif// #ifndef PORT_GENERATER_HPP