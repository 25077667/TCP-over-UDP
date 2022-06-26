#ifndef __TOU_HPP__
#define __TOU_HPP__

#include <arpa/inet.h>
#include <stdint.h>
#include <cstddef>
#include <string>
#include <vector>

typedef uint64_t Seqnum;

class TCP
{
    int sock;
    sockaddr_in addr;  // Remote info
    Seqnum seqnum;
    uint16_t my_port;  // For cache

    // For accept
    TCP(int sock, const sockaddr_in &addr, Seqnum seqnum, uint16_t my_port)
        : sock{sock}, addr{addr}, seqnum{seqnum}, my_port{my_port} {};

public:
    TCP() = default;

    // For server ctor, it would bind automatically
    TCP(uint16_t port);

    // For client ctor
    TCP(const std::string &ip, uint16_t port);
    ~TCP();

    // Return a new clent's TCP
    TCP accept();

    int send(const std::vector<char> &data);
    int recv(std::vector<char> &data);
};

#endif