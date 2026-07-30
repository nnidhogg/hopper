#ifndef HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSE_ERROR_HPP
#define HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSE_ERROR_HPP

#include <stdexcept>
#include <string>

#include "hopper/parse/source_span.hpp"

namespace hopper::parse
{
/**
 * @brief What went wrong: the input failed to tokenize, a token was not the expected one, or the input ended.
 */
enum class Parse_error_kind
{
    Lexical,
    Unexpected_token,
    Unexpected_end,
};

/**
 * @brief A parse failure with its kind and the source range it points at.
 *
 * Derives from std::runtime_error, so existing catch sites keep working; what() carries the message prefixed with
 * the 1-based "line:column:" of the span's begin. The span covers the offending token, or is empty at the end of
 * input for errors with nothing left to point at.
 */
class Parse_error : public std::runtime_error
{
public:
    /**
     * @brief Constructs an error from its kind, the span it points at, and a human-readable message.
     */
    Parse_error(Parse_error_kind kind, const Source_span& span, const std::string& message);

    /**
     * @brief What went wrong.
     */
    [[nodiscard]] Parse_error_kind kind() const noexcept;

    /**
     * @brief The source range the error points at.
     */
    [[nodiscard]] const Source_span& span() const noexcept;

private:
    Parse_error_kind kind_;

    Source_span span_;
};

} // namespace hopper::parse

#endif // HOPPER_LIBS_PARSE_INCLUDE_HOPPER_PARSE_PARSE_ERROR_HPP
