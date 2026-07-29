#include "hopper/cpp/parser.hpp"

#include <utility>

namespace hopper::cpp
{
Parser::Parser(Token_reader_t reader) : Parser_base{std::move(reader)}
{}

Parser::Parser(munch::core::Lexer lexer, const std::string& input)
    : Parser_base{Token_reader_t{std::move(lexer), input, &is_trivia}}
{}

Parser::Parser(munch::core::Lexer lexer, const std::filesystem::path& file)
    : Parser_base{Token_reader_t{std::move(lexer), file, &is_trivia}}
{}

ast::Expr Parser::parse_expression()
{
    auto expr{parse_assignment()};

    if (const auto trailing{peek_token()}; trailing)
    {
        syntax_error("Unexpected trailing input", *trailing);
    }

    return expr;
}

} // namespace hopper::cpp
