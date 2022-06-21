set(HDIR "${CMAKE_SOURCE_DIR}/include/lib")

add_library(tou STATIC tou.cpp ${HDIR}/tou.hpp)
target_include_directories(tou PUBLIC ${HDIR})

add_library(pkt_mgr STATIC pkt_mgr.cpp ${HDIR}/pkt_mgr.hpp)
target_include_directories(pkt_mgr PUBLIC ${HDIR})

add_library(util STATIC util.cpp ${HDIR}/util.hpp)
target_include_directories(util PUBLIC ${HDIR})

target_link_libraries(tou PUBLIC pkt_mgr util)
