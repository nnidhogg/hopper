#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_EXPR_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_EXPR_HPP

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "hopper/cpp/ast/type.hpp"
#include "hopper/parse/source_span.hpp"

namespace hopper::cpp::ast
{
struct Expr;

/**
 * @brief A prefix operator applied to a single operand.
 */
enum class Unary_op
{
    Plus,
    Minus,
    Not,
    Bitwise_not,
    Pre_increment,
    Pre_decrement,
    Address_of,
    Dereference,
};

/**
 * @brief An infix operator applied to two operands.
 */
enum class Binary_op
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Less,
    Greater,
    Less_equal,
    Greater_equal,
    Equal,
    Not_equal,
    Shift_left,
    Shift_right,
    Bitwise_and,
    Bitwise_xor,
    Bitwise_or,
    Logical_and,
    Logical_or,
};

/**
 * @brief A postfix operator applied to a single operand: `x++` or `x--`.
 */
enum class Postfix_op
{
    Increment,
    Decrement,
};

/**
 * @brief How a member is accessed: `object.member` or `object->member`.
 */
enum class Member_op
{
    Dot,
    Arrow,
};

/**
 * @brief An assignment operator: plain `=`, or a compound assignment combining it with a binary operator.
 */
enum class Assign_op
{
    Assign,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Bitwise_and,
    Bitwise_xor,
    Bitwise_or,
    Shift_left,
    Shift_right,
};

/**
 * @brief An integer literal, e.g. `42`.
 */
struct Int_literal
{
    long long value;
};

/**
 * @brief A floating-point literal, e.g. `3.14`.
 */
struct Float_literal
{
    double value;
};

/**
 * @brief A boolean literal: `true` or `false`.
 */
struct Bool_literal
{
    bool value;
};

/**
 * @brief A string literal, e.g. `"hello\n"`.
 *
 * Holds the characters between the quotes with escape sequences left exactly as written; decoding them is a
 * semantic concern.
 */
struct String_literal
{
    std::string value;
};

/**
 * @brief A character literal, e.g. `'x'` or `'\n'`.
 *
 * Holds the character or escape between the quotes, undecoded, like String_literal.
 */
struct Char_literal
{
    std::string value;
};

/**
 * @brief A reference to a named entity, e.g. `x`.
 */
struct Name
{
    std::string identifier;
};

/**
 * @brief A prefix operator applied to a single operand, e.g. `-x`, `!done`.
 */
struct Unary
{
    Unary_op op;
    std::unique_ptr<Expr> operand;
};

/**
 * @brief A postfix operator applied to a single operand, e.g. `x++`.
 */
struct Postfix
{
    Postfix_op op;
    std::unique_ptr<Expr> operand;
};

/**
 * @brief A function call, e.g. `f(a, b)`. `callee` need not be a `Name` (e.g. `get_callback()()`).
 */
struct Call
{
    std::unique_ptr<Expr> callee;
    std::vector<Expr> arguments;
};

/**
 * @brief A member access, e.g. `object.member` or `pointer->member`.
 */
struct Member
{
    Member_op op;
    std::unique_ptr<Expr> object;
    std::string member;
};

/**
 * @brief An array subscript, e.g. `array[index]`.
 */
struct Subscript
{
    std::unique_ptr<Expr> object;
    std::unique_ptr<Expr> index;
};

/**
 * @brief The four named casts.
 */
enum class Cast_kind
{
    Static,
    Dynamic,
    Const,
    Reinterpret,
};

/**
 * @brief A named cast, e.g. `static_cast<int>(value)`.
 *
 * Syntactic only, like the rest of the tree: whether the cast is meaningful for the types involved is a semantic
 * concern.
 */
struct Cast
{
    Cast_kind kind;
    Type_id type;
    std::unique_ptr<Expr> operand;
};

/**
 * @brief An infix operator applied to two operands, e.g. `a + b`.
 *
 * Left-associative for every operator in this grammar's precedence ladder.
 */
struct Binary
{
    Binary_op op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

/**
 * @brief A conditional (`?:`) expression, e.g. `cond ? then_branch : else_branch`.
 *
 * Right-associative: `a ? b : c ? d : e` parses as `a ? b : (c ? d : e)`.
 */
struct Ternary
{
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> then_branch;
    std::unique_ptr<Expr> else_branch;
};

/**
 * @brief An assignment, e.g. `x = y` or `x += y`.
 *
 * Right-associative: `a = b = c` parses as `a = (b = c)`. `target` is not restricted to names here; rejecting a
 * non-assignable target (e.g. `1 = 2`) is left to a later semantic pass, not the parser.
 */
struct Assign
{
    Assign_op op;
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> value;
};

/**
 * @brief An expression node, as a variant of plain structs rather than a class hierarchy.
 */
struct Expr
{
    /**
     * @brief The kinds of node an expression can be.
     */
    using Node_t = std::variant<
            Int_literal, Float_literal, Bool_literal, String_literal, Char_literal, Name, Unary, Postfix, Call, Member,
            Subscript, Binary, Ternary, Assign, Cast>;

    /**
     * @brief The node this expression holds.
     */
    Node_t node;

    /**
     * @brief The source range this expression was parsed from, including any enclosing parentheses.
     */
    parse::Source_span span{};
};

} // namespace hopper::cpp::ast

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_EXPR_HPP
