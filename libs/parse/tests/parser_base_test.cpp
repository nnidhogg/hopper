#include "hopper/parse/parser_base.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <munch/core/builder.hpp>
#include <munch/regex/regex.hpp>
#include <stdexcept>
#include <string>
#include <utility>

#include "hopper/parse/token_reader.hpp"

using namespace hopper::parse;

namespace
{
enum class Kind : uint8_t
{
    Word,
    Number,
    Whitespace,
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
    builder.add_token(plus(any_of(Set{' '})), Kind::Whitespace, 1);

    return builder.build();
}

/**
 * @brief Exposes the protected LL(1) primitives so they can be exercised directly.
 */
class Test_parser : public Parser_base<Kind>
{
public:
    explicit Test_parser(const std::string& input) : Parser_base{Token_reader<Kind>{build_lexer(), input, skip_trivia}}
    {}

    using Parser_base::accept;
    using Parser_base::check;
    using Parser_base::expect;
    using Parser_base::next_token;
    using Parser_base::peek_token;
};

} // namespace

TEST(Parser_base_test, Check_looks_without_consuming)
{
    Test_parser parser{"word 1"};

    EXPECT_TRUE(parser.check(Kind::Word));
    EXPECT_FALSE(parser.check(Kind::Number));
    EXPECT_TRUE(parser.check(Kind::Word));
}

TEST(Parser_base_test, Accept_consumes_only_on_a_match)
{
    Test_parser parser{"word 1"};

    EXPECT_FALSE(parser.accept(Kind::Number).has_value());
    EXPECT_EQ(parser.accept(Kind::Word)->lexeme(), "word");
    EXPECT_EQ(parser.accept(Kind::Number)->lexeme(), "1");
}

TEST(Parser_base_test, Expect_returns_the_token_or_throws)
{
    Test_parser parser{"word"};

    EXPECT_EQ(parser.expect(Kind::Word, "a word").lexeme(), "word");

    EXPECT_THROW(static_cast<void>(parser.expect(Kind::Word, "another word")), std::runtime_error);
}

TEST(Parser_base_test, Expect_names_the_offending_token)
{
    Test_parser parser{"1"};

    try
    {
        static_cast<void>(parser.expect(Kind::Word, "a word"));

        FAIL() << "expect() should have thrown";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_NE(std::string{error.what()}.find("a word"), std::string::npos);
        EXPECT_NE(std::string{error.what()}.find('1'), std::string::npos);
    }
}

TEST(Parser_base_test, Peek_and_next_report_end_of_input)
{
    Test_parser parser{""};

    EXPECT_FALSE(parser.peek_token().has_value());
    EXPECT_FALSE(parser.next_token().has_value());
}

TEST(Parser_base_test, Lexical_errors_become_exceptions)
{
    Test_parser parser{"@"};

    EXPECT_THROW(static_cast<void>(parser.next_token()), std::runtime_error);
}
