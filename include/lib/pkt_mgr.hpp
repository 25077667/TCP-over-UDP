#ifndef __PKT_MGR_HPP__
#define __PKT_MGR_HPP__

#include <stdint.h>
#include <vector>
#include <arpa/inet.h>

typedef uint64_t Seqnum;

struct Header
{
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t ofst : 4;
    uint16_t ____ : 6;
    uint16_t flag : 6;
    uint16_t rwnd;
    uint16_t chksum;
    uint16_t ugptr;

    Header() = default;
    Header(const char *begin);
    Header(Header &&) = default;
};

struct Pkt
{
    std::vector<char> data; // Contains a Header
    Pkt();                  // Reserve the header block
    void push_back(const std::vector<char> &data);
};

Header &Pkt2header(Pkt &p);

struct Pkt_wrapper
{
    int fd;
    std::vector<char> buf; // Also is a Pkt
    sockaddr_in addr;

    Pkt_wrapper() = delete;

    Pkt_wrapper(int fd, const std::vector<char> &buf, const sockaddr_in &addr) : fd{fd},
                                                                                 buf{buf},
                                                                                 addr{addr} {}
    Pkt_wrapper(int fd, const sockaddr_in &addr) : fd{fd},
                                                   addr{addr} {}
    Seqnum get_seqnum() const noexcept;
};

namespace pkt_mgr
{
    uint32_t send(Pkt_wrapper &&);
    uint32_t recv(Pkt_wrapper &) noexcept;

    void discard(uint64_t seqnum) noexcept; // Remvoe if it no need retransmit anymore.
};

#endif