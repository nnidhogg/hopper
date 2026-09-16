#ifndef HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_EXPR_HPP
#define HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_EXPR_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "hopper/clike/ast/type.hpp"
#include "hopper/parse/source_span.hpp"

namespace hopper::clike::ast
{
struct Expr;

/**
 * @brief A prefix operator applied to a single operand.
 */
enum class Unary_op : std::uint8_t
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
enum class Binary_op : std::uint8_t
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
enum class Postfix_op : std::uint8_t
{
    Increment,
    Decrement,
};

/**
 * @brief How a member is accessed: `object.member` or `object->member`.
 */
enum class Member_op : std::uint8_t
{
    Dot,
    Arrow,
};

/**
 * @brief An assignment operator: plain `=`, or a compound assignment combining it with a binary operator.
 */
enum class Assign_op : std::uint8_t
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
    /**
     * @brief The literal's value.
     */
    long long value;
};

/**
 * @brief A boolean literal: `true` or `false`.
 */
struct Bool_literal
{
    /**
     * @brief The literal's value.
     */
    bool value;
};

/**
 * @brief A string literal, e.g. `"hello"`.
 *
 * Holds the bytes between the quotes as written; the study grammar has no escapes, so a quote cannot appear inside.
 */
struct String_literal
{
    /**
     * @brief The bytes between the quotes.
     */
    std::string value;
};

/**
 * @brief A reference to a named entity, e.g. `x`.
 */
struct Name
{
    /**
     * @brief The identifier as spelled.
     */
    std::string identifier;
};

/**
 * @brief A prefix operator applied to a single operand, e.g. `-x`, `!done`.
 */
struct Unary
{
    /**
     * @brief The operator applied.
     */
    Unary_op op;

    /**
     * @brief The operand.
     */
    std::unique_ptr<Expr> operand;
};

/**
 * @brief A postfix operator applied to a single operand, e.g. `x++`.
 */
struct Postfix
{
    /**
     * @brief The operator applied.
     */
    Postfix_op op;

    /**
     * @brief The operand.
     */
    std::unique_ptr<Expr> operand;
};

/**
 * @brief A function call, e.g. `f(a, b)`. `callee` need not be a `Name` (e.g. `get_callback()()`).
 */
struct Call
{
    /**
     * @brief The expression called.
     */
    std::unique_ptr<Expr> callee;

    /**
     * @brief The arguments, in source order.
     */
    std::vector<Expr> arguments;
};

/**
 * @brief A member access, e.g. `object.member` or `pointer->member`.
 */
struct Member
{
    /**
     * @brief Whether the access is `.` or `->`.
     */
    Member_op op;

    /**
     * @brief The expression whose member is accessed.
     */
    std::unique_ptr<Expr> object;

    /**
     * @brief The member's name.
     */
    std::string member;
};

/**
 * @brief An array subscript, e.g. `array[index]`.
 */
struct Subscript
{
    /**
     * @brief The expression subscripted.
     */
    std::unique_ptr<Expr> object;

    /**
     * @brief The index expression.
     */
    std::unique_ptr<Expr> index;
};

/**
 * @brief The four named casts.
 */
enum class Cast_kind : std::uint8_t
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
    /**
     * @brief Which of the four named casts.
     */
    Cast_kind kind;

    /**
     * @brief The type cast to.
     */
    Type_id type;

    /**
     * @brief The expression cast.
     */
    std::unique_ptr<Expr> operand;
};

/**
 * @brief An infix operator applied to two operands, e.g. `a + b`.
 *
 * Left-associative for every operator in this grammar's precedence ladder.
 */
struct Binary
{
    /**
     * @brief The operator applied.
     */
    Binary_op op;

    /**
     * @brief The left operand.
     */
    std::unique_ptr<Expr> lhs;

    /**
     * @brief The right operand.
     */
    std::unique_ptr<Expr> rhs;
};

/**
 * @brief A conditional (`?:`) expression, e.g. `cond ? then_branch : else_branch`.
 *
 * Right-associative: `a ? b : c ? d : e` parses as `a ? b : (c ? d : e)`.
 */
struct Ternary
{
    /**
     * @brief The condition.
     */
    std::unique_ptr<Expr> condition;

    /**
     * @brief The value when the condition holds.
     */
    std::unique_ptr<Expr> then_branch;

    /**
     * @brief The value when it does not.
     */
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
    /**
     * @brief The assignment performed.
     */
    Assign_op op;

    /**
     * @brief The assigned-to expression.
     */
    std::unique_ptr<Expr> target;

    /**
     * @brief The assigned value.
     */
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
            Int_literal, Bool_literal, String_literal, Name, Unary, Postfix, Call, Member, Subscript, Binary, Ternary,
            Assign, Cast>;

    /**
     * @brief The expression itself.
     */
    Node_t node;

    /**
     * @brief The source range the expression was parsed from.
     */
    parse::Source_span span{};
};

} // namespace hopper::clike::ast

#endif // HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_EXPR_HPP
