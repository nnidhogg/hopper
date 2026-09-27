#include "hopper/parse/parse_error.hpp"

#include <string>
#include <string_view>

namespace hopper::parse
{
Parse_error::Parse_error(const Parse_error_kind kind, const Source_span& span, const std::string& message)
    : std::runtime_error{std::to_string(span.begin.line) + ":" + std::to_string(span.begin.column) + ": " + message}
    , kind_{kind}
    , span_{span}
{}

Parse_error Parse_error::unexpected_token(
        const Source_span& span, const std::string_view message, const std::string_view lexeme)
{
    return {Parse_error_kind::Unexpected_token, span,
            "Syntax error: " + std::string{message} + ", got '" + std::string{lexeme} + "'"};
}

Parse_error Parse_error::unexpected_end(const Source_position& at, const std::string_view message)
{
    return {Parse_error_kind::Unexpected_end, Source_span{.begin = at, .end = at},
            "Syntax error: " + std::string{message}};
}

Parse_error Parse_error::lexical(const Source_position& at, const std::string_view message)
{
    return {Parse_error_kind::Lexical, Source_span{.begin = at, .end = at}, "Lexical error: " + std::string{message}};
}

Parse_error_kind Parse_error::kind() const noexcept
{
    return kind_;
}

const Source_span& Parse_error::span() const noexcept
{
    return span_;
}

} // namespace hopper::parse
