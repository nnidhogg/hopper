#include "hopper/clike/parser.hpp"

#include <array>
#include <utility>

#include "hopper/clike/binary_operator.hpp"
#include "hopper/parse/parse_error.hpp"

namespace hopper::clike
{
namespace
{
/**
 * @brief The identifiers the language reserves; an identifier spelled like one is never a name.
 */
constexpr std::array<std::string_view, 19> keywords{"true",
                                                    "false",
                                                    "if",
                                                    "else",
                                                    "while",
                                                    "for",
                                                    "do",
                                                    "return",
                                                    "const",
                                                    "bool",
                                                    "char",
                                                    "int",
                                                    "float",
                                                    "double",
                                                    "void",
                                                    "static_cast",
                                                    "dynamic_cast",
                                                    "const_cast",
                                                    "reinterpret_cast"};

/**
 * @brief Whether a spelling is a reserved word.
 * @param spelling The identifier's text.
 * @return True for a keyword.
 */
bool is_keyword(const std::string_view spelling) noexcept
{
    for (const auto keyword : keywords)
    {
        if (keyword == spelling)
        {
            return true;
        }
    }

    return false;
}

} // namespace

Parser::Parser(Token_reader_t reader) : Parser_base{std::move(reader)}
{}

Parser::Parser(const std::string& input) : Parser{Token_reader_t{lexer(), input, &is_trivia}}
{}

Parser::Parser(const std::filesystem::path& file) : Parser{Token_reader_t{lexer(), file, &is_trivia}}
{}

parse::Source_position Parser::here()
{
    return pending_ ? pending_->span.begin : mark();
}

parse::Source_span Parser::close(const parse::Source_position& begin) const noexcept
{
    return pending_ ? parse::Source_span{.begin = begin, .end = pending_->before} : span_from(begin);
}

std::optional<Parser::Operator> Parser::peek_operator()
{
    if (pending_)
    {
        return pending_;
    }

    if (!check(Token_kind::Operator))
    {
        return std::nullopt;
    }

    const auto begin{mark()};

    const auto before{span_from(begin).end};

    std::string spelling{next_token()->lexeme()};

    auto span{span_from(begin)};

    // Fuse the following operator bytes while they touch the spelling and extend an operator the table knows.
    while (check(Token_kind::Operator) && mark().offset == span.end.offset)
    {
        const auto candidate{spelling + std::string{peek_token()->lexeme()}};

        if (!is_operator_prefix(candidate))
        {
            break;
        }

        (void)next_token();

        spelling = candidate;

        span = span_from(begin);
    }

    pending_ = Operator{.spelling = std::move(spelling), .span = span, .before = before};

    return pending_;
}

void Parser::take_operator() noexcept
{
    pending_.reset();
}

bool Parser::check_operator(const std::string_view spelling)
{
    const auto op{peek_operator()};

    return op && op->spelling == spelling;
}

bool Parser::accept_operator(const std::string_view spelling)
{
    if (!check_operator(spelling))
    {
        return false;
    }

    take_operator();

    return true;
}

void Parser::expect_operator(const std::string_view spelling, const std::string_view what)
{
    if (!accept_operator(spelling))
    {
        unexpected(what);
    }
}

bool Parser::check_punctuation(const char byte)
{
    if (pending_)
    {
        return false;
    }

    const auto token{peek_token()};

    return token && token->kind() == Token_kind::Punctuation && token->lexeme().front() == byte;
}

bool Parser::accept_punctuation(const char byte)
{
    if (!check_punctuation(byte))
    {
        return false;
    }

    (void)next_token();

    return true;
}

void Parser::expect_punctuation(const char byte, const std::string_view what)
{
    if (!accept_punctuation(byte))
    {
        unexpected(what);
    }
}

bool Parser::check_keyword(const std::string_view word)
{
    if (pending_)
    {
        return false;
    }

    const auto token{peek_token()};

    return token && token->kind() == Token_kind::Identifier && token->lexeme() == word;
}

bool Parser::accept_keyword(const std::string_view word)
{
    if (!check_keyword(word))
    {
        return false;
    }

    (void)next_token();

    return true;
}

void Parser::expect_keyword(const std::string_view word, const std::string_view what)
{
    if (!accept_keyword(word))
    {
        unexpected(what);
    }
}

std::optional<Parser::Token_t> Parser::accept_identifier()
{
    if (pending_)
    {
        return std::nullopt;
    }

    const auto token{peek_token()};

    if (!token || token->kind() != Token_kind::Identifier || is_keyword(token->lexeme()))
    {
        return std::nullopt;
    }

    return next_token();
}

Parser::Token_t Parser::expect_identifier(const std::string_view what)
{
    const auto token{accept_identifier()};

    if (!token)
    {
        unexpected(what);
    }

    return *token;
}

bool Parser::more()
{
    return pending_.has_value() || peek_token().has_value();
}

void Parser::unexpected(const std::string_view what)
{
    if (pending_)
    {
        throw parse::Parse_error{
                parse::Parse_error_kind::Unexpected_token, pending_->span,
                "Syntax error: Expected " + std::string(what) + ", got '" + pending_->spelling + "'"};
    }

    const auto token{next_token()};

    if (!token)
    {
        eof_error("Expected " + std::string(what) + " before end of input");
    }

    syntax_error("Expected " + std::string(what), *token);
}

} // namespace hopper::clike
