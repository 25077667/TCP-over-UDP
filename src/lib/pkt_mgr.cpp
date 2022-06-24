#include <pkt_mgr.hpp>

#include <memory>
#include <unordered_map>
#include <chrono>
#include <array>
#include <fmt/core.h>
#include <cstring>

#include <sys/timerfd.h>
#include <sys/epoll.h>

// TODO: Use io_uring to impl aio

typedef int Timer_fd;

namespace
{
    template <typename T1, typename T2>
    class bi_unordered_map // Bidirectional map
    {
        std::unordered_map<T1, T2> k2v;
        std::unordered_map<T2, T1> v2k;

    public:
        void intset(T1 k, T2 v)
        {
            k2v.insert(std::make_pair(k, v));
            v2k.insert(std::make_pair(v, k));
        }

        T2 get(T1 k) const
        {
            return k2v.find(k)->second;
        }

        T1 get(T2 v) const
        {
            return v2k.find(v)->second;
        }

        void discard(T1 k)
        {
            auto [key, value] = *k2v.find(k);
            v2k.erase(value);
            k2v.erase(key);
        }

        void discard(T2 v)
        {
            auto [key, value] = *v2k.find(v);
            k2v.erase(key);
            v2k.erase(value);
        }

        T2 &operator[](T1 k)
        {
            return k2v[k];
        }

        T1 &operator[](T2 v)
        {
            return v2k[v];
        }
    };

    constexpr int EPOLL_NUM = 64;
    constexpr int MAX_EVENTS = 64;

    class Mgr
    {
        int epfd = ::epoll_create1(0);
        std::unordered_map<Seqnum, std::unique_ptr<Pkt_wrapper>> pkt_table;
        bi_unordered_map<Seqnum, Timer_fd> timer_table;

    public:
        Mgr() = default;
        void regist(std::unique_ptr<Pkt_wrapper> &&p);
        uint32_t send() noexcept;
        void discard(Seqnum seqnum) noexcept;

    } sender;
};

namespace detail
{
    void add_event(int epoll_fd, int fd) noexcept
    {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = EPOLLIN;

        ::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    }

    using namespace std::chrono;
    //  setting ms to zero means disarm the timer
    void timer_update(Timer_fd timer_fd, std::chrono::nanoseconds start, std::chrono::nanoseconds interval = 1000ns) noexcept
    {
        const itimerspec its = {
            .it_interval = {.tv_sec = duration_cast<seconds>(interval).count(), .tv_nsec = interval.count()},
            .it_value = {.tv_sec = duration_cast<seconds>(start).count(), .tv_nsec = start.count()},
        };

        ::timerfd_settime(timer_fd, 0, &its, NULL);
    }
};

Header::Header(const char *begin)
{
    ::memcpy(this, begin, sizeof(Header));
}

Pkt::Pkt()
    : data{sizeof(Header)}
{
    std::array<char, sizeof(Header)> dummy = {0};
    data.insert(data.begin(), dummy.begin(), dummy.end());
}

void Pkt::push_back(const std::vector<char> &d)
{
    data.insert(data.end(), d.begin(), d.end());
}

Header &Pkt2header(Pkt &p)
{
    if (p.data.size() < sizeof(Header)) [[unlikely]]
        throw std::length_error("Bad package size");

    return *(Header *)p.data.data();
}

Seqnum Pkt_wrapper::get_seqnum() const noexcept
{
    auto h = Header(this->buf.data());
    return h.seq;
}

uint32_t pkt_mgr::send(Pkt_wrapper &&pp)
{
    auto p = std::make_unique<Pkt_wrapper>(std::forward<Pkt_wrapper &&>(pp));
    sender.regist(std::move(p));
    sender.send();
    return pp.buf.size();
}

uint32_t pkt_mgr::recv(Pkt_wrapper &pp) noexcept
{
    socklen_t socklen = sizeof(sockaddr_in);
    return ::recvfrom(pp.fd, pp.buf.data(), pp.buf.size(), 0, (sockaddr *)&pp.addr, &socklen);
}

void pkt_mgr::discard(Seqnum seqnum) noexcept
{
    sender.discard(seqnum);
}

void Mgr::regist(std::unique_ptr<Pkt_wrapper> &&p)
{
    if (p->buf.size() < sizeof(Header)) [[unlikely]]
        throw std::length_error(fmt::format("Bad package size {}", p->buf.size()));

    using namespace std::chrono;
    Timer_fd timer = ::timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);

    // Add into table
    const auto seqnum = p->get_seqnum();
    pkt_table.insert({seqnum, std::move(p)});
    timer_table.intset(seqnum, timer);

    // Update kernel timer
    detail::add_event(epfd, timer);
    detail::timer_update(timer, 1ms); // send it immediately
}

uint32_t Mgr::send() noexcept
{
    uint32_t tot_len = 0;
    std::array<epoll_event, MAX_EVENTS> events;
    int nfds = ::epoll_wait(epfd, events.data(), MAX_EVENTS, -1);
    for (int i = 0; i < nfds; i++)
    {
        const Timer_fd tfd = events[i].data.fd;
        uint64_t dummy;
        ::read(tfd, &dummy, sizeof(uint64_t));

        const auto &p = pkt_table[timer_table[tfd]];
        tot_len += ::sendto(p->fd, p->buf.data(), p->buf.size(), 0, (sockaddr *)&p->addr, sizeof(sockaddr_in));
    }

    return tot_len;
}

void Mgr::discard(Seqnum seqnum) noexcept
{
    const auto tfd = timer_table[seqnum];

    pkt_table.erase(seqnum);
    timer_table.discard(seqnum);

    close(tfd); // Will be removed from the epoll set automatically
}
