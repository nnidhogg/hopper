#include "hopper/parse/token_location.hpp"

#include <algorithm>

namespace hopper::parse
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

void Token_location::advance(const std::string_view lexeme) noexcept
{
    std::ranges::for_each(lexeme, [this](const char c) { column_ = c == '\n' ? (++line_, 1) : column_ + 1; });

    offset_ += lexeme.size();
}

} // namespace hopper::parse
