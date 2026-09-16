#include <cstddef>
#include <cstdint>
#include <string>

#include "hopper/clike/parser.hpp"
#include "hopper/parse/parse_error.hpp"

/**
 * @brief Parses the fuzz input as one C-like translation unit. The parser is built once, its lexer compiled once,
 *        and reloaded with every input, which is the library's own way of serving many texts. A Parse_error is the
 *        parser refusing malformed input, which is its contract; anything else that escapes, and any sanitizer
 *        report, is a finding.
 */
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* const data, const std::size_t size)
{
    static hopper::clike::Parser parser{std::string{}};
    const std::string input{reinterpret_cast<const char*>(data), size};
    try
    {
        parser.load(input);
        [[maybe_unused]] const auto unit{parser.parse_translation_unit()};
    }
    catch (const hopper::parse::Parse_error&)
    {}
    return 0;
}
