cmake_minimum_required(VERSION 3.20.6)
project(hopper VERSION 0.1.0 LANGUAGES CXX)


if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(HOPPER_IS_TOP_LEVEL ON)
else ()
    set(HOPPER_IS_TOP_LEVEL OFF)
endif ()

# The default build type is hopper's own choice only when it is the top-level project; a parent keeps its own.
if (HOPPER_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

# Requires Clang, which carries libFuzzer. The whole tree is instrumented, the munch submodule included, not just the
# harness: the fuzzer's coverage feedback and the sanitizers must see library code, so a fuzzer build is its own
# dedicated build tree.
option(HOPPER_BUILD_FUZZER "Build the libFuzzer harnesses for hopper (requires Clang)" OFF)
if (HOPPER_BUILD_FUZZER)
    add_compile_options(-fsanitize=address,undefined,fuzzer-no-link -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif ()

include(GNUInstallDirs)

# Installing hopper means installing targets that link munch, and an exported target can only refer to munch as an
# installed package, so the install rules require the system munch; the default build uses the submodule and does not
# install.
option(HOPPER_SYSTEM_MUNCH "Use an installed munch found with find_package instead of the submodule" OFF)
option(HOPPER_INSTALL "Generate install rules for hopper (requires HOPPER_SYSTEM_MUNCH)" OFF)

if (HOPPER_INSTALL AND NOT HOPPER_SYSTEM_MUNCH)
    message(FATAL_ERROR
            "HOPPER_INSTALL needs HOPPER_SYSTEM_MUNCH=ON: the exported targets refer to munch's installed package")
endif ()

# Found here rather than under external/, because imported targets are visible only from the directory that finds
# them downward, and every library under libs/ links munch.
if (HOPPER_SYSTEM_MUNCH)
    find_package(munch REQUIRED)
endif ()

add_subdirectory(external)
add_subdirectory(libs)
add_subdirectory(tools)

# The umbrella carries hopper's own libraries; munch arrives transitively through hopper_parse.
add_library(${PROJECT_NAME}
        INTERFACE
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
add_library(${PROJECT_NAME}::parse ALIAS hopper_parse)
add_library(${PROJECT_NAME}::json ALIAS hopper_json)
add_library(${PROJECT_NAME}::clike ALIAS hopper_clike)

target_link_libraries(${PROJECT_NAME}
        INTERFACE
        hopper_parse
        hopper_json
        hopper_clike
)

if (HOPPER_INSTALL)
    # The installed names are the aliases the build offers: hopper::parse, hopper::json, hopper::clike, hopper::hopper.
    set_target_properties(hopper_parse PROPERTIES EXPORT_NAME parse)
    set_target_properties(hopper_json PROPERTIES EXPORT_NAME json)
    set_target_properties(hopper_clike PROPERTIES EXPORT_NAME clike)

    install(TARGETS ${PROJECT_NAME} hopper_parse hopper_json hopper_clike EXPORT hopper_targets)

    install(DIRECTORY libs/parse/include/ libs/json/include/ libs/clike/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # The JSONTestSuite notice accompanies copies of the test data, which the install does not carry; the notice
    # travels anyway so the package's licences are complete on their own.
    install(FILES LICENSE THIRD_PARTY_NOTICES.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/hopper)

    install(EXPORT hopper_targets
            FILE hopper-targets.cmake
            NAMESPACE hopper::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hopper
    )

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/hopper-config-version.cmake
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            cmake/hopper-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/hopper-config-version.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hopper
    )
endif ()
