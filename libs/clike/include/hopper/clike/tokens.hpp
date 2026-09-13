#ifndef HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_TOKENS_HPP
#define HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_TOKENS_HPP

#include <cstdint>

#include <munch/core/lexer.hpp>

namespace hopper::clike
{
/**
 * @brief The token kinds of the C-like study grammar.
 *
 * This is the token set of the row "c-like conventional with strings and line comments" in munch's recovery-quality
 * campaign (tools/probes/src/recovery_quality.cpp in the munch repository), kept as that row spells it so a parser
 * over the measured grammar exists beside the measurements: an identifier, a digit run, one operator byte, one
 * punctuation byte, a string, a line comment and a whitespace run. Keywords are identifiers here and multi-byte
 * operators are runs of operator bytes; both are the parser's to recognize.
 */
enum class Token_kind : std::uint8_t
{
    Identifier,
    Number,
    Operator,
    Punctuation,
    String,
    Line_comment,
    Whitespace,
};

/**
 * @brief Whether a token kind is trivia the reader discards.
 * @param kind The kind to classify.
 * @return True for whitespace and line comments, false for every kind the parser consumes.
 */
[[nodiscard]] constexpr bool is_trivia(const Token_kind kind) noexcept
{
    return kind == Token_kind::Whitespace || kind == Token_kind::Line_comment;
}

/**
 * @brief Builds the study grammar's lexer over munch.
 *
 * The identifier is a letter or underscore followed by letters, digits and underscores; the number a run of decimal
 * digits; the operator one of the thirteen bytes + - * / < > = ! & | ^ % ~ and the punctuation one of the eleven
 * bytes ( ) [ ] { } ; , . : ?; the string a quote, any bytes but a quote or a newline, and a quote, with no escapes;
 * the line comment two slashes to the end of the line; the whitespace a run of spaces, tabs and newlines, so a
 * carriage return is not part of this grammar and an input carrying one does not tokenize.
 * @return The compiled lexer.
 */
[[nodiscard]] munch::core::Lexer lexer();

} // namespace hopper::clike

#endif // HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_TOKENS_HPP
