cmake_minimum_required(VERSION 3.20.6)
cmake_policy(SET CMP0097 NEW)
project(hopper_clike)


add_library(${PROJECT_NAME}
        src/lexer.cpp
        src/binary_operator.cpp
        src/parser.cpp
        src/parse_declaration.cpp
        src/parse_expression.cpp
        src/parse_statement.cpp
        src/parse_unit.cpp
)

# The exported target carries the standard it needs.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# The build reads the headers from the tree; an installed consumer reads them from the prefix's include directory.
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# munch is header-visible: the token kinds and the lexer accessor name its types, so consumers link it too.
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        hopper_parse
        munch::munch
)

if (HOPPER_BUILD_TESTS)
    add_executable(${PROJECT_NAME}_tests
            tests/lexer_test.cpp
            tests/parser_test.cpp
    )

    target_link_libraries(${PROJECT_NAME}_tests
            PRIVATE
            ${PROJECT_NAME}
            gtest_main
    )

    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)

    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE SOURCE_DIR="${HOPPER_SOURCE_DIR}")
endif ()
