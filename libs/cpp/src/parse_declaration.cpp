#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "hopper/cpp/parser.hpp"

namespace hopper::cpp
{
namespace
{
/**
 * @brief Maps a type keyword to its type kind, or nullopt for tokens that name no fundamental type.
 */
std::optional<ast::Type_kind> type_kind_for(const Token_kind kind)
{
    switch (kind)
    {
    case Token_kind::Keyword_bool:
        return ast::Type_kind::Bool;
    case Token_kind::Keyword_char:
        return ast::Type_kind::Char;
    case Token_kind::Keyword_int:
        return ast::Type_kind::Int;
    case Token_kind::Keyword_float:
        return ast::Type_kind::Float;
    case Token_kind::Keyword_double:
        return ast::Type_kind::Double;
    case Token_kind::Keyword_void:
        return ast::Type_kind::Void;
    default:
        return std::nullopt;
    }
}

} // namespace

bool Parser::is_declaration_start()
{
    const auto token{peek_token()};

    return token && (token->kind() == Token_kind::Keyword_const || type_kind_for(token->kind()).has_value());
}

ast::Stmt Parser::parse_declaration_statement()
{
    const auto begin{mark()};

    auto type{parse_type_specifier()};

    std::vector<ast::Declarator> declarators;

    declarators.push_back(parse_declarator());

    while (accept(Token_kind::Comma))
    {
        declarators.push_back(parse_declarator());
    }

    expect(Token_kind::Semicolon, "';' after the declaration");

    return {.node = ast::Declaration{.type = type, .declarators = std::move(declarators)}, .span = span_from(begin)};
}

ast::Type Parser::parse_type_specifier()
{
    auto is_const{accept(Token_kind::Keyword_const).has_value()};

    const auto token{next_token()};

    if (!token)
    {
        eof_error("Expected a type name before end of input");
    }

    const auto kind{type_kind_for(token->kind())};

    if (!kind)
    {
        syntax_error("Expected a type name", *token);
    }

    // The qualifier may also follow the type name (`int const`), but only one placement may be used.
    if (const auto duplicate{accept(Token_kind::Keyword_const)}; duplicate)
    {
        if (is_const)
        {
            syntax_error("Duplicate 'const' qualifier", *duplicate);
        }

        is_const = true;
    }

    return {.is_const = is_const, .kind = *kind};
}

ast::Type_id Parser::parse_type_id()
{
    const auto type{parse_type_specifier()};

    std::size_t pointers{0};

    while (accept(Token_kind::Star))
    {
        ++pointers;
    }

    const bool reference{accept(Token_kind::Amp).has_value()};

    return {.type = type, .pointers = pointers, .reference = reference};
}

ast::Declarator Parser::parse_declarator()
{
    std::size_t pointers{0};

    while (accept(Token_kind::Star))
    {
        ++pointers;
    }

    const auto reference{accept(Token_kind::Amp).has_value()};

    const auto name{expect(Token_kind::Identifier, "a declarator name")};

    std::optional<ast::Expr> initializer;

    if (accept(Token_kind::Equal))
    {
        initializer = parse_assignment();
    }

    return {.pointers = pointers,
            .reference = reference,
            .name = std::string{name.lexeme()},
            .initializer = std::move(initializer)};
}

} // namespace hopper::cpp
