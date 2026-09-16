#ifndef HOPPER_LIBS_CLIKE_SRC_BINARY_OPERATOR_HPP
#define HOPPER_LIBS_CLIKE_SRC_BINARY_OPERATOR_HPP

#include <optional>
#include <string_view>

#include "hopper/clike/ast/expr.hpp"

namespace hopper::clike
{
/**
 * @brief A binary operator's place in the precedence ladder and the node it builds.
 */
struct Binary_operator
{
    /**
     * @brief The rung on the ladder, one for the loosest binding operator upward.
     */
    int precedence;

    /**
     * @brief The node the operator builds.
     */
    ast::Binary_op op;
};

/**
 * @brief Looks an operator spelling up in the binary precedence ladder.
 *
 * The ladder is C's, left-associative throughout: logical or at 1, logical and at 2, bitwise or, xor and and at 3 to
 * 5, equality at 6, relational at 7, shifts at 8, additive at 9 and multiplicative at 10.
 * @param spelling The fused operator spelling.
 * @return The operator's precedence and node kind, or nothing when the spelling is not a binary operator.
 */
[[nodiscard]] std::optional<Binary_operator> binary_operator_for(std::string_view spelling) noexcept;

/**
 * @brief Whether a spelling is a prefix of an operator the language knows, itself included.
 *
 * The parser fuses adjacent operator bytes while this holds, so the fused spelling is always the longest operator
 * the bytes begin, as a C lexer would read them.
 * @param spelling The candidate spelling.
 * @return True when some operator starts with it.
 */
[[nodiscard]] bool is_operator_prefix(std::string_view spelling) noexcept;

} // namespace hopper::clike

#endif // HOPPER_LIBS_CLIKE_SRC_BINARY_OPERATOR_HPP
