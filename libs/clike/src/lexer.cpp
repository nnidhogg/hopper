#include <munch/core/builder.hpp>
#include <munch/regex/regex.hpp>
#include <munch/regex/set.hpp>

#include "hopper/clike/tokens.hpp"

namespace hopper::clike
{
namespace
{
/**
 * @brief Compiles the token set into a lexer.
 * @return The lexer.
 */
munch::core::Lexer build()
{
    using namespace munch::regex;

    munch::core::Builder builder;

    builder.add_token(
            concat(any_of(Set::alpha() + '_'), kleene(any_of(Set::alphanum() + '_'))), Token_kind::Identifier, 2);
    builder.add_token(plus(any_of(Set::digits())), Token_kind::Number, 2);
    builder.add_token(
            any_of(Set{'+', '-', '*', '/', '<', '>', '=', '!', '&', '|', '^', '%', '~'}), Token_kind::Operator, 2);
    builder.add_token(any_of(Set{'(', ')', '[', ']', '{', '}', ';', ',', '.', ':', '?'}), Token_kind::Punctuation, 2);
    builder.add_token(plus(any_of(Set{' ', '\t', '\n'})), Token_kind::Whitespace, 2);
    builder.add_token(
            concat(text("\""), kleene(any_of(Set::all() - Set{'"'} - Set{'\n'})), text("\"")), Token_kind::String, 2);
    builder.add_token(concat(text("//"), kleene(any_of(Set::all() - Set{'\n'}))), Token_kind::Line_comment, 1);

    return builder.build();
}

} // namespace

const munch::core::Lexer& lexer()
{
    static const munch::core::Lexer instance{build()};

    return instance;
}

} // namespace hopper::clike
