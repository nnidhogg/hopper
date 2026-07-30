#include "hopper/parse/token_location.hpp"

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
    // "\r\n" counts as one newline, and a lone '\r' counts as one too, so line and column stay right on any
    // platform's line endings while offsets keep indexing the original bytes.
    for (std::size_t index{0}; index < lexeme.size(); ++index)
    {
        const auto c{lexeme[index]};

        if (c == '\n' || (c == '\r' && (index + 1 == lexeme.size() || lexeme[index + 1] != '\n')))
        {
            ++line_;

            column_ = 1;
        }
        else if (c != '\r')
        {
            ++column_;
        }
    }

    offset_ += lexeme.size();
}

Source_position Token_location::position() const noexcept
{
    return {.offset = offset_, .line = line_, .column = column_};
}

} // namespace hopper::parse
