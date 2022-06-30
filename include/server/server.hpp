#ifndef __SERVER_HPP__
#define __SERVER_HPP__

#include <stdint.h>
#include <functional>
#include <memory>
#include <tou.hpp>
#include <vector>

using Routine = std::function<int(int)>;

class Server
{
    std::vector<TCP> sock;
    Routine task;
    std::unique_ptr<TCP> accept();

public:
    Server() : Server(4095){};
    ~Server() = default;
    explicit Server(uint16_t port);

    void set_task(const Routine &f_);
    void run();
};

#endif