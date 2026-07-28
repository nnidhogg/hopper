#include "hopper/cpp/token_lookahead.hpp"

#include <utility>

namespace hopper::cpp
{
const std::optional<Token_lookahead::Token_t>& Token_lookahead::token() const noexcept
{
    return token_;
}

const Token_location& Token_lookahead::location() const noexcept
{
    return location_;
}

std::optional<Token_lookahead::Token_t> Token_lookahead::consume() noexcept
{
    return std::exchange(token_, std::nullopt);
}

void Token_lookahead::reset() noexcept
{
    token_.reset();

    location_.reset();
}

void Token_lookahead::advance(Token_kind kind, std::string_view lexeme) noexcept
{
    token_.emplace(kind, lexeme);

    location_.advance(kind, lexeme);
}

} // namespace hopper::cpp
