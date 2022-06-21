#ifndef __TOU_HPP__
#define __TOU_HPP__

#include <stdint.h>
#include <cstddef>
#include <string>
#include <vector>
#include <arpa/inet.h>

class TCP
{
    uint32_t ip;   // converted
    uint16_t port; // converted
    int sock;
    sockaddr_in addr;

public:
    TCP() = default;

    // For server ctor, it would bind automatically
    TCP(uint16_t port);

    // For client ctor
    TCP(const std::string &ip, uint16_t port);
    ~TCP();

    int send(const std::vector<char> &data);
    int recv(std::vector<char> &data);
};

#endif