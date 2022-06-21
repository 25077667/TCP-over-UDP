set(HDIR "${CMAKE_SOURCE_DIR}/include/server")

add_library(server STATIC server.cpp ${HDIR}/server.hpp)
target_include_directories(server PUBLIC ${HDIR})
target_include_directories(server PUBLIC ${HDIR}/../lib/)

target_link_libraries(server PUBLIC)