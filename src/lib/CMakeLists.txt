set(HDIR "${CMAKE_SOURCE_DIR}/include/lib")

add_library(tou STATIC tou.cpp ${HDIR}/tou.hpp)
target_include_directories(tou PUBLIC ${HDIR})

target_link_libraries(tou PUBLIC)

