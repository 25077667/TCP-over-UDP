#include <tou.hpp>
#include <ipaddr.hpp>

#include <sys/timerfd.h>
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
        constexpr std::string fmtstr_ph_gen(auto &&arg)
        {
            (void)arg; // ignore type
            return "{} ";
        }

        template <typename... Args>
        constexpr std::string fmtstr_ph_gen(Args &&...args)
        {
            auto s = "{} " + fmtstr_ph_gen(std::forward<Args &&>(args)...);
            return s;
        }

        template <typename... Args>
        constexpr auto err_fmtstr_gen(Args &&...args)
        {
            auto s = "{} to " +
                     fmtstr_ph_gen(std::forward<Args &&>(args)...) +
                     "failed: {}";
            return s;
        }
    };

    template <typename... Args>
    void try_exec(const char *func_name, std::function<int(void)> to_be_exec, Args... args)
    {
        int _ = to_be_exec();
        if (_ == -1)
        {
            std::string_view efs = detail::err_fmtstr_gen(std::forward<Args &&>(args)...);
            throw std::runtime_error(fmt::format(efs, func_name, std::forward<Args &&>(args)..., std::strerror(_)));
        }
    }
};

// Bind the 0.0.0.0
TCP::TCP(uint16_t port)
    : ip{0},
      port{::htons(port)},
      sock{::socket(AF_INET, SOCK_DGRAM, 0)},
      timerfd{::timerfd_create(CLOCK_REALTIME, O_NONBLOCK)},
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
      timerfd{::timerfd_create(CLOCK_REALTIME, O_NONBLOCK)},
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
    close(this->timerfd);
}

int TCP::send(const std::vector<char> &data)
{
    return ::sendto(this->sock, data.data(), data.size(), 0, (sockaddr *)&this->addr, sizeof(sockaddr_in));
}

int TCP::recv(std::vector<char> &data)
{
    socklen_t addr_len = sizeof(sockaddr_in);
    return ::recvfrom(this->sock, data.data(), data.size(), 0, (sockaddr *)&this->addr, &addr_len);
}