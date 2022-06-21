add_executable(tou_test tou.cpp)
target_link_libraries(tou_test PUBLIC tou)

add_custom_command(
    TARGET tou_test 
    POST_BUILD 
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:tou_test>" ../exec/tou_test
    COMMENT "Copying tou_test to exec directory"
)