#include "hopper/idl/token_location.hpp"

#include <algorithm>

namespace hopper::idl
{
Token_location::Token_location() : line_{1}, column_{1}, offset_{0}
{}

std::size_t Token_location::line() const noexcept
{
    return line_;
}

std::size_t Token_location::column() const noexcept
{
    return column_;
}

std::size_t Token_location::offset() const noexcept
{
    return offset_;
}

void Token_location::reset() noexcept
{
    *this = Token_location{};
}

void Token_location::advance(const Token_kind kind, const std::string_view lexeme) noexcept
{
    if (kind == Token_kind::Newline || kind == Token_kind::Multi_line_comment)
    {
        std::ranges::for_each(lexeme, [this](const char c) { column_ = c == '\n' ? (++line_, 1) : column_ + 1; });
    }
    else
    {
        column_ += lexeme.size();
    }

    offset_ += lexeme.size();
}

} // namespace hopper::idl
