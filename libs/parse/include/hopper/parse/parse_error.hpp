#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSE_ERROR_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSE_ERROR_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

#include "hopper/parse/source_span.hpp"

namespace hopper::parse
{
/**
 * @brief What went wrong: the input failed to tokenize, a token was not the expected one, the input ended, a
 *        token the grammar admits spells a value the language rejects, such as a lone surrogate escape, or the
 *        file named as input could not be read.
 */
enum class Parse_error_kind : std::uint8_t
{
    Lexical,
    Unexpected_token,
    Unexpected_end,
    Invalid_literal,
    Unreadable_file,
};

/**
 * @brief A parse failure with its kind and the source range it points at.
 *
 * A std::runtime_error whose what() carries the message prefixed with the 1-based "line:column:" of the span's begin.
 * The span covers the offending token for a token error; a lexical error's span is empty where tokenization stopped, an
 * unexpected end's is empty just after the last token consumed, and an unreadable file's is empty at the start.
 */
class Parse_error : public std::runtime_error
{
public:
    /**
     * @brief Constructs an error from its kind, the span it points at, and a human-readable message.
     * @param kind What went wrong.
     * @param span The source range the error points at.
     * @param message The message, which what() prefixes with the span's line and column.
     */
    Parse_error(Parse_error_kind kind, const Source_span& span, const std::string& message);

    /**
     * @brief The error for a token the grammar does not admit where it stands.
     * @param span The token's span.
     * @param message What was expected there, which the error follows with the token's spelling.
     * @param lexeme The token's spelling.
     * @return An Unexpected_token error reading "Syntax error: <message>, got '<lexeme>'".
     */
    [[nodiscard]] static Parse_error unexpected_token(
            const Source_span& span, std::string_view message, std::string_view lexeme);

    /**
     * @brief The error for input that ended where the grammar needs more.
     * @param at Where the last token ended, which the error's empty span stands at.
     * @param message What the input ended before.
     * @return An Unexpected_end error reading "Syntax error: <message>".
     */
    [[nodiscard]] static Parse_error unexpected_end(const Source_position& at, std::string_view message);

    /**
     * @brief The error for input the lexer cannot tokenize.
     * @param at Where tokenizing stopped, which the error's empty span stands at.
     * @param message The tokenizer's account of the failure.
     * @return A Lexical error reading "Lexical error: <message>".
     */
    [[nodiscard]] static Parse_error lexical(const Source_position& at, std::string_view message);

    /**
     * @brief What went wrong.
     */
    [[nodiscard]] Parse_error_kind kind() const noexcept;

    /**
     * @brief The source range the error points at.
     */
    [[nodiscard]] const Source_span& span() const noexcept;

private:
    /**
     * @brief What went wrong.
     */
    Parse_error_kind kind_;

    /**
     * @brief The source range the error points at.
     */
    Source_span span_;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSE_ERROR_HPP
