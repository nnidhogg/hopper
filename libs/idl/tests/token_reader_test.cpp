#include "hopper/idl/token_reader.hpp"

#include <gtest/gtest.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <munch/core/builder.hpp>
#include <munch/regex/any_of.hpp>
#include <munch/regex/choice.hpp>
#include <munch/regex/concat.hpp>
#include <munch/regex/repeat.hpp>
#include <munch/regex/text.hpp>
#include <munch/tools/tokenizer/tokenizer.hpp>
#include <string>

#include "hopper/idl/token_reader.hpp"
#include "hopper/idl/tokens.hpp"

using namespace munch::core;
using namespace munch::regex;

using namespace hopper::idl;

namespace
{
class Token_reader_test : public testing::Test
{
public:
    static auto identifier_regex()
    {
        const auto identifier{concat(any_of(Set::alpha() + '_'), kleene(any_of(Set::alphanum() + '_')))};

        return identifier;
    }

    static auto integer_literal_regex()
    {
        const auto integer_literal{plus(any_of(Set::digits()))};

        return integer_literal;
    }

    static auto string_literal_regex()
    {
        const auto string_literal{concat(text("\""), kleene(any_of(Set::printable())), text("\""))};

        return string_literal;
    }

    static auto fixed_point_literal_regex()
    {
        const auto fixed_point_literal{concat(plus(any_of(Set::digits())), text("."), plus(any_of(Set::digits())))};

        return fixed_point_literal;
    }

    static auto floating_point_literal_regex()
    {
        const auto any_digit{any_of(Set::digits())};

        const auto sign_part{choice(text("+"), text("-"))};

        const auto exponent_part{concat(choice(text("e"), text("E")), optional(sign_part), plus(any_digit))};

        const auto leading_digits{concat(plus(any_digit), text("."), kleene(any_digit), optional(exponent_part))};

        const auto leading_decimal{concat(text("."), plus(any_digit), optional(exponent_part))};

        const auto forced_exponent{concat(plus(any_digit), exponent_part)};

        const auto fraction_part{choice(leading_digits, leading_decimal, forced_exponent)};

        const auto floating_point_literal{concat(optional(sign_part), fraction_part)};

        return floating_point_literal;
    }

    static auto single_line_comment_regex()
    {
        const auto single_line_comment{
                concat(text("//"), kleene(any_of(Set::printable() + Set::escape() - Set::newline())))};

        return single_line_comment;
    }

    static auto multi_line_comment_regex()
    {
        const auto multi_line_comment{concat(text("/*"), kleene(any_of(Set::printable() + Set::escape())), text("*/"))};

        return multi_line_comment;
    }

    static auto whitespace_regex()
    {
        const auto whitespace{plus(any_of(Set::whitespace()))};

        return whitespace;
    }

    static auto newline_regex()
    {
        const auto newline{plus(any_of(Set::newline()))};

        return newline;
    }
};

Lexer build_lexer()
{
    Builder builder;

    builder.add_token(text("boolean"), Token_kind::Keyword_boolean, 1);
    builder.add_token(text("char"), Token_kind::Keyword_char, 1);
    builder.add_token(text("string"), Token_kind::Keyword_string, 1);

    builder.add_token(Token_reader_test::identifier_regex(), Token_kind::Identifier, 4);

    builder.add_token(Token_reader_test::integer_literal_regex(), Token_kind::Integer_literal, 2);
    builder.add_token(Token_reader_test::string_literal_regex(), Token_kind::String_literal, 2);
    builder.add_token(Token_reader_test::fixed_point_literal_regex(), Token_kind::Fixed_point_literal, 2);
    builder.add_token(Token_reader_test::floating_point_literal_regex(), Token_kind::Floating_point_literal, 3);

    builder.add_token(Token_reader_test::single_line_comment_regex(), Token_kind::Single_line_comment, 0);
    builder.add_token(Token_reader_test::multi_line_comment_regex(), Token_kind::Multi_line_comment, 0);

    builder.add_token(Token_reader_test::whitespace_regex(), Token_kind::Whitespace, 0);

    builder.add_token(Token_reader_test::newline_regex(), Token_kind::Newline, 0);

    return builder.build();
}

} // namespace

TEST_F(Token_reader_test, Tokenize_from_string_stream)
{
    const std::string input{
            "boolean x 1234 \"hello\" 3.14 // comment\n"
            "string y 5.0e+1 /* block */"};

    auto lexer{build_lexer()};

    Token_reader reader{std::move(lexer), input};

    const auto advance = [&reader](const Token_kind expect_kind, const std::string_view expect_lexeme) {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), expect_kind);
        EXPECT_EQ(token.lexeme(), expect_lexeme);
    };

    const auto evaluate = [&reader, &advance] {
        advance(Token_kind::Keyword_boolean, "boolean");
        advance(Token_kind::Identifier, "x");
        advance(Token_kind::Integer_literal, "1234");
        advance(Token_kind::String_literal, "\"hello\"");
        advance(Token_kind::Fixed_point_literal, "3.14");
        advance(Token_kind::Single_line_comment, "// comment");
        advance(Token_kind::Keyword_string, "string");
        advance(Token_kind::Identifier, "y");
        advance(Token_kind::Floating_point_literal, "5.0e+1");
        advance(Token_kind::Multi_line_comment, "/* block */");

        EXPECT_TRUE(reader.next().end_of_input());
    };

    evaluate();

    reader.reset();

    evaluate();

    reader.load(input);

    evaluate();
}

TEST_F(Token_reader_test, Tokenize_from_file_stream)
{
    const auto path{std::filesystem::temp_directory_path() / "tokenizer_stream_test.idl"};
    {
        std::ofstream out{path};

        ASSERT_TRUE(out.is_open());

        out << "boolean x 1234 \"hello\" 3.14 // comment\n"
               "string y 5.0e+1 /* block */";
    }

    auto lexer{build_lexer()};

    Token_reader reader{std::move(lexer), path};

    const auto advance = [&reader](const Token_kind expect_kind, const std::string_view expect_lexeme) {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), expect_kind);
        EXPECT_EQ(token.lexeme(), expect_lexeme);
    };

    const auto evaluate = [&reader, &advance] {
        advance(Token_kind::Keyword_boolean, "boolean");
        advance(Token_kind::Identifier, "x");
        advance(Token_kind::Integer_literal, "1234");
        advance(Token_kind::String_literal, "\"hello\"");
        advance(Token_kind::Fixed_point_literal, "3.14");
        advance(Token_kind::Single_line_comment, "// comment");
        advance(Token_kind::Keyword_string, "string");
        advance(Token_kind::Identifier, "y");
        advance(Token_kind::Floating_point_literal, "5.0e+1");
        advance(Token_kind::Multi_line_comment, "/* block */");

        EXPECT_TRUE(reader.next().end_of_input());
    };

    evaluate();

    reader.reset();

    evaluate();

    reader.load(path);

    evaluate();

    std::filesystem::remove(path);
}

TEST_F(Token_reader_test, Fails_to_open_file)
{
    const auto missing_file{std::filesystem::temp_directory_path() / "nonexistent_lexer_input.idl"};

    std::filesystem::remove(missing_file);

    const auto lexer{build_lexer()};

    EXPECT_THROW(Token_reader(lexer, missing_file), std::runtime_error);

    Token_reader reader{lexer};

    EXPECT_THROW(reader.load(missing_file), std::runtime_error);
}

TEST_F(Token_reader_test, Unknown_character_reports_position)
{
    {
        const std::string input{"$boolean"}; // '$' not recognized by the grammar

        auto lexer{build_lexer()};

        Token_reader reader{std::move(lexer), input};

        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_error());

        const auto& error{expected.error()};
        EXPECT_FALSE(error.message().empty());
        EXPECT_EQ(error.position(), 0);
    }

    {
        const std::string input{"boolean$"}; // '$' not recognized by the grammar

        auto lexer{build_lexer()};

        Token_reader reader{std::move(lexer), input};

        ASSERT_TRUE(reader.next().has_token()); // First part will be recognized

        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_error());

        const auto& error{expected.error()};
        EXPECT_FALSE(error.message().empty());
        EXPECT_EQ(error.position(), 7);
    }

    {
        const std::string input{"bo$olean"}; // '$' not recognized by the grammar

        auto lexer{build_lexer()};

        Token_reader reader{std::move(lexer), input};

        ASSERT_TRUE(reader.next().has_token()); // First part will be recognized

        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_error());

        const auto& error{expected.error()};
        EXPECT_FALSE(error.message().empty());
        EXPECT_EQ(error.position(), 2);
    }
}

TEST_F(Token_reader_test, CRLF_is_normalized_and_locations_advance)
{
    const std::string input{"boolean\r\nx\r\n1234"};

    auto lexer{build_lexer()};

    Token_reader reader{std::move(lexer), input};

    const auto advance = [&reader](const Token_kind expect_kind, const std::string_view expect_lexeme) {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), expect_kind);
        EXPECT_EQ(token.lexeme(), expect_lexeme);
    };

    advance(Token_kind::Keyword_boolean, "boolean");
    EXPECT_EQ(reader.location().line(), 1);
    EXPECT_EQ(reader.location().column(), 8); // after 'boolean' (1-based)
    EXPECT_EQ(reader.location().offset(), 7);

    advance(Token_kind::Identifier, "x");
    EXPECT_EQ(reader.location().line(), 2);
    EXPECT_EQ(reader.location().column(), 2); // 1 char + 1-based
    EXPECT_EQ(reader.location().offset(), 9);

    advance(Token_kind::Integer_literal, "1234");
    EXPECT_EQ(reader.location().line(), 3);
    EXPECT_EQ(reader.location().column(), 5); // after '1234'
    EXPECT_EQ(reader.location().offset(), 14);

    EXPECT_TRUE(reader.next().end_of_input());
}

TEST_F(Token_reader_test, Peek_is_stable_until_next)
{
    const std::string input{"boolean x"};

    auto lexer{build_lexer()};

    Token_reader reader{std::move(lexer), input};

    const auto advance = [&reader](const Token_kind expect_kind, const std::string_view expect_lexeme) {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), expect_kind);
        EXPECT_EQ(token.lexeme(), expect_lexeme);
    };

    const auto peek = [&reader](const Token_kind expect_kind, const std::string_view expect_lexeme) {
        const auto expected{reader.peek()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), expect_kind);
        EXPECT_EQ(token.lexeme(), expect_lexeme);
    };

    peek(Token_kind::Keyword_boolean, "boolean");
    peek(Token_kind::Keyword_boolean, "boolean");
    advance(Token_kind::Keyword_boolean, "boolean");
    peek(Token_kind::Identifier, "x");
    advance(Token_kind::Identifier, "x");

    EXPECT_TRUE(reader.next().end_of_input());
}

TEST_F(Token_reader_test, Next_is_idempotent_at_eof)
{
    const std::string input{"boolean"};

    auto lexer{build_lexer()};

    Token_reader reader{std::move(lexer), input};

    {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), Token_kind::Keyword_boolean);
        EXPECT_EQ(token.lexeme(), "boolean");
    }

    for (int i = 0; i < 3; ++i)
    {
        EXPECT_TRUE(reader.next().end_of_input());
    }
}

TEST_F(Token_reader_test, Multiline_comment_updates_location)
{
    const std::string input{"/* a\nb\nc */x"};

    auto lexer{build_lexer()};

    Token_reader reader{std::move(lexer), input};

    {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), Token_kind::Multi_line_comment);
        EXPECT_EQ(token.lexeme(), "/* a\nb\nc */");

        const auto& location{reader.location()};
        EXPECT_EQ(location.line(), 3);
        EXPECT_EQ(location.column(), 5);
        EXPECT_EQ(location.offset(), 11);
    }

    {
        const auto expected{reader.next()};
        ASSERT_TRUE(expected.has_token());

        const auto& token{expected.token()};
        EXPECT_EQ(token.kind(), Token_kind::Identifier);
        EXPECT_EQ(token.lexeme(), "x");

        const auto& location{reader.location()};
        EXPECT_EQ(location.line(), 3);
        EXPECT_EQ(location.column(), 6);
        EXPECT_EQ(location.offset(), 12);
    }
}
