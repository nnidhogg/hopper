#include "hopper/idl/parser.hpp"

#include <stdexcept>
#include <utility>

namespace hopper::idl
{
Parser::Parser(Token_reader reader) : reader_{std::move(reader)}
{}

Parser::Parser(munch::core::Lexer lexer, const std::string& input) : reader_{std::move(lexer), input}
{}

Parser::Parser(munch::core::Lexer lexer, const std::filesystem::path& file) : reader_{std::move(lexer), file}
{}

std::optional<Parser::Token_t> Parser::next_token()
{
    auto res = reader_.next();
    if (res.has_error())
    {
        // propagate lexical error as an exception for now
        throw std::runtime_error{"Lexical error: " + res.error().message()};
    }
    if (res.has_token())
    {
        return res.token();
    }
    return std::nullopt;
}

std::optional<Parser::Token_t> Parser::peek_token()
{
    auto res = reader_.peek();
    if (res.has_error())
    {
        throw std::runtime_error{"Lexical error: " + res.error().message()};
    }
    if (res.has_token())
    {
        return res.token();
    }
    return std::nullopt;
}

// Optionally keep this if comments are still visible to the parser and you
// want to treat them as non-semantic here.
void Parser::skip_trivia()
{
    for (;;)
    {
        auto opt = peek_token();
        if (!opt)
        {
            return; // EOF
        }

        const auto kind = opt->kind();

        switch (kind)
        {
        case Token_kind::Whitespace:
        case Token_kind::Newline:
        case Token_kind::Single_line_comment:
        case Token_kind::Multi_line_comment:
            // Discard this token at parser level
            (void)next_token();
            continue;

        default:
            return;
        }
    }
}

Parser::Token_t Parser::expect(const Token_kind kind, const std::string_view what)
{
    skip_trivia();

    auto opt = next_token();
    if (!opt)
    {
        eof_error("Expected " + std::string(what) + " before end of input");
    }

    auto token = *opt;

    if (token.kind() != kind)
    {
        syntax_error("Expected " + std::string(what), token);
    }

    return token;
}

[[noreturn]] void Parser::syntax_error(const std::string_view message, const Token_t& where)
{
    // At this point you could query Token_location from Token_reader/Token_lookahead
    // to print "line:column". For now we just throw with lexeme context.
    throw std::runtime_error{"Syntax error: " + std::string(message) + ", got '" + std::string(where.lexeme()) + "'"};
}

[[noreturn]] void Parser::eof_error(const std::string_view message)
{
    throw std::runtime_error{"Syntax error: " + std::string(message)};
}

ast::File Parser::parse()
{
    ast::File file{};

    skip_trivia();

    while (true)
    {
        auto opt = peek_token();
        if (!opt)
        {
            break; // EOF
        }

        switch (opt->kind())
        {
        case Token_kind::Keyword_struct:
            file.structs.push_back(parse_struct());
            break;

        default:
            syntax_error("Unexpected token at top level", *opt);
        }

        skip_trivia();
    }

    return file;
}

ast::Struct Parser::parse_struct()
{
    // struct Name { field* }
    expect(Token_kind::Keyword_struct, "struct keyword");

    auto name_tok = expect(Token_kind::Identifier, "struct name");

    ast::Struct result{};
    result.name = std::string{name_tok.lexeme()};

    expect(Token_kind::Left_brace, "'{'");

    skip_trivia();

    // Zero or more fields until '}'
    while (true)
    {
        auto opt = peek_token();
        if (!opt)
        {
            eof_error("Expected '}' to close struct '" + result.name + "'");
        }

        if (opt->kind() == Token_kind::Right_brace)
        {
            (void)next_token(); // consume '}'
            break;
        }

        result.fields.push_back(parse_field());
        skip_trivia();
    }

    // Optionally, require semicolon after struct declaration if IDL syntax wants it
    // expect(Token_kind::Semicolon, "';' after struct");

    return result;
}

ast::Field Parser::parse_field()
{
    // e.g. "int32 x;"
    ast::Field field{};

    field.type = parse_type();

    auto name_tok = expect(Token_kind::Identifier, "field name");
    field.name = std::string{name_tok.lexeme()};

    expect(Token_kind::Semicolon, "semicolon ';' after field");

    return field;
}

ast::Type Parser::parse_type()
{
    skip_trivia();

    auto opt = next_token();
    if (!opt)
    {
        eof_error("Expected type name before end of input");
    }

    const auto token = *opt;

    ast::Type type{};

    switch (token.kind())
    {
    case Token_kind::Keyword_boolean:
        type.primitive = ast::Primitive_type::Boolean;
        break;

    case Token_kind::Keyword_int32:
        type.primitive = ast::Primitive_type::Int32;
        break;

    case Token_kind::Keyword_string:
        type.primitive = ast::Primitive_type::String;
        break;

    default:
        syntax_error("Expected primitive type (boolean, int32, string)", token);
    }

    return type;
}

} // namespace hopper::idl
