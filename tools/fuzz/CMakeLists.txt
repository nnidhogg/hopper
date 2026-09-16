# The two parsers fed arbitrary bytes. The contract is the libraries' own: a Parse_error is the answer for malformed
# input, and a crash, a sanitizer report or a hang is a finding. Each harness is its own target so that each corpus
# holds inputs that decode as its grammar and not as the other's. The JSON harness is seeded from the JSONTestSuite
# cases the tests carry; the C-like harness from the three programs under seeds/clike, since no corpus of that
# grammar exists elsewhere.
add_executable(hopper_json_fuzzer
        src/json_fuzzer.cpp
)

target_link_libraries(hopper_json_fuzzer
        PRIVATE
        hopper_json
)

# fuzzer rather than the tree-wide fuzzer-no-link: this target carries libFuzzer's driver and its main().
target_link_options(hopper_json_fuzzer
        PRIVATE
        -fsanitize=fuzzer
)

add_executable(hopper_clike_fuzzer
        src/clike_fuzzer.cpp
)

target_link_libraries(hopper_clike_fuzzer
        PRIVATE
        hopper_clike
)

target_link_options(hopper_clike_fuzzer
        PRIVATE
        -fsanitize=fuzzer
)
