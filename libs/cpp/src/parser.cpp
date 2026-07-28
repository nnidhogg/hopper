#include "hopper/cpp/parser.hpp"

#include <charconv>
#include <stdexcept>
#include <utility>

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

} // namespace

Parser::Parser(Token_reader reader) : reader_{std::move(reader)}
{}

Parser::Parser(munch::core::Lexer lexer, const std::string& input) : reader_{std::move(lexer), input}
{}

Parser::Parser(munch::core::Lexer lexer, const std::filesystem::path& file) : reader_{std::move(lexer), file}
{}

std::optional<Parser::Token_t> Parser::next_token()
{
    const auto result{reader_.next()};

    if (result.has_error())
    {
        throw std::runtime_error{"Lexical error: " + result.error().message()};
    }

    if (result.has_token())
    {
        return result.token();
    }

    return std::nullopt;
}

std::optional<Parser::Token_t> Parser::peek_token()
{
    const auto result{reader_.peek()};

    if (result.has_error())
    {
        throw std::runtime_error{"Lexical error: " + result.error().message()};
    }

    if (result.has_token())
    {
        return result.token();
    }

    return std::nullopt;
}

bool Parser::check(const Token_kind kind)
{
    const auto token{peek_token()};

    return token && token->kind() == kind;
}

std::optional<Parser::Token_t> Parser::accept(const Token_kind kind)
{
    if (!check(kind))
    {
        return std::nullopt;
    }

    return next_token();
}

Parser::Token_t Parser::expect(const Token_kind kind, const std::string_view what)
{
    const auto token{next_token()};

    if (!token)
    {
        eof_error("Expected " + std::string(what) + " before end of input");
    }

    if (token->kind() != kind)
    {
        syntax_error("Expected " + std::string(what), *token);
    }

    return *token;
}

[[noreturn]] void Parser::syntax_error(const std::string_view message, const Token_t& where)
{
    throw std::runtime_error{"Syntax error: " + std::string(message) + ", got '" + std::string(where.lexeme()) + "'"};
}

[[noreturn]] void Parser::eof_error(const std::string_view message)
{
    throw std::runtime_error{"Syntax error: " + std::string(message)};
}

ast::Expr Parser::parse_expression()
{
    auto expr{parse_ternary()};

    if (const auto trailing{peek_token()}; trailing)
    {
        syntax_error("Unexpected trailing input", *trailing);
    }

    return expr;
}

ast::Expr Parser::parse_ternary()
{
    auto condition{parse_logical_or()};

    if (!accept(Token_kind::Question))
    {
        return condition;
    }

    // The real C++ grammar takes a full `expression` here and an `assignment-expression` for the else branch;
    // both are simplified to `parse_ternary()` since this grammar has neither the comma operator nor assignment.
    auto then_branch{parse_ternary()};

    expect(Token_kind::Colon, "':' in conditional expression");

    auto else_branch{parse_ternary()};

    return {.node = ast::Ternary{
                    .condition = std::make_unique<ast::Expr>(std::move(condition)),
                    .then_branch = std::make_unique<ast::Expr>(std::move(then_branch)),
                    .else_branch = std::make_unique<ast::Expr>(std::move(else_branch)),
            }};
}

ast::Expr Parser::parse_logical_or()
{
    auto expr{parse_logical_and()};

    while (accept(Token_kind::Pipe_pipe))
    {
        expr = make_binary(ast::Binary_op::Logical_or, std::move(expr), parse_logical_and());
    }

    return expr;
}

ast::Expr Parser::parse_logical_and()
{
    auto expr{parse_equality()};

    while (accept(Token_kind::Amp_amp))
    {
        expr = make_binary(ast::Binary_op::Logical_and, std::move(expr), parse_equality());
    }

    return expr;
}

ast::Expr Parser::parse_equality()
{
    auto expr{parse_relational()};

    for (;;)
    {
        if (accept(Token_kind::Equal_equal))
        {
            expr = make_binary(ast::Binary_op::Equal, std::move(expr), parse_relational());
        }
        else if (accept(Token_kind::Bang_equal))
        {
            expr = make_binary(ast::Binary_op::Not_equal, std::move(expr), parse_relational());
        }
        else
        {
            return expr;
        }
    }
}

ast::Expr Parser::parse_relational()
{
    auto expr{parse_additive()};

    for (;;)
    {
        if (accept(Token_kind::Less))
        {
            expr = make_binary(ast::Binary_op::Less, std::move(expr), parse_additive());
        }
        else if (accept(Token_kind::Greater))
        {
            expr = make_binary(ast::Binary_op::Greater, std::move(expr), parse_additive());
        }
        else if (accept(Token_kind::Less_equal))
        {
            expr = make_binary(ast::Binary_op::Less_equal, std::move(expr), parse_additive());
        }
        else if (accept(Token_kind::Greater_equal))
        {
            expr = make_binary(ast::Binary_op::Greater_equal, std::move(expr), parse_additive());
        }
        else
        {
            return expr;
        }
    }
}

ast::Expr Parser::parse_additive()
{
    auto expr{parse_multiplicative()};

    for (;;)
    {
        if (accept(Token_kind::Plus))
        {
            expr = make_binary(ast::Binary_op::Add, std::move(expr), parse_multiplicative());
        }
        else if (accept(Token_kind::Minus))
        {
            expr = make_binary(ast::Binary_op::Subtract, std::move(expr), parse_multiplicative());
        }
        else
        {
            return expr;
        }
    }
}

ast::Expr Parser::parse_multiplicative()
{
    auto expr{parse_unary()};

    for (;;)
    {
        if (accept(Token_kind::Star))
        {
            expr = make_binary(ast::Binary_op::Multiply, std::move(expr), parse_unary());
        }
        else if (accept(Token_kind::Slash))
        {
            expr = make_binary(ast::Binary_op::Divide, std::move(expr), parse_unary());
        }
        else if (accept(Token_kind::Percent))
        {
            expr = make_binary(ast::Binary_op::Modulo, std::move(expr), parse_unary());
        }
        else
        {
            return expr;
        }
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

    return parse_primary();
}

ast::Expr Parser::parse_primary()
{
    if (const auto token{accept(Token_kind::Integer_literal)}; token)
    {
        long long value{};
        std::from_chars(token->lexeme().data(), token->lexeme().data() + token->lexeme().size(), value);

        return {.node = ast::Int_literal{.value = value}};
    }

    if (const auto token{accept(Token_kind::Floating_point_literal)}; token)
    {
        double value{};
        std::from_chars(token->lexeme().data(), token->lexeme().data() + token->lexeme().size(), value);

        return {.node = ast::Float_literal{.value = value}};
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
        auto expr{parse_ternary()};

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
