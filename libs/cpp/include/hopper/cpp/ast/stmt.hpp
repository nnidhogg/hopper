#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_STMT_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_STMT_HPP

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "hopper/cpp/ast/expr.hpp"

namespace hopper::cpp::ast
{
struct Stmt;

/**
 * @brief An expression evaluated for its effect, e.g. `f(x);`.
 */
struct Expr_stmt
{
    Expr expr;
};

/**
 * @brief The empty statement: a lone `;`.
 */
struct Empty
{
};

/**
 * @brief A brace-enclosed sequence of statements, e.g. `{ a = 1; b = 2; }`.
 */
struct Compound
{
    std::vector<Stmt> statements;
};

/**
 * @brief An `if` statement with an optional `else` branch.
 *
 * `else_branch` is null when there is no `else`. A dangling `else` binds to the nearest unmatched `if`, as in
 * real C++.
 */
struct If
{
    Expr condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch;
};

/**
 * @brief A `while` loop.
 */
struct While
{
    Expr condition;
    std::unique_ptr<Stmt> body;
};

/**
 * @brief A `return` statement with an optional value.
 */
struct Return
{
    std::optional<Expr> value;
};

/**
 * @brief A statement node, as a variant of plain structs rather than a class hierarchy.
 */
struct Stmt
{
    /**
     * @brief The kinds of node a statement can be.
     */
    using Node_t = std::variant<Expr_stmt, Empty, Compound, If, While, Return>;

    /**
     * @brief The node this statement holds.
     */
    Node_t node;
};

} // namespace hopper::cpp::ast

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_STMT_HPP
