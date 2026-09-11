#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hopper/clike/parser.hpp"

namespace hopper::clike
{
ast::Stmt Parser::parse_statement()
{
    auto stmt{parse_statement_node()};

    if (more())
    {
        unexpected("end of input");
    }

    return stmt;
}

ast::Stmt Parser::parse_statement_node()
{
    if (!more())
    {
        eof_error("Expected a statement before end of input");
    }

    const auto begin{here()};

    if (check_punctuation('{'))
    {
        return parse_compound_statement();
    }

    // The statements a keyword opens, dispatched by the keyword; each parser expects its own keyword again.
    static constexpr std::array<std::pair<std::string_view, ast::Stmt (Parser::*)()>, 5> keyworded{{
            {"if", &Parser::parse_if_statement},
            {"while", &Parser::parse_while_statement},
            {"for", &Parser::parse_for_statement},
            {"do", &Parser::parse_do_statement},
            {"return", &Parser::parse_return_statement},
    }};

    for (const auto& [word, parse] : keyworded)
    {
        if (check_keyword(word))
        {
            return (this->*parse)();
        }
    }

    if (is_declaration_start())
    {
        return parse_declaration_statement();
    }

    if (accept_punctuation(';'))
    {
        return {.node = ast::Empty{}, .span = close(begin)};
    }

    return parse_expression_statement("the expression");
}

ast::Stmt Parser::parse_compound_statement()
{
    const auto begin{here()};

    expect_punctuation('{', "'{' to open the block");

    std::vector<ast::Stmt> statements;

    while (!check_punctuation('}'))
    {
        if (!more())
        {
            eof_error("Expected '}' to close the block before end of input");
        }

        statements.push_back(parse_statement_node());
    }

    expect_punctuation('}', "'}' to close the block");

    return {.node = ast::Compound{.statements = std::move(statements)}, .span = close(begin)};
}

ast::Stmt Parser::parse_if_statement()
{
    const auto begin{here()};

    expect_keyword("if", "'if'");
    expect_punctuation('(', "'(' after 'if'");

    auto condition{parse_assignment()};

    expect_punctuation(')', "')' to close the condition");

    auto then_branch{parse_statement_node()};

    std::unique_ptr<ast::Stmt> else_branch;

    if (accept_keyword("else"))
    {
        else_branch = std::make_unique<ast::Stmt>(parse_statement_node());
    }

    return {.node =
                    ast::If{
                            .condition = std::move(condition),
                            .then_branch = std::make_unique<ast::Stmt>(std::move(then_branch)),
                            .else_branch = std::move(else_branch),
                    },
            .span = close(begin)};
}

ast::Stmt Parser::parse_while_statement()
{
    const auto begin{here()};

    expect_keyword("while", "'while'");
    expect_punctuation('(', "'(' after 'while'");

    auto condition{parse_assignment()};

    expect_punctuation(')', "')' to close the condition");

    auto body{parse_statement_node()};

    return {.node =
                    ast::While{
                            .condition = std::move(condition),
                            .body = std::make_unique<ast::Stmt>(std::move(body)),
                    },
            .span = close(begin)};
}

ast::Stmt Parser::parse_for_statement()
{
    const auto begin{here()};

    expect_keyword("for", "'for'");
    expect_punctuation('(', "'(' after 'for'");

    auto init{parse_for_initializer()};

    std::optional<ast::Expr> condition;

    if (!check_punctuation(';'))
    {
        condition = parse_assignment();
    }

    expect_punctuation(';', "';' after the loop condition");

    std::optional<ast::Expr> step;

    if (!check_punctuation(')'))
    {
        step = parse_assignment();
    }

    expect_punctuation(')', "')' to close the loop header");

    auto body{parse_statement_node()};

    return {.node =
                    ast::For{
                            .init = std::make_unique<ast::Stmt>(std::move(init)),
                            .condition = std::move(condition),
                            .step = std::move(step),
                            .body = std::make_unique<ast::Stmt>(std::move(body)),
                    },
            .span = close(begin)};
}

ast::Stmt Parser::parse_for_initializer()
{
    const auto begin{here()};

    if (accept_punctuation(';'))
    {
        return {.node = ast::Empty{}, .span = close(begin)};
    }

    if (is_declaration_start())
    {
        return parse_declaration_statement();
    }

    return parse_expression_statement("the loop initializer");
}

ast::Stmt Parser::parse_do_statement()
{
    const auto begin{here()};

    expect_keyword("do", "'do'");

    auto body{parse_statement_node()};

    expect_keyword("while", "'while' after the loop body");
    expect_punctuation('(', "'(' after 'while'");

    auto condition{parse_assignment()};

    expect_punctuation(')', "')' to close the condition");
    expect_punctuation(';', "';' after the do/while loop");

    return {.node =
                    ast::Do_while{
                            .body = std::make_unique<ast::Stmt>(std::move(body)),
                            .condition = std::move(condition),
                    },
            .span = close(begin)};
}

ast::Stmt Parser::parse_return_statement()
{
    const auto begin{here()};

    expect_keyword("return", "'return'");

    if (accept_punctuation(';'))
    {
        return {.node = ast::Return{.value = std::nullopt}, .span = close(begin)};
    }

    auto value{parse_assignment()};

    expect_punctuation(';', "';' after the return value");

    return {.node = ast::Return{.value = std::move(value)}, .span = close(begin)};
}

ast::Stmt Parser::parse_expression_statement(const std::string_view what)
{
    const auto begin{here()};

    auto expr{parse_assignment()};

    expect_punctuation(';', "';' after " + std::string{what});

    return {.node = ast::Expr_stmt{.expr = std::move(expr)}, .span = close(begin)};
}
} // namespace hopper::clike
