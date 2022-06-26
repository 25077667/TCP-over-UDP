#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <array>
#include <functional>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include <fmt/core.h>

namespace util
{
namespace big_endian
{
std::string_view ip(uint32_t ip);
std::string_view port(uint16_t port);
};  // namespace big_endian

uint32_t get_rand();

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
consteval auto cat_static_str(const std::array<char, N> a,
                              const std::array<char, M> b,
                              Rest... rest)
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
    return cat_static_str(to_array("{} to "), detail::fmtstr_ph_gen<Args>()...,
                          to_array("failed: {}"));
}


};  // namespace detail

template <typename... Args>
void try_exec(const char *func_name,
              std::function<int(void)> to_be_exec,
              Args &&...args)
{
    int _ = to_be_exec();
    if (_ == -1) [[unlikely]] {
        static constexpr auto to_be_gen = detail::err_fmtstr_gen<Args...>();
        throw std::runtime_error(fmt::format(to_be_gen.data(), func_name,
                                             args..., std::strerror(_)));
    }
}

};  // namespace util

#endif