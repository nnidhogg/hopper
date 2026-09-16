#ifndef HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_STMT_HPP
#define HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_STMT_HPP

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "hopper/clike/ast/decl.hpp"
#include "hopper/clike/ast/expr.hpp"

namespace hopper::clike::ast
{
struct Stmt;

/**
 * @brief An expression evaluated for its effect, e.g. `f(x);`.
 */
struct Expr_stmt
{
    /**
     * @brief The expression evaluated for its effect.
     */
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
    /**
     * @brief The statements, in source order.
     */
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
    /**
     * @brief The condition.
     */
    Expr condition;

    /**
     * @brief The statement run when the condition holds.
     */
    std::unique_ptr<Stmt> then_branch;

    /**
     * @brief The statement run when it does not, or null when there is no else.
     */
    std::unique_ptr<Stmt> else_branch;
};

/**
 * @brief A `while` loop.
 */
struct While
{
    /**
     * @brief The condition tested before each iteration.
     */
    Expr condition;

    /**
     * @brief The loop body.
     */
    std::unique_ptr<Stmt> body;
};

/**
 * @brief A `for` loop.
 *
 * `init` is always present and holds what stood before the first semicolon: a Declaration, an Expr_stmt, or Empty,
 * mirroring the C++ grammar's init-statement. `condition` and `step` are absent when their slots were left empty,
 * as in `for (;;)`.
 */
struct For
{
    /**
     * @brief The initializer: a declaration, an expression statement or the empty statement.
     */
    std::unique_ptr<Stmt> init;

    /**
     * @brief The condition, when there is one.
     */
    std::optional<Expr> condition;

    /**
     * @brief The step expression, when there is one.
     */
    std::optional<Expr> step;

    /**
     * @brief The loop body.
     */
    std::unique_ptr<Stmt> body;
};

/**
 * @brief A `do`/`while` loop: the body runs before the condition is first tested.
 */
struct Do_while
{
    /**
     * @brief The loop body.
     */
    std::unique_ptr<Stmt> body;

    /**
     * @brief The condition tested after each iteration.
     */
    Expr condition;
};

/**
 * @brief A `return` statement with an optional value.
 */
struct Return
{
    /**
     * @brief The returned value, when there is one.
     */
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
    using Node_t = std::variant<Expr_stmt, Empty, Compound, If, While, For, Do_while, Return, Declaration>;

    /**
     * @brief The statement itself.
     */
    Node_t node;

    /**
     * @brief The source range the statement was parsed from.
     */
    parse::Source_span span{};
};

} // namespace hopper::clike::ast

#endif // HOPPER_LIBS_CLIKE_INCLUDE_HOPPER_CLIKE_AST_STMT_HPP
