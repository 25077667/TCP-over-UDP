project(tou LANGUAGES CXX)

cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror -std=c++20")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -s")


# get all project files
# HACK this workaround is required to avoid qml files checking ^_^
file(GLOB_RECURSE ALL_SOURCE_FILES src/*/*.cpp include/*/*.hpp)
add_custom_target(format clang-format -i ${ALL_SOURCE_FILES})

add_custom_target(cppcheck cppcheck --enable=all --enable=information --force ${ALL_SOURCE_FILES})

add_subdirectory(src)
add_subdirectory(tests)
