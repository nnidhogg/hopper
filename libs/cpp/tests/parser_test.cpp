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
    case ast::Binary_op::Shift_left:
        return "<<";
    case ast::Binary_op::Shift_right:
        return ">>";
    case ast::Binary_op::Bitwise_and:
        return "&";
    case ast::Binary_op::Bitwise_xor:
        return "^";
    case ast::Binary_op::Bitwise_or:
        return "|";
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
    case ast::Unary_op::Bitwise_not:
        return "~";
    }

    return "?";
}

std::string operator_symbol(const ast::Postfix_op op)
{
    switch (op)
    {
    case ast::Postfix_op::Increment:
        return "++";
    case ast::Postfix_op::Decrement:
        return "--";
    }

    return "?";
}

std::string operator_symbol(const ast::Member_op op)
{
    switch (op)
    {
    case ast::Member_op::Dot:
        return ".";
    case ast::Member_op::Arrow:
        return "->";
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
                else if constexpr (std::is_same_v<Node_t, ast::Postfix>)
                {
                    return "(" + to_string(*node.operand) + operator_symbol(node.op) + ")";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Call>)
                {
                    std::string arguments;

                    for (const auto& argument : node.arguments)
                    {
                        arguments += (arguments.empty() ? "" : ",") + to_string(argument);
                    }

                    return to_string(*node.callee) + "(" + arguments + ")";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Member>)
                {
                    return "(" + to_string(*node.object) + operator_symbol(node.op) + node.member + ")";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Subscript>)
                {
                    return "(" + to_string(*node.object) + "[" + to_string(*node.index) + "])";
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
    // A space is required: "--5" is one Minus_minus token via maximal munch (see Maximal_munch_prefers_the_longest_
    // operator_token below), not two Minus tokens.
    EXPECT_EQ(to_string(parse("- -5")), "(-(-5))");
    EXPECT_EQ(to_string(parse("!!true")), "(!(!true))");
}

TEST(Parser_test, Unary_binds_tighter_than_multiplicative)
{
    EXPECT_EQ(to_string(parse("-2 * 3")), "((-2)*3)");
}

TEST(Parser_test, Maximal_munch_prefers_the_longest_operator_token)
{
    // "--5" lexes as a single Minus_minus token followed by "5", exactly as in real C++ (where this is the classic
    // reason "a---b" means "a-- -b", not "a - --b"). There is no prefix -- in this grammar, so it's a syntax error.
    EXPECT_THROW(parse("--5"), std::runtime_error);
}

TEST(Parser_test, Call_with_no_arguments)
{
    EXPECT_EQ(to_string(parse("f()")), "f()");
}

TEST(Parser_test, Call_with_one_argument)
{
    EXPECT_EQ(to_string(parse("f(1)")), "f(1)");
}

TEST(Parser_test, Call_with_several_arguments)
{
    EXPECT_EQ(to_string(parse("f(1, 2, 3)")), "f(1,2,3)");
}

TEST(Parser_test, Call_arguments_are_full_expressions)
{
    EXPECT_EQ(to_string(parse("f(1 + 2, x < y)")), "f((1+2),(x<y))");
}

TEST(Parser_test, Nested_calls)
{
    EXPECT_EQ(to_string(parse("f(g(1))")), "f(g(1))");
}

TEST(Parser_test, Calls_chain_left_to_right)
{
    // A call on the result of a call, e.g. a callback factory: get_callback()().
    EXPECT_EQ(to_string(parse("get_callback()()")), "get_callback()()");
}

TEST(Parser_test, Member_access)
{
    EXPECT_EQ(to_string(parse("object.field")), "(object.field)");
}

TEST(Parser_test, Arrow_member_access)
{
    EXPECT_EQ(to_string(parse("pointer->field")), "(pointer->field)");
}

TEST(Parser_test, Member_access_chains)
{
    EXPECT_EQ(to_string(parse("a.b.c")), "((a.b).c)");
    EXPECT_EQ(to_string(parse("a->b->c")), "((a->b)->c)");
    EXPECT_EQ(to_string(parse("a->b.c")), "((a->b).c)");
}

TEST(Parser_test, Method_call)
{
    EXPECT_EQ(to_string(parse("object.method(1, 2)")), "(object.method)(1,2)");
}

TEST(Parser_test, Subscript)
{
    EXPECT_EQ(to_string(parse("array[0]")), "(array[0])");
}

TEST(Parser_test, Subscript_index_is_a_full_expression)
{
    EXPECT_EQ(to_string(parse("array[i + 1]")), "(array[(i+1)])");
}

TEST(Parser_test, Subscript_chains)
{
    EXPECT_EQ(to_string(parse("matrix[i][j]")), "((matrix[i])[j])");
}

TEST(Parser_test, Postfix_increment_and_decrement)
{
    EXPECT_EQ(to_string(parse("x++")), "(x++)");
    EXPECT_EQ(to_string(parse("x--")), "(x--)");
}

TEST(Parser_test, Postfix_binds_tighter_than_unary)
{
    // -x++ means -(x++), not (-x)++.
    EXPECT_EQ(to_string(parse("-x++")), "(-(x++))");
}

TEST(Parser_test, Postfix_binds_tighter_than_multiplicative)
{
    EXPECT_EQ(to_string(parse("x++ * 2")), "((x++)*2)");
}

TEST(Parser_test, Mixed_postfix_chain)
{
    // a.b -> (a.b); [0] -> ((a.b)[0]); (1) -> ((a.b)[0])(1) (Call doesn't add its own parens); .c -> the whole thing
    // wrapped again by the outer Member.
    EXPECT_EQ(to_string(parse("a.b[0](1).c")), "(((a.b)[0])(1).c)");
}

TEST(Parser_test, Throws_on_unclosed_call)
{
    EXPECT_THROW(parse("f(1, 2"), std::runtime_error);
}

TEST(Parser_test, Throws_on_missing_argument_after_comma)
{
    EXPECT_THROW(parse("f(1,)"), std::runtime_error);
}

TEST(Parser_test, Throws_on_unclosed_subscript)
{
    EXPECT_THROW(parse("array[0"), std::runtime_error);
}

TEST(Parser_test, Throws_on_missing_member_name)
{
    EXPECT_THROW(parse("object."), std::runtime_error);
    EXPECT_THROW(parse("object.1"), std::runtime_error);
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

TEST(Parser_test, Shift_operators)
{
    EXPECT_EQ(to_string(parse("1 << 2")), "(1<<2)");
    EXPECT_EQ(to_string(parse("1 >> 2")), "(1>>2)");
}

TEST(Parser_test, Shift_is_left_associative)
{
    EXPECT_EQ(to_string(parse("1 << 2 << 3")), "((1<<2)<<3)");
}

TEST(Parser_test, Additive_binds_tighter_than_shift)
{
    EXPECT_EQ(to_string(parse("1 + 2 << 3")), "((1+2)<<3)");
}

TEST(Parser_test, Shift_binds_tighter_than_relational)
{
    EXPECT_EQ(to_string(parse("1 << 2 < 3")), "((1<<2)<3)");
}

TEST(Parser_test, Bitwise_operators)
{
    EXPECT_EQ(to_string(parse("1 & 2")), "(1&2)");
    EXPECT_EQ(to_string(parse("1 ^ 2")), "(1^2)");
    EXPECT_EQ(to_string(parse("1 | 2")), "(1|2)");
    EXPECT_EQ(to_string(parse("~1")), "(~1)");
}

TEST(Parser_test, Bitwise_operators_are_left_associative)
{
    EXPECT_EQ(to_string(parse("1 & 2 & 3")), "((1&2)&3)");
    EXPECT_EQ(to_string(parse("1 ^ 2 ^ 3")), "((1^2)^3)");
    EXPECT_EQ(to_string(parse("1 | 2 | 3")), "((1|2)|3)");
}

TEST(Parser_test, Bitwise_and_binds_tighter_than_bitwise_xor)
{
    EXPECT_EQ(to_string(parse("1 ^ 2 & 3")), "(1^(2&3))");
}

TEST(Parser_test, Bitwise_xor_binds_tighter_than_bitwise_or)
{
    EXPECT_EQ(to_string(parse("1 | 2 ^ 3")), "(1|(2^3))");
}

TEST(Parser_test, Equality_binds_tighter_than_bitwise_and)
{
    EXPECT_EQ(to_string(parse("1 & 2 == 3")), "(1&(2==3))");
}

TEST(Parser_test, Bitwise_or_binds_tighter_than_logical_and)
{
    EXPECT_EQ(to_string(parse("1 | 2 && 3")), "((1|2)&&3)");
}

TEST(Parser_test, Bitwise_not_binds_tighter_than_bitwise_and)
{
    EXPECT_EQ(to_string(parse("~1 & 2")), "((~1)&2)");
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
