#include "hopper/cpp/lexer.hpp"

#include <munch/core/builder.hpp>
#include <munch/regex/any_of.hpp>
#include <munch/regex/patterns.hpp>
#include <munch/regex/repeat.hpp>
#include <munch/regex/text.hpp>

#include "hopper/cpp/tokens.hpp"

namespace hopper::cpp
{
munch::core::Lexer build_lexer()
{
    using namespace munch::regex;

    munch::core::Builder builder;

    // Keywords outrank the identifier pattern, which would otherwise match the same lexeme.
    builder.add_token(text("true"), Token_kind::Keyword_true, 1);
    builder.add_token(text("false"), Token_kind::Keyword_false, 1);

    builder.add_token(patterns::identifier(), Token_kind::Identifier, 2);

    // decimal_float() requires no sign or exponent; unary +/- is a parser-level concern, not part of the lexeme.
    builder.add_token(patterns::decimal_float(), Token_kind::Floating_point_literal, 1);
    builder.add_token(patterns::decimal_integer(), Token_kind::Integer_literal, 1);

    builder.add_token(text("("), Token_kind::Left_paren, 1);
    builder.add_token(text(")"), Token_kind::Right_paren, 1);
    builder.add_token(text("["), Token_kind::Left_bracket, 1);
    builder.add_token(text("]"), Token_kind::Right_bracket, 1);
    builder.add_token(text("."), Token_kind::Dot, 1);
    builder.add_token(text("->"), Token_kind::Arrow, 1);
    builder.add_token(text(","), Token_kind::Comma, 1);
    builder.add_token(text("?"), Token_kind::Question, 1);
    builder.add_token(text(":"), Token_kind::Colon, 1);

    builder.add_token(text("+"), Token_kind::Plus, 1);
    builder.add_token(text("-"), Token_kind::Minus, 1);
    builder.add_token(text("*"), Token_kind::Star, 1);
    builder.add_token(text("/"), Token_kind::Slash, 1);
    builder.add_token(text("%"), Token_kind::Percent, 1);
    builder.add_token(text("!"), Token_kind::Bang, 1);
    builder.add_token(text("++"), Token_kind::Plus_plus, 1);
    builder.add_token(text("--"), Token_kind::Minus_minus, 1);
    builder.add_token(text("=="), Token_kind::Equal_equal, 1);
    builder.add_token(text("!="), Token_kind::Bang_equal, 1);
    builder.add_token(text("<"), Token_kind::Less, 1);
    builder.add_token(text(">"), Token_kind::Greater, 1);
    builder.add_token(text("<="), Token_kind::Less_equal, 1);
    builder.add_token(text(">="), Token_kind::Greater_equal, 1);
    builder.add_token(text("&&"), Token_kind::Amp_amp, 1);
    builder.add_token(text("||"), Token_kind::Pipe_pipe, 1);

    builder.add_token(plus(any_of(Set{' ', '\t'})), Token_kind::Whitespace, 0);
    builder.add_token(plus(any_of(Set::newline())), Token_kind::Newline, 0);

    return builder.build();
}

} // namespace hopper::cpp
