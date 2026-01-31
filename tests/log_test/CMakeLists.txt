set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/tests)

add_executable(log-test 
    log-test.cpp
)

target_link_libraries(log-test
    PRIVATE ${LOG_LIB}
)