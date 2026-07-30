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
    std::size_t offset{0};

    std::size_t line{1};

    std::size_t column{1};

    bool operator==(const Source_position&) const = default;
};

/**
 * @brief A half-open range of input: `begin` is the first byte of a construct, `end` is one past its last byte.
 */
struct Source_span
{
    Source_position begin{};

    Source_position end{};

    bool operator==(const Source_span&) const = default;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_SOURCE_SPAN_HPP
