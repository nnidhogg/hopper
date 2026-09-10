#ifndef HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_TOKENS_HPP
#define HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_TOKENS_HPP

#include <cstddef>

#include <munch/core/lexer.hpp>

namespace hopper::json
{
/**
 * @brief The token kinds of RFC 8259 JSON text.
 *
 * Whitespace is a token the reader discards; every other kind reaches the parser. The three literals and the six
 * structural characters are separate kinds so the parser dispatches on the kind alone, never on the lexeme.
 */
enum class Token_kind : std::size_t
{
    String,
    Number,
    True,
    False,
    Null,
    Left_brace,
    Right_brace,
    Left_bracket,
    Right_bracket,
    Colon,
    Comma,
    Whitespace,
};

/**
 * @brief Whether a token kind is trivia the reader discards.
 * @param kind The kind to classify.
 * @return True for whitespace, false for every kind the parser consumes.
 */
[[nodiscard]] constexpr bool is_trivia(const Token_kind kind) noexcept
{
    return kind == Token_kind::Whitespace;
}

/**
 * @brief Builds the JSON lexer over munch.
 *
 * The token set is RFC 8259's: strings whose interior is any code point from U+0020 up except the quote and the
 * backslash, encoded as well-formed UTF-8, or one of the eight simple escapes, or a \u escape of four hex digits;
 * numbers as the RFC grammar spells them, an optional minus, an integer part without leading zeros, an optional
 * fraction and an optional exponent; the literals true, false and null; the six structural characters; and runs of
 * the four whitespace bytes. A string whose bytes are not well-formed UTF-8 does not tokenize, so the parser never
 * sees one. The \u escapes are checked only for their hex digits here; pairing of surrogates is the parser's business,
 * since a lone surrogate is a valid token that names no character.
 * @return The compiled lexer.
 */
[[nodiscard]] munch::core::Lexer lexer();
} // namespace hopper::json

#endif // HOPPER_LIBS_JSON_INCLUDE_HOPPER_JSON_TOKENS_HPP
