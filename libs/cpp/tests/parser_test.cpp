#include "hopper/cpp/parser.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <type_traits>

#include "hopper/cpp/lexer.hpp"

using namespace hopper::cpp;

namespace
{
std::string operator_symbol(const ast::Binary_op op)
{
    switch (op)
    {
    case ast::Binary_op::Add:
        return "+";
    case ast::Binary_op::Subtract:
        return "-";
    case ast::Binary_op::Multiply:
        return "*";
    case ast::Binary_op::Divide:
        return "/";
    case ast::Binary_op::Modulo:
        return "%";
    case ast::Binary_op::Less:
        return "<";
    case ast::Binary_op::Greater:
        return ">";
    case ast::Binary_op::Less_equal:
        return "<=";
    case ast::Binary_op::Greater_equal:
        return ">=";
    case ast::Binary_op::Equal:
        return "==";
    case ast::Binary_op::Not_equal:
        return "!=";
    case ast::Binary_op::Logical_and:
        return "&&";
    case ast::Binary_op::Logical_or:
        return "||";
    }

    return "?";
}

std::string operator_symbol(const ast::Unary_op op)
{
    switch (op)
    {
    case ast::Unary_op::Plus:
        return "+";
    case ast::Unary_op::Minus:
        return "-";
    case ast::Unary_op::Not:
        return "!";
    }

    return "?";
}

// Renders the AST back as a fully-parenthesized expression, so precedence and associativity are visible directly
// in the expected string rather than in a deeply nested chain of std::get<> assertions.
std::string to_string(const ast::Expr& expr)
{
    return std::visit(
            [](const auto& node) -> std::string {
                using Node_t = std::decay_t<decltype(node)>;

                if constexpr (std::is_same_v<Node_t, ast::Int_literal>)
                {
                    return std::to_string(node.value);
                }
                else if constexpr (std::is_same_v<Node_t, ast::Float_literal>)
                {
                    return std::to_string(node.value);
                }
                else if constexpr (std::is_same_v<Node_t, ast::Bool_literal>)
                {
                    return node.value ? "true" : "false";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Name>)
                {
                    return node.identifier;
                }
                else if constexpr (std::is_same_v<Node_t, ast::Unary>)
                {
                    return "(" + operator_symbol(node.op) + to_string(*node.operand) + ")";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Binary>)
                {
                    return "(" + to_string(*node.lhs) + operator_symbol(node.op) + to_string(*node.rhs) + ")";
                }
                else
                {
                    static_assert(std::is_same_v<Node_t, ast::Ternary>);

                    return "(" + to_string(*node.condition) + "?" + to_string(*node.then_branch) + ":" +
                           to_string(*node.else_branch) + ")";
                }
            },
            expr.node);
}

ast::Expr parse(const std::string& input)
{
    Parser parser{build_lexer(), input};

    return parser.parse_expression();
}

} // namespace

TEST(Parser_test, Integer_literal)
{
    const auto expr{parse("42")};

    ASSERT_TRUE(std::holds_alternative<ast::Int_literal>(expr.node));
    EXPECT_EQ(std::get<ast::Int_literal>(expr.node).value, 42);
}

TEST(Parser_test, Floating_point_literal)
{
    const auto expr{parse("3.5")};

    ASSERT_TRUE(std::holds_alternative<ast::Float_literal>(expr.node));
    EXPECT_DOUBLE_EQ(std::get<ast::Float_literal>(expr.node).value, 3.5);
}

TEST(Parser_test, Boolean_literals)
{
    ASSERT_TRUE(std::holds_alternative<ast::Bool_literal>(parse("true").node));
    EXPECT_TRUE(std::get<ast::Bool_literal>(parse("true").node).value);

    ASSERT_TRUE(std::holds_alternative<ast::Bool_literal>(parse("false").node));
    EXPECT_FALSE(std::get<ast::Bool_literal>(parse("false").node).value);
}

TEST(Parser_test, Identifier)
{
    const auto expr{parse("counter")};

    ASSERT_TRUE(std::holds_alternative<ast::Name>(expr.node));
    EXPECT_EQ(std::get<ast::Name>(expr.node).identifier, "counter");
}

TEST(Parser_test, Simple_binary)
{
    EXPECT_EQ(to_string(parse("1 + 2")), "(1+2)");
    EXPECT_EQ(to_string(parse("1 - 2")), "(1-2)");
    EXPECT_EQ(to_string(parse("1 * 2")), "(1*2)");
    EXPECT_EQ(to_string(parse("1 / 2")), "(1/2)");
    EXPECT_EQ(to_string(parse("1 % 2")), "(1%2)");
}

TEST(Parser_test, Multiplicative_binds_tighter_than_additive)
{
    EXPECT_EQ(to_string(parse("1 + 2 * 3")), "(1+(2*3))");
    EXPECT_EQ(to_string(parse("1 * 2 + 3")), "((1*2)+3)");
}

TEST(Parser_test, Additive_is_left_associative)
{
    EXPECT_EQ(to_string(parse("1 - 2 - 3")), "((1-2)-3)");
}

TEST(Parser_test, Multiplicative_is_left_associative)
{
    EXPECT_EQ(to_string(parse("8 / 4 / 2")), "((8/4)/2)");
}

TEST(Parser_test, Parentheses_override_precedence)
{
    EXPECT_EQ(to_string(parse("(1 + 2) * 3")), "((1+2)*3)");
}

TEST(Parser_test, Nested_parentheses)
{
    EXPECT_EQ(to_string(parse("((1 + 2))")), "(1+2)");
}

TEST(Parser_test, Unary_operators)
{
    EXPECT_EQ(to_string(parse("-5")), "(-5)");
    EXPECT_EQ(to_string(parse("+5")), "(+5)");
    EXPECT_EQ(to_string(parse("!true")), "(!true)");
}

TEST(Parser_test, Unary_chains)
{
    EXPECT_EQ(to_string(parse("--5")), "(-(-5))");
    EXPECT_EQ(to_string(parse("!!true")), "(!(!true))");
}

TEST(Parser_test, Unary_binds_tighter_than_multiplicative)
{
    EXPECT_EQ(to_string(parse("-2 * 3")), "((-2)*3)");
}

TEST(Parser_test, Relational_and_equality)
{
    EXPECT_EQ(to_string(parse("1 < 2")), "(1<2)");
    EXPECT_EQ(to_string(parse("1 > 2")), "(1>2)");
    EXPECT_EQ(to_string(parse("1 <= 2")), "(1<=2)");
    EXPECT_EQ(to_string(parse("1 >= 2")), "(1>=2)");
    EXPECT_EQ(to_string(parse("1 == 2")), "(1==2)");
    EXPECT_EQ(to_string(parse("1 != 2")), "(1!=2)");
}

TEST(Parser_test, Relational_binds_tighter_than_equality)
{
    EXPECT_EQ(to_string(parse("1 < 2 == 3 < 4")), "((1<2)==(3<4))");
}

TEST(Parser_test, Additive_binds_tighter_than_relational)
{
    EXPECT_EQ(to_string(parse("1 + 2 < 3")), "((1+2)<3)");
}

TEST(Parser_test, Logical_operators_and_precedence)
{
    EXPECT_EQ(to_string(parse("true && false")), "(true&&false)");
    EXPECT_EQ(to_string(parse("true || false")), "(true||false)");
    // Logical-and binds tighter than logical-or.
    EXPECT_EQ(to_string(parse("true || false && true")), "(true||(false&&true))");
    // Equality binds tighter than logical-and.
    EXPECT_EQ(to_string(parse("1 == 1 && 2 == 2")), "((1==1)&&(2==2))");
}

TEST(Parser_test, Ternary)
{
    EXPECT_EQ(to_string(parse("true ? 1 : 2")), "(true?1:2)");
}

TEST(Parser_test, Ternary_is_right_associative)
{
    EXPECT_EQ(to_string(parse("true ? 1 : false ? 2 : 3")), "(true?1:(false?2:3))");
}

TEST(Parser_test, Ternary_binds_looser_than_logical_or)
{
    EXPECT_EQ(to_string(parse("true || false ? 1 : 2")), "((true||false)?1:2)");
}

TEST(Parser_test, Full_precedence_chain)
{
    EXPECT_EQ(to_string(parse("1 + 2 * 3 < 10 && !false")), "(((1+(2*3))<10)&&(!false))");
}

TEST(Parser_test, Throws_on_lexical_error)
{
    EXPECT_THROW(parse("1 @ 2"), std::runtime_error);
}

TEST(Parser_test, Throws_on_missing_operand)
{
    EXPECT_THROW(parse("1 +"), std::runtime_error);
}

TEST(Parser_test, Throws_on_unclosed_parenthesis)
{
    EXPECT_THROW(parse("(1 + 2"), std::runtime_error);
}

TEST(Parser_test, Throws_on_trailing_input)
{
    EXPECT_THROW(parse("1 2"), std::runtime_error);
}

TEST(Parser_test, Throws_on_empty_input)
{
    EXPECT_THROW(parse(""), std::runtime_error);
}

TEST(Parser_test, Throws_on_missing_ternary_colon)
{
    EXPECT_THROW(parse("true ? 1"), std::runtime_error);
}
