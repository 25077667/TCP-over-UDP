project(tou LANGUAGES CXX)

cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -std=c++20")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -fsanitizer=address")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -s")
add_subdirectory(src)
add_subdirectory(tests)
