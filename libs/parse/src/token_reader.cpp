#include "hopper/parse/token_reader.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "hopper/parse/parse_error.hpp"

namespace hopper::parse
{
std::string read_source(const std::filesystem::path& file)
{
    if (std::ifstream stream{file, std::ios::binary}; stream.is_open())
    {
        return {std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    }

    throw Parse_error{Parse_error_kind::Unreadable_file, Source_span{}, "Cannot open file: " + file.string()};
}

} // namespace hopper::parse
