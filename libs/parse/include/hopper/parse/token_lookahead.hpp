#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOOKAHEAD_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOOKAHEAD_HPP

#include <optional>
#include <string_view>
#include <utility>

#include <munch/tools/tokenizer/token.hpp>

#include "hopper/parse/token_location.hpp"

namespace hopper::parse
{
/**
 * @brief The one token a Token_reader holds ahead of the parser, with the positions around it.
 *
 * Three positions travel with the token: where it begins, where the cursor stands after it, and where the last
 * consumed token ended, which is where a construct that has just finished stops whatever is buffered ahead of it.
 * @tparam Kind The token kind type (enum or integral) produced by the lexer.
 */
template <typename Kind>
class Token_lookahead
{
public:
    /**
     * @brief The token type the lexer produces.
     */
    using Token_t = munch::tools::tokenizer::Token<Kind>;

    /**
     * @brief Constructs an empty lookahead at the input's first byte.
     */
    Token_lookahead() = default;

    /**
     * @brief The buffered token.
     * @return The token, or std::nullopt when nothing is buffered.
     */
    [[nodiscard]] const std::optional<Token_t>& token() const noexcept { return token_; }

    /**
     * @brief Where the buffered token begins.
     * @return The location of its first byte.
     */
    [[nodiscard]] const Token_location& location() const noexcept { return begin_; }

    /**
     * @brief The buffered token's span, from its first byte to one past its last.
     * @return The span.
     */
    [[nodiscard]] Source_span span() const noexcept { return {.begin = begin_.position(), .end = cursor_.position()}; }

    /**
     * @brief Where the most recently consumed token ended, whatever is buffered ahead of it.
     * @return The position one past that token's last byte.
     */
    [[nodiscard]] const Source_position& previous_end() const noexcept { return previous_end_; }

    /**
     * @brief Hands the buffered token over and clears the buffer, recording where the token ended.
     * @return The token, or std::nullopt when nothing was buffered.
     */
    std::optional<Token_t> consume() noexcept
    {
        if (token_)
        {
            previous_end_ = cursor_.position();
        }

        return std::exchange(token_, std::nullopt);
    }

    /**
     * @brief Returns to the input's first byte and clears the buffer.
     */
    void reset() noexcept
    {
        token_.reset();

        begin_.reset();

        cursor_.reset();

        previous_end_ = {};
    }

    /**
     * @brief Buffers a token just read and moves the cursor past it.
     *
     * The token begins where the cursor stood; the cursor runs ahead over the lexeme so the next token's beginning is
     * already known.
     * @param kind The token's kind.
     * @param lexeme The token's text.
     */
    void advance(const Kind kind, const std::string_view lexeme) noexcept
    {
        token_.emplace(kind, lexeme);

        // The exposed location is where this token begins; the cursor runs ahead over the lexeme so the next
        // token's beginning is already known.
        begin_ = cursor_;

        cursor_.advance(lexeme);
    }

    /**
     * @brief Moves the cursor over bytes skipped without a token, leaving the buffer empty and the span empty there.
     * @param bytes The bytes skipped.
     */
    void skip(const std::string_view bytes) noexcept
    {
        token_.reset();

        cursor_.advance(bytes);

        begin_ = cursor_;
    }

private:
    std::optional<Token_t> token_;

    Token_location begin_;

    Token_location cursor_;

    Source_position previous_end_{};
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOOKAHEAD_HPP
