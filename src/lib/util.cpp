#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <string_view>
#include <util.hpp>

#include <limits>
#include <numeric>
#include <random>


std::string_view util::big_endian::ip(uint32_t ip)
{
    int little = ::htonl(ip);
    return std::string_view(std::to_string(little & 0xFF000000) + ":" +
                            std::to_string(little & 0x00FF0000) + ":" +
                            std::to_string(little & 0x0000FF00) + ":" +
                            std::to_string(little & 0x000000FF));
}

std::string_view util::big_endian::port(uint16_t port)
{
    return std::string_view(std::to_string(::htons(port)));
}

uint32_t util::get_rand()
{
    std::default_random_engine gen(std::random_device{}());
    std::uniform_int_distribution<int> unif(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    return unif(gen);
}