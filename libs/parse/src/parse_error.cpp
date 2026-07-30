#include "hopper/parse/parse_error.hpp"

namespace hopper::parse
{
Parse_error::Parse_error(const Parse_error_kind kind, const Source_span& span, const std::string& message)
    : std::runtime_error{std::to_string(span.begin.line) + ":" + std::to_string(span.begin.column) + ": " + message}
    , kind_{kind}
    , span_{span}
{}

Parse_error_kind Parse_error::kind() const noexcept
{
    return kind_;
}

const Source_span& Parse_error::span() const noexcept
{
    return span_;
}

} // namespace hopper::parse
