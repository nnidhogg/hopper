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

std::string operator_symbol(const ast::Assign_op op)
{
    switch (op)
    {
    case ast::Assign_op::Assign:
        return "=";
    case ast::Assign_op::Add:
        return "+=";
    case ast::Assign_op::Subtract:
        return "-=";
    case ast::Assign_op::Multiply:
        return "*=";
    case ast::Assign_op::Divide:
        return "/=";
    case ast::Assign_op::Modulo:
        return "%=";
    case ast::Assign_op::Bitwise_and:
        return "&=";
    case ast::Assign_op::Bitwise_xor:
        return "^=";
    case ast::Assign_op::Bitwise_or:
        return "|=";
    case ast::Assign_op::Shift_left:
        return "<<=";
    case ast::Assign_op::Shift_right:
        return ">>=";
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
                else if constexpr (std::is_same_v<Node_t, ast::Ternary>)
                {
                    return "(" + to_string(*node.condition) + "?" + to_string(*node.then_branch) + ":" +
                           to_string(*node.else_branch) + ")";
                }
                else
                {
                    static_assert(std::is_same_v<Node_t, ast::Assign>);

                    return "(" + to_string(*node.target) + operator_symbol(node.op) + to_string(*node.value) + ")";
                }
            },
            expr.node);
}

std::string type_name(const ast::Type& type)
{
    const auto name{[&type]() -> std::string {
        switch (type.kind)
        {
        case ast::Type_kind::Bool:
            return "bool";
        case ast::Type_kind::Char:
            return "char";
        case ast::Type_kind::Int:
            return "int";
        case ast::Type_kind::Float:
            return "float";
        case ast::Type_kind::Double:
            return "double";
        case ast::Type_kind::Void:
            return "void";
        }

        return "?";
    }()};

    return (type.is_const ? "const " : "") + name;
}

std::string to_string(const ast::Declaration& declaration)
{
    std::string declarators;

    for (const auto& declarator : declaration.declarators)
    {
        declarators += (declarators.empty() ? "" : ",") + std::string(declarator.pointers, '*') +
                       (declarator.reference ? "&" : "") + declarator.name +
                       (declarator.initializer ? "=" + to_string(*declarator.initializer) : "");
    }

    return type_name(declaration.type) + " " + declarators + ";";
}

// Renders the AST back as compact source, bracing the branches of if and while so the two possible bindings of a
// dangling else render differently.
std::string to_string(const ast::Stmt& stmt)
{
    return std::visit(
            [](const auto& node) -> std::string {
                using Node_t = std::decay_t<decltype(node)>;

                if constexpr (std::is_same_v<Node_t, ast::Expr_stmt>)
                {
                    return to_string(node.expr) + ";";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Empty>)
                {
                    return ";";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Compound>)
                {
                    std::string statements;

                    for (const auto& statement : node.statements)
                    {
                        statements += to_string(statement);
                    }

                    return "{" + statements + "}";
                }
                else if constexpr (std::is_same_v<Node_t, ast::If>)
                {
                    return "if(" + to_string(node.condition) + "){" + to_string(*node.then_branch) + "}" +
                           (node.else_branch ? "else{" + to_string(*node.else_branch) + "}" : "");
                }
                else if constexpr (std::is_same_v<Node_t, ast::While>)
                {
                    return "while(" + to_string(node.condition) + "){" + to_string(*node.body) + "}";
                }
                else if constexpr (std::is_same_v<Node_t, ast::For>)
                {
                    // The init renders with its own trailing ';' in all three of its forms.
                    return "for(" + to_string(*node.init) + (node.condition ? to_string(*node.condition) : "") +
                           ";" + (node.step ? to_string(*node.step) : "") + "){" + to_string(*node.body) + "}";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Do_while>)
                {
                    return "do{" + to_string(*node.body) + "}while(" + to_string(node.condition) + ");";
                }
                else if constexpr (std::is_same_v<Node_t, ast::Return>)
                {
                    return node.value ? "return " + to_string(*node.value) + ";" : "return;";
                }
                else
                {
                    static_assert(std::is_same_v<Node_t, ast::Declaration>);

                    return to_string(node);
                }
            },
            stmt.node);
}

std::string to_string(const ast::Parameter& parameter)
{
    const auto declarator{std::string(parameter.pointers, '*') + (parameter.reference ? "&" : "") + parameter.name +
                          (parameter.default_value ? "=" + to_string(*parameter.default_value) : "")};

    return declarator.empty() ? type_name(parameter.type) : type_name(parameter.type) + " " + declarator;
}

std::string to_string(const ast::Function& function)
{
    std::string parameters;

    for (const auto& parameter : function.parameters)
    {
        parameters += (parameters.empty() ? "" : ",") + to_string(parameter);
    }

    return type_name(function.return_type) + " " + std::string(function.pointers, '*') +
           (function.reference ? "&" : "") + function.name + "(" + parameters + ")" +
           (function.body ? to_string(*function.body) : ";");
}

std::string to_string(const ast::Translation_unit& unit)
{
    std::string items;

    for (const auto& item : unit.items)
    {
        items += std::visit([](const auto& node) { return to_string(node); }, item);
    }

    return items;
}

ast::Expr parse(const std::string& input)
{
    Parser parser{build_lexer(), input};

    return parser.parse_expression();
}

ast::Stmt parse_stmt(const std::string& input)
{
    Parser parser{build_lexer(), input};

    return parser.parse_statement();
}

ast::Translation_unit parse_unit(const std::string& input)
{
    Parser parser{build_lexer(), input};

    return parser.parse_translation_unit();
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

TEST(Parser_test, Simple_assignment)
{
    EXPECT_EQ(to_string(parse("x = 1")), "(x=1)");
}

TEST(Parser_test, Compound_assignments)
{
    EXPECT_EQ(to_string(parse("x += 1")), "(x+=1)");
    EXPECT_EQ(to_string(parse("x -= 1")), "(x-=1)");
    EXPECT_EQ(to_string(parse("x *= 1")), "(x*=1)");
    EXPECT_EQ(to_string(parse("x /= 1")), "(x/=1)");
    EXPECT_EQ(to_string(parse("x %= 1")), "(x%=1)");
    EXPECT_EQ(to_string(parse("x &= 1")), "(x&=1)");
    EXPECT_EQ(to_string(parse("x |= 1")), "(x|=1)");
    EXPECT_EQ(to_string(parse("x ^= 1")), "(x^=1)");
    EXPECT_EQ(to_string(parse("x <<= 1")), "(x<<=1)");
    EXPECT_EQ(to_string(parse("x >>= 1")), "(x>>=1)");
}

TEST(Parser_test, Assignment_is_right_associative)
{
    EXPECT_EQ(to_string(parse("a = b = c")), "(a=(b=c))");
}

TEST(Parser_test, Assignment_binds_looser_than_ternary)
{
    EXPECT_EQ(to_string(parse("a = true ? 1 : 2")), "(a=(true?1:2))");
}

TEST(Parser_test, Ternary_branches_are_assignment_expressions)
{
    EXPECT_EQ(to_string(parse("true ? a = 1 : b = 2")), "(true?(a=1):(b=2))");
}

TEST(Parser_test, Assignment_target_can_be_a_postfix_expression)
{
    EXPECT_EQ(to_string(parse("object.field = 1")), "((object.field)=1)");
    EXPECT_EQ(to_string(parse("array[0] = 1")), "((array[0])=1)");
}

TEST(Parser_test, Assignment_in_call_arguments)
{
    EXPECT_EQ(to_string(parse("f(a = 1)")), "f((a=1))");
}

TEST(Parser_test, Assignment_in_subscript_index)
{
    EXPECT_EQ(to_string(parse("array[a = 1]")), "(array[(a=1)])");
}

TEST(Parser_test, Assignment_in_parentheses)
{
    EXPECT_EQ(to_string(parse("(a = 1)")), "(a=1)");
}

TEST(Parser_test, Throws_on_missing_assignment_value)
{
    EXPECT_THROW(parse("x ="), std::runtime_error);
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

TEST(Parser_test, Expression_statement)
{
    EXPECT_EQ(to_string(parse_stmt("f(x);")), "f(x);");
    EXPECT_EQ(to_string(parse_stmt("x = 1;")), "(x=1);");
}

TEST(Parser_test, Empty_statement)
{
    EXPECT_EQ(to_string(parse_stmt(";")), ";");
}

TEST(Parser_test, Compound_statement)
{
    EXPECT_EQ(to_string(parse_stmt("{ x = 1; y = 2; }")), "{(x=1);(y=2);}");
    EXPECT_EQ(to_string(parse_stmt("{}")), "{}");
    EXPECT_EQ(to_string(parse_stmt("{ { x; } }")), "{{x;}}");
}

TEST(Parser_test, If_statement)
{
    EXPECT_EQ(to_string(parse_stmt("if (a) b;")), "if(a){b;}");
}

TEST(Parser_test, If_else_statement)
{
    EXPECT_EQ(to_string(parse_stmt("if (a) b; else c;")), "if(a){b;}else{c;}");
}

TEST(Parser_test, Dangling_else_binds_to_the_nearest_if)
{
    EXPECT_EQ(to_string(parse_stmt("if (a) if (b) c; else d;")), "if(a){if(b){c;}else{d;}}");
}

TEST(Parser_test, While_statement)
{
    EXPECT_EQ(to_string(parse_stmt("while (a < 10) a += 1;")), "while((a<10)){(a+=1);}");
    EXPECT_EQ(to_string(parse_stmt("while (true) { f(); }")), "while(true){{f();}}");
}

TEST(Parser_test, Return_statement)
{
    EXPECT_EQ(to_string(parse_stmt("return;")), "return;");
    EXPECT_EQ(to_string(parse_stmt("return x + 1;")), "return (x+1);");
}

TEST(Parser_test, Statements_compose)
{
    EXPECT_EQ(to_string(parse_stmt("{ x = 0; while (x < 3) { x += 1; } if (x == 3) return x; else return 0; }")),
              "{(x=0);while((x<3)){{(x+=1);}}if((x==3)){return x;}else{return 0;}}");
}

TEST(Parser_test, Throws_on_missing_statement_semicolon)
{
    EXPECT_THROW(parse_stmt("x = 1"), std::runtime_error);
    EXPECT_THROW(parse_stmt("return x"), std::runtime_error);
}

TEST(Parser_test, Throws_on_unclosed_block)
{
    EXPECT_THROW(parse_stmt("{ x;"), std::runtime_error);
}

TEST(Parser_test, Throws_on_malformed_if)
{
    EXPECT_THROW(parse_stmt("if a) b;"), std::runtime_error);
    EXPECT_THROW(parse_stmt("if (a) b"), std::runtime_error);
    EXPECT_THROW(parse_stmt("if (a)"), std::runtime_error);
}

TEST(Parser_test, Throws_on_lone_else)
{
    EXPECT_THROW(parse_stmt("else x;"), std::runtime_error);
}

TEST(Parser_test, Throws_on_trailing_statement_input)
{
    EXPECT_THROW(parse_stmt("x; y;"), std::runtime_error);
}

TEST(Parser_test, Throws_on_empty_statement_input)
{
    EXPECT_THROW(parse_stmt(""), std::runtime_error);
}

TEST(Parser_test, Throws_when_a_keyword_is_used_as_an_expression)
{
    EXPECT_THROW(parse_stmt("x = if;"), std::runtime_error);
    EXPECT_THROW(parse_stmt("while;"), std::runtime_error);
}

TEST(Parser_test, Declaration_of_a_single_variable)
{
    EXPECT_EQ(to_string(parse_stmt("int x;")), "int x;");
}

TEST(Parser_test, Declaration_with_initializer)
{
    EXPECT_EQ(to_string(parse_stmt("int x = 1 + 2;")), "int x=(1+2);");
}

TEST(Parser_test, Declaration_of_multiple_declarators)
{
    EXPECT_EQ(to_string(parse_stmt("int a, b = 2, c;")), "int a,b=2,c;");
}

TEST(Parser_test, Declaration_types)
{
    EXPECT_EQ(to_string(parse_stmt("bool flag = true;")), "bool flag=true;");
    EXPECT_EQ(to_string(parse_stmt("char letter;")), "char letter;");
    EXPECT_EQ(to_string(parse_stmt("float ratio;")), "float ratio;");
    EXPECT_EQ(to_string(parse_stmt("double value = 3.5;")), "double value=3.500000;");
    EXPECT_EQ(to_string(parse_stmt("void* opaque;")), "void *opaque;");
}

TEST(Parser_test, Declaration_pointers_and_references)
{
    EXPECT_EQ(to_string(parse_stmt("int* p;")), "int *p;");
    EXPECT_EQ(to_string(parse_stmt("int** pp = q;")), "int **pp=q;");
    EXPECT_EQ(to_string(parse_stmt("double& r = d;")), "double &r=d;");
    // A reference to pointer; each declarator carries its own pointer and reference shape.
    EXPECT_EQ(to_string(parse_stmt("int *&rp = p, plain;")), "int *&rp=p,plain;");
}

TEST(Parser_test, Declaration_const_placement)
{
    EXPECT_EQ(to_string(parse_stmt("const int x = 1;")), "const int x=1;");
    EXPECT_EQ(to_string(parse_stmt("int const x = 1;")), "const int x=1;");
    EXPECT_THROW(parse_stmt("const int const x;"), std::runtime_error);
}

TEST(Parser_test, Declaration_initializer_commas_belong_to_the_call)
{
    // The comma inside f(1, 2) is an argument separator; only the comma after ')' starts the next declarator.
    EXPECT_EQ(to_string(parse_stmt("int a = f(1, 2), b;")), "int a=f(1,2),b;");
}

TEST(Parser_test, Declarations_inside_blocks)
{
    EXPECT_EQ(to_string(parse_stmt("{ int i = 0; i = i + 1; }")), "{int i=0;(i=(i+1));}");
}

TEST(Parser_test, Throws_on_malformed_declarations)
{
    EXPECT_THROW(parse_stmt("int;"), std::runtime_error);
    EXPECT_THROW(parse_stmt("int x"), std::runtime_error);
    EXPECT_THROW(parse_stmt("int x = ;"), std::runtime_error);
    EXPECT_THROW(parse_stmt("int 5;"), std::runtime_error);
    EXPECT_THROW(parse_stmt("int x,;"), std::runtime_error);
    EXPECT_THROW(parse_stmt("const x;"), std::runtime_error);
}

TEST(Parser_test, For_with_declaration_init)
{
    EXPECT_EQ(to_string(parse_stmt("for (int i = 0; i < 10; i++) f(i);")), "for(int i=0;(i<10);(i++)){f(i);}");
}

TEST(Parser_test, For_with_expression_init)
{
    EXPECT_EQ(to_string(parse_stmt("for (i = 0; i < 10; i++) f(i);")), "for((i=0);(i<10);(i++)){f(i);}");
}

TEST(Parser_test, For_with_empty_header)
{
    EXPECT_EQ(to_string(parse_stmt("for (;;) f();")), "for(;;){f();}");
}

TEST(Parser_test, For_header_slots_are_independent)
{
    EXPECT_EQ(to_string(parse_stmt("for (; i < 10;) f(i);")), "for(;(i<10);){f(i);}");
    EXPECT_EQ(to_string(parse_stmt("for (i = 0;; i++) f(i);")), "for((i=0);;(i++)){f(i);}");
}

TEST(Parser_test, For_init_declares_multiple_variables)
{
    EXPECT_EQ(to_string(parse_stmt("for (int i = 0, n = limit(); i < n; i++) f(i);")),
              "for(int i=0,n=limit();(i<n);(i++)){f(i);}");
}

TEST(Parser_test, For_bodies_nest)
{
    EXPECT_EQ(to_string(parse_stmt("for (;;) for (;;) f();")), "for(;;){for(;;){f();}}");
}

TEST(Parser_test, Do_while)
{
    EXPECT_EQ(to_string(parse_stmt("do f(x); while (x < 10);")), "do{f(x);}while((x<10));");
}

TEST(Parser_test, Do_while_with_block_body)
{
    EXPECT_EQ(to_string(parse_stmt("do { x = x + 1; f(x); } while (x < 10);")),
              "do{{(x=(x+1));f(x);}}while((x<10));");
}

TEST(Parser_test, Throws_on_malformed_for)
{
    EXPECT_THROW(parse_stmt("for () f();"), std::runtime_error);
    EXPECT_THROW(parse_stmt("for (int i = 0) f();"), std::runtime_error);
    EXPECT_THROW(parse_stmt("for (;; f();"), std::runtime_error);
    EXPECT_THROW(parse_stmt("for (;;)"), std::runtime_error);
}

TEST(Parser_test, Throws_on_malformed_do_while)
{
    EXPECT_THROW(parse_stmt("do f();"), std::runtime_error);
    EXPECT_THROW(parse_stmt("do f(); while (x)"), std::runtime_error);
    EXPECT_THROW(parse_stmt("do while (x);"), std::runtime_error);
}

TEST(Parser_test, Function_definition)
{
    EXPECT_EQ(to_string(parse_unit("int main() { return 0; }")), "int main(){return 0;}");
}

TEST(Parser_test, Function_with_parameters)
{
    EXPECT_EQ(to_string(parse_unit("int add(int a, int b) { return a + b; }")), "int add(int a,int b){return (a+b);}");
}

TEST(Parser_test, Function_prototype_with_pointer_shapes)
{
    EXPECT_EQ(to_string(parse_unit("const char* find(const char* haystack, char needle);")),
              "const char *find(const char *haystack,char needle);");
}

TEST(Parser_test, Function_parameters_may_be_unnamed_or_defaulted)
{
    EXPECT_EQ(to_string(parse_unit("void log(int, double level = 1.5);")), "void log(int,double level=1.500000);");
}

TEST(Parser_test, Function_returning_a_reference)
{
    EXPECT_EQ(to_string(parse_unit("int& at(int i) { return x; }")), "int &at(int i){return x;}");
}

TEST(Parser_test, Translation_unit_mixes_declarations_and_functions)
{
    EXPECT_EQ(to_string(parse_unit("int counter = 0; void tick(); int get() { return counter; }")),
              "int counter=0;void tick();int get(){return counter;}");
}

TEST(Parser_test, Translation_unit_holds_several_definitions)
{
    EXPECT_EQ(to_string(parse_unit("void a() {} void b() { a(); }")), "void a(){}void b(){a();}");
}

TEST(Parser_test, Empty_translation_unit)
{
    EXPECT_TRUE(parse_unit("").items.empty());
}

TEST(Parser_test, Function_bodies_use_the_full_statement_grammar)
{
    EXPECT_EQ(to_string(parse_unit("int sum(int n) { int total = 0; for (int i = 0; i < n; i++) total += i; return total; }")),
              "int sum(int n){int total=0;for(int i=0;(i<n);(i++)){(total+=i);}return total;}");
}

TEST(Parser_test, Throws_on_malformed_functions)
{
    EXPECT_THROW(parse_unit("int f("), std::runtime_error);
    EXPECT_THROW(parse_unit("int f()"), std::runtime_error);
    EXPECT_THROW(parse_unit("int f(x);"), std::runtime_error);
    EXPECT_THROW(parse_unit("int f(int a { return a; }"), std::runtime_error);
    EXPECT_THROW(parse_unit("int f() { return 0; } }"), std::runtime_error);
    EXPECT_THROW(parse_unit(";"), std::runtime_error);
}
