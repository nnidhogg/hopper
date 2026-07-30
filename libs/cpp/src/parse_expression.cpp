#include <charconv>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hopper/cpp/binary_operator.hpp"
#include "hopper/cpp/parser.hpp"

namespace hopper::cpp
{
namespace
{
ast::Expr make_unary(const ast::Unary_op op, ast::Expr operand)
{
    return {.node = ast::Unary{.op = op, .operand = std::make_unique<ast::Expr>(std::move(operand))}};
}

ast::Expr make_binary(const ast::Binary_op op, ast::Expr lhs, ast::Expr rhs)
{
    return {.node = ast::Binary{
                    .op = op,
                    .lhs = std::make_unique<ast::Expr>(std::move(lhs)),
                    .rhs = std::make_unique<ast::Expr>(std::move(rhs)),
            }};
}

ast::Expr make_assign(const ast::Assign_op op, ast::Expr target, ast::Expr value)
{
    return {.node = ast::Assign{
                    .op = op,
                    .target = std::make_unique<ast::Expr>(std::move(target)),
                    .value = std::make_unique<ast::Expr>(std::move(value)),
            }};
}

std::optional<ast::Assign_op> assign_operator_for(const Token_kind kind)
{
    switch (kind)
    {
    case Token_kind::Equal:
        return ast::Assign_op::Assign;
    case Token_kind::Plus_equal:
        return ast::Assign_op::Add;
    case Token_kind::Minus_equal:
        return ast::Assign_op::Subtract;
    case Token_kind::Star_equal:
        return ast::Assign_op::Multiply;
    case Token_kind::Slash_equal:
        return ast::Assign_op::Divide;
    case Token_kind::Percent_equal:
        return ast::Assign_op::Modulo;
    case Token_kind::Amp_equal:
        return ast::Assign_op::Bitwise_and;
    case Token_kind::Pipe_equal:
        return ast::Assign_op::Bitwise_or;
    case Token_kind::Caret_equal:
        return ast::Assign_op::Bitwise_xor;
    case Token_kind::Less_less_equal:
        return ast::Assign_op::Shift_left;
    case Token_kind::Greater_greater_equal:
        return ast::Assign_op::Shift_right;
    default:
        return std::nullopt;
    }
}

} // namespace

ast::Expr Parser::parse_assignment()
{
    auto expr{parse_ternary()};

    const auto token{peek_token()};
    const auto op{token ? assign_operator_for(token->kind()) : std::nullopt};

    if (!op)
    {
        return expr;
    }

    (void)next_token();

    // Right-associative: the right-hand side is itself a full assignment-expression, so `a = b = c` parses as
    // `a = (b = c)`. `expr` (the target) is not checked for being an lvalue here; see ast::Assign.
    return make_assign(*op, std::move(expr), parse_assignment());
}

ast::Expr Parser::parse_ternary()
{
    // 1 is Logical_or's precedence in binary_operator_for(), the loosest-binding binary operator.
    auto condition{parse_binary(1)};

    if (!accept(Token_kind::Question))
    {
        return condition;
    }

    // The real C++ grammar takes a full `expression` here; simplified to parse_assignment() since this grammar
    // has no comma operator. The else branch is exactly an assignment-expression in real C++ too.
    auto then_branch{parse_assignment()};

    expect(Token_kind::Colon, "':' in conditional expression");

    auto else_branch{parse_assignment()};

    return {.node = ast::Ternary{
                    .condition = std::make_unique<ast::Expr>(std::move(condition)),
                    .then_branch = std::make_unique<ast::Expr>(std::move(then_branch)),
                    .else_branch = std::make_unique<ast::Expr>(std::move(else_branch)),
            }};
}

ast::Expr Parser::parse_binary(const int min_precedence)
{
    auto expr{parse_unary()};

    for (;;)
    {
        const auto token{peek_token()};
        const auto info{token ? binary_operator_for(token->kind()) : std::nullopt};

        if (!info || info->precedence < min_precedence)
        {
            return expr;
        }

        (void)next_token();

        // info->precedence + 1 stops the recursive call from also consuming an operator of the same precedence,
        // which is what makes every level in this ladder left-associative.
        expr = make_binary(info->op, std::move(expr), parse_binary(info->precedence + 1));
    }
}

ast::Expr Parser::parse_unary()
{
    if (accept(Token_kind::Plus))
    {
        return make_unary(ast::Unary_op::Plus, parse_unary());
    }

    if (accept(Token_kind::Minus))
    {
        return make_unary(ast::Unary_op::Minus, parse_unary());
    }

    if (accept(Token_kind::Bang))
    {
        return make_unary(ast::Unary_op::Not, parse_unary());
    }

    if (accept(Token_kind::Tilde))
    {
        return make_unary(ast::Unary_op::Bitwise_not, parse_unary());
    }

    if (accept(Token_kind::Plus_plus))
    {
        return make_unary(ast::Unary_op::Pre_increment, parse_unary());
    }

    if (accept(Token_kind::Minus_minus))
    {
        return make_unary(ast::Unary_op::Pre_decrement, parse_unary());
    }

    // '&' and '*' are prefix operators here and binary operators after a left operand; the position decides,
    // exactly as in C++.
    if (accept(Token_kind::Amp))
    {
        return make_unary(ast::Unary_op::Address_of, parse_unary());
    }

    if (accept(Token_kind::Star))
    {
        return make_unary(ast::Unary_op::Dereference, parse_unary());
    }

    return parse_postfix();
}

ast::Expr Parser::parse_postfix()
{
    auto expr{parse_primary()};

    for (;;)
    {
        if (accept(Token_kind::Left_paren))
        {
            std::vector<ast::Expr> arguments;

            if (!check(Token_kind::Right_paren))
            {
                arguments.push_back(parse_assignment());

                while (accept(Token_kind::Comma))
                {
                    arguments.push_back(parse_assignment());
                }
            }

            expect(Token_kind::Right_paren, "')' to close the argument list");

            expr = {.node = ast::Call{
                            .callee = std::make_unique<ast::Expr>(std::move(expr)),
                            .arguments = std::move(arguments),
                    }};
        }
        else if (accept(Token_kind::Left_bracket))
        {
            auto index{parse_assignment()};

            expect(Token_kind::Right_bracket, "']' to close the subscript");

            expr = {.node = ast::Subscript{
                            .object = std::make_unique<ast::Expr>(std::move(expr)),
                            .index = std::make_unique<ast::Expr>(std::move(index)),
                    }};
        }
        else if (accept(Token_kind::Dot))
        {
            const auto member{expect(Token_kind::Identifier, "a member name after '.'")};

            expr = {.node = ast::Member{
                            .op = ast::Member_op::Dot,
                            .object = std::make_unique<ast::Expr>(std::move(expr)),
                            .member = std::string{member.lexeme()},
                    }};
        }
        else if (accept(Token_kind::Arrow))
        {
            const auto member{expect(Token_kind::Identifier, "a member name after '->'")};

            expr = {.node = ast::Member{
                            .op = ast::Member_op::Arrow,
                            .object = std::make_unique<ast::Expr>(std::move(expr)),
                            .member = std::string{member.lexeme()},
                    }};
        }
        else if (accept(Token_kind::Plus_plus))
        {
            expr = {.node = ast::Postfix{
                            .op = ast::Postfix_op::Increment,
                            .operand = std::make_unique<ast::Expr>(std::move(expr))}};
        }
        else if (accept(Token_kind::Minus_minus))
        {
            expr = {.node = ast::Postfix{
                            .op = ast::Postfix_op::Decrement,
                            .operand = std::make_unique<ast::Expr>(std::move(expr))}};
        }
        else
        {
            return expr;
        }
    }
}

ast::Expr Parser::parse_primary()
{
    if (const auto token{accept(Token_kind::Integer_literal)}; token)
    {
        auto lexeme{token->lexeme()};

        // Hexadecimal and binary literals arrive with their prefix; from_chars expects the bare digits.
        int base{10};

        if (lexeme.starts_with("0x"))
        {
            base = 16;

            lexeme.remove_prefix(2);
        }
        else if (lexeme.starts_with("0b"))
        {
            base = 2;

            lexeme.remove_prefix(2);
        }

        long long value{};
        std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value, base);

        return {.node = ast::Int_literal{.value = value}};
    }

    if (const auto token{accept(Token_kind::Floating_point_literal)}; token)
    {
        double value{};
        std::from_chars(token->lexeme().data(), token->lexeme().data() + token->lexeme().size(), value);

        return {.node = ast::Float_literal{.value = value}};
    }

    if (const auto token{accept(Token_kind::String_literal)}; token)
    {
        const auto lexeme{token->lexeme()};

        return {.node = ast::String_literal{.value = std::string{lexeme.substr(1, lexeme.size() - 2)}}};
    }

    if (const auto token{accept(Token_kind::Character_literal)}; token)
    {
        const auto lexeme{token->lexeme()};

        return {.node = ast::Char_literal{.value = std::string{lexeme.substr(1, lexeme.size() - 2)}}};
    }

    if (accept(Token_kind::Keyword_true))
    {
        return {.node = ast::Bool_literal{.value = true}};
    }

    if (accept(Token_kind::Keyword_false))
    {
        return {.node = ast::Bool_literal{.value = false}};
    }

    if (const auto token{accept(Token_kind::Identifier)}; token)
    {
        return {.node = ast::Name{.identifier = std::string{token->lexeme()}}};
    }

    if (accept(Token_kind::Left_paren))
    {
        // Real C++ takes a full `expression` here (assignment and the comma operator both allowed); simplified
        // to parse_assignment() since this grammar has no comma operator.
        auto expr{parse_assignment()};

        expect(Token_kind::Right_paren, "')' to close '('");

        return expr;
    }

    const auto token{next_token()};

    if (token)
    {
        syntax_error("Expected an expression", *token);
    }

    eof_error("Expected an expression before end of input");
}

} // namespace hopper::cpp
