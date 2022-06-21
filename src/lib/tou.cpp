#include <tou.hpp>
#include <ipaddr.hpp>
#include <pkt_mgr.hpp>

#include <functional>
#include <fcntl.h>
#include <cstring>
#include <numeric>
#include <stdexcept>
#include <array>

#define FMT_HEADER_ONLY
#include <fmt/core.h>

constexpr int DEFAULT_LISTEN_NUM = 5;

namespace
{
    namespace detail
    {
        template <auto N>
        consteval auto to_array(const char (&l)[N])
        {
            std::array<char, N> ret{};
            std::copy_n(l, N, ret.begin());
            return ret;
        }

        template <auto N, auto M, typename... Rest>
        consteval auto cat_static_str(const std::array<char, N> a, const std::array<char, M> b, Rest... rest)
        {
            // Take care of the null char
            std::array<char, N + M - 1> ret{};
            std::copy(a.cbegin(), a.cend() - 1, ret.begin());
            std::copy(b.cbegin(), b.cend(), ret.begin() + N - 1);
            if constexpr (sizeof...(Rest))
                return cat_static_str(ret, rest...);
            else
                return ret;
        }

        template <typename Arg>
        consteval auto fmtstr_ph_gen()
        {
            return to_array("{} ");
        }

        template <typename... Args>
        consteval auto err_fmtstr_gen()
        {
            return cat_static_str(
                to_array("{} to "),
                detail::fmtstr_ph_gen<Args>()...,
                to_array("failed: {}"));
        }
    };

    template <typename... Args>
    void try_exec(const char *func_name, std::function<int(void)> to_be_exec, Args &&...args)
    {
        int _ = to_be_exec();
        if (_ == -1) [[unlikely]]
        {
            static constexpr auto to_be_gen = detail::err_fmtstr_gen<Args...>();
            throw std::runtime_error(fmt::format(to_be_gen.data(), func_name, args..., std::strerror(_)));
        }
    }

};

// Bind the 0.0.0.0
TCP::TCP(uint16_t port)
    : ip{0},
      port{::htons(port)},
      sock{::socket(AF_INET, SOCK_DGRAM, 0)},
      addr{
          .sin_family = AF_INET,
          .sin_port = this->port,
          .sin_addr = this->ip,
          .sin_zero = {0},
      }
{
    auto binding = std::bind(::bind, this->sock, (sockaddr *)(&this->addr), sizeof(sockaddr_in));
    try_exec("bind", binding, port);
}

TCP::TCP(const std::string &ip, uint16_t port)
    : ip{::inet_addr(ip.c_str())},
      port{::htons(port)},
      sock{::socket(AF_INET, SOCK_DGRAM, 0)},
      addr{
          .sin_family = AF_INET,
          .sin_port = this->port,
          .sin_addr = this->ip,
          .sin_zero = {0},
      }
{
}

TCP::~TCP()
{
    if (this->ip == 0) // If I have binded anything, I have to close it.
        close(this->sock);
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