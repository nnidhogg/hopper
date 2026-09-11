#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_SOURCE_SPAN_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_SOURCE_SPAN_HPP

#include <cstddef>

namespace hopper::parse
{
/**
 * @brief One position in the original input: a byte offset plus the 1-based line and column it falls on.
 *
 * Offsets index the input exactly as given, with no newline normalization; columns count bytes, not code points.
 */
struct Source_position
{
    bool operator==(const Source_position&) const = default;

    /**
     * @brief The byte offset from the start of the input, counted from zero.
     */
    std::size_t offset{0};

    /**
     * @brief The line, counted from one.
     */
    std::size_t line{1};

    /**
     * @brief The column within the line, counted from one in bytes.
     */
    std::size_t column{1};
};

/**
 * @brief A half-open range of input: `begin` is the first byte of a construct, `end` is one past its last byte.
 */
struct Source_span
{
    bool operator==(const Source_span&) const = default;

    /**
     * @brief The first byte of the construct.
     */
    Source_position begin{};

    /**
     * @brief One past the construct's last byte.
     */
    Source_position end{};
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_SOURCE_SPAN_HPP
