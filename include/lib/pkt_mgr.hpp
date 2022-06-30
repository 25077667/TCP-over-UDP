#ifndef __PKT_MGR_HPP__
#define __PKT_MGR_HPP__

#include <arpa/inet.h>
#include <stdint.h>
#include <vector>

typedef uint64_t Seqnum;

enum PacketType {
    SYN = 0b010000,
    ACK = 0b000010,
    PSH = 0b000100,
    FIN = 0b100000
};

struct Header {
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
    explicit Header(const char *begin);
    Header(Header &&) = default;
    ~Header() = default;
};

struct Pkt {
    std::vector<char> data;  // Contains a Header
    Pkt();                   // Reserve the header block
    explicit Pkt(std::vector<char> &&);
    void push_back(const std::vector<char> &data);
};

struct Pkt_wrapper {
    int fd;
    std::vector<char> buf;  // Also is a Pkt
    sockaddr_in addr;

    Pkt_wrapper() = delete;

    Pkt_wrapper(int fd, const std::vector<char> &buf, const sockaddr_in &addr)
        : fd{fd}, buf{buf}, addr{addr}
    {
    }
    Pkt_wrapper(int fd, const sockaddr_in &addr, int reserved_size = 0)
        : fd{fd}, buf{std::vector<char>(reserved_size)}, addr{addr}
    {
    }
    explicit Pkt_wrapper(int fd) : fd{fd} {}
    Seqnum get_seqnum() const noexcept;
};

Header &Pkt2header(Pkt &p);
Header &Pkt2header(Pkt_wrapper &p);
Header &Pkt2header(std::vector<char> &p);

namespace pkt_mgr
{
uint32_t send(Pkt_wrapper &&);
uint32_t recv(Pkt_wrapper &) noexcept;

void discard(
    uint64_t seqnum) noexcept;  // Remove if it no need retransmit anymore.
};                              // namespace pkt_mgr

#endif