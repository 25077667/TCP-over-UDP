#include <pkt_mgr.hpp>
#include <tou.hpp>
#include <util.hpp>

#include <fcntl.h>
#include <cstring>

#ifdef DEBUG
#pragma message("Debug mode")
#include <iostream>
#endif

constexpr int DEFAULT_LISTEN_NUM = 5;

namespace
{
// You should have already binded the socket
uint16_t get_port(int sock)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    auto f = std::bind(::getsockname, sock, (sockaddr *) &addr, &len);

    util::try_exec("getsockname", f, sock);

    return addr.sin_port;
}

// Return the port number which is binded
uint16_t bind_anony_port(int sock)
{
    auto addr = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = 0,
        .sin_addr = INADDR_ANY,
        .sin_zero = {0},
    };

    auto binding = std::bind(::bind, sock, (sockaddr *) (&addr), sizeof(addr));
    util::try_exec("bind", binding, 0);

    return get_port(sock);
}

};  // namespace

namespace hand_shake
{
Pkt_wrapper fetch_pkt(int sock)
{
    auto p = Pkt_wrapper(sock);
    pkt_mgr::recv(p);

    Pkt_wrapper ret(sock);
    ret.addr = p.addr;
    ret.buf.swap(p.buf);
    return ret;
}

Pkt_wrapper fetch_pkt(int sock, const sockaddr_in &addr)
{
    auto p = Pkt_wrapper(sock, addr);
    pkt_mgr::recv(p);

    Pkt_wrapper ret(sock, addr);
    ret.buf.swap(p.buf);
    return ret;
}
};  // namespace hand_shake

// Bind the 0.0.0.0
TCP::TCP(uint16_t port)
    : sock{::socket(AF_INET, SOCK_DGRAM, 0)},
      addr{
          .sin_family = AF_INET,
          .sin_port = ::htons(port),
          .sin_addr = 0,
          .sin_zero = {0},
      },
      my_port{port}
{
    auto binding = std::bind(::bind, this->sock, (sockaddr *) (&this->addr),
                             sizeof(sockaddr_in));
    util::try_exec("bind", binding, port);
}

TCP::TCP(const std::string &ip, uint16_t port)
    : sock{::socket(AF_INET, SOCK_DGRAM, 0)},
      addr{
          .sin_family = AF_INET,
          .sin_port = ::htons(port),
          .sin_addr = ::inet_addr(ip.c_str()),
          .sin_zero = {0},
      },
      my_port{bind_anony_port(this->sock)}
{
    // Do the 3-way handshaking

    // SYN
    auto syn = Pkt_wrapper(sock, addr, sizeof(Header));
    auto &syn_header = Pkt2header(syn);
    syn_header.src_port = my_port;
    syn_header.dst_port = addr.sin_port;
    syn_header.seq = util::get_rand();
    syn_header.ack = 0;
    syn_header.ofst = 0;
    syn_header.____ = 0;
    syn_header.flag = PacketType::SYN;
    syn_header.rwnd = 0;
    syn_header.chksum = 0;
    syn_header.ugptr = 0;
    pkt_mgr::send(std::move(syn));

#ifdef DEBUG
    std::clog << "Client sent SYN to "
              << util::big_endian::ip(syn.addr.sin_addr.s_addr) << ":"
              << util::big_endian::port(syn.addr.sin_port)
              << " , which seq :" << std::to_string(syn_header.seq)
              << std::endl;
#endif

    // SYN-ACK
    auto syn_ack = hand_shake::fetch_pkt(sock, addr);
    const auto &remote_h = Pkt2header(syn_ack);
    pkt_mgr::discard(syn_header.seq);  // Discard syn

#ifdef DEBUG
    std::clog << "Client got SYN-ACK from "
              << util::big_endian::ip(syn_ack.addr.sin_addr.s_addr) << ":"
              << util::big_endian::port(syn_ack.addr.sin_port)
              << " , which seq: " << std::to_string(remote_h.seq) << std::endl;
#endif

    // ACK
    auto ack = Pkt_wrapper(sock, addr, sizeof(Header));
    auto &ack_h = Pkt2header(ack);
    ack_h.src_port = my_port;
    ack_h.dst_port = addr.sin_port;
    ack_h.seq = remote_h.ack + 1;
    ack_h.ack = remote_h.seq;
    ack_h.ofst = 0;
    ack_h.____ = 0;
    ack_h.flag = PacketType::ACK;
    ack_h.rwnd = 0;
    ack_h.chksum = 0;
    ack_h.ugptr = 0;
    pkt_mgr::send(std::move(ack));

#ifdef DEBUG
    std::clog << "Client sent ACK to "
              << util::big_endian::ip(ack.addr.sin_addr.s_addr) << ":"
              << util::big_endian::port(ack.addr.sin_port)
              << " , which seq :" << std::to_string(ack_h.seq) << std::endl;
#endif

    // Discard ACK immediately
    pkt_mgr::discard(ack_h.seq);
}

TCP::~TCP()
{
    if (this->addr.sin_addr.s_addr == 0)
        close(this->sock);  // If I have binded anything, I have to close it.
}

TCP TCP::accept()
{
    // 3-way handshaking
    // Got SYN
    auto syn = hand_shake::fetch_pkt(sock);
    const auto &remote_h = Pkt2header(syn);

#ifdef DEBUG
    std::clog << "Server got SYN from "
              << util::big_endian::ip(syn.addr.sin_addr.s_addr) << ":"
              << util::big_endian::port(syn.addr.sin_port)
              << " , which seq: " << std::to_string(remote_h.seq) << std::endl;
#endif

    // Gen SYN-ACK
    Pkt_wrapper syn_ack(sock, syn.addr, sizeof(Header));
    auto &syn_ack_h = Pkt2header(syn_ack);
    syn_ack_h.src_port = my_port;
    syn_ack_h.dst_port = remote_h.src_port;
    syn_ack_h.seq = util::get_rand();  // new seq num
    syn_ack_h.ack = remote_h.seq;
    syn_ack_h.ofst = 0;
    syn_ack_h.____ = 0;
    syn_ack_h.flag = PacketType::SYN | PacketType::ACK;
    syn_ack_h.rwnd = 0;
    syn_ack_h.chksum = 0;
    syn_ack_h.ugptr = 0;
    pkt_mgr::send(std::move(syn_ack));

#ifdef DEBUG
    std::clog << "Sever sent SYN-ACK to "
              << util::big_endian::ip(syn_ack.addr.sin_addr.s_addr) << ":"
              << util::big_endian::port(syn_ack.addr.sin_port)
              << " , witch seq: " << std::to_string(syn_ack_h.seq) << std::endl;
#endif

    // Got ACK
    auto ack = hand_shake::fetch_pkt(sock, syn.addr);
    pkt_mgr::discard(syn_ack_h.seq);  // Discard the syn-ack pkt

#ifdef DEBUG
    const auto &ack_h = Pkt2header(ack);
    std::clog << "Server got ACK from "
              << util::big_endian::ip(ack.addr.sin_addr.s_addr) << ":"
              << util::big_endian::port(ack.addr.sin_port)
              << " , which seq: " << std::to_string(ack_h.seq) << std::endl;
#endif

    return TCP(sock, syn.addr, syn_ack_h.seq + 1, my_port);
}

int TCP::send(const std::vector<char> &data)
{
    Pkt p;
    p.push_back(data);
    return pkt_mgr::send(Pkt_wrapper(this->sock, p.data, this->addr));
}

int TCP::recv(std::vector<char> &data)
{
    auto p = Pkt_wrapper(this->sock, this->addr);
    p.buf.swap(data);
    auto ret = pkt_mgr::recv(p);
    p.buf.swap(data);

    return ret;
}