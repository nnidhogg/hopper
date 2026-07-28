#ifndef HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_PARSER_HPP
#define HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_PARSER_HPP

#include <filesystem>
#include <munch/core/lexer.hpp>
#include <string>

#include "hopper/idl/ast/ast.hpp"
#include "hopper/idl/token_reader.hpp"
#include "hopper/idl/tokens.hpp"

namespace hopper::idl
{
/**
 * @brief Recursive-descent parser for a minimal subset of IDL.
 *
 * Consumes tokens from a Token_reader and builds an AST representing structs
 * with primitive fields.
 */
class Parser
{
public:
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
     * @brief Parse an entire file: zero or more struct declarations.
     */
    [[nodiscard]] ast::File parse();

private:
    Token_reader reader_;

    [[nodiscard]] Token_t expect(Token_kind kind, std::string_view what);
    [[nodiscard]] std::optional<Token_t> next_token();
    [[nodiscard]] std::optional<Token_t> peek_token();

    [[noreturn]] void syntax_error(std::string_view message, const Token_t& where);
    [[noreturn]] void eof_error(std::string_view message);

    ast::Struct parse_struct();
    ast::Field parse_field();
    ast::Type parse_type();
};

} // namespace hopper::idl

#endif // HOPPER_LIBS_IDL_INCLUDE_HOPPER_IDL_PARSER_HPP
