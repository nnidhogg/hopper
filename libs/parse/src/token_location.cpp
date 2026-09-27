#include "hopper/parse/token_location.hpp"

namespace hopper::parse
{
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

Source_position Token_location::position() const noexcept
{
    return {.offset = offset_, .line = line_, .column = column_};
}

void Token_location::reset() noexcept
{
    *this = Token_location{};
}

void Token_location::advance(const std::string_view lexeme) noexcept
{
    // A '\r' ends a line at once, and a '\n' ends one unless it completes a "\r\n" pair, whose line its '\r' already
    // ended, even when the pair is split between two calls.
    for (const auto c : lexeme)
    {
        if (c == '\r' || (c == '\n' && !after_carriage_return_))
        {
            ++line_;

            column_ = 1;
        }
        else if (c != '\n')
        {
            ++column_;
        }

        after_carriage_return_ = c == '\r';
    }

    offset_ += lexeme.size();
}

} // namespace hopper::parse
