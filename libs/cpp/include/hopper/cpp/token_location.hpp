#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKEN_LOCATION_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKEN_LOCATION_HPP

#include <cstddef>
#include <string_view>

#include "tokens.hpp"

namespace hopper::cpp
{
/**
 * @brief Tracks the current position within the input source.
 *
 * Maintains line, column, and byte offset counters that are advanced as tokens are consumed.
 * Used to provide accurate diagnostic information and source mapping during parsing.
 */
class Token_location
{
public:
    /**
     * @brief Construct a new location initialized to the start of the file (line 1, column 1).
     */
    Token_location();

    /**
     * @brief Current line number (1-based).
     */
    [[nodiscard]] std::size_t line() const noexcept;

    /**
     * @brief Current column number (1-based).
     */
    [[nodiscard]] std::size_t column() const noexcept;

    /**
     * @brief Current byte offset (0-based from the start of input).
     */
    [[nodiscard]] std::size_t offset() const noexcept;

    /**
     * @brief Reset the reading position to the beginning of the current input.
     */
    void reset() noexcept;

    /**
     * @brief Advance the position based on a token.
     *
     * Increments line, column, and offset counters according to the characters in the token's lexeme.
     * Line endings (`\n`) reset the column counter and increment the line number.
     *
     * @param kind   The kind of token being processed.
     * @param lexeme The text of the token.
     */
    void advance(Token_kind kind, std::string_view lexeme) noexcept;

private:
    std::size_t line_;

    std::size_t column_;

    std::size_t offset_;
};

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_TOKEN_LOCATION_HPP
