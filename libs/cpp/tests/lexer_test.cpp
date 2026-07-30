#include "hopper/cpp/lexer.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "hopper/cpp/tokens.hpp"
#include "hopper/parse/parse_error.hpp"
#include "hopper/parse/token_reader.hpp"

using namespace hopper::cpp;

namespace
{
/**
 * @brief Tokenizes the whole input without trivia skipping, so comment and whitespace tokens stay visible.
 */
std::vector<std::pair<Token_kind, std::string>> lex(const std::string& input)
{
    hopper::parse::Token_reader<Token_kind> reader{build_lexer(), input};

    std::vector<std::pair<Token_kind, std::string>> tokens;

    for (;;)
    {
        const auto result{reader.next()};

        if (result.end_of_input())
        {
            return tokens;
        }

        if (result.has_error())
        {
            throw hopper::parse::Parse_error{hopper::parse::Parse_error_kind::Lexical, {}, result.error().message()};
        }

        tokens.emplace_back(result.token().kind(), std::string{result.token().lexeme()});
    }
}

} // namespace

TEST(Lexer_test, Comment_delimiters_inside_strings_are_string_content)
{
    const auto tokens{lex("\"/* not a comment */\"")};

    ASSERT_EQ(tokens.size(), 1U);
    EXPECT_EQ(tokens.front().first, Token_kind::String_literal);
}

TEST(Lexer_test, Comments_swallow_string_quotes)
{
    const auto tokens{lex("/* \" */")};

    ASSERT_EQ(tokens.size(), 1U);
    EXPECT_EQ(tokens.front().first, Token_kind::Block_comment);
}

TEST(Lexer_test, Comment_delimiters_adjacent_to_operators)
{
    // "a/*c*/b" is exactly three tokens: the comment binds the '/' and '*' that would otherwise be operators.
    const auto tokens{lex("a/*c*/b")};

    ASSERT_EQ(tokens.size(), 3U);
    EXPECT_EQ(tokens[0].first, Token_kind::Identifier);
    EXPECT_EQ(tokens[1].first, Token_kind::Block_comment);
    EXPECT_EQ(tokens[2].first, Token_kind::Identifier);

    // Outside a comment, "*/" is two ordinary operators.
    const auto operators{lex("a * / b")};

    ASSERT_EQ(operators.size(), 7U);
    EXPECT_EQ(operators[2].first, Token_kind::Star);
    EXPECT_EQ(operators[4].first, Token_kind::Slash);
}

TEST(Lexer_test, Base_prefixes_accept_both_cases)
{
    EXPECT_EQ(lex("0x1F").front(), (std::pair{Token_kind::Integer_literal, std::string{"0x1F"}}));
    EXPECT_EQ(lex("0X1f").front(), (std::pair{Token_kind::Integer_literal, std::string{"0X1f"}}));
    EXPECT_EQ(lex("0b10").front(), (std::pair{Token_kind::Integer_literal, std::string{"0b10"}}));
    EXPECT_EQ(lex("0B01").front(), (std::pair{Token_kind::Integer_literal, std::string{"0B01"}}));
}

TEST(Lexer_test, Maximal_munch_keeps_literals_whole)
{
    // One token each, not "0" followed by an identifier.
    ASSERT_EQ(lex("0xFF").size(), 1U);
    ASSERT_EQ(lex("0b11").size(), 1U);

    // An escape keeps the string open: the quote after the backslash does not terminate it.
    const auto tokens{lex("\"a\\\"b\"")};

    ASSERT_EQ(tokens.size(), 1U);
    EXPECT_EQ(tokens.front().second, "\"a\\\"b\"");
}

TEST(Lexer_test, Unterminated_strings_and_characters_are_lexical_errors)
{
    EXPECT_THROW(static_cast<void>(lex("\"abc")), hopper::parse::Parse_error);
    EXPECT_THROW(static_cast<void>(lex("'a")), hopper::parse::Parse_error);
}

TEST(Lexer_test, Unterminated_block_comments_fall_back_to_operators)
{
    // A "/*" that never closes is not a lexical error: maximal munch falls back to '/' and '*' operator tokens
    // and the parser reports a syntax error later. A dedicated opening-delimiter token could sharpen this
    // diagnostic one day; until then this test documents the actual behavior.
    const auto tokens{lex("/* open")};

    ASSERT_EQ(tokens.size(), 4U);
    EXPECT_EQ(tokens[0].first, Token_kind::Slash);
    EXPECT_EQ(tokens[1].first, Token_kind::Star);
    EXPECT_EQ(tokens[3].first, Token_kind::Identifier);
}
