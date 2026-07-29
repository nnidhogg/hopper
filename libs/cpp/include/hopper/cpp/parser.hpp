#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP

#include <filesystem>
#include <munch/core/lexer.hpp>
#include <string>

#include "hopper/cpp/ast/expr.hpp"
#include "hopper/cpp/tokens.hpp"
#include "hopper/parse/parser_base.hpp"
#include "hopper/parse/token_reader.hpp"

namespace hopper::cpp
{
/**
 * @brief Recursive-descent, precedence-climbing parser for a subset of C++ expressions.
 *
 * Covers primary expressions (literals, identifiers, parenthesized subexpressions), postfix operators (calls,
 * `.`, `->`, `[]`, `++`, `--`), unary `+`/`-`/`!`/`~`, the multiplicative/additive/shift/relational/equality/
 * bitwise-and/bitwise-xor/bitwise-or/logical-and/logical-or binary ladder (all left-associative, implemented as one
 * precedence-table-driven parse_binary() rather than one function per level), the ternary conditional, and
 * assignment (both right-associative). Casts are not covered yet.
 *
 * The token-stream plumbing lives in parse::Parser_base; this class holds only the grammar, with one
 * implementation file per grammar area (parse_expression.cpp, ...).
 */
class Parser : public parse::Parser_base<Token_kind>
{
public:
    /**
     * @brief The token stream type feeding this parser.
     */
    using Token_reader_t = parse::Token_reader<Token_kind>;

    /**
     * @brief Construct a parser from an existing token stream.
     */
    explicit Parser(Token_reader_t reader);

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
    [[nodiscard]] ast::Expr parse_assignment();
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
};

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP
