#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "hopper/cpp/parser.hpp"

namespace hopper::cpp
{
ast::Stmt Parser::parse_statement()
{
    auto stmt{parse_statement_node()};

    if (const auto trailing{peek_token()}; trailing)
    {
        syntax_error("Unexpected trailing input", *trailing);
    }

    return stmt;
}

ast::Stmt Parser::parse_statement_node()
{
    if (!peek_token())
    {
        eof_error("Expected a statement before end of input");
    }

    const auto begin{mark()};

    if (check(Token_kind::Left_brace))
    {
        return parse_compound_statement();
    }

    if (check(Token_kind::Keyword_if))
    {
        return parse_if_statement();
    }

    if (check(Token_kind::Keyword_while))
    {
        return parse_while_statement();
    }

    if (check(Token_kind::Keyword_for))
    {
        return parse_for_statement();
    }

    if (check(Token_kind::Keyword_do))
    {
        return parse_do_statement();
    }

    if (check(Token_kind::Keyword_return))
    {
        return parse_return_statement();
    }

    if (is_declaration_start())
    {
        return parse_declaration_statement();
    }

    if (accept(Token_kind::Semicolon))
    {
        return {.node = ast::Empty{}, .span = span_from(begin)};
    }

    auto expr{parse_assignment()};

    expect(Token_kind::Semicolon, "';' after the expression");

    return {.node = ast::Expr_stmt{.expr = std::move(expr)}, .span = span_from(begin)};
}

ast::Stmt Parser::parse_compound_statement()
{
    const auto begin{mark()};

    expect(Token_kind::Left_brace, "'{' to open the block");

    std::vector<ast::Stmt> statements;

    while (!check(Token_kind::Right_brace))
    {
        if (!peek_token())
        {
            eof_error("Expected '}' to close the block before end of input");
        }

        statements.push_back(parse_statement_node());
    }

    expect(Token_kind::Right_brace, "'}' to close the block");

    return {.node = ast::Compound{.statements = std::move(statements)}, .span = span_from(begin)};
}

ast::Stmt Parser::parse_if_statement()
{
    const auto begin{mark()};

    expect(Token_kind::Keyword_if, "'if'");

    expect(Token_kind::Left_paren, "'(' after 'if'");

    auto condition{parse_assignment()};

    expect(Token_kind::Right_paren, "')' to close the condition");

    auto then_branch{parse_statement_node()};

    // Accepting the else here, at the innermost if still being parsed, is what binds a dangling else to the
    // nearest unmatched if.
    std::unique_ptr<ast::Stmt> else_branch;

    if (accept(Token_kind::Keyword_else))
    {
        else_branch = std::make_unique<ast::Stmt>(parse_statement_node());
    }

    return {.node =
                    ast::If{
                            .condition = std::move(condition),
                            .then_branch = std::make_unique<ast::Stmt>(std::move(then_branch)),
                            .else_branch = std::move(else_branch),
                    },
            .span = span_from(begin)};
}

ast::Stmt Parser::parse_while_statement()
{
    const auto begin{mark()};

    expect(Token_kind::Keyword_while, "'while'");

    expect(Token_kind::Left_paren, "'(' after 'while'");

    auto condition{parse_assignment()};

    expect(Token_kind::Right_paren, "')' to close the condition");

    auto body{parse_statement_node()};

    return {.node =
                    ast::While{
                            .condition = std::move(condition),
                            .body = std::make_unique<ast::Stmt>(std::move(body)),
                    },
            .span = span_from(begin)};
}

ast::Stmt Parser::parse_for_statement()
{
    const auto begin{mark()};

    expect(Token_kind::Keyword_for, "'for'");

    expect(Token_kind::Left_paren, "'(' after 'for'");

    // The init-statement carries its own semicolon in all three of its forms, exactly as in the C++ grammar: a
    // declaration, an expression statement, or the empty statement.
    auto init{[this]() -> ast::Stmt {
        const auto init_begin{mark()};

        if (accept(Token_kind::Semicolon))
        {
            return {.node = ast::Empty{}, .span = span_from(init_begin)};
        }

        if (is_declaration_start())
        {
            return parse_declaration_statement();
        }

        auto expr{parse_assignment()};

        expect(Token_kind::Semicolon, "';' after the loop initializer");

        return {.node = ast::Expr_stmt{.expr = std::move(expr)}, .span = span_from(init_begin)};
    }()};

    std::optional<ast::Expr> condition;

    if (!check(Token_kind::Semicolon))
    {
        condition = parse_assignment();
    }

    expect(Token_kind::Semicolon, "';' after the loop condition");

    std::optional<ast::Expr> step;

    if (!check(Token_kind::Right_paren))
    {
        step = parse_assignment();
    }

    expect(Token_kind::Right_paren, "')' to close the loop header");

    auto body{parse_statement_node()};

    return {.node =
                    ast::For{
                            .init = std::make_unique<ast::Stmt>(std::move(init)),
                            .condition = std::move(condition),
                            .step = std::move(step),
                            .body = std::make_unique<ast::Stmt>(std::move(body)),
                    },
            .span = span_from(begin)};
}

ast::Stmt Parser::parse_do_statement()
{
    const auto begin{mark()};

    expect(Token_kind::Keyword_do, "'do'");

    auto body{parse_statement_node()};

    expect(Token_kind::Keyword_while, "'while' after the loop body");

    expect(Token_kind::Left_paren, "'(' after 'while'");

    auto condition{parse_assignment()};

    expect(Token_kind::Right_paren, "')' to close the condition");

    expect(Token_kind::Semicolon, "';' after the do/while loop");

    return {.node =
                    ast::Do_while{
                            .body = std::make_unique<ast::Stmt>(std::move(body)),
                            .condition = std::move(condition),
                    },
            .span = span_from(begin)};
}

ast::Stmt Parser::parse_return_statement()
{
    const auto begin{mark()};

    expect(Token_kind::Keyword_return, "'return'");

    if (accept(Token_kind::Semicolon))
    {
        return {.node = ast::Return{.value = std::nullopt}, .span = span_from(begin)};
    }

    auto value{parse_assignment()};

    expect(Token_kind::Semicolon, "';' after the return value");

    return {.node = ast::Return{.value = std::move(value)}, .span = span_from(begin)};
}

} // namespace hopper::cpp
