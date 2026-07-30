#ifndef HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_UNIT_HPP
#define HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_UNIT_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "hopper/cpp/ast/decl.hpp"
#include "hopper/cpp/ast/stmt.hpp"
#include "hopper/parse/source_span.hpp"

namespace hopper::cpp::ast
{
/**
 * @brief One function parameter, e.g. `const char* name` in a parameter list.
 *
 * The name is empty for an unnamed parameter. A default value is the `= expression` form; requiring defaulted
 * parameters to trail the others is left to a later semantic pass, like other non-syntactic rules.
 */
struct Parameter
{
    Type type;
    std::size_t pointers;
    bool reference;
    std::string name;
    std::optional<Expr> default_value;
};

/**
 * @brief A function definition or prototype.
 *
 * The return type carries the same pointer and reference shape as a declarator. `body` is null for a prototype
 * (`int f(int);`) and holds the compound statement of a definition.
 */
struct Function
{
    Type return_type;
    std::size_t pointers;
    bool reference;
    std::string name;
    std::vector<Parameter> parameters;
    std::unique_ptr<Stmt> body;
};

/**
 * @brief A whole translation unit: functions and declarations in source order.
 */
struct Translation_unit
{
    /**
     * @brief One item with the source range it was parsed from.
     */
    struct Item
    {
        /**
         * @brief The kinds of node an item can be.
         */
        using Node_t = std::variant<Declaration, Function>;

        /**
         * @brief The node this item holds.
         */
        Node_t node;

        /**
         * @brief The source range this item was parsed from.
         */
        parse::Source_span span{};
    };

    /**
     * @brief The items in source order.
     */
    std::vector<Item> items;
};

} // namespace hopper::cpp::ast

#endif // HOPPER_LIBS_CPP_INCLUDE_HOPPER_CPP_AST_UNIT_HPP
