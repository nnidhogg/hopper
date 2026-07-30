#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSER_BASE_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSER_BASE_HPP

#include <munch/tools/tokenizer/token.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "hopper/parse/parse_error.hpp"
#include "hopper/parse/source_span.hpp"
#include "hopper/parse/token_reader.hpp"

namespace hopper::parse
{
/**
 * @brief Base class providing the token-stream plumbing every recursive-descent parser needs.
 *
 * Wraps a Token_reader and exposes the standard LL(1) primitives: peeking, consuming, conditional acceptance,
 * required expectation, and structured error reporting. A concrete parser derives from this and adds only its
 * grammar functions.
 * @tparam Kind The token kind type (enum or integral) produced by the lexer.
 */
template <typename Kind>
class Parser_base
{
public:
    /**
     * @brief The token type produced by the underlying lexer.
     */
    using Token_t = munch::tools::tokenizer::Token<Kind>;

protected:
    /**
     * @brief Construct the base around an existing token stream.
     */
    explicit Parser_base(Token_reader<Kind> reader) : reader_{std::move(reader)} {}

    ~Parser_base() = default;

    /**
     * @brief Retrieve the next token, throwing on a lexical error.
     * @return The token, or std::nullopt at end of input.
     */
    [[nodiscard]] std::optional<Token_t> next_token()
    {
        const auto result{reader_.next()};

        if (result.has_error())
        {
            lexical_error(result.error().message());
        }

        if (result.has_token())
        {
            return result.token();
        }

        return std::nullopt;
    }

    /**
     * @brief Look at the next token without consuming it, throwing on a lexical error.
     * @return The token, or std::nullopt at end of input.
     */
    [[nodiscard]] std::optional<Token_t> peek_token()
    {
        const auto result{reader_.peek()};

        if (result.has_error())
        {
            lexical_error(result.error().message());
        }

        if (result.has_token())
        {
            return result.token();
        }

        return std::nullopt;
    }

    /**
     * @brief Check whether the next token has the given kind, without consuming it.
     */
    [[nodiscard]] bool check(const Kind kind)
    {
        const auto token{peek_token()};

        return token && token->kind() == kind;
    }

    /**
     * @brief Consume and return the next token if it has the given kind.
     */
    [[nodiscard]] std::optional<Token_t> accept(const Kind kind)
    {
        if (!check(kind))
        {
            return std::nullopt;
        }

        return next_token();
    }

    /**
     * @brief Require the next token to have the given kind, consuming it.
     * @param kind The required token kind.
     * @param what A human-readable description of what was expected, used in the error message.
     * @throws std::runtime_error If the next token has a different kind, or the input ends first.
     */
    Token_t expect(const Kind kind, const std::string_view what)
    {
        const auto token{next_token()};

        if (!token)
        {
            eof_error("Expected " + std::string(what) + " before end of input");
        }

        if (token->kind() != kind)
        {
            syntax_error("Expected " + std::string(what), *token);
        }

        return *token;
    }

    /**
     * @brief The position where the next construct will begin: the next token's start, or where input ended.
     *
     * Capture this before parsing a construct and close the span with span_from() after it.
     */
    [[nodiscard]] Source_position mark()
    {
        if (peek_token())
        {
            return reader_.span().begin;
        }

        return reader_.previous_end();
    }

    /**
     * @brief The span from a captured mark to the end of the most recently consumed token.
     */
    [[nodiscard]] Source_span span_from(const Source_position& begin) const noexcept
    {
        return {begin, reader_.previous_end()};
    }

    /**
     * @brief Throw a syntax error naming and pointing at the offending token.
     *
     * The reader's current span covers the offending token whether it was just consumed or is still buffered, so
     * the error points at the right source range either way.
     */
    [[noreturn]] void syntax_error(const std::string_view message, const Token_t& where)
    {
        throw Parse_error{
                Parse_error_kind::Unexpected_token, reader_.span(),
                "Syntax error: " + std::string(message) + ", got '" + std::string(where.lexeme()) + "'"};
    }

    /**
     * @brief Throw a syntax error for input that ended too early, pointing one past the last consumed token.
     */
    [[noreturn]] void eof_error(const std::string_view message)
    {
        const auto& at{reader_.previous_end()};

        throw Parse_error{Parse_error_kind::Unexpected_end, {at, at}, "Syntax error: " + std::string(message)};
    }

    /**
     * @brief Throw for input the lexer rejected, pointing at where tokenization stopped.
     */
    [[noreturn]] void lexical_error(const std::string& message)
    {
        const auto at{reader_.span().end};

        throw Parse_error{Parse_error_kind::Lexical, {at, at}, "Lexical error: " + message};
    }

private:
    Token_reader<Kind> reader_;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSER_BASE_HPP
