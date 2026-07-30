cmake_minimum_required(VERSION 3.20)
project(hopper)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(HOPPER_IS_TOP_LEVEL ON)
else ()
    set(HOPPER_IS_TOP_LEVEL OFF)
endif ()

# hopper's own root for test data paths; CMAKE_SOURCE_DIR names the embedding project's root under add_subdirectory().
set(HOPPER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# GCC-style flags; threading is carried by munch's targets, not by a global flag.
add_compile_options(-Wall -Wunused -Wextra)

# Warnings are errors for hopper's own development; a consumer's newer compiler must not break their build over a
# warning hopper has not seen yet.
option(HOPPER_WERROR "Treat warnings as errors" ${HOPPER_IS_TOP_LEVEL})
if (HOPPER_WERROR)
    add_compile_options(-Werror)
endif ()

add_compile_options(
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-O2>
)

# Developer-only targets stay out of consumer builds.
option(HOPPER_BUILD_TESTS "Build tests for hopper" ${HOPPER_IS_TOP_LEVEL})
if (HOPPER_BUILD_TESTS)
    enable_testing()
endif ()

add_subdirectory(external)
add_subdirectory(libs)

# The umbrella carries hopper's own libraries; munch arrives transitively through hopper_parse.
add_library(${PROJECT_NAME}
        INTERFACE
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
add_library(${PROJECT_NAME}::parse ALIAS hopper_parse)
add_library(${PROJECT_NAME}::cpp ALIAS hopper_cpp)

target_link_libraries(${PROJECT_NAME}
        INTERFACE
        hopper_parse
        hopper_cpp
)
