#include "hopper/clike/binary_operator.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace hopper::clike
{
namespace
{
/**
 * @brief The precedence ladder, one entry per binary operator spelling.
 *
 * A table rather than a chain of comparisons, so the ladder reads as the grammar states it, rung by rung, and adding
 * an operator is one line that cannot fall out of order.
 */
constexpr std::array<std::pair<std::string_view, Binary_operator>, 18> ladder{{
        {"||", {.precedence = 1, .op = ast::Binary_op::Logical_or}},
        {"&&", {.precedence = 2, .op = ast::Binary_op::Logical_and}},
        {"|", {.precedence = 3, .op = ast::Binary_op::Bitwise_or}},
        {"^", {.precedence = 4, .op = ast::Binary_op::Bitwise_xor}},
        {"&", {.precedence = 5, .op = ast::Binary_op::Bitwise_and}},
        {"==", {.precedence = 6, .op = ast::Binary_op::Equal}},
        {"!=", {.precedence = 6, .op = ast::Binary_op::Not_equal}},
        {"<", {.precedence = 7, .op = ast::Binary_op::Less}},
        {">", {.precedence = 7, .op = ast::Binary_op::Greater}},
        {"<=", {.precedence = 7, .op = ast::Binary_op::Less_equal}},
        {">=", {.precedence = 7, .op = ast::Binary_op::Greater_equal}},
        {"<<", {.precedence = 8, .op = ast::Binary_op::Shift_left}},
        {">>", {.precedence = 8, .op = ast::Binary_op::Shift_right}},
        {"+", {.precedence = 9, .op = ast::Binary_op::Add}},
        {"-", {.precedence = 9, .op = ast::Binary_op::Subtract}},
        {"*", {.precedence = 10, .op = ast::Binary_op::Multiply}},
        {"/", {.precedence = 10, .op = ast::Binary_op::Divide}},
        {"%", {.precedence = 10, .op = ast::Binary_op::Modulo}},
}};

/**
 * @brief Every operator spelling the language knows, the assignment and unary forms included, for prefix fusion.
 */
constexpr std::array<std::string_view, 34> operators{
        "+",  "-",  "*",  "/",  "%",  "<",  ">",  "=",  "!",  "&",  "|",  "^",  "~",  "++", "--", "==",  "!=",
        "<=", ">=", "<<", ">>", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "<<=", ">>="};
} // namespace

std::optional<Binary_operator> binary_operator_for(const std::string_view spelling)
{
    const auto found{std::ranges::find(ladder, spelling, &std::pair<std::string_view, Binary_operator>::first)};

    return found != ladder.end() ? std::optional{found->second} : std::nullopt;
}

bool is_operator_prefix(const std::string_view spelling) noexcept
{
    return std::ranges::any_of(
            operators, [spelling](const std::string_view candidate) { return candidate.starts_with(spelling); });
}
} // namespace hopper::clike
