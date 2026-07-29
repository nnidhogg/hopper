#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_BINARY_OPERATOR_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_BINARY_OPERATOR_HPP

#include <optional>

#include "hopper/cpp/ast/expr.hpp"
#include "hopper/cpp/tokens.hpp"

namespace hopper::cpp
{
/**
 * @brief A binary operator's precedence (higher binds tighter) and the AST node it produces.
 */
struct Binary_operator
{
    int precedence;
    ast::Binary_op op;
};

/**
 * @brief Look up the given token kind's place in the left-associative binary operator ladder.
 * @return The operator's precedence and AST node, or std::nullopt if `kind` is not a binary operator.
 */
[[nodiscard]] std::optional<Binary_operator> binary_operator_for(Token_kind kind);

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_BINARY_OPERATOR_HPP
