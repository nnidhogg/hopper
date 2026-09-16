#include <algorithm>
#include <array>
#include <charconv>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "binary_operator.hpp"
#include "hopper/clike/parser.hpp"
#include "tables.hpp"

namespace hopper::clike
{
namespace
{
/**
 * @brief The prefix operators, in the order the parser tries them.
 */
constexpr std::array<std::pair<std::string_view, ast::Unary_op>, 8> prefixes{{
        {"+", ast::Unary_op::Plus},
        {"-", ast::Unary_op::Minus},
        {"!", ast::Unary_op::Not},
        {"~", ast::Unary_op::Bitwise_not},
        {"++", ast::Unary_op::Pre_increment},
        {"--", ast::Unary_op::Pre_decrement},
        {"&", ast::Unary_op::Address_of},
        {"*", ast::Unary_op::Dereference},
}};

/**
 * @brief The assignment operators and what each performs.
 */
constexpr std::array<std::pair<std::string_view, ast::Assign_op>, 11> assignments{{
        {"=", ast::Assign_op::Assign},
        {"+=", ast::Assign_op::Add},
        {"-=", ast::Assign_op::Subtract},
        {"*=", ast::Assign_op::Multiply},
        {"/=", ast::Assign_op::Divide},
        {"%=", ast::Assign_op::Modulo},
        {"&=", ast::Assign_op::Bitwise_and},
        {"|=", ast::Assign_op::Bitwise_or},
        {"^=", ast::Assign_op::Bitwise_xor},
        {"<<=", ast::Assign_op::Shift_left},
        {">>=", ast::Assign_op::Shift_right},
}};

/**
 * @brief The four cast keywords and the cast each selects.
 */
constexpr std::array<std::pair<std::string_view, ast::Cast_kind>, 4> casts{{
        {"static_cast", ast::Cast_kind::Static},
        {"dynamic_cast", ast::Cast_kind::Dynamic},
        {"const_cast", ast::Cast_kind::Const},
        {"reinterpret_cast", ast::Cast_kind::Reinterpret},
}};

/**
 * @brief The value an integer literal spells, when the platform's integer holds it.
 *
 * Read with std::from_chars rather than a library conversion that throws, so an overflow is the parser's syntax
 * error at the token and not an exception with no position.
 * @param lexeme The literal's digits.
 * @return The value, or std::nullopt when it is out of range.
 */
[[nodiscard]] std::optional<long long> integer_value(const std::string_view lexeme)
{
    long long value{};

    const auto [end, error]{std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value)};

    if (error != std::errc{} || end != lexeme.data() + lexeme.size())
    {
        return std::nullopt;
    }

    return value;
}

/**
 * @brief Wraps an operand in a unary node.
 * @param op The operator applied.
 * @param operand The operand.
 * @return The node, its span unset.
 */
[[nodiscard]] ast::Expr make_unary(const ast::Unary_op op, ast::Expr operand)
{
    return {.node = ast::Unary{.op = op, .operand = std::make_unique<ast::Expr>(std::move(operand))}};
}

/**
 * @brief Wraps an operand in a postfix node.
 * @param op The operator applied.
 * @param operand The operand.
 * @return The node, its span unset.
 */
[[nodiscard]] ast::Expr make_postfix(const ast::Postfix_op op, ast::Expr operand)
{
    return {.node = ast::Postfix{.op = op, .operand = std::make_unique<ast::Expr>(std::move(operand))}};
}

/**
 * @brief Joins two operands in a binary node.
 * @param op The operator applied.
 * @param lhs The left operand.
 * @param rhs The right operand.
 * @return The node, its span unset.
 */
[[nodiscard]] ast::Expr make_binary(const ast::Binary_op op, ast::Expr lhs, ast::Expr rhs)
{
    return {.node = ast::Binary{
                    .op = op,
                    .lhs = std::make_unique<ast::Expr>(std::move(lhs)),
                    .rhs = std::make_unique<ast::Expr>(std::move(rhs)),
            }};
}

/**
 * @brief Joins a target and a value in an assignment node.
 * @param op The assignment performed.
 * @param target The assigned-to expression.
 * @param value The assigned value.
 * @return The node, its span unset.
 */
[[nodiscard]] ast::Expr make_assign(const ast::Assign_op op, ast::Expr target, ast::Expr value)
{
    return {.node = ast::Assign{
                    .op = op,
                    .target = std::make_unique<ast::Expr>(std::move(target)),
                    .value = std::make_unique<ast::Expr>(std::move(value)),
            }};
}

} // namespace

ast::Expr Parser::parse_expression()
{
    auto expr{parse_assignment()};

    if (more())
    {
        unexpected("end of input");
    }

    return expr;
}

ast::Expr Parser::parse_assignment()
{
    const auto begin{here()};

    auto expr{parse_ternary()};

    const auto op{peek_operator()};

    const auto assign{op ? lookup(assignments, op->spelling) : std::nullopt};

    if (!assign)
    {
        return expr;
    }

    take_operator();

    // Right-associative by recursing into this same level for the value.
    auto node{make_assign(*assign, std::move(expr), parse_assignment())};

    node.span = close(begin);

    return node;
}

ast::Expr Parser::parse_ternary()
{
    const auto begin{here()};

    auto condition{parse_binary(1)};

    if (!accept_punctuation('?'))
    {
        return condition;
    }

    auto then_branch{parse_assignment()};

    expect_punctuation(':', "':' in the conditional expression");

    auto else_branch{parse_assignment()};

    return {.node =
                    ast::Ternary{
                            .condition = std::make_unique<ast::Expr>(std::move(condition)),
                            .then_branch = std::make_unique<ast::Expr>(std::move(then_branch)),
                            .else_branch = std::make_unique<ast::Expr>(std::move(else_branch)),
                    },
            .span = close(begin)};
}

ast::Expr Parser::parse_binary(const int min_precedence)
{
    const auto begin{here()};

    auto expr{parse_unary()};

    for (;;)
    {
        const auto op{peek_operator()};

        const auto info{op ? binary_operator_for(op->spelling) : std::nullopt};

        if (!info || info->precedence < min_precedence)
        {
            return expr;
        }

        take_operator();

        // The right operand may only bind tighter, which is what makes each level left-associative.
        expr = make_binary(info->op, std::move(expr), parse_binary(info->precedence + 1));

        expr.span = close(begin);
    }
}

ast::Expr Parser::parse_unary()
{
    const auto begin{here()};

    for (const auto& [spelling, op] : prefixes)
    {
        if (accept_operator(spelling))
        {
            auto expr{make_unary(op, parse_unary())};

            expr.span = close(begin);

            return expr;
        }
    }

    return parse_postfix();
}

ast::Expr Parser::parse_postfix()
{
    const auto begin{here()};

    auto expr{parse_primary()};

    for (;;)
    {
        if (accept_punctuation('('))
        {
            expr = parse_call(std::move(expr));
        }
        else if (accept_punctuation('['))
        {
            expr = parse_subscript(std::move(expr));
        }
        else if (accept_punctuation('.'))
        {
            expr = parse_member(std::move(expr), ast::Member_op::Dot);
        }
        else if (accept_operator("->"))
        {
            expr = parse_member(std::move(expr), ast::Member_op::Arrow);
        }
        else if (accept_operator("++"))
        {
            expr = make_postfix(ast::Postfix_op::Increment, std::move(expr));
        }
        else if (accept_operator("--"))
        {
            expr = make_postfix(ast::Postfix_op::Decrement, std::move(expr));
        }
        else
        {
            return expr;
        }

        expr.span = close(begin);
    }
}

ast::Expr Parser::parse_call(ast::Expr callee)
{
    std::vector<ast::Expr> arguments;

    if (!check_punctuation(')'))
    {
        arguments.push_back(parse_assignment());

        while (accept_punctuation(','))
        {
            arguments.push_back(parse_assignment());
        }
    }

    expect_punctuation(')', "')' to close the argument list");

    return {.node = ast::Call{
                    .callee = std::make_unique<ast::Expr>(std::move(callee)),
                    .arguments = std::move(arguments),
            }};
}

ast::Expr Parser::parse_subscript(ast::Expr object)
{
    auto index{parse_assignment()};

    expect_punctuation(']', "']' to close the subscript");

    return {.node = ast::Subscript{
                    .object = std::make_unique<ast::Expr>(std::move(object)),
                    .index = std::make_unique<ast::Expr>(std::move(index)),
            }};
}

ast::Expr Parser::parse_member(ast::Expr object, const ast::Member_op op)
{
    const auto member{
            expect_identifier(op == ast::Member_op::Dot ? "a member name after '.'" : "a member name after '->'")};

    return {.node = ast::Member{
                    .op = op,
                    .object = std::make_unique<ast::Expr>(std::move(object)),
                    .member = std::string{member.lexeme()},
            }};
}

ast::Expr Parser::parse_primary()
{
    const auto begin{here()};

    if (auto literal{parse_literal()})
    {
        return std::move(*literal);
    }

    if (accept_keyword("true"))
    {
        return {.node = ast::Bool_literal{.value = true}, .span = close(begin)};
    }

    if (accept_keyword("false"))
    {
        return {.node = ast::Bool_literal{.value = false}, .span = close(begin)};
    }

    if (const auto token{accept_identifier()})
    {
        return {.node = ast::Name{.identifier = std::string{token->lexeme()}}, .span = close(begin)};
    }

    if (auto cast{parse_cast()})
    {
        return std::move(*cast);
    }

    if (accept_punctuation('('))
    {
        auto expr{parse_assignment()};

        expect_punctuation(')', "')' to close '('");

        expr.span = close(begin);

        return expr;
    }

    unexpected("an expression");
}

std::optional<ast::Expr> Parser::parse_literal()
{
    if (pending_)
    {
        return std::nullopt;
    }

    const auto begin{here()};

    if (const auto token{accept(Token_kind::Number)})
    {
        const auto value{integer_value(token->lexeme())};

        if (!value)
        {
            syntax_error("Integer literal is out of range", *token);
        }

        return ast::Expr{.node = ast::Int_literal{.value = *value}, .span = close(begin)};
    }

    if (const auto token{accept(Token_kind::String)})
    {
        const auto lexeme{token->lexeme()};

        // The quotes are the token's, not the value's, and the grammar admits no escapes to decode.
        return ast::Expr{
                .node = ast::String_literal{.value = std::string{lexeme.substr(1, lexeme.size() - 2)}},
                .span = close(begin)};
    }

    return std::nullopt;
}

std::optional<ast::Expr> Parser::parse_cast()
{
    if (pending_)
    {
        return std::nullopt;
    }

    const auto token{peek_token()};

    const auto kind{token && token->kind() == Token_kind::Identifier ? lookup(casts, token->lexeme()) : std::nullopt};

    if (!kind)
    {
        return std::nullopt;
    }

    const auto begin{here()};

    static_cast<void>(next_token());

    expect_operator("<", "'<' after the cast keyword");

    const auto type{parse_type_id()};

    expect_operator(">", "'>' to close the cast type");
    expect_punctuation('(', "'(' to open the cast operand");

    auto operand{parse_assignment()};

    expect_punctuation(')', "')' to close the cast operand");

    return ast::Expr{
            .node = ast::Cast{.kind = *kind, .type = type, .operand = std::make_unique<ast::Expr>(std::move(operand))},
            .span = close(begin)};
}

} // namespace hopper::clike
