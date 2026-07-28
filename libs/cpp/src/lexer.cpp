#include "hopper/cpp/lexer.hpp"

#include <munch/core/builder.hpp>
#include <munch/regex/any_of.hpp>
#include <munch/regex/choice.hpp>
#include <munch/regex/concat.hpp>
#include <munch/regex/repeat.hpp>
#include <munch/regex/text.hpp>

#include "hopper/cpp/tokens.hpp"

namespace hopper::cpp
{
namespace
{
using namespace munch::regex;

/**
 * @brief The regex matching identifiers: a letter or underscore, then any number of letters, digits, or underscores.
 */
Regex identifier_regex()
{
    return concat(any_of(Set::alpha() + '_'), kleene(any_of(Set::alphanum() + '_')));
}

/**
 * @brief The regex matching integer literals: one or more digits.
 */
Regex integer_literal_regex()
{
    return plus(any_of(Set::digits()));
}

/**
 * @brief The regex matching floating-point literals: digits, a decimal point, then more digits.
 *
 * A leading sign is deliberately not part of the lexeme; unary `+`/`-` are handled by the parser.
 */
Regex floating_point_literal_regex()
{
    return concat(plus(any_of(Set::digits())), text("."), plus(any_of(Set::digits())));
}

} // namespace

munch::core::Lexer build_lexer()
{
    munch::core::Builder builder;

    // Keywords outrank the identifier regex, which would otherwise match the same lexeme.
    builder.add_token(text("true"), Token_kind::Keyword_true, 1);
    builder.add_token(text("false"), Token_kind::Keyword_false, 1);

    builder.add_token(identifier_regex(), Token_kind::Identifier, 2);

    builder.add_token(floating_point_literal_regex(), Token_kind::Floating_point_literal, 1);
    builder.add_token(integer_literal_regex(), Token_kind::Integer_literal, 1);

    builder.add_token(text("("), Token_kind::Left_paren, 1);
    builder.add_token(text(")"), Token_kind::Right_paren, 1);
    builder.add_token(text("?"), Token_kind::Question, 1);
    builder.add_token(text(":"), Token_kind::Colon, 1);

    builder.add_token(text("+"), Token_kind::Plus, 1);
    builder.add_token(text("-"), Token_kind::Minus, 1);
    builder.add_token(text("*"), Token_kind::Star, 1);
    builder.add_token(text("/"), Token_kind::Slash, 1);
    builder.add_token(text("%"), Token_kind::Percent, 1);
    builder.add_token(text("!"), Token_kind::Bang, 1);
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
