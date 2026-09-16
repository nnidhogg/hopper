#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "hopper/clike/parser.hpp"

namespace hopper::clike
{
ast::Translation_unit Parser::parse_translation_unit()
{
    ast::Translation_unit unit;

    while (more())
    {
        const auto begin{here()};

        auto node{parse_external_declaration()};

        unit.items.push_back({.node = std::move(node), .span = close(begin)});
    }

    return unit;
}

ast::Translation_unit::Item::Node_t Parser::parse_external_declaration()
{
    const auto type{parse_type_specifier()};

    const auto indirection{parse_indirection()};

    const auto name{expect_identifier("a declarator name")};

    if (check_punctuation('('))
    {
        return parse_function(type, indirection, std::string{name.lexeme()});
    }

    std::vector<ast::Declarator> declarators;

    declarators.push_back(
            {.indirection = indirection, .name = std::string{name.lexeme()}, .initializer = parse_initializer()});

    while (accept_punctuation(','))
    {
        declarators.push_back(parse_declarator());
    }

    expect_punctuation(';', "';' after the declaration");

    return ast::Declaration{.type = type, .declarators = std::move(declarators)};
}

ast::Function Parser::parse_function(const ast::Type type, const ast::Indirection indirection, std::string name)
{
    expect_punctuation('(', "'(' to open the parameter list");

    std::vector<ast::Parameter> parameters;

    if (!check_punctuation(')'))
    {
        parameters.push_back(parse_parameter());

        while (accept_punctuation(','))
        {
            parameters.push_back(parse_parameter());
        }
    }

    expect_punctuation(')', "')' to close the parameter list");

    // A prototype ends here; a definition carries its body, and nothing else tells the two apart.
    std::unique_ptr<ast::Stmt> body;

    if (!accept_punctuation(';'))
    {
        body = std::make_unique<ast::Stmt>(parse_compound_statement());
    }

    return {.return_type = type,
            .indirection = indirection,
            .name = std::move(name),
            .parameters = std::move(parameters),
            .body = std::move(body)};
}

ast::Parameter Parser::parse_parameter()
{
    const auto type{parse_type_specifier()};

    const auto indirection{parse_indirection()};

    std::string name;

    if (const auto token{accept_identifier()})
    {
        name = std::string{token->lexeme()};
    }

    return {.type = type, .indirection = indirection, .name = std::move(name), .default_value = parse_initializer()};
}

} // namespace hopper::clike
