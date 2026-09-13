#include "hopper/json/parser.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "hopper/json/value.hpp"
#include "hopper/parse/parse_error.hpp"

namespace
{
using hopper::json::Array;
using hopper::json::Null;
using hopper::json::Number;
using hopper::json::Object;
using hopper::json::Parser;
using hopper::json::Value;
using hopper::parse::Parse_error;
using hopper::parse::Parse_error_kind;

/**
 * @brief Parses a text with a fresh parser.
 */
Value parse(const std::string& text)
{
    Parser parser{text};

    return parser.parse();
}

/**
 * @brief The kind of the error a text raises; a text that parses is a test failure.
 */
Parse_error_kind failure(const std::string& text)
{
    try
    {
        (void)parse(text);
    }
    catch (const Parse_error& error)
    {
        return error.kind();
    }

    ADD_FAILURE() << "parsed without error: " << text;

    return Parse_error_kind::Lexical;
}

} // namespace

TEST(Json, Scalars_parse_to_their_kinds)
{
    EXPECT_TRUE(parse("null").is_null());
    EXPECT_TRUE(parse(" true ").as_bool());
    EXPECT_FALSE(parse("false").as_bool());
    EXPECT_EQ(parse("-12.5e3").as_number().text, "-12.5e3");
    EXPECT_EQ(parse("\"a\"").as_string(), "a");
}

TEST(Json, Numbers_convert_to_the_nearest_double_and_saturate_past_the_range)
{
    EXPECT_DOUBLE_EQ(Number{.text = "1.5"}.to_double(), 1.5);
    EXPECT_DOUBLE_EQ(Number{.text = "-0"}.to_double(), -0.0);
    EXPECT_DOUBLE_EQ(Number{.text = "1e2"}.to_double(), 100.0);
    EXPECT_TRUE(std::isinf(Number{.text = "1e400"}.to_double()));
    EXPECT_TRUE(std::isinf(Number{.text = "-1e400"}.to_double()));
    EXPECT_LT(Number{.text = "-1e400"}.to_double(), 0.0);
    EXPECT_DOUBLE_EQ(Number{.text = "1e-400"}.to_double(), 0.0);
    EXPECT_DOUBLE_EQ(Number{.text = "0.0e400"}.to_double(), 0.0);
    EXPECT_DOUBLE_EQ(Number{.text = "123456789"}.to_double(), 123456789.0);
}

TEST(Json, Strings_resolve_every_escape_to_utf8)
{
    EXPECT_EQ(parse(R"("\"\\\/\b\f\n\r\t")").as_string(), "\"\\/\b\f\n\r\t");
    EXPECT_EQ(parse(R"("\u0041\u00e9\u20ac")").as_string(), "A\xC3\xA9\xE2\x82\xAC");
    EXPECT_EQ(parse(R"("\ud83d\ude00")").as_string(), "\xF0\x9F\x98\x80");
    EXPECT_EQ(parse("\"\xC3\xA9\"").as_string(), "\xC3\xA9");
}

TEST(Json, A_lone_surrogate_escape_is_an_invalid_literal_at_the_string)
{
    EXPECT_EQ(failure(R"("\ud83d")"), Parse_error_kind::Invalid_literal);
    EXPECT_EQ(failure(R"("\ude00")"), Parse_error_kind::Invalid_literal);
    EXPECT_EQ(failure(R"("\ud83d\u0041")"), Parse_error_kind::Invalid_literal);

    try
    {
        (void)parse("[1, \"\\udc00\"]");
    }
    catch (const Parse_error& error)
    {
        EXPECT_EQ(error.span().begin.offset, 4U);
        EXPECT_EQ(error.span().end.offset, 12U);
    }
}

TEST(Json, Containers_keep_document_order_and_duplicates)
{
    const auto value{parse(R"({"a": [1, 2, {"b": null}], "a": true, "c": {}})")};

    const auto& object{value.as_object()};

    ASSERT_EQ(object.members.size(), 3U);
    EXPECT_EQ(object.members[0].name, "a");
    EXPECT_EQ(object.members[1].name, "a");
    EXPECT_EQ(object.members[2].name, "c");
    EXPECT_EQ(object.find("a"), 1U);
    EXPECT_TRUE(object.at("a").as_bool());
    EXPECT_FALSE(object.find("missing").has_value());
    EXPECT_THROW((void)object.at("missing"), std::out_of_range);

    const auto& array{object.members[0].value.as_array()};

    ASSERT_EQ(array.elements.size(), 3U);
    EXPECT_EQ(array.elements[1].as_number().text, "2");
    EXPECT_TRUE(array.elements[2].as_object().at("b").is_null());
    EXPECT_TRUE(object.members[2].value.as_object().members.empty());
}

TEST(Json, Spans_cover_each_value_from_its_first_byte_to_one_past_its_last)
{
    const auto value{parse("  [ 10 , \"x\" , {\"k\": [ ] } ]  ")};

    EXPECT_EQ(value.span.begin.offset, 2U);
    EXPECT_EQ(value.span.end.offset, 28U);

    const auto& elements{value.as_array().elements};

    EXPECT_EQ(elements[0].span.begin.offset, 4U);
    EXPECT_EQ(elements[0].span.end.offset, 6U);
    EXPECT_EQ(elements[1].span.begin.offset, 9U);
    EXPECT_EQ(elements[1].span.end.offset, 12U);
    EXPECT_EQ(elements[2].span.begin.offset, 15U);
    EXPECT_EQ(elements[2].span.end.offset, 26U);

    const auto& inner{elements[2].as_object().members[0].value};

    EXPECT_EQ(inner.span.begin.offset, 21U);
    EXPECT_EQ(inner.span.end.offset, 24U);
    EXPECT_EQ(inner.span.begin.line, 1U);
    EXPECT_EQ(inner.span.begin.column, 22U);
}

TEST(Json, Deep_nesting_parses_without_recursion)
{
    const std::string open(200000, '[');
    const std::string close(200000, ']');

    const auto value{parse(open + close)};

    const Value* at{&value};

    for (int depth{0}; depth < 200000; ++depth)
    {
        ASSERT_EQ(at->as_array().elements.size(), depth == 199999 ? 0U : 1U);

        if (depth != 199999)
        {
            at = &at->as_array().elements.front();
        }
    }
}

TEST(Json, Errors_name_their_kind_and_place)
{
    EXPECT_EQ(failure(""), Parse_error_kind::Unexpected_end);
    EXPECT_EQ(failure("["), Parse_error_kind::Unexpected_end);
    EXPECT_EQ(failure("{\"a\""), Parse_error_kind::Unexpected_end);
    EXPECT_EQ(failure("[1,]"), Parse_error_kind::Unexpected_token);
    EXPECT_EQ(failure("{\"a\" 1}"), Parse_error_kind::Unexpected_token);
    EXPECT_EQ(failure("{1: 2}"), Parse_error_kind::Unexpected_token);
    EXPECT_EQ(failure("1 2"), Parse_error_kind::Unexpected_token);
    EXPECT_EQ(failure("]"), Parse_error_kind::Unexpected_token);
    EXPECT_EQ(failure("tru"), Parse_error_kind::Lexical);
    EXPECT_EQ(failure("\"tab\there\""), Parse_error_kind::Lexical);
    EXPECT_EQ(failure("01"), Parse_error_kind::Unexpected_token);
    EXPECT_EQ(failure("\"\xff\""), Parse_error_kind::Lexical);
}

TEST(Json, Values_compare_by_content_and_not_by_span)
{
    EXPECT_EQ(parse("[1, {\"a\": \"b\"}]"), parse("  [ 1 , { \"a\" : \"\\u0062\" } ]"));
    EXPECT_NE(parse("[1]"), parse("[1.0]"));
    EXPECT_EQ(parse("null"), (Value{Null{}, {}}));
}
