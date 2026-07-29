#include "hopper/cpp/binary_operator.hpp"

namespace hopper::cpp
{
std::optional<Binary_operator> binary_operator_for(const Token_kind kind)
{
    switch (kind)
    {
    case Token_kind::Pipe_pipe:
        return Binary_operator{1, ast::Binary_op::Logical_or};
    case Token_kind::Amp_amp:
        return Binary_operator{2, ast::Binary_op::Logical_and};
    case Token_kind::Pipe:
        return Binary_operator{3, ast::Binary_op::Bitwise_or};
    case Token_kind::Caret:
        return Binary_operator{4, ast::Binary_op::Bitwise_xor};
    case Token_kind::Amp:
        return Binary_operator{5, ast::Binary_op::Bitwise_and};
    case Token_kind::Equal_equal:
        return Binary_operator{6, ast::Binary_op::Equal};
    case Token_kind::Bang_equal:
        return Binary_operator{6, ast::Binary_op::Not_equal};
    case Token_kind::Less:
        return Binary_operator{7, ast::Binary_op::Less};
    case Token_kind::Greater:
        return Binary_operator{7, ast::Binary_op::Greater};
    case Token_kind::Less_equal:
        return Binary_operator{7, ast::Binary_op::Less_equal};
    case Token_kind::Greater_equal:
        return Binary_operator{7, ast::Binary_op::Greater_equal};
    case Token_kind::Less_less:
        return Binary_operator{8, ast::Binary_op::Shift_left};
    case Token_kind::Greater_greater:
        return Binary_operator{8, ast::Binary_op::Shift_right};
    case Token_kind::Plus:
        return Binary_operator{9, ast::Binary_op::Add};
    case Token_kind::Minus:
        return Binary_operator{9, ast::Binary_op::Subtract};
    case Token_kind::Star:
        return Binary_operator{10, ast::Binary_op::Multiply};
    case Token_kind::Slash:
        return Binary_operator{10, ast::Binary_op::Divide};
    case Token_kind::Percent:
        return Binary_operator{10, ast::Binary_op::Modulo};
    default:
        return std::nullopt;
    }
}

} // namespace hopper::cpp
