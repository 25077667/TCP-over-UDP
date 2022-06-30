#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <stdint.h>
#include <functional>
#include <tou.hpp>

using Routine = std::function<int(int)>;

class Client
{
    TCP sock;
    Routine task;

public:
    Client() = delete;
    explicit Client(uint32_t ip, uint16_t port = 4095);
    ~Client() = default;

    void set_task(const Routine &f_);
    void run();
};

#endif