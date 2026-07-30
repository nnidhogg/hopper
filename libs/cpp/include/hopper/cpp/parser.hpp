#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP

#include <cstddef>
#include <filesystem>
#include <munch/core/lexer.hpp>
#include <string>

#include "hopper/cpp/ast/expr.hpp"
#include "hopper/cpp/ast/stmt.hpp"
#include "hopper/cpp/ast/unit.hpp"
#include "hopper/cpp/tokens.hpp"
#include "hopper/parse/parser_base.hpp"
#include "hopper/parse/token_reader.hpp"

namespace hopper::cpp
{
/**
 * @brief Recursive-descent, precedence-climbing parser for a subset of C++.
 *
 * Expressions cover primaries (literals, identifiers, parenthesized subexpressions), postfix operators (calls,
 * `.`, `->`, `[]`, `++`, `--`), unary `+`/`-`/`!`/`~`/`++`/`--`/`&`/`*` (address-of and dereference), the
 * multiplicative/additive/shift/relational/equality/
 * bitwise-and/bitwise-xor/bitwise-or/logical-and/logical-or binary ladder (all left-associative, implemented as one
 * precedence-table-driven parse_binary() rather than one function per level), the ternary conditional, and
 * assignment (both right-associative). Statements cover the expression statement, the empty statement, compound
 * `{}` blocks, `if`/`else` (a dangling `else` binding to the nearest `if`), `while`, `for` (with a declaration,
 * expression, or empty init-statement), `do`/`while`, `return`, and declarations: a possibly const-qualified
 * fundamental type followed by comma-separated pointer/reference declarators with optional initializers. A
 * translation unit is a sequence of function definitions, function prototypes, and variable declarations, with
 * parameters carrying the same type and declarator shapes plus optional defaults. Casts are not covered yet.
 *
 * The token-stream plumbing lives in parse::Parser_base; this class holds only the grammar, with one
 * implementation file per grammar area (parse_expression.cpp, parse_statement.cpp, ...).
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

    /**
     * @brief Parse a single statement, consuming the whole input.
     *
     * A compound `{}` block counts as one statement, so any statement sequence can be parsed by enclosing it in
     * braces.
     * @throws std::runtime_error If a lexical or syntax error occurs, or trailing input remains.
     */
    [[nodiscard]] ast::Stmt parse_statement();

    /**
     * @brief Parse a whole translation unit, consuming the whole input.
     *
     * A translation unit is a sequence of function definitions, function prototypes, and variable declarations.
     * @throws std::runtime_error If a lexical or syntax error occurs.
     */
    [[nodiscard]] ast::Translation_unit parse_translation_unit();

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

    /**
     * @brief Parse one statement, dispatching on its leading token.
     */
    [[nodiscard]] ast::Stmt parse_statement_node();

    [[nodiscard]] ast::Stmt parse_compound_statement();
    [[nodiscard]] ast::Stmt parse_if_statement();
    [[nodiscard]] ast::Stmt parse_while_statement();

    /**
     * @brief Parse a `for` loop, whose init-statement may be a declaration, an expression statement, or empty.
     */
    [[nodiscard]] ast::Stmt parse_for_statement();

    [[nodiscard]] ast::Stmt parse_do_statement();
    [[nodiscard]] ast::Stmt parse_return_statement();

    /**
     * @brief Whether the next token can begin a declaration: a fundamental type keyword or the const qualifier.
     *
     * Types are restricted to keywords deliberately: an identifier in type position would make `a * b;` ambiguous
     * between a declaration and an expression, which real C++ resolves through a symbol table the parser does not
     * have.
     */
    [[nodiscard]] bool is_declaration_start();

    [[nodiscard]] ast::Stmt parse_declaration_statement();

    /**
     * @brief Parse a type specifier, accepting the const qualifier before or after the type name.
     */
    [[nodiscard]] ast::Type parse_type_specifier();

    /**
     * @brief Parse one declarator: pointers, an optional reference, the name, and an optional initializer.
     */
    [[nodiscard]] ast::Declarator parse_declarator();

    /**
     * @brief Parse one translation unit item, forking between function and declaration after the first
     * declarator's name: a '(' begins a parameter list, anything else continues a variable declaration.
     */
    [[nodiscard]] ast::Translation_unit::Item_t parse_external_declaration();

    /**
     * @brief Parse a function's parameter list and body (or prototype ';'), the return declarator already parsed.
     */
    [[nodiscard]] ast::Function parse_function(ast::Type type, std::size_t pointers, bool reference,
                                               std::string name);

    [[nodiscard]] ast::Parameter parse_parameter();
};

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_PARSER_HPP
