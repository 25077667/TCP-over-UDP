#include <tou.hpp>
#include <iostream>
#include <functional>
#include <unistd.h>

constexpr int DEFAULT_SERVER_PORT = 4095;

auto client_routine = []() -> void
{
    TCP client = TCP("127.0.0.1", DEFAULT_SERVER_PORT);
    const std::string data = {"THIS IS SECRET DATA"};
    const std::vector<char> secret = {data.begin(), data.end()};

    client.send(secret);
};

auto server_routine = []() -> void
{
    TCP server = TCP(DEFAULT_SERVER_PORT);

    std::vector<char> server_buf(1024);
    server.recv(server_buf);

    std::cout << std::string(server_buf.begin(), server_buf.end()) << std::endl;
};

int main(void)
{
    if (::fork())
        server_routine();
    else
        client_routine();

    return 0;
}