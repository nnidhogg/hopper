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
        return {.node = ast::Empty{}};
    }

    auto expr{parse_assignment()};

    expect(Token_kind::Semicolon, "';' after the expression");

    return {.node = ast::Expr_stmt{.expr = std::move(expr)}};
}

ast::Stmt Parser::parse_compound_statement()
{
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

    return {.node = ast::Compound{.statements = std::move(statements)}};
}

ast::Stmt Parser::parse_if_statement()
{
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

    return {.node = ast::If{
                    .condition = std::move(condition),
                    .then_branch = std::make_unique<ast::Stmt>(std::move(then_branch)),
                    .else_branch = std::move(else_branch),
            }};
}

ast::Stmt Parser::parse_while_statement()
{
    expect(Token_kind::Keyword_while, "'while'");

    expect(Token_kind::Left_paren, "'(' after 'while'");

    auto condition{parse_assignment()};

    expect(Token_kind::Right_paren, "')' to close the condition");

    auto body{parse_statement_node()};

    return {.node = ast::While{
                    .condition = std::move(condition),
                    .body = std::make_unique<ast::Stmt>(std::move(body)),
            }};
}

ast::Stmt Parser::parse_return_statement()
{
    expect(Token_kind::Keyword_return, "'return'");

    if (accept(Token_kind::Semicolon))
    {
        return {.node = ast::Return{.value = std::nullopt}};
    }

    auto value{parse_assignment()};

    expect(Token_kind::Semicolon, "';' after the return value");

    return {.node = ast::Return{.value = std::move(value)}};
}

} // namespace hopper::cpp
