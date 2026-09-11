#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOCATION_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOCATION_HPP

#include <cstddef>
#include <string_view>

#include "hopper/parse/source_span.hpp"

namespace hopper::parse
{
/**
 * @brief A position in the input, kept as a line, a column and a byte offset, advanced over consumed text.
 *
 * The offset counts every byte of the input as given, so it indexes the original text on any platform's line
 * endings; the line and column are what a diagnostic prints.
 */
class Token_location
{
public:
    /**
     * @brief Constructs the position of the input's first byte: line one, column one, offset zero.
     */
    Token_location() = default;

    /**
     * @brief The line, counted from one.
     * @return The line number.
     */
    [[nodiscard]] std::size_t line() const noexcept;

    /**
     * @brief The column within the line, counted from one in bytes.
     * @return The column number.
     */
    [[nodiscard]] std::size_t column() const noexcept;

    /**
     * @brief The byte offset from the start of the input, counted from zero.
     * @return The offset.
     */
    [[nodiscard]] std::size_t offset() const noexcept;

    /**
     * @brief The position as a value, for a span to hold.
     * @return The position.
     */
    [[nodiscard]] Source_position position() const noexcept;

    /**
     * @brief Returns to the input's first byte.
     */
    void reset() noexcept;

    /**
     * @brief Advances over consumed text.
     *
     * A "\n", a "\r\n" pair and a lone '\r' each end a line: the line count rises by one and the column restarts at
     * one after it, while the offset counts every byte, so a token spanning lines advances the position exactly.
     * @param lexeme The text consumed.
     */
    void advance(std::string_view lexeme) noexcept;

private:
    /**
     * @brief The line, counted from one.
     */
    std::size_t line_{1};

    /**
     * @brief The column within the line, counted from one in bytes.
     */
    std::size_t column_{1};

    /**
     * @brief The byte offset from the start of the input, counted from zero.
     */
    std::size_t offset_{0};
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_TOKEN_LOCATION_HPP
