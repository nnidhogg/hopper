#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "hopper/clike/parser.hpp"

namespace hopper::clike
{
namespace
{
/**
 * @brief The fundamental types and the keyword naming each.
 */
constexpr std::array<std::pair<std::string_view, ast::Type_kind>, 6> types{{
        {"bool", ast::Type_kind::Bool},
        {"char", ast::Type_kind::Char},
        {"int", ast::Type_kind::Int},
        {"float", ast::Type_kind::Float},
        {"double", ast::Type_kind::Double},
        {"void", ast::Type_kind::Void},
}};

/**
 * @brief The fundamental type a keyword names.
 * @param word The identifier's spelling.
 * @return The type kind, or std::nullopt when the word names no type.
 */
[[nodiscard]] std::optional<ast::Type_kind> type_kind_for(const std::string_view word)
{
    const auto found{std::ranges::find(types, word, &std::pair<std::string_view, ast::Type_kind>::first)};

    return found != types.end() ? std::optional{found->second} : std::nullopt;
}
} // namespace

bool Parser::is_declaration_start()
{
    if (pending_)
    {
        return false;
    }

    const auto token{peek_token()};

    return token && token->kind() == Token_kind::Identifier &&
           (token->lexeme() == "const" || type_kind_for(token->lexeme()).has_value());
}

ast::Stmt Parser::parse_declaration_statement()
{
    const auto begin{here()};

    auto type{parse_type_specifier()};

    std::vector<ast::Declarator> declarators;

    declarators.push_back(parse_declarator());

    while (accept_punctuation(','))
    {
        declarators.push_back(parse_declarator());
    }

    expect_punctuation(';', "';' after the declaration");

    return {.node = ast::Declaration{.type = type, .declarators = std::move(declarators)}, .span = close(begin)};
}

ast::Type Parser::parse_type_specifier()
{
    auto is_const{accept_keyword("const")};

    if (pending_)
    {
        unexpected("a type name");
    }

    const auto token{next_token()};

    if (!token)
    {
        eof_error("Expected a type name before end of input");
    }

    const auto kind{token->kind() == Token_kind::Identifier ? type_kind_for(token->lexeme()) : std::nullopt};

    if (!kind)
    {
        syntax_error("Expected a type name", *token);
    }

    if (check_keyword("const"))
    {
        if (is_const)
        {
            unexpected("a declarator, not a second 'const'");
        }

        (void)accept_keyword("const");

        is_const = true;
    }

    return {.is_const = is_const, .kind = *kind};
}

ast::Type_id Parser::parse_type_id()
{
    const auto type{parse_type_specifier()};

    const auto [pointers, reference]{parse_indirection()};

    return {.type = type, .pointers = pointers, .reference = reference};
}

Parser::Indirection Parser::parse_indirection()
{
    std::size_t pointers{0};

    while (accept_operator("*"))
    {
        ++pointers;
    }

    return {.pointers = pointers, .reference = accept_operator("&")};
}

ast::Declarator Parser::parse_declarator()
{
    const auto [pointers, reference]{parse_indirection()};

    const auto name{expect_identifier("a declarator name")};

    return {.pointers = pointers,
            .reference = reference,
            .name = std::string{name.lexeme()},
            .initializer = parse_initializer()};
}

std::optional<ast::Expr> Parser::parse_initializer()
{
    if (!accept_operator("="))
    {
        return std::nullopt;
    }

    return parse_assignment();
}
} // namespace hopper::clike
