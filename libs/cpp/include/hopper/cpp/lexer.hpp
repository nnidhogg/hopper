#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_LEXER_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_LEXER_HPP

#include <munch/core/lexer.hpp>

namespace hopper::cpp
{
/**
 * @brief Builds the Lexer recognizing the expression grammar's Token_kind vocabulary.
 * @return A Lexer ready to be handed to a Token_reader or Parser.
 */
[[nodiscard]] munch::core::Lexer build_lexer();

} // namespace hopper::cpp

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_LEXER_HPP
