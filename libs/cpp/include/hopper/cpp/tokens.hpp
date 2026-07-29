#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKENS_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKENS_HPP

#include <cstdint>

namespace hopper::cpp
{
/**
 * @brief The lexical token kinds recognized by the expression grammar.
 *
 * Scoped to expressions only: literals, identifiers, the boolean keywords, parentheses, the operators needed for
 * the postfix / unary / multiplicative / additive / shift / relational / equality / bitwise-and / bitwise-xor /
 * bitwise-or / logical-and / logical-or / ternary / assignment precedence ladder, and calls, member access, and
 * subscript. Casts, declarations, and statements are not covered yet.
 */
enum class Token_kind : uint8_t
{
    // Literals and identifiers
    Integer_literal,
    Floating_point_literal,
    Identifier,
    Keyword_true,
    Keyword_false,

    // Grouping, calls, member access, subscript, and the ternary operator
    Left_paren,
    Right_paren,
    Left_bracket,
    Right_bracket,
    Dot,
    Arrow,
    Comma,
    Question,
    Colon,

    // Unary, postfix, and binary operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Bang,
    Tilde,
    Plus_plus,
    Minus_minus,
    Equal_equal,
    Bang_equal,
    Less,
    Greater,
    Less_equal,
    Greater_equal,
    Less_less,
    Greater_greater,
    Amp,
    Pipe,
    Caret,
    Amp_amp,
    Pipe_pipe,
    Equal,
    Plus_equal,
    Minus_equal,
    Star_equal,
    Slash_equal,
    Percent_equal,
    Amp_equal,
    Pipe_equal,
    Caret_equal,
    Less_less_equal,
    Greater_greater_equal,

    // Trivia
    Whitespace,
    Newline,
};

/**
 * @brief Returns true for token kinds the parser discards between meaningful tokens.
 */
[[nodiscard]] constexpr bool is_trivia(const Token_kind kind) noexcept
{
    return kind == Token_kind::Whitespace || kind == Token_kind::Newline;
}

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKENS_HPP
