set(HDIR "${CMAKE_SOURCE_DIR}/include/client")

add_library(client STATIC client.cpp ${HDIR}/client.hpp)
target_include_directories(client PUBLIC ${HDIR})
target_include_directories(client PUBLIC ${HDIR}/../lib/)

target_link_libraries(client PUBLIC)