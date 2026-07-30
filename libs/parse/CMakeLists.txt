cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0097 NEW)
project(hopper_parse)

add_library(${PROJECT_NAME}
        src/token_location.cpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        munch
)

if (HOPPER_BUILD_TESTS)
    add_executable(${PROJECT_NAME}_tests
            tests/token_reader_test.cpp
            tests/parser_base_test.cpp
    )

    target_link_libraries(${PROJECT_NAME}_tests
            PRIVATE
            ${PROJECT_NAME}
            munch
            gtest_main
    )

    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif ()
