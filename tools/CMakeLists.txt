# Probes back claims made outside the libraries; a consumer building hopper as a dependency needs neither them nor
# their corpora, so they follow the same contract as the tests.
if (HOPPER_BUILD_TESTS)
    add_subdirectory(probes)
endif ()

# The fuzz harnesses need Clang's libFuzzer and an instrumented tree, so they build only in a tree configured for it.
if (HOPPER_BUILD_FUZZER)
    add_subdirectory(fuzz)
endif ()
