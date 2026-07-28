#ifndef PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKEN_LOOKAHEAD_HPP
#define PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKEN_LOOKAHEAD_HPP

#include <lexer/tools/tokenizer/token.hpp>
#include <optional>

#include "token_location.hpp"
#include "tokens.hpp"

namespace parser::idl
{
/**
 * @brief Holds the currently buffered (lookahead) token and its source location.
 *
 * Used by the parser to support one-token lookahead behavior. Stores both the token
 * returned from the lexer and its associated location in the input.
 */
class Token_lookahead
{
public:
    /**
     * @brief Alias for the token type produced by the underlying lexer.
     *
     * Represents a lexical token classified by `Token_kind`, containing both the token
     * kind and its corresponding lexeme.
     */
    using Token_t = lexer::tools::tokenizer::Token<Token_kind>;

    /**
     * @brief Constructs an empty lookahead state.
     */
    Token_lookahead() = default;

    /**
     * @brief Access the current lookahead token, if any.
     */
    [[nodiscard]] const std::optional<Token_t>& token() const noexcept;

    /**
     * @brief Access the location associated with the current token.
     */
    [[nodiscard]] const Token_location& location() const noexcept;

    /**
     * @brief Consume and clear the buffered token.
     *
     * Returns the currently stored token (if any) and resets the internal optional to an empty state.
     */
    std::optional<Token_t> consume() noexcept;

    /**
     * @brief Reset the reading position to the beginning of the current input and clear token.
     */
    void reset() noexcept;

    /**
     * @brief Update the lookahead token and advance the source location.
     *
     * Called when a new token is read from the lexer. It updates both the stored token
     * and the internal source position tracker.
     */
    void advance(Token_kind kind, std::string_view lexeme) noexcept;

private:
    std::optional<Token_t> token_;

    Token_location location_;
};

} // namespace parser::idl

#endif // PARSER_LIBS_IDL_INCLUDE_PARSER_IDL_TOKEN_LOOKAHEAD_HPP
