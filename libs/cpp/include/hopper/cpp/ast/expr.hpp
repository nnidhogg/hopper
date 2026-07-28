#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_EXPR_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_EXPR_HPP

#include <memory>
#include <string>
#include <variant>

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
    Logical_and,
    Logical_or,
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
 * @brief An expression node, as a variant of plain structs rather than a class hierarchy.
 */
struct Expr
{
    /**
     * @brief The kinds of node an expression can be.
     */
    using Node_t = std::variant<Int_literal, Float_literal, Bool_literal, Name, Unary, Binary, Ternary>;

    /**
     * @brief The node this expression holds.
     */
    Node_t node;
};

} // namespace hopper::cpp::ast

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_EXPR_HPP
