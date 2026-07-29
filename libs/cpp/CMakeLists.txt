cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0097 NEW)
project(hopper_cpp)

add_library(${PROJECT_NAME}
        src/lexer.cpp
        src/binary_operator.cpp
        src/parser.cpp
        src/parse_declaration.cpp
        src/parse_expression.cpp
        src/parse_statement.cpp
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        include
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        hopper_parse
        PRIVATE
        munch
)

if (HOPPER_BUILD_TESTS)
    add_executable(${PROJECT_NAME}_tests
            tests/parser_test.cpp
    )

    target_link_libraries(${PROJECT_NAME}_tests
            PRIVATE
            ${PROJECT_NAME}
            munch
            gtest_main
    )

    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)

    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif ()
