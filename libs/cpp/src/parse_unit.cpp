#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "hopper/cpp/parser.hpp"

namespace hopper::cpp
{
ast::Translation_unit Parser::parse_translation_unit()
{
    ast::Translation_unit unit;

    while (peek_token())
    {
        const auto begin{mark()};

        auto node{parse_external_declaration()};

        unit.items.push_back({.node = std::move(node), .span = span_from(begin)});
    }

    return unit;
}

ast::Translation_unit::Item::Node_t Parser::parse_external_declaration()
{
    auto type{parse_type_specifier()};

    std::size_t pointers{0};

    while (accept(Token_kind::Star))
    {
        ++pointers;
    }

    const auto reference{accept(Token_kind::Amp).has_value()};

    const auto name{expect(Token_kind::Identifier, "a declarator name")};

    if (check(Token_kind::Left_paren))
    {
        return parse_function(type, pointers, reference, std::string{name.lexeme()});
    }

    // Not a function, so this was the first declarator of a variable declaration; the rest parses exactly as in
    // a declaration statement.
    std::optional<ast::Expr> initializer;

    if (accept(Token_kind::Equal))
    {
        initializer = parse_assignment();
    }

    std::vector<ast::Declarator> declarators;

    declarators.push_back(
            {.pointers = pointers,
             .reference = reference,
             .name = std::string{name.lexeme()},
             .initializer = std::move(initializer)});

    while (accept(Token_kind::Comma))
    {
        declarators.push_back(parse_declarator());
    }

    expect(Token_kind::Semicolon, "';' after the declaration");

    return ast::Declaration{.type = type, .declarators = std::move(declarators)};
}

ast::Function Parser::parse_function(
        const ast::Type type, const std::size_t pointers, const bool reference, std::string name)
{
    expect(Token_kind::Left_paren, "'(' to open the parameter list");

    std::vector<ast::Parameter> parameters;

    if (!check(Token_kind::Right_paren))
    {
        parameters.push_back(parse_parameter());

        while (accept(Token_kind::Comma))
        {
            parameters.push_back(parse_parameter());
        }
    }

    expect(Token_kind::Right_paren, "')' to close the parameter list");

    // A prototype ends here; a definition continues with its compound-statement body.
    if (accept(Token_kind::Semicolon))
    {
        return {.return_type = type,
                .pointers = pointers,
                .reference = reference,
                .name = std::move(name),
                .parameters = std::move(parameters),
                .body = nullptr};
    }

    auto body{parse_compound_statement()};

    return {.return_type = type,
            .pointers = pointers,
            .reference = reference,
            .name = std::move(name),
            .parameters = std::move(parameters),
            .body = std::make_unique<ast::Stmt>(std::move(body))};
}

ast::Parameter Parser::parse_parameter()
{
    auto type{parse_type_specifier()};

    std::size_t pointers{0};

    while (accept(Token_kind::Star))
    {
        ++pointers;
    }

    const auto reference{accept(Token_kind::Amp).has_value()};

    std::string name;

    if (const auto token{accept(Token_kind::Identifier)}; token)
    {
        name = std::string{token->lexeme()};
    }

    std::optional<ast::Expr> default_value;

    if (accept(Token_kind::Equal))
    {
        default_value = parse_assignment();
    }

    return {.type = type,
            .pointers = pointers,
            .reference = reference,
            .name = std::move(name),
            .default_value = std::move(default_value)};
}

} // namespace hopper::cpp
