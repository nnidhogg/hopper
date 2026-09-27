cmake_minimum_required(VERSION 3.20.6)
project(hopper_probes)

# What an edit costs a JSON document that relexes from a certified start to a shared boundary: the edit theorem run as
# a program.
# Runs as a test because the generated corpus's stream equality against a whole scan after every edit is a hard
# assertion; the corpus form takes a JSON file and prints the figures docs/design.md quotes.
add_executable(hopper_edit_relex src/edit_relex.cpp)

target_link_libraries(hopper_edit_relex
        PRIVATE
        hopper_json
)

add_test(NAME hopper_edit_relex COMMAND hopper_edit_relex)
