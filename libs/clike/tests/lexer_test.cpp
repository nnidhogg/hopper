#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

#include "hopper/clike/tokens.hpp"
#include "hopper/parse/parse_error.hpp"
#include "hopper/parse/token_reader.hpp"

namespace
{
using hopper::clike::is_trivia;
using hopper::clike::lexer;
using hopper::clike::Token_kind;

/**
 * @brief Every token of an input with its kind and lexeme, trivia included; a lexical error ends the list with an
 *        entry whose lexeme is the error's message.
 */
std::vector<std::pair<Token_kind, std::string>> tokens(const std::string& input)
{
    hopper::parse::Token_reader<Token_kind> reader{lexer(), input};

    std::vector<std::pair<Token_kind, std::string>> out;

    for (;;)
    {
        const auto result{reader.next()};

        if (result.has_token())
        {
            out.emplace_back(result.token().kind(), std::string{result.token().lexeme()});
        }
        else if (result.has_error())
        {
            out.emplace_back(Token_kind::Whitespace, "error: " + std::string{result.error().message()});
            return out;
        }
        else
        {
            return out;
        }
    }
}

/**
 * @brief The kinds of an input's tokens, trivia included.
 */
std::vector<Token_kind> kinds(const std::string& input)
{
    std::vector<Token_kind> out;

    for (const auto& [kind, lexeme] : tokens(input))
    {
        out.push_back(kind);
    }

    return out;
}

} // namespace

TEST(Lexer_test, The_study_grammar_has_seven_token_kinds_and_no_finer)
{
    using enum Token_kind;

    EXPECT_EQ(
            kinds("x1 42 + ( \"s\" // c\n\t"),
            (std::vector{
                    Identifier, Whitespace, Number, Whitespace, Operator, Whitespace, Punctuation, Whitespace, String,
                    Whitespace, Line_comment, Whitespace}));
}

TEST(Lexer_test, Keywords_are_identifiers_and_multi_byte_operators_are_runs_of_operator_bytes)
{
    const auto listed{tokens("if x<<=1")};

    ASSERT_EQ(listed.size(), 7U);
    EXPECT_EQ(listed[0], std::make_pair(Token_kind::Identifier, std::string{"if"}));
    EXPECT_EQ(listed[2], std::make_pair(Token_kind::Identifier, std::string{"x"}));
    EXPECT_EQ(listed[3], std::make_pair(Token_kind::Operator, std::string{"<"}));
    EXPECT_EQ(listed[4], std::make_pair(Token_kind::Operator, std::string{"<"}));
    EXPECT_EQ(listed[5], std::make_pair(Token_kind::Operator, std::string{"="}));
    EXPECT_EQ(listed[6], std::make_pair(Token_kind::Number, std::string{"1"}));
}

TEST(Lexer_test, A_line_comment_outranks_the_slash_operator_by_length_and_ends_at_the_newline)
{
    const auto listed{tokens("a / b // c / d\ne")};

    EXPECT_EQ(listed[2], std::make_pair(Token_kind::Operator, std::string{"/"}));
    EXPECT_EQ(listed[6], std::make_pair(Token_kind::Line_comment, std::string{"// c / d"}));
    EXPECT_EQ(listed[7], std::make_pair(Token_kind::Whitespace, std::string{"\n"}));
    EXPECT_EQ(listed[8], std::make_pair(Token_kind::Identifier, std::string{"e"}));
}

TEST(Lexer_test, Strings_have_no_escapes_and_cannot_cross_a_line)
{
    EXPECT_EQ(tokens("\"a\\\"")[0], std::make_pair(Token_kind::String, std::string{"\"a\\\""}));
    EXPECT_EQ(
            tokens("\"// not a comment\"")[0], std::make_pair(Token_kind::String, std::string{"\"// not a comment\""}));
    EXPECT_TRUE(tokens("\"a\nb\"").front().second.starts_with("error:"));
    EXPECT_TRUE(tokens("\"abc").front().second.starts_with("error:"));
}

TEST(Lexer_test, Numbers_are_digit_runs_and_the_dot_is_punctuation)
{
    using enum Token_kind;

    EXPECT_EQ(kinds("3.5"), (std::vector{Number, Punctuation, Number}));
    EXPECT_EQ(kinds("0x1F"), (std::vector{Number, Identifier}));
}

TEST(Lexer_test, A_carriage_return_is_outside_the_grammar)
{
    EXPECT_TRUE(tokens("a\r\nb")[1].second.starts_with("error:"));
}

TEST(Lexer_test, Trivia_is_whitespace_and_line_comments_only)
{
    EXPECT_TRUE(is_trivia(Token_kind::Whitespace));
    EXPECT_TRUE(is_trivia(Token_kind::Line_comment));
    EXPECT_FALSE(is_trivia(Token_kind::String));
    EXPECT_FALSE(is_trivia(Token_kind::Operator));
}
