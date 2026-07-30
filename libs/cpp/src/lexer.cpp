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
    builder.add_token(text("if"), Token_kind::Keyword_if, 1);
    builder.add_token(text("else"), Token_kind::Keyword_else, 1);
    builder.add_token(text("while"), Token_kind::Keyword_while, 1);
    builder.add_token(text("for"), Token_kind::Keyword_for, 1);
    builder.add_token(text("do"), Token_kind::Keyword_do, 1);
    builder.add_token(text("return"), Token_kind::Keyword_return, 1);
    builder.add_token(text("const"), Token_kind::Keyword_const, 1);
    builder.add_token(text("bool"), Token_kind::Keyword_bool, 1);
    builder.add_token(text("char"), Token_kind::Keyword_char, 1);
    builder.add_token(text("int"), Token_kind::Keyword_int, 1);
    builder.add_token(text("float"), Token_kind::Keyword_float, 1);
    builder.add_token(text("double"), Token_kind::Keyword_double, 1);
    builder.add_token(text("void"), Token_kind::Keyword_void, 1);
    builder.add_token(text("static_cast"), Token_kind::Keyword_static_cast, 1);
    builder.add_token(text("dynamic_cast"), Token_kind::Keyword_dynamic_cast, 1);
    builder.add_token(text("const_cast"), Token_kind::Keyword_const_cast, 1);
    builder.add_token(text("reinterpret_cast"), Token_kind::Keyword_reinterpret_cast, 1);

    builder.add_token(patterns::identifier(), Token_kind::Identifier, 2);

    // decimal_float() requires no sign or exponent; unary +/- is a parser-level concern, not part of the lexeme.
    builder.add_token(patterns::decimal_float(), Token_kind::Floating_point_literal, 1);
    builder.add_token(patterns::decimal_integer(), Token_kind::Integer_literal, 1);

    // Hexadecimal and binary integers share the integer token kind; the parser decodes by prefix. Maximal munch
    // keeps "0x1F" one token rather than "0" followed by an identifier.
    builder.add_token(
            concat(choice(text("0x"), text("0X")),
                   plus(any_of(Set::digits() + Set::range('a', 'f') + Set::range('A', 'F')))),
            Token_kind::Integer_literal, 1);
    builder.add_token(
            concat(choice(text("0b"), text("0B")), plus(any_of(Set{'0', '1'}))), Token_kind::Integer_literal, 1);

    // A string literal body: any byte except the closing quote, a backslash, or a raw newline, with a backslash
    // escape taking the next printable character verbatim; which escapes are meaningful is a semantic concern.
    const auto escape{concat(text("\\"), any_of(Set::printable() + ' '))};

    builder.add_token(
            concat(text("\""), kleene(choice(any_of(Set::all() - '"' - '\\' - '\n' - '\r'), escape)), text("\"")),
            Token_kind::String_literal, 1);

    // A character literal holds exactly one character or escape; multicharacter literals are not covered.
    builder.add_token(
            concat(text("'"), choice(any_of(Set::all() - '\'' - '\\' - '\n' - '\r'), escape), text("'")),
            Token_kind::Character_literal, 1);

    // Comments are trivia: recognized as tokens here, discarded by the parser's skip predicate. The block comment
    // is the classic automaton where a star run only closes the comment when '/' follows it.
    builder.add_token(concat(text("//"), kleene(any_of(Set::all() - '\n' - '\r'))), Token_kind::Line_comment, 1);

    builder.add_token(
            concat(text("/*"),
                   kleene(choice(any_of(Set::all() - '*'), concat(plus(text("*")), any_of(Set::all() - '*' - '/')))),
                   plus(text("*")), text("/")),
            Token_kind::Block_comment, 1);

    builder.add_token(text("("), Token_kind::Left_paren, 1);
    builder.add_token(text(")"), Token_kind::Right_paren, 1);
    builder.add_token(text("["), Token_kind::Left_bracket, 1);
    builder.add_token(text("]"), Token_kind::Right_bracket, 1);
    builder.add_token(text("{"), Token_kind::Left_brace, 1);
    builder.add_token(text("}"), Token_kind::Right_brace, 1);
    builder.add_token(text(";"), Token_kind::Semicolon, 1);
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
    builder.add_token(text("~"), Token_kind::Tilde, 1);
    builder.add_token(text("++"), Token_kind::Plus_plus, 1);
    builder.add_token(text("--"), Token_kind::Minus_minus, 1);
    builder.add_token(text("=="), Token_kind::Equal_equal, 1);
    builder.add_token(text("!="), Token_kind::Bang_equal, 1);
    builder.add_token(text("<"), Token_kind::Less, 1);
    builder.add_token(text(">"), Token_kind::Greater, 1);
    builder.add_token(text("<="), Token_kind::Less_equal, 1);
    builder.add_token(text(">="), Token_kind::Greater_equal, 1);
    builder.add_token(text("<<"), Token_kind::Less_less, 1);
    builder.add_token(text(">>"), Token_kind::Greater_greater, 1);
    builder.add_token(text("&"), Token_kind::Amp, 1);
    builder.add_token(text("|"), Token_kind::Pipe, 1);
    builder.add_token(text("^"), Token_kind::Caret, 1);
    builder.add_token(text("&&"), Token_kind::Amp_amp, 1);
    builder.add_token(text("||"), Token_kind::Pipe_pipe, 1);

    builder.add_token(text("="), Token_kind::Equal, 1);
    builder.add_token(text("+="), Token_kind::Plus_equal, 1);
    builder.add_token(text("-="), Token_kind::Minus_equal, 1);
    builder.add_token(text("*="), Token_kind::Star_equal, 1);
    builder.add_token(text("/="), Token_kind::Slash_equal, 1);
    builder.add_token(text("%="), Token_kind::Percent_equal, 1);
    builder.add_token(text("&="), Token_kind::Amp_equal, 1);
    builder.add_token(text("|="), Token_kind::Pipe_equal, 1);
    builder.add_token(text("^="), Token_kind::Caret_equal, 1);
    builder.add_token(text("<<="), Token_kind::Less_less_equal, 1);
    builder.add_token(text(">>="), Token_kind::Greater_greater_equal, 1);

    builder.add_token(plus(any_of(Set{' ', '\t'})), Token_kind::Whitespace, 0);
    // The reader no longer normalizes line endings, so all three conventions are one newline token each.
    builder.add_token(choice(text("\r\n"), text("\r"), text("\n")), Token_kind::Newline, 0);

    return builder.build();
}

} // namespace hopper::cpp
