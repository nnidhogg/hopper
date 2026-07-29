#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP

#include <filesystem>
#include <munch/core/lexer.hpp>
#include <munch/tools/tokenizer/token.hpp>
#include <optional>
#include <string>
#include <string_view>

#include "hopper/cpp/ast/expr.hpp"
#include "hopper/cpp/token_reader.hpp"
#include "hopper/cpp/tokens.hpp"

namespace hopper::cpp
{
/**
 * @brief Recursive-descent, precedence-climbing parser for a subset of C++ expressions.
 *
 * Covers primary expressions (literals, identifiers, parenthesized subexpressions), postfix operators (calls,
 * `.`, `->`, `[]`, `++`, `--`), unary `+`/`-`/`!`/`~`, the multiplicative/additive/shift/relational/equality/
 * bitwise-and/bitwise-xor/bitwise-or/logical-and/logical-or binary ladder (all left-associative, implemented as one
 * precedence-table-driven parse_binary() rather than one function per level), and the ternary conditional
 * (right-associative). Assignment and casts are not covered yet.
 */
class Parser
{
public:
    /**
     * @brief The token type produced by the underlying lexer.
     */
    using Token_t = munch::tools::tokenizer::Token<Token_kind>;

    /**
     * @brief Construct a parser from an existing Token_reader.
     */
    explicit Parser(Token_reader reader);

    /**
     * @brief Construct a parser from a lexer and an in-memory string.
     */
    explicit Parser(munch::core::Lexer lexer, const std::string& input);

    /**
     * @brief Construct a parser from a lexer and a file path.
     */
    explicit Parser(munch::core::Lexer lexer, const std::filesystem::path& file);

    /**
     * @brief Parse a single expression, consuming the whole input.
     * @throws std::runtime_error If a lexical or syntax error occurs, or trailing input remains.
     */
    [[nodiscard]] ast::Expr parse_expression();

private:
    /**
     * @brief Retrieve the next token, throwing on a lexical error.
     * @return The token, or std::nullopt at end of input.
     */
    [[nodiscard]] std::optional<Token_t> next_token();

    /**
     * @brief Look at the next token without consuming it, throwing on a lexical error.
     * @return The token, or std::nullopt at end of input.
     */
    [[nodiscard]] std::optional<Token_t> peek_token();

    /**
     * @brief Check whether the next token has the given kind, without consuming it.
     */
    [[nodiscard]] bool check(Token_kind kind);

    /**
     * @brief Consume and return the next token if it has the given kind.
     */
    [[nodiscard]] std::optional<Token_t> accept(Token_kind kind);

    /**
     * @brief Require the next token to have the given kind, consuming it.
     * @param kind The required token kind.
     * @param what A human-readable description of what was expected, used in the error message.
     * @throws std::runtime_error If the next token has a different kind, or the input ends first.
     */
    Token_t expect(Token_kind kind, std::string_view what);

    [[noreturn]] void syntax_error(std::string_view message, const Token_t& where);
    [[noreturn]] void eof_error(std::string_view message);

    [[nodiscard]] ast::Expr parse_ternary();

    /**
     * @brief Parse the left-associative binary operator ladder via precedence climbing.
     * @param min_precedence The lowest operator precedence this call is allowed to consume; a lower-precedence
     *        operator is left for an enclosing call to handle.
     */
    [[nodiscard]] ast::Expr parse_binary(int min_precedence);

    [[nodiscard]] ast::Expr parse_unary();
    [[nodiscard]] ast::Expr parse_postfix();
    [[nodiscard]] ast::Expr parse_primary();

    Token_reader reader_;
};

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP
