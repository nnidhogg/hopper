#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKENS_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKENS_HPP

#include <cstdint>

namespace hopper::cpp
{
/**
 * @brief The lexical token kinds recognized by the expression grammar.
 *
 * Scoped to expressions only: literals, identifiers, the boolean keywords, parentheses, and the operators needed
 * for the unary / multiplicative / additive / relational / equality / logical-and / logical-or / ternary precedence
 * ladder. Declarations, statements, and postfix operators (calls, member access, subscript) are not covered yet.
 */
enum class Token_kind : uint8_t
{
    // Literals and identifiers
    Integer_literal,
    Floating_point_literal,
    Identifier,
    Keyword_true,
    Keyword_false,

    // Grouping and the ternary operator
    Left_paren,
    Right_paren,
    Question,
    Colon,

    // Unary and binary operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Bang,
    Equal_equal,
    Bang_equal,
    Less,
    Greater,
    Less_equal,
    Greater_equal,
    Amp_amp,
    Pipe_pipe,

    // Trivia
    Whitespace,
    Newline,
};

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKENS_HPP
