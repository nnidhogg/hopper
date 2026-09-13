#include <munch/core/builder.hpp>
#include <munch/regex/regex.hpp>
#include <munch/regex/set.hpp>
#include <munch/regex/utf8.hpp>

#include "hopper/json/tokens.hpp"

namespace hopper::json
{
munch::core::Lexer lexer()
{
    using namespace munch::regex;

    munch::core::Builder builder;

    const auto hex{any_of(Set::digits() + Set{'a', 'b', 'c', 'd', 'e', 'f', 'A', 'B', 'C', 'D', 'E', 'F'})};

    const auto escape{concat(
            text("\\"),
            choice(any_of(Set{'"', '\\', '/', 'b', 'f', 'n', 'r', 't'}), concat(text("u"), hex, hex, hex, hex)))};

    // One code point from U+0020 up, well-formed UTF-8, except the quote (U+0022) and the backslash (U+005C).
    const auto unescaped{choice(utf8::range(0x20, 0x21), utf8::range(0x23, 0x5B), utf8::range(0x5D, 0x10FFFF))};

    builder.add_token(concat(text("\""), kleene(choice(unescaped, escape)), text("\"")), Token_kind::String, 1);

    const auto digits{plus(any_of(Set::digits()))};
    const auto integer{choice(text("0"), concat(any_of(Set::digits() - Set{'0'}), kleene(any_of(Set::digits()))))};
    const auto fraction{concat(text("."), digits)};
    const auto exponent{concat(any_of(Set{'e', 'E'}), optional(any_of(Set{'+', '-'})), digits)};

    builder.add_token(
            concat(optional(text("-")), integer, optional(fraction), optional(exponent)), Token_kind::Number, 1);

    builder.add_token(text("true"), Token_kind::True, 1);
    builder.add_token(text("false"), Token_kind::False, 1);
    builder.add_token(text("null"), Token_kind::Null, 1);

    builder.add_token(text("{"), Token_kind::Left_brace, 1);
    builder.add_token(text("}"), Token_kind::Right_brace, 1);
    builder.add_token(text("["), Token_kind::Left_bracket, 1);
    builder.add_token(text("]"), Token_kind::Right_bracket, 1);
    builder.add_token(text(":"), Token_kind::Colon, 1);
    builder.add_token(text(","), Token_kind::Comma, 1);

    builder.add_token(plus(any_of(Set{' ', '\t', '\n', '\r'})), Token_kind::Whitespace, 1);

    return builder.build();
}

} // namespace hopper::json
