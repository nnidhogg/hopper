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
