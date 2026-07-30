#include "hopper/parse/token_reader.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <munch/core/builder.hpp>
#include <munch/regex/regex.hpp>
#include <string>

using namespace hopper::parse;

namespace
{
enum class Kind : uint8_t
{
    Word,
    Number,
    Whitespace,
    Newline,
};

bool skip_trivia(const Kind kind)
{
    return kind == Kind::Whitespace;
}

munch::core::Lexer build_lexer()
{
    using namespace munch::regex;

    munch::core::Builder builder;

    builder.add_token(plus(any_of(Set::alpha())), Kind::Word, 1);
    builder.add_token(plus(any_of(Set::digits())), Kind::Number, 1);
    builder.add_token(plus(any_of(Set{' ', '\t'})), Kind::Whitespace, 1);
    builder.add_token(choice(text("\r\n"), text("\r"), text("\n")), Kind::Newline, 1);

    return builder.build();
}

} // namespace

TEST(Token_reader_test, Yields_tokens_in_order_then_end_of_input)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"one 2 three"}, skip_trivia};

    EXPECT_EQ(reader.next().token().kind(), Kind::Word);
    EXPECT_EQ(reader.next().token().kind(), Kind::Number);
    EXPECT_EQ(reader.next().token().kind(), Kind::Word);

    EXPECT_TRUE(reader.next().end_of_input());
    EXPECT_TRUE(reader.next().end_of_input());
}

TEST(Token_reader_test, Peek_does_not_consume)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"one 2"}, skip_trivia};

    EXPECT_EQ(reader.peek().token().lexeme(), "one");
    EXPECT_EQ(reader.peek().token().lexeme(), "one");

    EXPECT_EQ(reader.next().token().lexeme(), "one");
    EXPECT_EQ(reader.peek().token().lexeme(), "2");
}

TEST(Token_reader_test, Skip_predicate_discards_trivia)
{
    Token_reader<Kind> with_skip{build_lexer(), std::string{"a b"}, skip_trivia};

    EXPECT_EQ(with_skip.next().token().kind(), Kind::Word);
    EXPECT_EQ(with_skip.next().token().kind(), Kind::Word);
    EXPECT_TRUE(with_skip.next().end_of_input());

    // Without the predicate, the whitespace token surfaces like any other.
    Token_reader<Kind> without_skip{build_lexer(), std::string{"a b"}};

    EXPECT_EQ(without_skip.next().token().kind(), Kind::Word);
    EXPECT_EQ(without_skip.next().token().kind(), Kind::Whitespace);
    EXPECT_EQ(without_skip.next().token().kind(), Kind::Word);
}

TEST(Token_reader_test, Lexical_errors_surface_with_their_position)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"ok @"}, skip_trivia};

    EXPECT_EQ(reader.next().token().lexeme(), "ok");

    const auto result{reader.next()};

    ASSERT_TRUE(result.has_error());
    EXPECT_EQ(result.error().position(), 3U);
}

TEST(Token_reader_test, Load_replaces_the_input_and_reset_rewinds_it)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"first"}, skip_trivia};

    EXPECT_EQ(reader.next().token().lexeme(), "first");

    reader.load(std::string{"second"});

    EXPECT_EQ(reader.next().token().lexeme(), "second");

    reader.reset();

    EXPECT_EQ(reader.next().token().lexeme(), "second");
}

TEST(Token_reader_test, Windows_newlines_are_one_token_and_offsets_stay_original)
{
    // The input is tokenized exactly as given: "\r\n" is one Newline token, and offsets index the original
    // bytes, so a diagnostic pointing at 'b' matches the file on disk byte for byte.
    Token_reader<Kind> reader{build_lexer(), std::string{"a\r\nb"}, skip_trivia};

    EXPECT_EQ(reader.next().token().kind(), Kind::Word);

    EXPECT_EQ(reader.next().token().lexeme(), "\r\n");

    EXPECT_EQ(reader.next().token().kind(), Kind::Word);
    EXPECT_EQ(reader.location().line(), 2U);
    EXPECT_EQ(reader.location().column(), 1U);
    EXPECT_EQ(reader.location().offset(), 3U);

    EXPECT_TRUE(reader.next().end_of_input());
}

TEST(Token_reader_test, Location_tracks_lines_and_columns)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"one\ntwo"}, skip_trivia};

    EXPECT_EQ(reader.next().token().lexeme(), "one");
    EXPECT_EQ(reader.location().line(), 1U);

    EXPECT_EQ(reader.next().token().kind(), Kind::Newline);
    EXPECT_EQ(reader.next().token().lexeme(), "two");
    EXPECT_EQ(reader.location().line(), 2U);
}

TEST(Token_reader_test, Location_reports_the_current_tokens_start)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"one two"}, skip_trivia};

    EXPECT_EQ(reader.next().token().lexeme(), "one");
    EXPECT_EQ(reader.location().line(), 1U);
    EXPECT_EQ(reader.location().column(), 1U);
    EXPECT_EQ(reader.location().offset(), 0U);

    EXPECT_EQ(reader.next().token().lexeme(), "two");
    EXPECT_EQ(reader.location().column(), 5U);
    EXPECT_EQ(reader.location().offset(), 4U);
}

TEST(Token_reader_test, Location_starts_lines_after_a_newline)
{
    Token_reader<Kind> reader{build_lexer(), std::string{"one\r\ntwo"}, skip_trivia};

    EXPECT_EQ(reader.next().token().lexeme(), "one");
    EXPECT_EQ(reader.next().token().kind(), Kind::Newline);

    EXPECT_EQ(reader.next().token().lexeme(), "two");
    EXPECT_EQ(reader.location().line(), 2U);
    EXPECT_EQ(reader.location().column(), 1U);

    // Offsets index the original bytes: "one\r\n" is five of them, so "two" begins at offset 5.
    EXPECT_EQ(reader.location().offset(), 5U);
}
