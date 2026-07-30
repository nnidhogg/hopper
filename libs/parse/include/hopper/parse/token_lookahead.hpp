#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOOKAHEAD_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOOKAHEAD_HPP

#include <munch/tools/tokenizer/token.hpp>
#include <optional>
#include <string_view>
#include <utility>

#include "hopper/parse/token_location.hpp"

namespace hopper::parse
{
/**
 * @brief Holds the currently buffered (lookahead) token and its source location.
 *
 * Used by a parser to support one-token lookahead behavior. Stores both the token returned from the lexer and its
 * associated location in the input.
 * @tparam Kind The token kind type (enum or integral) produced by the lexer.
 */
template <typename Kind>
class Token_lookahead
{
public:
    /**
     * @brief Alias for the token type produced by the underlying lexer.
     */
    using Token_t = munch::tools::tokenizer::Token<Kind>;

    /**
     * @brief Constructs an empty lookahead state.
     */
    Token_lookahead() = default;

    /**
     * @brief Access the current lookahead token, if any.
     */
    [[nodiscard]] const std::optional<Token_t>& token() const noexcept { return token_; }

    /**
     * @brief Access the location of the current token's first character.
     */
    [[nodiscard]] const Token_location& location() const noexcept { return begin_; }

    /**
     * @brief The span of the current token: its first byte to one past its last.
     */
    [[nodiscard]] Source_span span() const noexcept { return {begin_.position(), cursor_.position()}; }

    /**
     * @brief The end position of the most recently consumed token.
     *
     * This is where a grammar construct that just finished actually stops, unaffected by any token already
     * buffered ahead of it.
     */
    [[nodiscard]] const Source_position& last_end() const noexcept { return last_end_; }

    /**
     * @brief Consume and clear the buffered token.
     *
     * Returns the currently stored token (if any) and resets the internal optional to an empty state.
     */
    std::optional<Token_t> consume() noexcept
    {
        if (token_)
        {
            last_end_ = cursor_.position();
        }

        return std::exchange(token_, std::nullopt);
    }

    /**
     * @brief Reset the reading position to the beginning of the current input and clear the token.
     */
    void reset() noexcept
    {
        token_.reset();

        begin_.reset();

        cursor_.reset();

        last_end_ = {};
    }

    /**
     * @brief Update the lookahead token and advance the source location.
     *
     * Called when a new token is read from the lexer. It updates both the stored token and the internal source
     * position tracker.
     */
    void advance(const Kind kind, const std::string_view lexeme) noexcept
    {
        token_.emplace(kind, lexeme);

        // The exposed location is where this token begins; the cursor runs ahead over the lexeme so the next
        // token's beginning is already known.
        begin_ = cursor_;

        cursor_.advance(lexeme);
    }

private:
    std::optional<Token_t> token_;

    Token_location begin_;

    Token_location cursor_;

    Source_position last_end_{};
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOOKAHEAD_HPP
